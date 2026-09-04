# beanwm-v2

Minimal X11 tiling window manager. i made this after just scrolling around in [queue](https://queue.halceon.dev/) and clicking on one of its creators and seeing their project https://github.com/simon0302010/gridwm

# features
- muti screen support (might be a bit choppy. wip)
- floating and tiled modes with basic functionality
- hot reload runtime config 
- smol codebase

## Build

```bash
make clean && make
Xephyr -ac -br -glamor_gles2 -screen 1280x720 :2 # makes a x server on :2
make test                     # DISPLAY=:2 ./build/beanwm
```

Build flags: `g++ -Wall -Wextra -Wpedantic -std=c++23 -g -O0 -fsanitize=address,undefined -Iinclude -lX11`

## installation (only for arch right now) 
if you want to just install this as a package for now (will upload to aur soon. its ready)

```sh
makepkg -si
```

this will install using pacman and you can easily delete to using 

```sh
pacman -R beanwm
```

## Default Configuration

| Tunable | Default | Description |
|---------|---------|-------------|
| `GAP` | `5` | Uniform gap |
| `WORKSPACE_COUNT` | `9` | Workspaces 1..9 |
| `MODKEY` | `Mod4Mask` | Super (or `Mod1Mask` for Alt) |
| `TERMINAL` | `"alacritty"` | Spawned on Mod+Return |

Changes take effect after hot reloading the configuration using mod shift r. check out the config in `$XDG_HOME/.config/beanwm/config` for more info

theres also an autostart section in the config where you can put stuff like polybar picom and feh (i use these) 
mine looks exec bash autostart and autostart being a script to initalize my stuff. pretty neat

## Keybindings

Default keybindings are in the config file

| Keys | Action |
|------|--------|
| `Mod + Return` | Spawn terminal |
| `Mod + 1 .. 9` | Switch workspace |
| `Mod + Shift + 1 .. 9` | Move focused window to workspace |
| `Mod + Shift + q` | Quit WM |
| `Mod + Shift + r` | Soft reload WM |
| `Mod + Shift + f` | Toggle focused window to floating |

`Mod` = `Mod4Mask` (Super) by default, changeable via `modkey` in `config`.


<h2> Ai was used to try and optimize the code and refactor it multiple times </h2>