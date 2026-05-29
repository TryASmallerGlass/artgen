#include "artgen/algorithms/Physarum.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace artgen {

namespace {

// Simple LCG RNG returning float in [0, 1)
struct LCG {
    uint32_t state;
    explicit LCG(uint32_t s) : state(s) {}
    uint32_t next_u32() {
        state = state * 1664525u + 1013904223u;
        return state;
    }
    float next_f() {
        return static_cast<float>(next_u32()) / 4294967296.0f;
    }
};

struct Agent {
    float x, y, angle; // angle in radians
};

// Sample trail at a given position with toroidal wrap
inline float sample_trail(const std::vector<float>& trail, int W, int H,
                           float fx, float fy)
{
    int x = static_cast<int>(fx + 0.5f);
    int y = static_cast<int>(fy + 0.5f);
    x = ((x % W) + W) % W;
    y = ((y % H) + H) % H;
    return trail[static_cast<size_t>(y) * W + x];
}

} // anonymous namespace

void PhysarumAlgorithm::render(PixelBuffer& buf, const Viewport& vp) const {
    const int W = vp.pixel_width;
    const int H = vp.pixel_height;
    const float pi  = 3.14159265358979323846f;
    const float sa  = sensor_angle   * pi / 180.0f;  // rad
    const float ra  = rotation_angle * pi / 180.0f;  // rad

    // Trail grid (toroidal)
    std::vector<float> trail(static_cast<size_t>(W) * H, 0.0f);
    std::vector<float> blurred(static_cast<size_t>(W) * H, 0.0f);

    // Initialise agents uniformly across the canvas
    LCG rng(seed);
    std::vector<Agent> agents(static_cast<size_t>(num_agents));
    for (auto& ag : agents) {
        ag.x     = rng.next_f() * static_cast<float>(W);
        ag.y     = rng.next_f() * static_cast<float>(H);
        ag.angle = rng.next_f() * 2.0f * pi;
    }

    // ── Simulation loop ────────────────────────────────────────────────────────
    for (int step = 0; step < steps; ++step) {

        // 1. Sense + motor for each agent
        for (auto& ag : agents) {
            const float fx = ag.x, fy = ag.y, fa = ag.angle;

            // Sensor positions: straight, left, right
            auto sensor_pos = [&](float offset) -> std::pair<float, float> {
                return { fx + sensor_dist * std::cos(fa + offset),
                         fy + sensor_dist * std::sin(fa + offset) };
            };
            auto [sx,  sy ] = sensor_pos(0.0f);
            auto [slx, sly] = sensor_pos(-sa);
            auto [srx, sry] = sensor_pos( sa);

            float f  = sample_trail(trail, W, H, sx,  sy );
            float fl = sample_trail(trail, W, H, slx, sly);
            float fr = sample_trail(trail, W, H, srx, sry);

            // Steer: turn toward strongest sensor, or rotate randomly if equal
            if (f > fl && f > fr) {
                // keep heading
            } else if (fl > fr) {
                ag.angle -= ra;
            } else if (fr > fl) {
                ag.angle += ra;
            } else {
                // tied — random jitter
                ag.angle += (rng.next_u32() & 1u) ? ra : -ra;
            }

            // Move forward
            ag.x = std::fmod(ag.x + step_size * std::cos(ag.angle) + static_cast<float>(W), static_cast<float>(W));
            ag.y = std::fmod(ag.y + step_size * std::sin(ag.angle) + static_cast<float>(H), static_cast<float>(H));

            // Deposit
            int ix = static_cast<int>(ag.x) % W;
            int iy = static_cast<int>(ag.y) % H;
            trail[static_cast<size_t>(iy) * W + ix] += deposit;
        }

        // 2. Diffuse (3×3 box blur) + decay
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                float sum = 0.0f;
                for (int dy = -1; dy <= 1; ++dy) {
                    int ny = ((y + dy) % H + H) % H;
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = ((x + dx) % W + W) % W;
                        sum += trail[static_cast<size_t>(ny) * W + nx];
                    }
                }
                blurred[static_cast<size_t>(y) * W + x] = sum * (1.0f / 9.0f) * decay;
            }
        }
        trail.swap(blurred);
    }

    // ── Visualise: log-normalised trail → palette ──────────────────────────────
    float max_trail = 0.0f;
    for (float v : trail) if (v > max_trail) max_trail = v;

    if (max_trail == 0.0f) {
        buf.fill({0.0f, 0.0f, 0.0f, 1.0f});
        return;
    }
    const float log_max = std::log(1.0f + max_trail);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float v = trail[static_cast<size_t>(y) * W + x];
            float t = (v == 0.0f) ? 0.0f : std::log(1.0f + v) / log_max;
            buf.set(x, y, palette.sample(t));
        }
    }
}

} // namespace artgen
