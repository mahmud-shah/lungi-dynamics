# Lungi Dynamics: A Kalboishakhi Survival Simulator

My individual project for CSE 402 (Computer Graphics & Image Processing Lab),
Metropolitan University, Sylhet.

A lungi, simulated as a cloth mesh, is worn by a simple character during a
Kalboishakhi storm. Wind and gravity pull on the cloth, and a simulated waist
knot holds until its load passes a threshold. You can also load a photo of real
lungi fabric, and my image-processing code turns it into the cloth texture.

## Status

Work in progress. Current milestone: the knot and the Kalboishakhi wind.

The lungi is tied the Bangladeshi way. The tube is wider than the waist, so the
extra cloth is gathered at the front, the top edge is rolled down into a thick
band, and a twisted fold is tucked in at the front. Gusting wind pushes on every
cloth triangle, and the knot measures the force it has to hold. When the load
stays above its strength, the roll unrolls, the twist slips out and the
waistline slides down, until the lungi comes off. The character wears
three-quarter pants underneath.

## Features (planned)

- Position-Based Dynamics cloth tube (about 40 x 60 particles)
- Capsule body collision and a waist knot that can fail
- Gusting Kalboishakhi wind with storm stages
- Built-in fabric patterns and photo-based pattern extraction
  (grayscale, normalisation, intensity profiles, DFT, k-means)
- Toon-shaded, hand-painted look, HUD, Lungi Survival Score
- Evaluation mode that logs FPS and stability to CSV

## Controls

| Key | Action |
|-----|--------|
| 1 / 2 / 3 / 4 | Wind preset: Mridu Batash, Jhor Asche!, Kalboishakhi, Nor'wester Mode |
| G | Send a gust now |
| W | Wind on / off |
| R | Retie the lungi and restart the timer |
| P | Pause physics |
| D | Debug panel (FPS, particles, wind vector, knot force, stretch) |
| + / - | More / fewer solver iterations |
| Mouse drag / arrow keys | Orbit the camera |
| Mouse wheel | Zoom |
| F | Toggle fullscreen |
| Esc | Quit |

## How to build (Code::Blocks + MinGW-w64)

1. Install Code::Blocks with MinGW-w64 (I use GCC 8.1, x86_64).
2. Open `LungiDynamics.cbp`.
3. Copy `third_party/freeglut/bin/libfreeglut.dll` into `bin/Debug/` and
   `bin/Release/` (create the folders if they do not exist). On my machine,
   `setup_freeglut.bat` does this step.
4. Choose the Debug or Release target and press F9 (Build and run).

FreeGLUT headers and the 64-bit import library are included in `third_party/`,
so no extra installation is needed.

## Folder layout

| Folder | Contents |
|--------|----------|
| `src/` | All source code |
| `third_party/` | FreeGLUT and stb_image |
| `assets/` | Textures, fabric photos, sounds |
| `docs/` | Report material, test results |

## Screenshots

Added when the cloth simulation is running.

## Future work

This version runs on Windows desktop only. A web version and a mobile version
are planned as future work. I kept platform-specific code (file dialog, audio)
in separate files to make a later port easier.

## Credits

- FreeGLUT: windowing and input (MIT/X11-style licence).
- stb_image by Sean Barrett: image loading (public domain / MIT).
