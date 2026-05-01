# Guidance for Coding Agents

This document provides context and instructions for automated coding agents (e.g.
GitHub Copilot coding agent) working on this repository.

## UI Screenshot Verification

Every CI run produces a **`ui-screenshots` artifact** containing PNG screenshots
of every screen and menu in the simulator, captured at each supported board
resolution.  Subdirectories are named `<board>_<W>x<H>/` and each contains 13
numbered PNGs:

| File                       | Screen shown                  |
|----------------------------|-------------------------------|
| `01_splash.png`            | Animated splash screen        |
| `02_login.png`             | Login menu                    |
| `03_pin_entry.png`         | PIN unlock screen             |
| `04_about.png`             | About page                    |
| `05_login_settings.png`    | Login settings                |
| `06_new_mnemonic_menu.png` | New mnemonic menu             |
| `07_load_mnemonic_menu.png`| Load mnemonic menu            |
| `08_home.png`              | Home menu (demo key loaded)   |
| `09_addresses.png`         | Addresses (with QR codes)     |
| `10_backup_menu.png`       | Backup menu                   |
| `11_mnemonic_qr.png`       | Mnemonic QR code backup       |
| `12_mnemonic_words.png`    | Mnemonic words backup         |
| `13_public_key.png`        | Extended public key (xpub)    |

Boards and resolutions:

| Board           | Resolution  |
|-----------------|-------------|
| wave_4b         | 720 × 720   |
| wave_35         | 320 × 480   |
| wave_5          | 720 × 1280  |
| crowpanel_101   | 1024 × 600  |

**Agents must examine these screenshots after any UI change** to confirm that the
layout scales correctly on all supported resolutions before merging.  In
particular, look for:

- QR codes clipped, overlapping other elements, or illegibly small.
- Text cut off at screen edges or overflowing its container.
- Buttons or icons misaligned or pushed off-screen.
- Content that looks correct on a square/portrait screen but breaks on landscape
  (1024 × 600) or on the small 320 × 480 display.

To regenerate screenshots locally, run:

```bash
just sim-build <board>   # e.g. just sim-build crowpanel_101
mkdir -p /tmp/screenshots
SDL_VIDEODRIVER=offscreen SDL_RENDER_DRIVER=software \
  ./simulator/build/kern_simulator \
  --screenshot-tour /tmp/screenshots
```

The `--screenshot-tour` flag navigates through every screen/menu automatically,
saves one PNG per screen, then exits.  The wallet screens use the standard BIP39
all-abandon test vector (never use for real funds).

A single-screenshot mode is still available for quick checks:

```bash
SDL_VIDEODRIVER=offscreen SDL_RENDER_DRIVER=software \
  ./simulator/build/kern_simulator \
  --screenshot /tmp/screenshot.png \
  --screenshot-delay 5000
```

## Supported Board Resolutions

The board → resolution mapping is maintained in the `justfile` (`_sim_h_res` /
`_sim_v_res` recipes) and mirrored in the CI workflow
(`.github/workflows/github-actions-test.yml`).  Keep these in sync when adding
new board profiles.

## Code Style

Run `./scripts/format.sh` before committing.  The CI `format-check` job will
reject unformatted code.
