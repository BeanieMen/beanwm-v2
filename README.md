# beanwm-v2

Minimal X11 tiling window manager — dwindle layout, vector-based, C++23.

## Build

```bash
make clean && make            # -> build/beanwm
DISPLAY=:2 ./build/beanwm     # run under Xephyr :2
make test                     # DISPLAY=:2 ./build/beanwm
```

Build flags: `g++ -Wall -Wextra -Wpedantic -std=c++23 -g -O0 -fsanitize=address,undefined -Iinclude -lX11`

## Configuration

### Compiled configuration — `include/config.def.h` → `include/config.h`

Tracked defaults `include/config.def.h` (commit), untracked user copy `include/config.h` (gitignored, auto-created on first `make`).

```bash
# first build auto-creates include/config.h
make
vi include/config.h   # GAP, WORKSPACE_COUNT, MODKEY, TERMINAL, DEFAULT_BINDS
make clean && make
make config           # reset to defaults
```

| Tunable | Default | Description |
|---------|---------|-------------|
| `GAP` | `5` | Uniform gap |
| `WORKSPACE_COUNT` | `9` | Workspaces 1..9 |
| `MODKEY` | `Mod4Mask` | Super (or `Mod1Mask` for Alt) |
| `TERMINAL` | `"alacritty"` | Spawned on Mod+Return |

Changes take effect after hot reloading the configuration.

## Keybindings

Keybindings are compiled from `include/config.h`:

| Keys | Action |
|------|--------|
| `Mod + Return` | Spawn terminal |
| `Mod + 1 .. 9` | Switch workspace |
| `Mod + Shift + 1 .. 9` | Move focused window to workspace |
| `Mod + Shift + q` | Quit WM |
| `Mod + Shift + f` | Toggle focused window to floating |

Hold `Mod` and drag with the left mouse button to move a floating window or swap a tiled window with the window under the pointer.

`Mod` = `Mod4Mask` (Super) by default, changeable via `MODKEY` in `config.h`.

## Project Structure

```
include/
  types.h            # Client {Window, int workspace}, Area
  window_manager.h   # vector<Client> ownership
  config.def.h       # tracked compiled defaults
  config.h           # untracked user copy (auto-created)
src/
  main.cpp           # WindowManager wm; wm.run();
  management.cpp     # dwindleTile with uniform gap
  window_manager.cpp # X11 events, dragging, and window management
  keybindings.cpp    # compiled keybinding parsing and actions
build/beanwm         # binary (ASan+UBSan)
```

## Notes

- Vector-based ownership — no `malloc`/`free`, uniform gaps, ASan clean.
- Configuration is loaded at runtime; use the `reload_config` binding after changing it.
