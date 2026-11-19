# Lungi Dynamics: A Kalboishakhi Survival Simulator

My individual project for CSE 402 (Computer Graphics & Image Processing Lab),
Metropolitan University, Sylhet.

A lungi, simulated as a cloth mesh, is worn by a simple character during a
Kalboishakhi storm. Wind and gravity pull on the cloth, and a simulated waist
knot holds until its load passes a threshold. You can also load a photo of real
lungi fabric, and my image-processing code turns it into the cloth texture.

## Status

Work in progress. Current milestone: the playable game and the lungi patterns.

The game starts on a title screen where I pick a difficulty and one of five
woven lungi patterns. The storm then builds up on its own, from a calm
Boishakh afternoon to Nor'wester Mode. Holding Space makes the character grab
the knot, which makes it 60% stronger, but grip stamina runs out, so it has to
be saved for the big gusts. When the knot gives up, a results screen shows the
Lungi Survival Score and a score tier.

The lungi is tied the Bangladeshi way: the extra cloth is gathered at the
front, the top edge is rolled into a band, and a double twist is tucked in.
When the knot is overloaded, the roll unrolls, the twist slips out and the
waistline slides down. The character wears three-quarter pants underneath.

### Score

Lungi Survival Score = (10 x seconds survived + 4 x strongest gust survived in
m/s - lungi displacement penalty) x difficulty multiplier (Easy 0.8, Normal 1.0,
Hard 1.3). Tiers: Lungi Newbie (below 300), Gaye Joth Ase (300+),
Kalboishakhi Warrior (450+), Lungi Legend (600+).

## Features (planned)

- Position-Based Dynamics cloth tube (about 40 x 60 particles)
- Capsule body collision and a waist knot that can fail
- Gusting Kalboishakhi wind with storm stages
- Built-in fabric patterns and photo-based pattern extraction
  (grayscale, normalisation, intensity profiles, DFT, k-means)
- Toon-shaded, hand-painted look, HUD, Lungi Survival Score
- Evaluation mode that logs FPS and stability to CSV

## Controls

| Key | Title screen | In the storm |
|-----|--------------|--------------|
| Enter | Start | - |
| Up / Down | Difficulty | Orbit camera |
| Left / Right | Lungi pattern | Orbit camera |
| Space | Start | Hold to grab the knot |
| L | Next lungi pattern | Next lungi pattern |
| S | - | Slow motion on / off |
| P | - | Pause |
| R | - | Restart the run |
| T / Esc | Quit (Esc) | Back to title |
| 1 / 2 / 3 / 4 | - | Sandbox: fix the wind preset |
| G / W | - | Send a gust / wind on-off |
| + / - | - | More / fewer solver iterations |
| D | Debug panel | Debug panel |
| F | Fullscreen | Fullscreen |
| Mouse drag / wheel | Camera | Camera |

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
