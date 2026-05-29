# Algorithm Deep Dive

A mathematical and conceptual reference for every algorithm in Keeper.  For JSON
configuration keys, see [algorithms.md](algorithms.md).

---

## Table of Contents

1. [Escape-Time Fractals — Overview](#1-escape-time-fractals--overview)
   - 1.7 [Nova Fractal](#17-nova-fractal)
   - 1.1 [Mandelbrot Set](#11-mandelbrot-set)
   - 1.2 [Julia Sets](#12-julia-sets)
   - 1.3 [Burning Ship](#13-burning-ship)
   - 1.4 [Tricorn (Mandelbar)](#14-tricorn-mandelbar)
   - 1.5 [Multibrot](#15-multibrot)
   - 1.6 [Coloring Techniques](#16-coloring-techniques)
2. [Newton Fractal](#2-newton-fractal)
3. [Strange Attractors](#3-strange-attractors)
   - 3.3 [Ikeda Map](#33-ikeda-map)
4. [Fractal Brownian Motion (Simplex Noise)](#4-fractal-brownian-motion-simplex-noise)
5. [Plasma / Diamond-Square](#5-plasma--diamond-square)
6. [Voronoi Diagrams](#6-voronoi-diagrams)
7. [Reaction-Diffusion (Gray-Scott)](#7-reaction-diffusion-gray-scott)
8. [L-Systems](#8-l-systems)
9. [Lyapunov Fractals](#9-lyapunov-fractals)
10. [Cyclic Cellular Automata](#10-cyclic-cellular-automata)
11. [Physarum Polycephalum (Slime Mould)](#11-physarum-polycephalum-slime-mould)

---

## 1. Escape-Time Fractals — Overview

Escape-time algorithms share a common structure: every pixel maps to a point in the
complex plane **c**, an orbit is computed by repeatedly applying some function
_z ← f(z, c)_, and the pixel's color is determined by how quickly (if ever) that
orbit escapes a bounding region.

**Why complex numbers?**  The complex plane gives a natural 2-D domain for iteration.
Multiplication of complex numbers encodes both scaling and rotation, so even the
simple map _z ↦ z² + c_ produces rich geometric behavior — fixed points, periodic
orbits, and chaotic regions all emerge from the same formula.

**The escape condition** is typically `|z| > R` for some radius R.  For z² + c,
once |z| > max(2, |c|) the orbit is guaranteed to diverge, so R = 2 is the
conventional threshold.

---

### 1.1 Mandelbrot Set

**Iteration:** `z₀ = 0`,  `zₙ₊₁ = zₙ² + c`

where **c** is the pixel coordinate and z is initialized to 0 each time.

#### History

Described by Robert Brooks and Peter Matelski in 1978 and independently popularized
by Benoît Mandelbrot in 1980, the set became the defining image of chaos theory and
fractal mathematics.  Mandelbrot coined the term "fractal" (from Latin _fractus_,
broken) to describe objects with non-integer Hausdorff dimension.  The boundary of
the Mandelbrot set has Hausdorff dimension exactly 2 — it is as "rough" as a solid
two-dimensional region, despite being a curve.

#### Mathematical Principles

The Mandelbrot set **M** is the set of all c ∈ ℂ for which the orbit of 0 under
_f_c(z) = z² + c_ remains bounded:

```
M = { c ∈ ℂ : sup_n |f_c^n(0)| < ∞ }
```

**Connectedness.**  M is connected (Douady–Hubbard theorem, 1982).  Every isolated
island visible at high zoom is in fact connected to the main cardioid by an infinitely
thin filament.

**Self-similarity.**  M is not strictly self-similar (unlike the Cantor set), but it
contains infinitely many miniature copies of itself near every point of the boundary.
These "Mandelbrot babies" appear at the roots of the main bulbs.

**The main cardioid** is the largest bulb.  Points inside it have attracting fixed
points; the equation of its boundary is:

```
c = μ/2 − μ²/4,  μ = e^(iθ)
```

**Period-n bulbs** are the secondary circles.  The large circle to the left of the
cardioid is the period-2 bulb; points inside it have attracting 2-cycles.

**Expanding the iteration:**

```
z = a + ib,   c = p + iq
z² = (a² − b²) + 2abi
z² + c = (a² − b² + p) + (2ab + q)i
```

In code this becomes two real multiplications per step (avoiding `std::complex`
overhead):

```cpp
double zr_new = zr*zr - zi*zi + cr;
double zi_new = 2.0*zr*zi + ci;
```

---

### 1.2 Julia Sets

**Iteration:** `z₀ = pixel`,  `zₙ₊₁ = zₙ² + c`

where **c** is a fixed complex constant (the _seed_) and z₀ is the pixel coordinate.

#### Relationship to the Mandelbrot Set

The Mandelbrot set and Julia sets are two views of the same dynamical system:

- **Mandelbrot:** fix z₀ = 0, vary c — asks _"for which seeds is the critical orbit
  bounded?"_
- **Julia:** fix c, vary z₀ — asks _"for which starting points does the orbit stay
  bounded under this particular seed?"_

This duality means the structure of the Julia set for a given c is directly
predicted by the location of c in the Mandelbrot set:

| c is inside M | Julia set is connected (one piece) |
|---|---|
| c is outside M | Julia set is totally disconnected (Cantor dust) |
| c is on ∂M | Julia set is a dendrite |

The Mandelbrot set is therefore a *map of Julia sets* — every point in it tells you
what the corresponding Julia set looks like.

#### Parameter Space

The complex plane of c values is often called *parameter space* (the Mandelbrot
set lives there), while the plane iterated for a fixed c is called *dynamical space*
(Julia sets live there).

#### Notable Seeds

| Seed | Character | Notes |
|---|---|---|
| −0.7 + 0.27015i | Siegel disc | Quasiperiodic spiral; no attracting cycle |
| 0.285 + 0.01i | Cauliflower | Near the main cardioid boundary |
| −0.4 + 0.6i | Dendrite rabbit | Parabolic point; period-3 bulb |
| −0.835 − 0.2321i | San Marco | Parabolic, near period-2 |
| 0 + 0.8i | Near-circular | Simple boundary, roughly circular |
| −2 + 0i | Basilica | On the real axis; period-2 Cantor set |

---

### 1.3 Burning Ship

**Iteration:** `zₙ₊₁ = (|Re(zₙ)| + i|Im(zₙ)|)² + c`

Before squaring, both components are made positive by taking absolute values.

#### Why Absolute Values?

In the standard Mandelbrot iteration, the sign of Im(z) alternates, causing the
orbit to spiral in both directions symmetrically.  Folding the imaginary axis
(`|Im|`) breaks this symmetry, forcing the orbit to always approach from one
direction.  The result is a fractal with bilateral symmetry along the real axis but
pronounced asymmetry in the y direction — producing the characteristic
"ship" shape and flame-like upper detail.

Discovered by Michael Michelitsch and Otto E. Rössler in 1992.

**Expanded iteration:**

```
a = |Re(z)|,  b = |Im(z)|
Re(z') = a² − b² + Re(c)
Im(z') = 2ab + Im(c)
```

This is identical to Mandelbrot except a = |a_prev| and b = |b_prev|.

The conventional viewport is flipped vertically from Mandelbrot convention because
the "ship" appears in the lower half of the complex plane.

---

### 1.4 Tricorn (Mandelbar)

**Iteration:** `zₙ₊₁ = conj(zₙ)² + c`

The complex conjugate `conj(z) = Re(z) − iIm(z)` flips the imaginary axis before
squaring.

#### Mathematical Character

Whereas z² is an analytic (holomorphic) map, `conj(z)²` is **antiholomorphic** — it
reverses orientation.  This makes the Tricorn fundamentally different in character from
the Mandelbrot family: the Tricorn is **not locally connected** and likely not even
path-connected (this is an open problem in complex dynamics).

The three-fold symmetry comes from the fixed points of the combined map: if z is a
fixed point of z → conj(z)² + c, then composing twice gives z → (conj(z)²)² + ... ,
and the resulting degree-4 polynomial has three prominent lobes.

**Expanded iteration:**

```
Re(z') = Re(z)² − Im(z)² + Re(c)
Im(z') = −2·Re(z)·Im(z) + Im(c)   ← negated vs. Mandelbrot
```

The only difference from Mandelbrot is the sign of Im(z').

---

### 1.5 Multibrot

**Iteration:** `zₙ₊₁ = zₙᵈ + c`  for real (possibly non-integer) exponent d.

#### Polar Form

Integer powers of z can be computed via the binomial theorem, but for real d the
cleanest approach is polar form:

```
z = r·e^(iθ)   ⟹   zᵈ = rᵈ · e^(idθ)

Re(zᵈ) = rᵈ · cos(dθ)
Im(zᵈ) = rᵈ · sin(dθ)
```

This requires `atan2`, `pow`, `cos`, and `sin` per iteration — slower than the
algebraic expansion for integer powers but correct for any real d.

#### Effect of Exponent

| d | Character |
|---|---|
| 2 | Mandelbrot set (one main cardioid, one period-2 bulb) |
| 3 | Two-fold symmetric; main shape is a nephroid |
| 4 | Three-fold symmetric; square cardioid |
| d → 1 | Boundary approaches a circle |
| d < 0 | Inversion; interior and exterior swap roles |
| Non-integer | Irrational structure, subtle rotational asymmetry |

For integer d, the set has (d−1)-fold rotational symmetry and its boundary is
a fractal with Hausdorff dimension 2.

---

### 1.6 Coloring Techniques

#### Naïve (Integer) Coloring

Map `iterations / max_iterations` directly to a palette index.  Simple, but
produces harsh **banding** — pixels with the same integer escape count get identical
colors, forming rings.

#### Smooth (Continuous) Coloring — Linas Vepstas Formula

When the orbit escapes at iteration n with `|z| = R`, the "true" fractional escape
count is approximated by:

```
ν = log(log(R) / log(escape_radius)) / log(power)
smooth_n = n + 1 − ν
```

This works because for large R the orbit grows geometrically: after one more step
|z| would be approximately R^power, so the extra fraction `1 − ν` measures how
far through that step we were when we crossed the threshold.  The result is a
continuous value in [0, ∞) that eliminates banding entirely.

#### Histogram Equalization

1. Render the full image and record how many pixels escape at each iteration count.
2. Build a cumulative distribution function (CDF) over iteration counts.
3. Re-map each pixel's iteration count through the CDF so that all palette values
   are used with equal frequency.

This maximizes perceived contrast and reveals detail in both sparsely and densely
iterated regions.  The downside: it requires a two-pass render (one for histogram,
one for coloring), and the mapping changes with the viewport.

#### Orbit Trap

Instead of coloring by escape count, color by the minimum value of some *trap
function* `T(z)` observed during the orbit.  Common traps:

| Trap | Function |
|---|---|
| Circle trap | `T(z) = |z|` (minimum distance to origin) |
| Point trap | `T(z) = |z − p|` for fixed p |
| Line trap | `T(z) = |Im(z)|` (distance to real axis) |
| Cross trap | `T(z) = min(|Re(z)|, |Im(z)|)` |

Orbit traps dramatically alter the visual character while preserving the fractal
structure — they reveal "orbits" as visible curves rather than level sets.

---

### 1.7 Nova Fractal

**Iteration:**

```
z_{n+1} = z_n − R · (z_n^p − 1) / (p · z_n^{p-1}) + c
```

The Nova fractal (sometimes called *Novagrad*) was popularized in the fractal
community in the mid-1990s.  It is the Newton fractal with two modifications:

1. **Relaxation constant R** — multiplying the correction step by R ≠ 1 slows or
   accelerates convergence, creating elaborate extra detail near the basin boundaries.
   R = 1 reproduces standard Newton iteration.
2. **Additive injection of c** — adding a complex constant c at each step destroys
   the pure root-finding character of Newton's method: some orbits now *escape* to
   infinity rather than converging.  This introduces Mandelbrot-style escape-time
   structure alongside the Newton basin structure.

#### Two Modes

**Mandelbrot-type** (`nova_type = "mandelbrot"`): `z₀ = 1`, c = pixel.
The fixed starting point `z₀ = 1` is a root of z^p − 1 (for integer p), so the
constant injection c is the only source of divergence.  This mode produces a
parameter-space map analogous to the Mandelbrot set.

**Julia-type** (`nova_type = "julia"`): `z₀ = pixel`, c = fixed seed.
The starting point is varied while c is held constant.  This produces a dynamical
space map analogous to a Julia set — showing which initial conditions converge to
which root for a given c.

#### Layered Coloring

The hybrid nature of the Nova requires two coloring layers:

- **Converged pixels** (`|z^p − 1| < tolerance`): colored by Newton basin rules —
  hue from the angle of the final z, brightness from iteration count.
- **Escaped pixels** (`|z| > escape_radius`): colored by smooth escape time via
  the palette — the same log-based formula used by the Mandelbrot family.
- **Non-converging, non-escaping pixels**: rare boundary cases — drawn at the dark
  end of the palette.

The result is a fractal that visually blends the rainbow basin structure of Newton
with the layered bands of escape-time fractals.

---

## 2. Newton Fractal

**Function:** `f(z) = zⁿ − 1`

Newton's method finds roots of f by iterating:

```
zₖ₊₁ = zₖ − f(zₖ)/f'(zₖ) = zₖ − (zₖⁿ − 1) / (n·zₖⁿ⁻¹)
       = ((n−1)·zₖⁿ + 1) / (n·zₖⁿ⁻¹)
```

#### History

Isaac Newton described the method in 1669 (for polynomials); it was later generalized
by Joseph Raphson (1690).  The fractal structure of the basins of attraction in the
complex plane was studied extensively in the 1970s–80s.  John Hubbard's 1981 paper
on Newton's method for complex polynomials showed that the basin boundaries are
**always fractal** for polynomials with three or more roots.

#### The Roots

`zⁿ = 1` has exactly n roots evenly spaced on the unit circle:

```
ωₖ = e^(2πik/n),  k = 0, 1, …, n−1
```

For n = 3:  `1`,  `e^(2πi/3) ≈ −0.5 + 0.866i`,  `e^(4πi/3) ≈ −0.5 − 0.866i`.

#### Basin of Attraction

The *basin of attraction* of root ωₖ is the set of starting points z₀ that converge
to ωₖ under Newton iteration.  These basins partition the complex plane, and their
shared boundaries form the Newton fractal.

**Cayley's theorem** (1879) showed that for n = 2 the boundary is a straight line —
the imaginary axis for f(z) = z² − 1.  For n ≥ 3, Hubbard showed the boundaries
are infinitely complex fractals.

#### Coloring

- **Hue** = angle of the final z relative to the origin: `arg(z) / (2π)`.  Since the
  roots are evenly spaced on the unit circle, each root maps to a distinct hue band.
- **Value (brightness)** = `1 − (k / max_iterations)` scaled to [0.2, 1.0], where k
  is the number of iterations taken.  Fast convergence → bright; slow → dark.

This coloring directly visualizes *which root* and *how fast* simultaneously.

#### Sensitivity to Initial Conditions

Near any boundary point between two basins there is always a third basin nearby —
the boundaries are three-way everywhere (for n ≥ 3).  This means that no matter
how small a neighborhood you zoom into on the boundary, you will always find all
three (or more) colors interleaved.

---

## 3. Strange Attractors

Strange attractors are sets in phase space toward which chaotic dynamical systems
converge, yet on which the motion never exactly repeats.  The word "strange" refers
to their fractal dimension — they occupy a non-integer-dimensional subset of the
phase space.

Keeper implements two classical 2-D iterated-function strange attractors rendered as
density histograms.

### 3.1 Clifford Attractor

**Equations:**

```
xₙ₊₁ = sin(a·yₙ) + c·cos(a·xₙ)
yₙ₊₁ = sin(b·xₙ) + d·cos(b·yₙ)
```

Discovered by Clifford Pickover.  The four parameters a, b, c, d control the shape;
the system is chaotic for most parameter combinations.  The trigonometric functions
ensure the attractor remains bounded regardless of parameters.

### 3.2 De Jong Attractor

**Equations:**

```
xₙ₊₁ = sin(a·yₙ) − cos(b·xₙ)
yₙ₊₁ = sin(c·xₙ) − cos(d·yₙ)
```

Discovered by Peter de Jong.  Similar to Clifford but with a subtraction rather than
addition, producing qualitatively different shapes for the same parameter ranges.

### 3.3 Rendering: Density Histogram

Neither attractor has a closed-form solution — they are visualized by *simulation*:

1. Pick a random initial point (x₀, y₀).
2. Discard the first `warmup` iterations (transient behavior that hasn't yet settled
   onto the attractor).
3. For each remaining iteration, project (x, y) to the nearest pixel and increment a
   counter (the *density histogram*).
4. After N million iterations, color each pixel by its density value:

```
t = log(1 + density[px]) / log(1 + max_density)
color = palette.sample(t)
```

The logarithm compresses the enormous dynamic range — attractor cores receive orders
of magnitude more visits than peripheral regions.

#### Why It Can't Be Tiled

The attractor is a *global* object.  No subset of pixels can be rendered without
running the full orbit, because every point on the attractor may be visited by a
single trajectory regardless of where it starts.

### 3.3 Ikeda Map

**Equations:**

```
τ = 0.4 − 6 / (1 + x² + y²)

x' = 1 + u·(x·cos τ − y·sin τ)
y' =     u·(x·sin τ + y·cos τ)
```

Proposed by Kensuke Ikeda in 1979 as a model for light bouncing between two mirrors
inside a nonlinear optical resonator.  The parameter `u` controls the degree of
nonlinearity — for `u ≲ 0.6` the map has simple fixed points; as `u` increases
toward 1 the attractor becomes progressively more complex, and for `u ≈ 0.9` the
system is fully chaotic.

#### Physical Interpretation

`τ` is a phase shift that depends on the squared distance from the origin — closer
to the origin means a larger (more negative) phase shift.  The outer `u·(rotation)`
term then applies a `u`-scaled rotation by angle `τ` to the current point.  The
constant `+1` breaks the symmetry and prevents the origin from being a fixed point
of the rotation, which is what produces the attractor's characteristic folded-sheet
structure.

#### Visual Character

Unlike the smooth, nebula-like density clouds of Clifford and De Jong, the Ikeda
attractor produces sharp, geometric folded ribbons — vivid examples of *phase-space
folding*, the core mechanism behind deterministic chaos.  Rendered as a density
histogram, high-density folds appear as bright streaks against a dark field.

**Key parameter:** `attractor_u` (default 0.9) — increase toward 1 for more chaotic
folding; decrease below 0.7 for simpler periodic structures.

---

## 4. Fractal Brownian Motion (Simplex Noise)

### 4.1 Simplex Noise

Simplex noise (Ken Perlin, 2001) is an improvement over classic Perlin noise:

- **Grid:** Instead of a square grid, uses a *simplex grid* (triangles in 2D).
  Each evaluation point lies in exactly one simplex and receives gradient
  contributions from its 3 corners.
- **Falloff:** Contribution from each corner uses a smooth radial falloff:
  `w(d) = max(0, 0.5 − d²)⁴` — this ensures C¹ continuity and no axis-aligned
  artifacts.
- **Gradients:** Each grid point is assigned a pseudo-random unit gradient from a
  small fixed set `{(±1,0), (0,±1), (±1,±1)}`.

The result for a point **p** is:

```
noise(p) = Σ_corner [ w(|p − corner|²) · (gradient_corner · (p − corner)) ]
```

normalized to approximately [-1, 1].

#### Why Not Classic Perlin Noise?

Classic Perlin noise uses a cubic lattice — evaluation at the 4 corners of the
containing square, interpolated with a smooth step.  This produces visible
axis-aligned artifacts at certain frequencies.  Simplex noise eliminates these by
using a simplex tessellation where no two edges are axis-aligned.

### 4.2 Fractal Brownian Motion (fBm)

A single octave of noise is smooth and featureless.  fBm stacks octaves to build
self-similar detail:

```
fBm(p) = Σ(i=0 to n-1) [ persistenceⁱ · noise(lacunarityⁱ · scale · p) ]
```

| Term | Role |
|---|---|
| `persistence` (H) | Amplitude falloff.  `0.5` → each octave is half as loud.  H < 0.5 = smooth; H → 1 = rough |
| `lacunarity` | Frequency multiplier.  `2.0` = each octave is twice as fine |
| `octaves` | Number of layers; beyond ~8 the added detail is below pixel resolution |
| `scale` | Base spatial frequency; controls the overall "zoom" of the texture |

**Hurst exponent** H = log(persistence)/log(1/lacunarity).  For H = 0.5 and
lacunarity = 2, this matches fractional Brownian motion with exponent 0.5 —
the statistical model of Brownian noise.

The fBm value is then normalized to [0, 1] and sampled through the chosen palette.

---

## 5. Plasma / Diamond-Square

### 5.1 Diamond-Square Algorithm

The diamond-square algorithm (Fournier, Fussell & Carpenter, 1982) generates a
heightfield with fractal characteristics by *midpoint displacement* on a 2D grid.
The grid size must be `(2^octaves + 1) × (2^octaves + 1)`.

**Initialization:**  Assign random values in [−1, 1] to the four corners.

**Iterative refinement** proceeds from the largest scale to the smallest.  At each
level the step size `s` halves:

**Diamond step:**  For each square of side `s`, set the center to the average of the
four corners plus a random offset:

```
center = avg(NW, NE, SW, SE) + rand() · roughness^level
```

**Square step:**  For each diamond shape (four points in a diamond pattern), set the
center to the average of up to four neighbors plus a random offset:

```
edge_center = avg(available_neighbors) + rand() · roughness^level
```

After all levels, the grid is normalized to [0, 1] and bilinearly sampled into the
output image.

**Roughness** controls the amplitude of the random offset at each level.  Lower
roughness → smoother terrain; higher → jagged, rocky.  In the frequency domain,
diamond-square produces a power spectrum approximating `1/f^β` noise, with β
controlled by roughness.

#### Why It Can't Be Tiled

Each diamond and square step depends on its neighbors across the entire grid.  There
is no way to compute a sub-region independently; the algorithm is inherently global.

---

## 6. Voronoi Diagrams

### 6.1 Definition

Given a set of *seed points* S = {s₁, s₂, …, sₙ} in the plane, the Voronoi cell
of seed sᵢ is:

```
V(sᵢ) = { p ∈ ℝ² : d(p, sᵢ) ≤ d(p, sⱼ) for all j ≠ i }
```

The collection of all cells is the *Voronoi diagram*, and the boundaries between
cells form the *Voronoi edges*.  The dual graph (connecting seeds whose cells share
an edge) is the *Delaunay triangulation*.

Discovered by Georgy Voronoi (1908); independently by Dirichlet (1850) — also known
as *Dirichlet tessellation* or *Thiessen polygons* in geography.

### 6.2 Distance Metrics

Different distance functions produce qualitatively different cell shapes:

| Metric | Formula | Cell shape |
|---|---|---|
| Euclidean | `√(dx² + dy²)` | Polygonal, circular in the limit |
| Manhattan (L1) | `|dx| + |dy|` | Axis-aligned diamond cells |
| Chebyshev (L∞) | `max(|dx|, |dy|)` | Axis-aligned square cells |

The general *Minkowski* distance is `(|dx|^p + |dy|^p)^(1/p)`.  L1 and L∞ are
p = 1 and p = ∞; Euclidean is p = 2.

### 6.3 Rendering Modes

**Cells mode:** Each cell assigned a unique color by hashing its seed index.  Reveals
the partition structure directly.

**Distance mode:** Pixel color proportional to d₁ (distance to nearest seed).  Cells
appear as radial gradients; boundaries are where the gradient "kinks."

**Edge mode:** Pixel color proportional to `(d₂ − d₁) / (d₂ + d₁)` where d₁ and d₂
are distances to the two nearest seeds.  This ratio is zero on Voronoi edges and
positive elsewhere, so edges appear as bright lines.  Equivalent to the *F₂ − F₁*
cellular noise function popularized by Steven Worley (1996).

### 6.4 Applications

Voronoi diagrams model natural phenomena: foam bubble structure, giraffe spots, plant
cell packing, airport catchment areas.  The distance and edge modes are widely used
in procedural texturing (stone, leather, organic cells).

---

## 7. Reaction-Diffusion (Gray-Scott)

### 7.1 The Gray-Scott Model

Proposed by P. Gray and S.K. Scott (1984), the Gray-Scott model describes two
abstract chemical species U and V undergoing:

- **Diffusion:** Each species spreads according to Fick's law (∇² term).
- **Autocatalytic reaction:** U + 2V → 3V — V catalyzes its own production,
  consuming U.
- **Feed:** U is continuously supplied at rate f.
- **Kill:** V is continuously removed at rate k + f.

The PDEs are:

```
∂u/∂t = Dᵤ·∇²u  −  u·v²  +  f·(1 − u)
∂v/∂t = Dᵥ·∇²v  +  u·v²  −  (f + k)·v
```

where:

| Symbol | Meaning |
|---|---|
| u, v | Concentrations of U and V ∈ [0, 1] |
| Dᵤ, Dᵥ | Diffusion coefficients (Dᵤ ≈ 2·Dᵥ typically) |
| f | Feed rate — how fast U is replenished |
| k | Kill rate — extra removal rate of V above f |
| ∇² | Laplacian (diffusion operator) |

### 7.2 Discrete Simulation

The domain is a 2-D grid with **toroidal** (wrap-around) boundary conditions.
The Laplacian is approximated by the 5-point stencil:

```
∇²u[x,y] ≈ u[x+1,y] + u[x−1,y] + u[x,y+1] + u[x,y−1] − 4·u[x,y]
```

Each time step applies Euler integration:

```
u' = clamp(u + dt·(Dᵤ·∇²u − u·v² + f·(1−u)),  0, 1)
v' = clamp(v + dt·(Dᵥ·∇²v + u·v² − (f+k)·v),  0, 1)
```

Double buffering (two grids, alternating read/write) prevents the current step from
reading partially-updated values.

### 7.3 Phase Diagram and Presets

The (f, k) parameter space has been extensively mapped.  Small changes in f and k
produce qualitatively different patterns (Turing instability):

```
                k →
           0.04     0.06     0.08
f ↑ 0.08  [worms]  [holes]  [dots]
    0.05  [coral]  [spots]  [dense]
    0.03  [maze ]  [spot2]  [unstbl]
```

| Preset | f | k | Pattern |
|---|---|---|---|
| `coral` | 0.0545 | 0.062 | Branching coral-like filaments |
| `mitosis` | 0.0367 | 0.0649 | Circular spots that split |
| `worms` | 0.082 | 0.061 | Sinuous worm-like stripes |
| `maze` | 0.029 | 0.057 | Dense labyrinthine channels |
| `spots` | 0.037 | 0.061 | Leopard-spot dots |
| `fingerprint` | 0.037 | 0.065 | Concentric whorls |

### 7.4 Connection to Biology

Gray-Scott kinetics are a toy model for real biological pattern-formation mechanisms
proposed by Alan Turing in 1952 (_"The Chemical Basis of Morphogenesis"_).  Turing
showed that a *reaction-diffusion* system with two morphogens (an activator that
promotes its own production and an inhibitor that suppresses it) can spontaneously
break spatial symmetry and generate patterns from a uniform initial condition —
now called *Turing patterns*.  Stripe and spot patterns on animals (zebras, leopards,
tropical fish) are thought to arise from similar mechanisms.

---

## 8. L-Systems

### 8.1 Formal Grammar Background

An L-system (Lindenmayer system) is a formal grammar introduced by the biologist
Aristid Lindenmayer in 1968 to model plant cell growth.

An L-system consists of:

- **Alphabet** V — set of symbols
- **Axiom** ω — the starting string
- **Production rules** P — rewriting rules of the form `A → successor_string`

At each iteration, every symbol in the current string is simultaneously replaced by
its successor (or kept unchanged if it has no rule).  This is *parallel rewriting* —
unlike context-free grammars which apply one rule at a time.

**Example — Algae growth (Lindenmayer's original):**

```
Axiom: A
Rules: A → AB,  B → A
Iteration 0:  A
Iteration 1:  AB
Iteration 2:  ABA
Iteration 3:  ABAAB
Iteration 4:  ABAABABA
```

The lengths follow the Fibonacci sequence.

### 8.2 String Length Growth

Because rules typically expand one symbol to multiple, string length grows
exponentially with iteration count.  For a rule that expands every symbol by
factor r, after n iterations the string has length `|ω| · rⁿ`.  This limits
practical iteration counts — the L-system presets are calibrated so the string
fits in memory and the line count is renderable.

### 8.3 Turtle Graphics Interpretation

After expansion, the string is interpreted by a *turtle* — a drawing cursor with
state `(x, y, θ)` (position and heading):

| Symbol | Turtle Action |
|---|---|
| `F`, `G` | Move forward by step length; draw line from old to new position |
| `f` | Move forward without drawing (pen up) |
| `+` | Turn left (counter-clockwise) by `angle_deg` |
| `-` | Turn right (clockwise) by `angle_deg` |
| `[` | Push `(x, y, θ)` onto stack |
| `]` | Pop `(x, y, θ)` from stack |
| `\|` | Rotate 180° (reverse direction) |
| Any letter | No-op (production variable, not rendered) |

The stack `[...]` is the key to branching — it lets the turtle return to a saved
position after drawing a branch, enabling recursive tree structures.

### 8.4 The Presets

#### Plant (`"plant"`)

```
Axiom: X
Rules: X → F+[[X]-X]-F[-FX]+X
       F → FF
Angle: 25°,  Iterations: 6
```

X is a "virtual" symbol that never draws but expands into a branching structure.
F draws the stem segments.  The `[` / `]` brackets produce secondary branches.

#### Dragon Curve (`"dragon"`)

```
Axiom: FX
Rules: X → X+YF+
       Y → -FX-Y
Angle: 90°,  Iterations: 12
```

The Heighway dragon curve: fold a strip of paper in half 12 times and unfold it
at 90° — the resulting crease pattern is exactly this curve.  It tiles the plane
with two copies and has Hausdorff dimension 2.

#### Sierpiński Triangle (`"sierpinski"`)

```
Axiom: F-G-G
Rules: F → F-G+F+G-F
       G → GG
Angle: 120°,  Iterations: 6
```

F and G both draw lines but G never subdivides in the fractal sense — this
encodes the removal of the central triangle at each scale.  The result is the
Sierpiński triangle (Wacław Sierpiński, 1915) with Hausdorff dimension
log(3)/log(2) ≈ 1.585.

#### Hilbert Curve (`"hilbert"`)

```
Axiom: A
Rules: A → +BF-AFA-FB+
       B → -AF+BFB+FA-
Angle: 90°,  Iterations: 6
```

A and B are orientation-tracking variables.  The Hilbert curve (David Hilbert,
1891) is a space-filling curve: in the limit it passes through every point in the
unit square.  Its Hausdorff dimension is 2 despite being a single continuous curve.
It is used in computer science for cache-efficient array traversal.

#### Tree (`"tree"`)

```
Axiom: F
Rules: F → FF+[+F-F-F]-[-F+F+F]
Angle: 22.5°,  Iterations: 4
```

A symmetric self-similar tree.  The inner `[...]` groups produce upward-branching
sub-trees; the outer `-[...]` groups produce downward ones.  Four iterations already
produces thousands of branches.

### 8.5 Two-Pass Rendering

Because the L-system's scale and position depend on the full expanded string, the
renderer makes two passes:

1. **Bounds pass:** Run the turtle with unit step size (step = 1.0) and record the
   bounding box `(x_min, x_max, y_min, y_max)`.
2. **Draw pass:** Compute scale factor so the drawing fills 90% of the canvas in
   both dimensions.  Run the turtle again with the scaled step, centered on the
   canvas.

Lines are rasterized with Bresenham's line algorithm — the classic integer-only
algorithm (Jack Bresenham, 1962) that draws a pixel-perfect line using only addition
and comparison.

---

---

## 9. Lyapunov Fractals

### 9.1 Background

Lyapunov fractals were introduced by Mario Markus of the Max Planck Institute in 1990
(popularized in *Scientific American*, 1991).  They visualize the stability landscape
of a parametrically driven logistic map — one of the simplest models capable of
deterministic chaos.

### 9.2 The Logistic Map

The logistic map `x_{n+1} = r · x_n · (1 − x_n)` was introduced as a population
model by Pierre François Verhulst in 1845 and rediscovered in the chaos context by
Robert May in 1976.  For a fixed r:

| r range | Behavior |
|---|---|
| 0 < r ≤ 1 | x → 0 (extinction) |
| 1 < r ≤ 3 | x → stable fixed point |
| 3 < r ≤ 3.57 | Period-doubling cascade |
| r > 3.57 | Mostly chaotic, with "windows" of periodicity |
| r > 4 | x leaves [0,1]; diverges |

The famous bifurcation diagram of the logistic map shows the transition from order
to chaos as r increases.

### 9.3 The Lyapunov Exponent

The Lyapunov exponent λ measures the *average exponential rate of divergence* of
nearby trajectories:

```
λ = lim_{N→∞} (1/N) Σ ln|f'(xₙ)|
  = (1/N) Σ ln|r_n · (1 − 2·xₙ)|
```

where `f'(x) = r·(1 − 2x)` is the derivative of the logistic map.

- **λ < 0**: nearby trajectories converge → the system is *stable*, *predictable*
- **λ = 0**: marginal (bifurcation boundary)
- **λ > 0**: nearby trajectories diverge → the system is *chaotic*, *sensitive* to
  initial conditions

### 9.4 The Fractal Construction

Instead of a fixed r, the sequence alternates between two parameters a and b
(the pixel coordinates) according to a binary string (e.g. `"AB"`, `"AABAB"`).
The result is a 2-D map of stability: each point (a, b) is colored by its λ value.

The stable (blue) regions form intricate organic shapes; the black chaotic regions
carve out their complement.  The boundary between them is fractal.  The shapes depend
heavily on the sequence string — different sequences produce entirely different
visual landscapes from the same underlying equation.

### 9.5 Viewport

The meaningful parameter range for the logistic map is r ∈ (0, 4].  The classic
Lyapunov image uses a = real axis ∈ [2, 4], b = imaginary axis ∈ [2, 4].  Values
outside this range will produce degenerate results (x leaves [0, 1]).

---

## 10. Cyclic Cellular Automata

### 10.1 Background

Cyclic Cellular Automata (CCA) were studied extensively by Robert Fisch, Janko
Gravner, and David Griffeath at the University of Wisconsin starting in the late
1980s.  Griffeath's 1994 book *Primordial Soup Kitchen* and associated web pages
brought them to wider attention.  They are among the simplest systems that exhibit
*excitable medium* dynamics — the same class of behavior seen in cardiac tissue,
Belousov-Zhabotinsky chemical waves, and retinal waves.

### 10.2 The Update Rule

Each cell holds a state in `{0, 1, …, N−1}`.  At each step:

> A cell in state c advances to (c+1) mod N if **any** neighbor holds state (c+1) mod N.

All cells update simultaneously.  Starting from random noise:

1. Small clusters of "ahead" cells begin consuming their neighbors.
2. These clusters grow into expanding rings.
3. Rings collide and annihilate, creating spiral structures.
4. Eventually the lattice locks into stable rotating spirals.

### 10.3 Neighborhoods

**Moore (8-neighbor):** All 8 surrounding cells (including diagonals).  Produces
square-ish, geometrically crisp spirals.

**Von Neumann (4-neighbor):** The 4 orthogonal neighbors only (N, S, E, W).  The
diamond-shaped distance metric creates smoother, more circular wave fronts.

### 10.4 Effect of State Count N

| N | Character |
|---|---|
| 3–5 | Rapid, aggressive waves; fast lock-in |
| 8–12 | Moderate complexity; clear spirals |
| 14–20 | Many competing wave fronts; slow evolution |
| > 20 | Very slow self-organization; may not fully develop in limited steps |

### 10.5 Connection to Excitable Media

CCA is a discrete model of excitable media.  An excitable cell:
- Has a *resting* state (0)
- Can be *excited* by a neighbor in state 1
- Must pass through a *refractory* period (states 1 through N−1) before becoming
  excitable again

The refractory period prevents re-excitation and is what causes wave fronts to be
one-sided — explaining why spirals form rather than simple expanding discs.

---

## 11. Physarum Polycephalum (Slime Mould)

### 11.1 The Real Organism

*Physarum polycephalum* is a unicellular organism (technically a plasmodial slime
mould, not a true fungus) that forages for nutrients by extending and retracting
tubular networks.  It has no nervous system, yet routinely solves maze navigation and
has been shown to reconstruct the Tokyo rail network when oat flakes are placed at
city positions (Tero et al., *Science* 2010).

The organism's network optimization is driven by a simple feedback loop: tubes
carrying more flow grow thicker; tubes carrying little flow atrophy.  The
result is a Steiner-tree-like minimum-cost network connecting food sources.

### 11.2 The Agent-Based Model

Jeff Jones (2010, *Journal of Bionic Engineering*) showed that the organism's
macro-scale behavior can be reproduced by thousands of independent agents following
three simple rules with no global communication:

1. **Sense** — each agent samples a *trail* (chemical concentration) at three forward
   sensors (straight, left ±α, right ±α at distance d).  It rotates toward the
   strongest signal.
2. **Move** — the agent steps forward and deposits a fixed amount of trail.
3. **Diffuse + evaporate** — the trail grid undergoes a Gaussian blur and is
   multiplied by an evaporation factor each step.

The diffusion spreads chemical signals; evaporation prevents saturation and creates
time-limited gradients.  Together they create a self-organizing network: agents
follow each other's trails, reinforcing paths between dense clusters.

### 11.3 Key Parameters and Their Effects

| Parameter | Effect |
|---|---|
| `sensor_angle` | Narrow (≈15°): straight tubes and highways.  Wide (≈60°): branchy coral networks |
| `sensor_dist` | Short: tight, fine networks.  Long: broad, sweeping paths |
| `rotation_angle` | Small: gradual curves.  Large (90°): angular, grid-like |
| `decay` | Near 1.0: trails persist, dense networks.  Lower (0.8): transient, wispy |
| `deposit` | Higher: trails saturate quickly, creating strong highways |
| `num_agents` | More agents: denser, more uniform coverage |

### 11.4 Visualization

The trail grid is log-normalized before palette mapping, identical to the attractor
density histogram:

```
t = log(1 + trail) / log(1 + max_trail)
```

High-traffic paths appear bright; unexplored regions remain dark.  A dark-to-bright
palette (e.g. `"electric"`, `"fire"`) gives the most naturalistic result.

### 11.5 Connection to Network Theory

The emergent networks are near-optimal solutions to the *minimum spanning tree*
problem for the agent cluster positions — this is why *P. polycephalum* performs so
well at infrastructure design problems.  The diffusion-evaporation mechanism is
analogous to pheromone-based ant colony optimization algorithms.

---

*For configuration keys and JSON examples, see [algorithms.md](algorithms.md).
For palette and viewport configuration, see [config-reference.md](config-reference.md).*
