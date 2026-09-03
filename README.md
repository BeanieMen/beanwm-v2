# beanwm-v2

Minimal X11 tiling WM, dwindle layout, vector-based — dwm-inspired, C++23.

## Build

```bash
make clean && make            # -> build/beanwm
DISPLAY=:2 ./build/beanwm     # run under Xephyr :2
make test                     # DISPLAY=:2 ./build/beanwm
```

Build flags: `g++ -Wall -Wextra -Wpedantic -std=c++23 -g -O0 -fsanitize=address,undefined -Iinclude -lX11`

## Configuration (dwm-style)

beanwm uses dwm's `config.def.h → config.h` pattern. Inspired by dwm 6.8:

- https://git.suckless.org/dwm/plain/config.def.h
- https://git.suckless.org/dwm/plain/Makefile (`${OBJ}: config.h` + `config.h: cp config.def.h $@`)
- Details: `.opencode/docs/dwm-config.md` (HIGH confidence)

### Quick start

```bash
# first build auto-creates include/config.h from include/config.def.h
make

# edit tunables
vi include/config.h

# rebuild after editing
make clean && make
```

Manual fallback (if auto-create disabled):

```bash
cp include/config.def.h include/config.h
# edit include/config.h
make
```

Reset to defaults:

```bash
make config          # cp include/config.def.h include/config.h
# or
cp include/config.def.h include/config.h
```

### Tunables (`include/config.def.h` → `include/config.h`)

Tracked defaults `include/config.def.h`, untracked user copy `include/config.h` (gitignored via `.gitignore: include/config.h`). All values are `inline constexpr` (C++23, header-only ODR-safe); macro fallback `#define GAP 5` also works.

| Tunable | Default | Description |
|---------|---------|-------------|
| `GAP` | `5` | Uniform gap (pixels) — outer (screen edge → window) == inner (window → window). Change `5 → 10` for more breathing room. Mirrors beanwm uniform-gap fix (`gap,gap,screen-2*gap` inset + `(old.width-gap)/2` split). |
| `WORKSPACE_COUNT` | `9` | Number of workspaces (integer 1..9, like dwm `tags[] = {"1",...,"9"}`). Keybindings `Mod+1..9` / `Mod+Shift+1..9` are generated via loop `for i=1..WORKSPACE_COUNT`. |
| `MODKEY` | `Mod4Mask` | Modifier for all shortcuts (Super/Windows). Alternative: `Mod1Mask` (Alt) — change to `inline constexpr unsigned int MODKEY = Mod1Mask;`. Consumed as `WindowManager::MOD_MASK` and `XGrabKey` modifier. |
| `TERMINAL` | `"alacritty"` | Spawned on `Mod+Return` via `fork+execlp(TERMINAL, TERMINAL, nullptr)`. Mirrors dwm `static const char *termcmd[] = {"st", NULL};`. Also `TERMINAL_CMD[] = {"alacritty", nullptr}` for argv form. |
| `BORDER_PX` / `SNAP` / `SHOW_BAR` | commented | Extension points — uncomment to enable (mirrors dwm `borderpx`, `snap`, `showbar`, `mfact`, `nmaster`). |

Example — change gap:

```cpp
// include/config.h
inline constexpr int GAP = 10; // was 5
```
```bash
make clean && make
```

### .gitignore

```
build
include/config.h   # user copy — never committed
.opencode
.vscode
```

`include/config.def.h` is tracked (commit your defaults). `include/config.h` is ignored (personal overrides).

## Keybindings

Generated in `src/window_manager.cpp::setupKeybindings()` from config (dwm `TAGKEYS` style loop):

| Keys | Action |
|------|--------|
| `Mod + Return` | Spawn terminal (`TERMINAL`) |
| `Mod + 1 .. 9` | Switch workspace `1..WORKSPACE_COUNT` |
| `Mod + Shift + 1 .. 9` | Move focused window to workspace `n` |
| `Mod + Shift + q` | Quit WM (`XCloseDisplay` + `exit(0)`) |

`Mod` = `MODKEY` (`Mod4Mask` = Super by default, `Mod1Mask` = Alt if you change config).

## Project Structure

```
include/
  types.h            # Client {Window, int workspace} (no next), Area (non-owning)
  window_manager.h   # vector<Client> ownership, config-driven WORKSPACE_COUNT/MOD_MASK
  config.def.h       # tracked defaults (GAP, WORKSPACE_COUNT, MODKEY, TERMINAL)
  config.h           # untracked user copy (auto-created)
src/
  main.cpp           # WindowManager wm; wm.run();
  tile.cpp           # dwindleTile(Display*, const vector<Client>&, int gap, int workspace)
  window_manager.cpp # X11 event loop, add/remove/find (find_if/erase), tile/switch/move
build/beanwm         # binary (ASan+UBSan)
```

## Notes

- Vector refactor (M1-M3): `std::vector<Client>` owns clients — no `malloc`/`free`/`next` leaks, ASan clean. Uniform gap fix: outer==inner==GAP.
- Config refactor (M4-M6): hardcodes removed (`dwindleTile(...,5,...)`, `WORKSPACE_COUNT=9` literal, `Mod4Mask`, `"alacritty"`) → now `GAP`/`WORKSPACE_COUNT`/`MODKEY`/`TERMINAL` from `config.h`.
