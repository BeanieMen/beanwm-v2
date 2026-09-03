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

beanwm has two layers — compiled defaults and runtime editable config (i3-like).

### Runtime config — `config` (i3-like, no recompilation)

A plain-text file at `./config` (project root) — editable like i3's config, but local for now (not in `~/.config` per request).

**Edit and restart — no `make` needed:**

```bash
vi config          # gap, mod, terminal, workspaces, bindsym
DISPLAY=:2 ./build/beanwm   # restart to apply
```

**Example `config`:**

```
# gap between windows (outer == inner)
gap 5

# modifier: Mod4 = Super, Mod1 = Alt
mod Mod4

# terminal
terminal alacritty

# workspaces
workspaces 9

# keybinds — i3-like bindsym
bindsym Mod4+Return exec terminal
bindsym Mod4+1 workspace 1
bindsym Mod4+Shift+1 move 1
bindsym Mod4+Shift+q quit
```

Supported directives:
- `gap <int>` — uniform gap (pixels)
- `mod <Mod4|Mod1>` — Super or Alt
- `terminal <cmd>` — e.g., `alacritty`, `kitty`
- `workspaces <int>` — 1..20
- `bindsym <Mod>+<key> <action>` — actions: `exec terminal` / `exec <cmd>`, `workspace <n>`, `move <n>`, `quit`

If `config` is missing, beanwm falls back to compiled defaults and prints `No runtime config found...`.

### Compiled defaults — `include/config.def.h` → `include/config.h`

Tracked defaults `include/config.def.h` (commit), untracked user copy `include/config.h` (gitignored, auto-created on first `make`).

```bash
# first build auto-creates include/config.h
make
vi include/config.h   # GAP, WORKSPACE_COUNT, MODKEY, TERMINAL
make clean && make
make config           # reset to defaults
```

| Tunable | Default | Description |
|---------|---------|-------------|
| `GAP` | `5` | Uniform gap |
| `WORKSPACE_COUNT` | `9` | Workspaces 1..9 |
| `MODKEY` | `Mod4Mask` | Super (or `Mod1Mask` for Alt) |
| `TERMINAL` | `"alacritty"` | Spawned on Mod+Return |

Runtime `config` overrides these at startup.

## Keybindings

Default generated from config (or `config` bindsym lines if present):

| Keys | Action |
|------|--------|
| `Mod + Return` | Spawn terminal |
| `Mod + 1 .. 9` | Switch workspace |
| `Mod + Shift + 1 .. 9` | Move focused window to workspace |
| `Mod + Shift + q` | Quit WM |

`Mod` = `Mod4Mask` (Super) by default, changeable via `mod Mod1` in `config` or `MODKEY` in `config.h`.

## Project Structure

```
include/
  types.h            # Client {Window, int workspace}, Area
  window_manager.h   # vector<Client> ownership
  config.def.h       # tracked compiled defaults
  config.h           # untracked user copy (auto-created)
config               # runtime editable config (i3-like, no recompile)
src/
  main.cpp           # WindowManager wm; wm.run();
  tile.cpp           # dwindleTile with uniform gap
  window_manager.cpp # X11 events, runtime config loader
build/beanwm         # binary (ASan+UBSan)
```

## Notes

- Vector-based ownership — no `malloc`/`free`, uniform gaps, ASan clean.
- Runtime `config` is unique to beanwm — i3-inspired but project-local for now (future: `~/.config/beanwm/config`).
