# Future Algorithm Ideas

A collection of mathematically rich, highly aesthetic 2-D algorithms recommended for integration into Keeper. These algorithms leverage existing system architectures like complex plane viewports, discrete simulation grids, and density histograms.

---

## Table of Contents

1. [Lyapunov Fractals](#1-lyapunov-fractals)
2. [Physarum Polycephalum (Slime Mold)](#2-physarum-polycephalum-slime-mold)
3. [Cyclic Cellular Automata (CCA)](#3-cyclic-cellular-automata-cca)
4. [Nova Fractal](#4-nova-fractal)
5. [Ikeda Map](#5-ikeda-map)

---

## 1. Lyapunov Fractals

### 1.1 Mathematical Principles

Instead of iterating a fixed formula across the complex plane, a Lyapunov fractal uses an alternating sequence of two different growth parameters ($a$ and $b$) within the classical logistic map. 

The system alternates based on a repeating binary sequence string (e.g., `"AB"` or `"AABAB"`). For a given pixel mapped to the coordinate $(a, b)$, the sequence dictates which parameter is utilized in each iteration step:

$$x_{n+1} = r_n x_n(1 - x_n)$$

Where $r_n = a$ if the sequence character at index $n \pmod{\text{length}}$ is `'A'`, and $r_n = b$ if it is `'B'`.

The final pixel color is derived from the **Lyapunov exponent** $\lambda$, which calculates the average rate of exponential separation of infinitesimally close trajectories over $N$ iterations:

$$\lambda = \lim_{N \to \infty} \frac{1}{N} \sum_{n=1}^{N} \ln |r_n (1 - 2x_n)|$$

### 1.2 Rendering & Duality

* **Stable Regions ($\lambda < 0$):** Trajectories attract to predictable periodic orbits. These are typically colored using smooth, deep gradients proportional to the degree of stability (how negative $\lambda$ is).
* **Chaotic Regions ($\lambda > 0$):** Trajectories exhibit extreme sensitivity to initial conditions and diverge chaotically. These are traditionally rendered as solid black or a uniform contrasting color.

> **Architecture Fit:** Maps perfectly to Keeper's existing 2-D viewport and pixel loop. It replaces complex squaring with a modulated 1-D chaotic map evaluation per pixel.

---

## 2. Physarum Polycephalum (Slime Mold)

### 2.1 The Multi-Agent Simulation

Based on Jeff Jones’ 2010 design, this algorithm simulates the organic, decentralized foraging behavior of acellular slime mold using thousands of autonomous, simple particles moving across a discrete grid.



### 2.2 The Three-Step Loop

The simulation state updates continuously over three phases:

1.  **Sensory Step:** Each agent has three forward-facing "sensors" oriented at fixed offset angles (forward, forward-left, and forward-right). The agent samples the intensity of a shared trail grid in those three directions.
2.  **Motor Step:** The agent rotates towards the strongest detected trail concentration and moves forward by a step distance. Upon arriving at its new coordinate, it deposits a fixed amount of trail substance into that grid cell.
3.  **Diffuse and Evaporate:** The trail grid is processed globally every frame. It undergoes a small Gaussian blur (diffusion) to spread the chemical signatures, and is subsequently multiplied by an evaporation factor (e.g., `0.95`) to prevent total saturation.

### 2.3 Implementation Architecture

Unlike strange attractors that calculate a single, continuous trajectory, this requires an array of agents processed simultaneously:

```cpp
struct Agent {
    double x, y;
    double angle;
};

The emergent behavior yields highly optimized transport networks that look identical to biological highways, matching the visual weight of the Gray-Scott Reaction-Diffusion system.

---

## 3. Cyclic Cellular Automata (CCA)

### 3.1 Definition

Popularized by David Griffeath, Cyclic Cellular Automata model self-organizing cyclic configurations. They simulate fluid swirling, crystal growth, and undulating clockwork patterns from purely random initial noise.

### 3.2 Ruleset

Each cell on a 2-D grid contains a discrete state integer in the range $[0, N-1]$. At each step, a cell examines its neighborhood—either a 4-neighbor **Von Neumann** neighborhood or an 8-neighbor **Moore** neighborhood.

* If a cell currently holds state $c$, it searches for any neighboring cell that holds state $c + 1 \pmod N$.
* If such a neighbor is present, the cell is "consumed," advancing its own internal state to $c + 1 \pmod N$.

### 3.3 Emergent Phenomena

Starting from uniform random noise, the grid spontaneously organizes into blocks of color that chase one another, eventually stabilizing into beautiful, undulating spirals and geometric whirlpools.

| Configuration Parameter | Practical Selection | Visual Output |
|---|---|---|
| Neighborhood | Moore (8-way) | Squarish, geometric crystal structures |
| Neighborhood | Von Neumann (4-way) | Diamond-shaped, smooth spiral waves |
| States ($N$) | $12$ to $16$ | High contrast, rapid phase-locking patterns |

---

## 4. Nova Fractal

### 4.1 Mathematical Principles

The Nova fractal is a variant of the Newton fractal that introduces an escape-time "feed" factor reminiscent of the Mandelbrot set. It is an implementation of a relaxed Newton-Raphson method applied to a complex polynomial, injected with the pixel coordinate $c$.

### 4.2 Iteration Formula

A relaxation step is added to the root-finding step for $z^3 - 1 = 0$:

$$z_{n+1} = z_n - R \frac{f(z_n)}{f'(z_n)} + c$$

Where $R$ is a relaxation constant (traditionally $R = 1$). 

* **Julia-type Nova:** Hold $c$ constant as a global seed and vary the initial starting coordinate $z_0$ across the pixel grid.
* **Mandelbrot-type Nova:** Fix $z_0 = 1$ and vary the injection parameter $c$ according to the pixel coordinate.

### 4.3 Divergence vs. Convergence

Unlike the standard Newton fractal—which purely tracks *which* root a point settles into—the addition of $+c$ allows some orbits to escape completely to infinity. This allows for a layered hybrid coloring style:

* **Interior Basins:** Colored by the standard Newton root destination hue (based on the angle of final convergence).
* **Exterior Edges:** Colored by smooth Mandelbrot-style escape time counts, highlighting the chaotic boundaries where points failed to converge.

---

## 5. Ikeda Map

### 5.1 System Equations

The Ikeda map is a discrete-time dynamical system that models the trajectory of light bouncing around inside a nonlinear optical cavity. 

$$x_{n+1} = 1 + u(x_n \cos \tau - y_n \sin \tau)$$

$$y_{n+1} = u(x_n \sin \tau + y_n \cos \tau)$$

Where $u$ is a tuning parameter (typically $u \approx 0.9$ for chaotic behavior) and $\tau$ is a non-linear phase factor that depends on the current distance from the origin:

$$\tau = 0.4 - \frac{6}{1 + x_n^2 + y_n^2}$$

### 5.2 Visual Character

While Clifford and De Jong attractors produce soft, wispy probability clouds, the Ikeda map maps cleanly to a **chaotic sheet structure**.



When rendered using a density histogram pipeline, it yields razor-sharp, folded geometric ribbons that perfectly showcase the concept of phase space stretching and folding.

#### Why It Can't Be Tiled
Like Keeper's other strange attractors, the Ikeda map is an inherently global simulation. Trajectories must be calculated continuously from an initial point, filling a density histogram over millions of iterations rather than computing pixels independently.