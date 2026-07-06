# swayng

swayng is an opinionated [sway]-compatible [Wayland] compositor. 

## Origins and rationale

The developers behind [sway](https://github.com/swaywm/sway) have the philosophy to make sway 100% compatible with
i3 and actively disregard any issue/PR that collides with this philosophy. Things
like transparency, rounded borders, animations, fake fulscreen will not be merged 
into sway.

SwayNG is a lousy attemp to free from those constrains. We try to be compatible 
with sway, while adding modern eye-candy features.

## Dawser's Scroll and source code

Before getting into my own fork and modifications, I tried [Niri](https://github.com/niri-wm/niri) and 
[Scroll](https://github.com/dawsers/scroll), but they didn't click with my workflow. I feel much more confortable
with I3.

At the same time, as much as I liked my I3 setup and X11 in general, it's starting
to show it's age, specially on modern displays (HiDPI, mixed resolutions, color profiles)

Scroll is a very nice modern iteration over Sway, it has transparency, dim
animations, rounded borders and more. So I decided to borrow its code (the parts
I like) and merge it back into Sway. The result, a modernized Sway.

### Features ported from Scroll

I've decided to include only a sub-set of scroll features, and only the ones I
like and care about. This is the list
* wlroots: There are differences in Scroll that serve as a basis for the rest of the changes
* Rounded borders, with some quirks in the titlebar
* Dim unfocused windowss
* Fake Fullscreen

### Features _not_ ported

This are things I don't mind having, so I didn't even bothered
* Animations. Actually, I do mind, but they were super erratic and removed them
* Jump/trails/marks/overview
* Alignment and movement

## AI coding in the year 2026
I'm not going to lie here, I'm not a developer. I can _read_ code to some degree, 
but my skills are not good enough to code all of this. But in this day, we have AI.

I have requested my bots to port the features I like, by reading (locally) from 
both repos.

### AI pull requests

It would be very hyprocrit from me to not accept AI code. However, I will not accept
AI slop. You are still responsible for the code you submit.

In other words: I refuse to spend my time and tokens to review and understand code 
you did not bother to spend your own time and tokens to understand.

## Installation

### From Packages

Sway is available in many distributions. Try installing the "sway" package for
yours.

### Compiling from Source

Check out [this wiki page][Development setup] if you want to build the HEAD of
sway and wlroots for testing or development.

Install dependencies:

* meson \*
* [wlroots]
* wayland
* wayland-protocols \*
* pcre2
* json-c
* pango
* cairo
* gdk-pixbuf2 (optional: additional image formats for system tray)
* [swaybg] (optional: wallpaper)
* [scdoc] (optional: man pages) \*
* git (optional: version info) \*

_\* Compile-time dep_

Run these commands:

    meson setup build/
    ninja -C build/
    sudo ninja -C build/ install

## Configuration

If you already use i3 or sway, then copy your  config to `~/.config/swayng/config` and
it'll work out of the box. Otherwise, copy the sample configuration file to
`~/.config/swayng/config`. It is usually located at `/etc/swayng/config`.
Run `man 5 swayng` for information on the configuration.

## Running

Run `swayng` from a TTY or from a display manager.

[i3]: https://i3wm.org/
[Wayland]: http://wayland.freedesktop.org/
[FAQ]: https://github.com/swaywm/sway/wiki
[IRC channel]: https://web.libera.chat/gamja/?channels=#sway
[GitHub releases]: https://github.com/swaywm/sway/releases
[Development setup]: https://github.com/swaywm/sway/wiki/Development-Setup
[wlroots]: https://gitlab.freedesktop.org/wlroots/wlroots
