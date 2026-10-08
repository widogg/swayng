# swayng

swayng is an opinionated [sway]-compatible [Wayland] compositor.

## Origins and rationale

The developers behind [sway] have the philosophy to make sway 100% compatible with
i3 and actively disregard any issue/PR that collides with this philosophy. Things
like transparency, rounded borders, animations, fake fullscreen will not be merged
into sway.

SwayNG is a lousy attempt to free from those constraints. We try to be compatible
with sway, while adding modern eye-candy features.

## Base: SwayFX and SceneFX

swayng is built on top of [SwayFX], which is sway 1.12 rendered through [SceneFX]
(the wlroots scene API with blur, shadows and rounded corners) on stock
wlroots 0.20. Earlier versions carried a patched wlroots with features borrowed
from [Scroll]; that fork is gone.

### Features from SwayFX

All are off by default; see `man 5 swayng`.

* Rounded corners: `corner_radius <px>`, `smart_corner_radius`
* Dim unfocused windows: `default_dim_inactive`, `dim_inactive_colors.*`
* Blur: `blur`, `blur_passes`, `blur_radius`, …
* Shadows: `shadows`, `shadow_blur_radius`, `shadow_color`, …
* Animations: `animation_duration_ms`
* Layer shell effects: `layer_effects <namespace> <effects>`
* `titlebar_separator`, `scratchpad_minimize`

### swayng features

* **Outermost-tile rounding**: only the outer corners of a tile round. Tabbed and
  stacked title bars, nested containers and window junctions stay square.
* **Title bar icons**: application icon left of the title. Enabled by default;
  disable with `title_window_icon off`.
* **Application (fake) fullscreen**: `fullscreen application toggle` tells the
  client it is fullscreen while the window stays tiled. Plain `fullscreen toggle`
  no longer forces client fullscreen.

### Migrating from the patched-wlroots swayng

| Old | New |
|-----|-----|
| `default_decoration border_radius 12` | `corner_radius 12` |
| `default_decoration dim true dim_color #00000025` | `default_dim_inactive 1.0` and `dim_inactive_colors.unfocused #00000025` |
| `decoration dim ...` (per window) | `for_window [...] dim_inactive <0.0-1.0>` |
| `titlebar_border_radius` | follows `corner_radius` |

`corner_radius` raises the title bar padding to at least the radius.

## AI coding in the year 2026

I'm not going to lie here, I'm not a developer. I can _read_ code to some degree,
but my skills are not good enough to code all of this. But in this day, we have AI.

### AI pull requests

It would be very hypocritical of me to not accept AI code. However, I will not accept
AI slop. You are still responsible for the code you submit.

In other words: I refuse to spend my time and tokens to review and understand code
you did not bother to spend your own time and tokens to understand.

## Compiling from source

Dependencies: meson, wayland, wayland-protocols, pcre2, json-c, pango, cairo,
xcb-ewmh, gdk-pixbuf2 (optional), scdoc (optional). wlroots 0.20 and SceneFX are
fetched as meson subprojects (`subprojects/*.wrap`) when not installed.

    meson setup build/
    ninja -C build/
    sudo ninja -C build/ install

### Following upstream

* swayng commits sit on top of SwayFX: `git fetch swayfx && git rebase swayfx/master`.
* `subprojects/scenefx.wrap` pins the SceneFX commit from SwayFX's `flake.lock`;
  bump it with SwayFX, then `meson subprojects update`.
* `subprojects/wlroots.wrap` pins the wlroots 0.20 release tag.

## Configuration

If you already use i3 or sway, copy your config to `~/.config/swayng/config` and
it'll work out of the box. Otherwise, copy the sample configuration file from
`/etc/swayng/config`. Run `man 5 swayng` for the configuration reference.

## Running

Run `swayng` from a TTY or from a display manager.

[sway]: https://github.com/swaywm/sway
[Wayland]: https://wayland.freedesktop.org/
[SwayFX]: https://github.com/wlrfx/swayfx
[SceneFX]: https://github.com/wlrfx/scenefx
[Scroll]: https://github.com/dawsers/scroll
