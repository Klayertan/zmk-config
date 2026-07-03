# Corne Supernano ZMK Config

Custom ZMK firmware for a Corne build with:

- animated anime OLED status art on the left half
- connection/layer/key-press status on the right half
- RGB underglow enabled at startup
- an `ADJUST` lighting layer when `LOWER` and `RAISE` are held together
- Japanese input keys on the lower layer

## OLED Screens

The OLED firmware is side-specific:

| Half | Display |
| --- | --- |
| Left | Dancing monochrome anime girl pixel art |
| Right | Connection status, active layer, last key label, and a tiny key-position grid |

For the setup where the right half is your main/USB side, flash:

- left keyboard: `corne_left.uf2`
- right keyboard: `corne_right_central.uf2`

For a normal left-central setup, flash:

- left keyboard: `corne_left.uf2`
- right keyboard: `corne_right.uf2`

OLEDs are 128x32 monochrome, so the anime art is pixel art rather than a full-color image.

## RGB Shortcuts

Hold both thumb layer keys:

```text
LOWER + RAISE
```

Then tap one of these keys:

| Key | Action |
| --- | --- |
| `Q` | Toggle LEDs on/off |
| `W` | Previous RGB effect |
| `E` | Next RGB effect |
| `R` | Slower effect animation |
| `T` | Faster effect animation |
| `A` | Dimmer |
| `S` | Brighter |
| `D` | Hue down |
| `F` | Hue up |
| `G` | Saturation down |
| `H` | Saturation up |
| `Y` | Purple preset |
| `U` | Cyan preset |
| `I` | Pink preset |
| `O` | Red preset |
| `P` | Green preset |
| `Backspace` | Blue preset |
| `J` | Warm preset |
| `K` | Ice preset |
| `L` | Dim purple preset |
| `Z` | Red preset |
| `X` | Orange preset |
| `C` | Yellow preset |
| `V` | Green preset |
| `B` | Blue preset |
| `N` | Teal preset |
| `M` | Violet preset |
| `,` | LEDs off |
| `.` | LEDs on |

You can still use the older quick RGB controls on the `RAISE` layer:

- `RAISE + A`: toggle LEDs
- `RAISE + S`: next effect
- `RAISE + X`: previous effect
- `RAISE + D/F/G`: hue up / saturation up / brightness up
- `RAISE + C/V/B`: hue down / saturation down / brightness down

## Flashing

1. Push changes to GitHub.
2. Open the repo's **Actions** tab.
3. Download the newest firmware artifact after the build passes.
4. Double-tap reset on one keyboard half.
5. Drag the matching `.uf2` file onto the mounted bootloader drive.
6. Repeat for the other half.

The current build matrix creates:

- `corne_left`
- `corne_right`
- `corne_right_central`
