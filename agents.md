# Guidance for Coding Agents

This document provides context and instructions for automated coding agents (e.g.
GitHub Copilot coding agent) working on this repository.

## UI Screenshot Verification

Every CI run produces a **`ui-screenshots` artifact** containing PPM screenshots
of the simulator rendered at each supported board resolution:

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

The screenshots are captured from the simulator's login page (shown after the
3-second splash screen) by running each board variant with
`SDL_VIDEODRIVER=offscreen` and the `--screenshot` flag.  To regenerate them
locally, run:

```bash
just sim-build <board>   # e.g. just sim-build crowpanel_101
SDL_VIDEODRIVER=offscreen SDL_RENDER_DRIVER=software \
  ./simulator/build/kern_simulator \
  --screenshot /tmp/screenshot.ppm \
  --screenshot-delay 5000
```

PPM files can be viewed with most image viewers (e.g. `feh`, `eog`, GIMP, or
converted to PNG with `convert screenshot.ppm screenshot.png`).

## Supported Board Resolutions

The board → resolution mapping is maintained in the `justfile` (`_sim_h_res` /
`_sim_v_res` recipes) and mirrored in the CI workflow
(`.github/workflows/github-actions-test.yml`).  Keep these in sync when adding
new board profiles.

## Code Style

Run `./scripts/format.sh` before committing.  The CI `format-check` job will
reject unformatted code.
