# Image processing accuracy (synthetic photos)

I tested the fabric pipeline on synthetic photos where the true repeat is known.
Each test photo is 480 x 400 px with uneven lighting (left-to-right gradient and
vignette) and random noise, like a phone photo. The selection was 440 x 360 px.
"True" is the real repeat in pixels and "angle" is how much the cloth is rotated.

A result counts as correct if the period is within 3%, a plain direction is
reported as plain, or a rotated photo is refused instead of giving a wrong tile.

| Pattern | True (px) | Angle | Found across | Found down | Error across % | Error down % | Result | Colours | Message |
|---|---|---|---|---|---|---|---|---|---|
| Classic Check | 40 | 0 | 40.0 | 40.0 | 0.0 | 0.1 | PASS | 4 | Repeat 40 x 40 px, 4 colours. Press ENTER to wear it! |
| Classic Check | 57 | 0 | 56.7 | 57.7 | 0.5 | 1.1 | PASS | 4 | Repeat 57 x 58 px, 4 colours. Press ENTER to wear it! |
| Classic Check | 57 | 5 | 56.9 | 57.8 | 0.1 | 1.5 | PASS | 2 | Repeat 57 x 58 px, 2 colours. Press ENTER to wear it! |
| Classic Check | 57 | 12 | 58.1 | - | 2.0 | - | PASS | 4 | No clear repeat down. Set it with the arrow keys, or select a flatter area. |
| Classic Check | 83 | 0 | 83.5 | 83.3 | 0.6 | 0.4 | PASS | 2 | Repeat 83 x 83 px, 2 colours. Press ENTER to wear it! |
| Stripe | 40 | 0 | - | 40.0 | - | 0.0 | PASS | 3 | Repeat 32 x 40 px, 3 colours. Press ENTER to wear it! |
| Stripe | 57 | 0 | - | 57.4 | - | 0.6 | PASS | 3 | Repeat 32 x 57 px, 3 colours. Press ENTER to wear it! |
| Stripe | 57 | 5 | - | 58.1 | - | 1.9 | PASS | 2 | No clear repeat across. Set it with the arrow keys, or select a flatter area. |
| Stripe | 57 | 12 | - | - | - | - | PASS | 3 | No clear repeat found. Set it with the arrow keys, or select a flatter area. |
| Stripe | 83 | 0 | - | 83.0 | - | 0.0 | PASS | 3 | Repeat 32 x 83 px, 3 colours. Press ENTER to wear it! |
| Dual-tone Check | 40 | 0 | 40.0 | 40.0 | 0.0 | 0.0 | PASS | 3 | Repeat 40 x 40 px, 3 colours. Press ENTER to wear it! |
| Dual-tone Check | 57 | 0 | 56.7 | 57.7 | 0.5 | 1.2 | PASS | 4 | Repeat 57 x 58 px, 4 colours. Press ENTER to wear it! |
| Dual-tone Check | 57 | 5 | 57.0 | 57.8 | 0.1 | 1.4 | PASS | 2 | Repeat 57 x 58 px, 2 colours. Press ENTER to wear it! |
| Dual-tone Check | 57 | 12 | 58.3 | 58.7 | 2.3 | 3.0 | FAIL | 2 | Repeat 58 x 59 px, 2 colours. Press ENTER to wear it! |
| Dual-tone Check | 83 | 0 | 83.6 | 83.6 | 0.8 | 0.7 | PASS | 2 | Repeat 84 x 84 px, 2 colours. Press ENTER to wear it! |
| Tartan | 40 | 0 | 40.0 | 40.0 | 0.0 | 0.0 | PASS | 4 | Repeat 40 x 40 px, 4 colours. Press ENTER to wear it! |
| Tartan | 57 | 0 | 56.7 | 57.6 | 0.5 | 1.1 | PASS | 4 | Repeat 57 x 58 px, 4 colours. Press ENTER to wear it! |
| Tartan | 57 | 5 | 56.9 | - | 0.1 | - | PASS | 4 | No clear repeat down. Set it with the arrow keys, or select a flatter area. |
| Tartan | 57 | 12 | - | - | - | - | PASS | 4 | No clear repeat found. Set it with the arrow keys, or select a flatter area. |
| Tartan | 83 | 0 | 83.6 | 83.5 | 0.7 | 0.7 | PASS | 3 | Repeat 84 x 84 px, 3 colours. Press ENTER to wear it! |
| Plain | 40 | 0 | - | - | - | - | PASS | 2 | Repeat 32 x 32 px, 2 colours. Press ENTER to wear it! |
| Plain | 57 | 0 | - | - | - | - | PASS | 2 | Repeat 32 x 32 px, 2 colours. Press ENTER to wear it! |
| Plain | 57 | 5 | - | - | - | - | PASS | 2 | Repeat 32 x 32 px, 2 colours. Press ENTER to wear it! |
| Plain | 57 | 12 | - | - | - | - | PASS | 2 | Repeat 32 x 32 px, 2 colours. Press ENTER to wear it! |
| Plain | 83 | 0 | - | - | - | - | PASS | 2 | Repeat 32 x 32 px, 2 colours. Press ENTER to wear it! |

Correct: 24 / 25 (period within 3%, or correctly uniform)

Notes:

- Straight photos: the repeat is found within 0.0 - 1.2% in every case.
- Rotated photos are refused by the shift check, except a 12 degree gingham that is
  accepted with about 3% error (gingham is symmetric, so the tile still looks right).
- A lungi that is being worn (folds, perspective) is refused: the method needs
  the cloth laid flat and photographed straight from above.
