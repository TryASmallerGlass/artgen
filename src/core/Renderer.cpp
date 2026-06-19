#include "artgen/Renderer.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace artgen {

// ── Minimal thread pool ───────────────────────────────────────────────────────

class ThreadPool {
public:
    explicit ThreadPool(int n) {
        for (int i = 0; i < n; ++i) {
            m_workers.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lk(m_mtx);
                        m_cv.wait(lk, [this]{ return m_stop || !m_queue.empty(); });
                        if (m_stop && m_queue.empty()) return;
                        task = std::move(m_queue.front());
                        m_queue.pop();
                    }
                    task();
                }
            });
        }
    }

    void enqueue(std::function<void()> f) {
        { std::lock_guard<std::mutex> lk(m_mtx); m_queue.push(std::move(f)); }
        m_cv.notify_one();
    }

    ~ThreadPool() {
        { std::lock_guard<std::mutex> lk(m_mtx); m_stop = true; }
        m_cv.notify_all();
        for (auto& t : m_workers) t.join();
    }

private:
    std::vector<std::thread>           m_workers;
    std::queue<std::function<void()>>  m_queue;
    std::mutex                         m_mtx;
    std::condition_variable            m_cv;
    bool                               m_stop = false;
};

// ── TileRenderer helpers ──────────────────────────────────────────────────────

Viewport TileRenderer::make_tile_vp(const Viewport& p,
                                     int tx, int ty, int tw, int th) {
    Viewport v;
    v.pixel_width  = tw;
    v.pixel_height = th;
    v.real_min = p.pixel_to_real(tx);
    v.real_max = (tw > 1) ? p.pixel_to_real(tx + tw - 1) : p.pixel_to_real(tx);
    v.imag_min = p.pixel_to_imag(ty);
    v.imag_max = (th > 1) ? p.pixel_to_imag(ty + th - 1) : p.pixel_to_imag(ty);
    return v;
}

void TileRenderer::copy_tile(PixelBuffer& dst, const PixelBuffer& src,
                               int ox, int oy) {
    for (int y = 0; y < src.height(); ++y)
        for (int x = 0; x < src.width(); ++x)
            dst.set(ox + x, oy + y, src.get(x, y));
}

void TileRenderer::downsample(const PixelBuffer& src, PixelBuffer& dst,
                               int factor) {
    float inv = 1.0f / static_cast<float>(factor * factor);
    for (int dy = 0; dy < dst.height(); ++dy) {
        for (int dx = 0; dx < dst.width(); ++dx) {
            float r=0,g=0,b=0,a=0;
            for (int sy = 0; sy < factor; ++sy)
                for (int sx = 0; sx < factor; ++sx) {
                    RGBA p = src.get(dx*factor+sx, dy*factor+sy);
                    r+=p.r; g+=p.g; b+=p.b; a+=p.a;
                }
            dst.set(dx, dy, {r*inv, g*inv, b*inv, a*inv});
        }
    }
}

void TileRenderer::render_tile(IAlgorithm& algo, PixelBuffer& dst,
                                const Viewport& vp, const Tile& t) const {
    int aa = std::max(1, aa_samples);
    Viewport tv = make_tile_vp(vp, t.x, t.y, t.w, t.h);

    if (aa == 1) {
        PixelBuffer tbuf(t.w, t.h, dst.depth());
        algo.render(tbuf, tv);
        copy_tile(dst, tbuf, t.x, t.y);
    } else {
        Viewport aavp  = tv;
        aavp.pixel_width  = t.w * aa;
        aavp.pixel_height = t.h * aa;
        PixelBuffer aabuf(t.w * aa, t.h * aa, dst.depth());
        algo.render(aabuf, aavp);
        PixelBuffer tbuf(t.w, t.h, dst.depth());
        downsample(aabuf, tbuf, aa);
        copy_tile(dst, tbuf, t.x, t.y);
    }
}

// ── Main entry point ──────────────────────────────────────────────────────────

void TileRenderer::render(IAlgorithm& algo, PixelBuffer& buf,
                           const Viewport& vp) const {
    // Algorithms with global spatial state must run as one pass
    if (!algo.is_tileable()) {
        algo.render(buf, vp);
        if (progress_cb) progress_cb(1, 1);
        return;
    }

    int n = (thread_count <= 0)
        ? static_cast<int>(std::thread::hardware_concurrency())
        : thread_count;
    n = std::max(1, n);

    int W = vp.pixel_width, H = vp.pixel_height;
    int ts = std::max(1, tile_size);

    std::vector<Tile> tiles;
    for (int ty = 0; ty < H; ty += ts)
        for (int tx = 0; tx < W; tx += ts)
            tiles.push_back({tx, ty,
                             std::min(ts, W - tx),
                             std::min(ts, H - ty)});

    int total = static_cast<int>(tiles.size());

    if (show_progress)
        std::printf("  Threads: %d | Tiles: %d | AA: %d×\n",
                    n, total, std::max(1, aa_samples));

    std::atomic<int> done{0};

    auto do_tile = [&](const Tile& tile) {
        if (cancel_flag && cancel_flag->load(std::memory_order_relaxed)) return;
        render_tile(algo, buf, vp, tile);
        int d = ++done;
        if (show_progress && (d % std::max(1, total / 20) == 0 || d == total))
            std::printf("  [%4d/%4d] %.0f%%\n", d, total,
                        100.0 * d / total);
        if (progress_cb) progress_cb(d, total);
    };

    if (n == 1) {
        for (const auto& tile : tiles) do_tile(tile);
        return;
    }

    // Parallel dispatch
    std::atomic<int> remaining{total};
    std::mutex        wait_mtx;
    std::condition_variable wait_cv;

    {
        ThreadPool pool(n);
        for (const auto& tile : tiles) {
            pool.enqueue([&, tile] {
                do_tile(tile);
                if (--remaining == 0)
                    wait_cv.notify_all();
            });
        }
        std::unique_lock<std::mutex> lk(wait_mtx);
        wait_cv.wait(lk, [&]{ return remaining.load() == 0; });
    } // pool joins here — all workers exit cleanly
}

} // namespace artgen
