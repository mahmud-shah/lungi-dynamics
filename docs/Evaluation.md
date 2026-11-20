# Evaluation results

Produced with the in-game evaluation mode (E on the title screen). Each row is
6 s of measurement after a 1.5 s warm-up. The raw file is
`docs/evaluation_sample.csv`.

This run was made on a Linux virtual machine with software OpenGL, so the FPS
and frame-time columns in the CSV are not meaningful and are left out here.
The FPS numbers for the report come from running the same mode on my laptop.

| Time step | Iterations | Wind | Max stretch | Mean stretch | Avg knot force | Recoveries |
|---|---|---|---|---|---|---|
| 1/60 s | 30 | Mridu Batash | 72% | 0.93% | 5.1 N | 0 |
| 1/60 s | 30 | Jhor Asche! | 53% | 1.70% | 8.9 N | 0 |
| 1/60 s | 30 | Kalboishakhi | 69% | 2.02% | 16.8 N | 0 |
| 1/60 s | 30 | Nor'wester Mode | 61% | 3.13% | 49.1 N | 0 |
| 1/120 s | 15 | Mridu Batash | 53% | 0.72% | 5.0 N | 0 |
| 1/120 s | 15 | Jhor Asche! | 43% | 1.28% | 9.8 N | 0 |
| 1/120 s | 15 | Kalboishakhi | 57% | 1.63% | 18.1 N | 0 |
| 1/120 s | 15 | Nor'wester Mode | 65% | 2.66% | 42.6 N | 0 |
| 1/240 s | 15 | Mridu Batash | 25% | 0.41% | 5.3 N | 0 |
| 1/240 s | 15 | Jhor Asche! | 53% | 0.94% | 11.5 N | 0 |
| 1/240 s | 15 | Kalboishakhi | 46% | 1.11% | 21.8 N | 0 |
| 1/240 s | 15 | Nor'wester Mode | 51% | 1.72% | 64.7 N | 0 |

Stretch is the length of a structural link compared with its rest length.
Mean stretch is averaged over all links and frames; max stretch is the single
worst link seen at any moment in the whole 6 s. The cloth stays stable (no NaN recoveries, no runaway
stretch) at every time step and wind preset.

## What changed to get here

The first version of the evaluation showed that at 1/60 s in the two strongest
wind presets single links stretched to over 1000%. Two causes:

1. With half as many steps per second, the solver did half as many passes per
   simulated second, so the cloth was softer and snagged around the legs. A
   longer time step now gets proportionally more iterations.
2. A fast particle could move past the middle of a leg capsule in one step and
   was then pushed out on the far side. Collisions now push a particle out on
   the side it came from, and while the knot holds a particle may move at
   most 5 cm per step.

Before: 1/60 s, Nor'wester, max 1341%, mean 9.8%. After: max 61%, mean 3.1%.
