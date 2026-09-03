#pragma once

// beanwm — dwm-style user configuration defaults
// Tracked file: edit values here, then `cp include/config.def.h include/config.h` and rebuild.
// See .opencode/docs/dwm-config.md for dwm 6.8 inspiration (HIGH confidence):
//   https://git.suckless.org/dwm/plain/config.def.h  — borderpx/snap/tags/MODKEY/termcmd/keys/TAGKEYS
//   https://git.suckless.org/dwm/plain/Makefile        — `${OBJ}: config.h` + `config.h: cp config.def.h $@`
// C++23 adaptation: dwm uses `static const` / `#define`; beanwm uses `inline constexpr`
// for type-safe, header-only ODR-safe constants. Macro fallback (`#define GAP 5`) also works
// but `inline constexpr` is preferred. See docs for trade-off.
// User-editable copy: `include/config.h` (untracked, gitignored, auto-created by Makefile).

#include <X11/Xlib.h>
#include <X11/keysym.h>

// ── Appearance ───────────────────────────────────────────────────────────────
// Gap between windows — uniform: outer gap (screen edge → window) == inner gap
// (window → window) == GAP pixels. Mirrors beanwm uniform-gap fix where
// `gap, gap, screen-2*gap` inset + `(old.width - gap)/2` split gives outer==inner.
// Change 5 → 10 for more breathing room: `make clean && make` after edit.
inline constexpr int GAP = 5;

// ── Workspaces (tags) ──────────────────────────────────────────────────────
// Number of workspaces, like dwm `static const char *tags[] = {"1",...,"9"}`.
// Beanwm uses integer workspaces 1..WORKSPACE_COUNT (default 9).
// Keybindings Mod+1..9 switch, Mod+Shift+1..9 move are generated from this count.
// If you change this, ensure XK_1..XK_9 coverage still makes sense (loop handles it).
inline constexpr int WORKSPACE_COUNT = 9;

// Optional dwm-style tag labels (not yet consumed — integer workspaces used).
// Leave as extension point for future `tags[]` display or EWMH _NET_WM_DESKTOP_NAMES.
// Mirrors dwm: `static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };`
// inline constexpr const char* TAGS[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

// ── Modifier key ───────────────────────────────────────────────────────────
// Modifier for all WM shortcuts — dwm ` #define MODKEY Mod1Mask` (Alt).
// Beanwm default = Mod4Mask (Super/Windows) to avoid Alt conflicts.
// Alternative: `inline constexpr unsigned int MODKEY = Mod1Mask;` for Alt.
// Consumed as `MODKEY` → WindowManager::MOD_MASK and XGrabKey modifier.
inline constexpr unsigned int MODKEY = Mod4Mask; // Alt alternative: Mod1Mask

// ── Terminal ───────────────────────────────────────────────────────────────
// Spawned on Mod+Return via fork+execlp. Mirrors dwm `static const char *termcmd[] = { "st", NULL };`
// Beanwm uses constexpr array for type safety; `spawnTerminal()` does `execlp(TERMINAL, TERMINAL, nullptr)`.
inline constexpr const char TERMINAL[] = "alacritty";
// Null-terminated argv for exec — mirrors dwm termcmd
inline constexpr const char* TERMINAL_CMD[] = { "alacritty", nullptr };

// ── Keybinding hints (consumed in window_manager.cpp setupKeybindings) ────
// Default bindings generated from the constants above (no need to edit here
// unless you want full dwm-style `static const Key keys[]` table):
//   MODKEY + Return        → spawn terminal (TERMINAL)
//   MODKEY + 1..9          → switchWorkspace(n)
//   MODKEY + Shift + 1..9  → moveToWorkspace(n)  (focused window → workspace n)
//   MODKEY + Shift + q     → quit WM
// See .opencode/docs/dwm-config.md keys/TAGKEYS section for dwm original.

// ── Future tunables (extension points, placeholders) ───────────────────────
// Uncomment/define when you need them — mirrors dwm appearance knobs:
// inline constexpr unsigned int BORDER_PX = 1; // dwm `borderpx`
// inline constexpr unsigned int SNAP      = 32; // dwm `snap`
// inline constexpr int SHOW_BAR           = 1;  // dwm `showbar`
// inline constexpr int TOP_BAR            = 1;  // dwm `topbar`
// inline constexpr float MFACT            = 0.55; // dwm `mfact`  master area factor
// inline constexpr int NMASTER            = 1;   // dwm `nmaster` windows in master
