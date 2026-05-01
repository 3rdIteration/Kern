/**
 * Kern Desktop Simulator — Entry Point
 *
 * Mirrors the initialization sequence from main/main.c but uses
 * SDL2 for display and mouse input instead of ESP32-P4 hardware.
 */

#include "lvgl.h"
#include "src/drivers/sdl/lv_sdl_window.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "ui/theme.h"
#include "ui/assets/kern_logo_lvgl.h"
#include "core/settings.h"
#include "core/pin.h"
#include "core/session.h"
#include "core/key.h"
#include "core/wallet.h"
#include "pages/pin/pin_page.h"
#include "pages/login/login.h"
#include "pages/login/about.h"
#include "pages/login/login_settings.h"
#include "pages/new_mnemonic/new_mnemonic_menu.h"
#include "pages/load_mnemonic/load_menu.h"
#include "pages/home/home.h"
#include "pages/home/addresses.h"
#include "pages/home/public_key.h"
#include "pages/home/backup/backup_menu.h"
#include "pages/home/backup/mnemonic_qr.h"
#include "pages/home/backup/mnemonic_words.h"
#include "ui/nav.h"
#include "esp_lvgl_port.h"
#include "utils/bip39_filter.h"
#include <wally_core.h>
#include <nvs_flash.h>
#include <esp_err.h>
#include "sim_video.h"
#include "sim_nvs.h"
#include "sim_sdcard.h"
#include <bsp/pmic.h>
#include <SDL2/SDL.h>
#include <errno.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/mman.h>
#if defined(__linux__)
#include <sys/prctl.h>
#endif
#include <sys/stat.h>
#include "esp_log.h"
#include "../platform/video_sim/stb_image_write.h"

#ifndef SIM_LCD_H_RES
#define SIM_LCD_H_RES 720
#endif
#ifndef SIM_LCD_V_RES
#define SIM_LCD_V_RES 720
#endif

/* -------------------------------------------------------------------------- */
/* Screenshot support                                                          */
/* -------------------------------------------------------------------------- */

static char *screenshot_path = NULL;
static int   screenshot_delay_ms = 5000;

/* Write LVGL draw-buffer as a PNG file via stb_image_write. */
static void save_screenshot_png(const char *path, const lv_draw_buf_t *buf) {
    uint32_t w      = buf->header.w;
    uint32_t h      = buf->header.h;
    uint32_t stride = buf->header.stride;
    const void *data = buf->data;
    int ok = stbi_write_png(path, (int)w, (int)h, 3, data, (int)stride);
    if (!ok) {
        fprintf(stderr, "Screenshot: stbi_write_png failed for '%s'\n", path);
        return;
    }
    printf("Screenshot saved: %s (%ux%u)\n", path, w, h);
}

static void screenshot_timer_cb(lv_timer_t *timer) {
    lv_timer_delete(timer);
    lv_obj_t *scr = lv_screen_active();
    lv_draw_buf_t *snap = lv_snapshot_take(scr, LV_COLOR_FORMAT_RGB888);
    if (snap) {
        save_screenshot_png(screenshot_path, snap);
        lv_snapshot_free(snap);
        exit(0);
    } else {
        fprintf(stderr, "Screenshot: lv_snapshot_take failed\n");
        exit(1);
    }
}

/* -------------------------------------------------------------------------- */
/* Screenshot tour                                                             */
/*                                                                             */
/* Captures one PNG per major screen/menu so that agents and developers can   */
/* verify layout on every supported board resolution.  Activated via the      */
/* --screenshot-tour <dir> flag.  Screens that require a loaded key use the   */
/* standard BIP39 all-abandon test vector (public, never use for real funds). */
/* -------------------------------------------------------------------------- */

#define TOUR_INTER_STEP_MS 500

/* Standard BIP39 "all-abandon" 12-word public test vector. */
#define DEMO_MNEMONIC \
    "abandon abandon abandon abandon abandon abandon " \
    "abandon abandon abandon abandon abandon about"

static char tour_output_dir[512];

typedef enum {
    TOUR_IDLE = -1,
    TOUR_CAPTURE_SPLASH = 0,
    TOUR_CAPTURE_LOGIN,
    TOUR_CAPTURE_PIN,
    TOUR_CAPTURE_ABOUT,
    TOUR_CAPTURE_LOGIN_SETTINGS,
    TOUR_CAPTURE_NEW_MNEMONIC,
    TOUR_CAPTURE_LOAD_MNEMONIC,
    TOUR_CAPTURE_HOME,
    TOUR_CAPTURE_ADDRESSES,
    TOUR_CAPTURE_BACKUP_MENU,
    TOUR_CAPTURE_MNEMONIC_QR,
    TOUR_CAPTURE_MNEMONIC_WORDS,
    TOUR_CAPTURE_PUBLIC_KEY,
    TOUR_DONE,
} tour_state_t;

static tour_state_t tour_state = TOUR_IDLE;
static int tour_screenshot_count = 0;

static void tour_advance(lv_timer_t *timer);

static void tour_schedule_next(void) {
    lv_timer_t *t = lv_timer_create(tour_advance, TOUR_INTER_STEP_MS, NULL);
    lv_timer_set_repeat_count(t, 1);
}

static void tour_capture(const char *name) {
    char path[768];
    lv_refr_now(NULL);
    lv_obj_t *scr = lv_screen_active();
    lv_draw_buf_t *snap = lv_snapshot_take(scr, LV_COLOR_FORMAT_RGB888);
    if (!snap) {
        fprintf(stderr, "Tour: lv_snapshot_take failed for '%s'\n", name);
        return;
    }
    snprintf(path, sizeof(path), "%s/%s.png", tour_output_dir, name);
    save_screenshot_png(path, snap);
    lv_snapshot_free(snap);
    tour_screenshot_count++;
}

static void tour_reset_screen(void) {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);
    theme_apply_screen(scr);
    lv_refr_now(NULL);
}

static void tour_advance(lv_timer_t *timer) {
    (void)timer;
    lv_obj_t *scr = lv_screen_active();

    switch (tour_state) {
        case TOUR_CAPTURE_SPLASH:
            /* Splash animation is running — capture it. */
            tour_capture("01_splash");
            /* The existing splash_done_cb fires at 3000 ms from startup.
             * We were scheduled at 500 ms, so wait another 3500 ms so the
             * login page is fully rendered before we capture it. */
            tour_state = TOUR_CAPTURE_LOGIN;
            {
                lv_timer_t *t = lv_timer_create(tour_advance, 3500, NULL);
                lv_timer_set_repeat_count(t, 1);
            }
            return;

        case TOUR_CAPTURE_LOGIN:
            /* splash_done_cb has already created the login page. */
            tour_capture("02_login");
            break;

        case TOUR_CAPTURE_PIN:
            login_page_destroy();
            tour_reset_screen();
            pin_page_create(scr, PIN_PAGE_UNLOCK, NULL, NULL);
            lv_refr_now(NULL);
            tour_capture("03_pin_entry");
            pin_page_destroy();
            break;

        case TOUR_CAPTURE_ABOUT:
            tour_reset_screen();
            about_page_create(scr, NULL);
            about_page_show();
            lv_refr_now(NULL);
            tour_capture("04_about");
            about_page_destroy();
            break;

        case TOUR_CAPTURE_LOGIN_SETTINGS:
            tour_reset_screen();
            login_settings_page_create(scr, NULL);
            login_settings_page_show();
            lv_refr_now(NULL);
            tour_capture("05_login_settings");
            login_settings_page_destroy();
            break;

        case TOUR_CAPTURE_NEW_MNEMONIC:
            tour_reset_screen();
            new_mnemonic_menu_page_create(scr, NULL);
            new_mnemonic_menu_page_show();
            lv_refr_now(NULL);
            tour_capture("06_new_mnemonic_menu");
            new_mnemonic_menu_page_destroy();
            break;

        case TOUR_CAPTURE_LOAD_MNEMONIC:
            tour_reset_screen();
            load_menu_page_create(scr, NULL);
            load_menu_page_show();
            lv_refr_now(NULL);
            tour_capture("07_load_mnemonic_menu");
            load_menu_page_destroy();
            /* Load the demo key so that wallet screens are accessible. */
            if (!key_load_from_mnemonic(DEMO_MNEMONIC, "", false) ||
                !wallet_init(WALLET_NETWORK_MAINNET)) {
                fprintf(stderr, "Tour: demo key/wallet init failed\n");
                exit(1);
            }
            break;

        case TOUR_CAPTURE_HOME:
            tour_reset_screen();
            nav_init(scr);
            home_page_create(scr);
            home_page_show();
            lv_refr_now(NULL);
            tour_capture("08_home");
            break;

        case TOUR_CAPTURE_ADDRESSES:
            home_page_hide();
            addresses_page_create(scr, NULL);
            addresses_page_show();
            lv_refr_now(NULL);
            tour_capture("09_addresses");
            addresses_page_destroy();
            home_page_show();
            break;

        case TOUR_CAPTURE_BACKUP_MENU:
            home_page_hide();
            backup_menu_page_create(scr, NULL);
            backup_menu_page_show();
            lv_refr_now(NULL);
            tour_capture("10_backup_menu");
            /* Keep backup_menu alive; sub-pages are children of scr. */
            backup_menu_page_hide();
            break;

        case TOUR_CAPTURE_MNEMONIC_QR:
            mnemonic_qr_page_create(scr, NULL);
            mnemonic_qr_page_show();
            lv_refr_now(NULL);
            tour_capture("11_mnemonic_qr");
            mnemonic_qr_page_destroy();
            break;

        case TOUR_CAPTURE_MNEMONIC_WORDS:
            mnemonic_words_page_create(scr, NULL);
            mnemonic_words_page_show();
            lv_refr_now(NULL);
            tour_capture("12_mnemonic_words");
            mnemonic_words_page_destroy();
            backup_menu_page_destroy();
            home_page_show();
            break;

        case TOUR_CAPTURE_PUBLIC_KEY:
            home_page_hide();
            public_key_page_create(scr, NULL);
            public_key_page_show();
            lv_refr_now(NULL);
            tour_capture("13_public_key");
            public_key_page_destroy();
            home_page_destroy();
            key_unload();
            wallet_unload();
            break;

        case TOUR_DONE:
            printf("Screenshot tour complete: %d screenshots saved to %s/\n",
                   tour_screenshot_count, tour_output_dir);
            exit(0);

        default:
            break;
    }

    tour_state++;
    tour_schedule_next();
}

static void tour_start(void) {
    tour_state = TOUR_CAPTURE_SPLASH;
    /* Fire first capture at 500 ms (splash animation in progress). */
    lv_timer_t *t = lv_timer_create(tour_advance, 500, NULL);
    lv_timer_set_repeat_count(t, 1);
}

/* -------------------------------------------------------------------------- */
/* Forward declarations                                                        */
/* -------------------------------------------------------------------------- */
static void splash_done_cb(lv_timer_t *t);
static void post_unlock_cb(void);
static void session_expired_handler(void);

/* -------------------------------------------------------------------------- */
/* Session expiry handler                                                      */
/* -------------------------------------------------------------------------- */

static void session_expired_handler(void) {
    /* Lock device: clear screen and show PIN unlock page */
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);
    pin_page_create(scr, PIN_PAGE_UNLOCK, post_unlock_cb, NULL);
}

/* -------------------------------------------------------------------------- */
/* Post-unlock callback                                                        */
/* -------------------------------------------------------------------------- */

static void post_unlock_cb(void) {
    pin_page_destroy();

    /* Start session timeout */
    uint16_t timeout = pin_get_session_timeout();
    if (timeout > 0)
        session_start(timeout);

    /* Show login page (wallet selector / main menu) */
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);
    nav_init(scr);
    login_page_create(scr);
}

/* -------------------------------------------------------------------------- */
/* Splash → PIN transition (fired by one-shot LVGL timer after 3 s)          */
/* -------------------------------------------------------------------------- */

static void splash_done_cb(lv_timer_t *t) {
    lv_timer_delete(t);

    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    if (pin_is_configured()) {
        pin_page_create(scr, PIN_PAGE_UNLOCK, post_unlock_cb, NULL);
    } else {
        nav_init(scr);
        login_page_create(scr);
    }
}

/* -------------------------------------------------------------------------- */
/* Main                                                                        */
/* -------------------------------------------------------------------------- */

static void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS]\n\n", prog);
    printf("Kern Desktop Simulator\n\n");
    printf("Options:\n");
    printf("  -q, --qr-image <path>   Load QR image for camera simulation\n");
    printf("  -Q, --qr-dir <path>     Load QR images from directory\n");
    printf("  -d, --data-dir <path>   Data directory (default: simulator/sim_data/)\n");
    printf("  -W, --width <N>         Display width in pixels (default: %d)\n", SIM_LCD_H_RES);
    printf("  -H, --height <N>        Display height in pixels (default: %d)\n", SIM_LCD_V_RES);
    printf("  -w, --webcam [device]   Use webcam (default: /dev/video0)\n");
    printf("  -s, --screenshot <path>  Save a single screenshot (PNG) after --screenshot-delay ms then exit\n");
    printf("  -S, --screenshot-delay <ms>  Delay before screenshot (default: %d ms)\n", screenshot_delay_ms);
    printf("  -t, --screenshot-tour <dir>  Capture a PNG for every screen/menu into <dir> then exit\n");
    printf("  -v, --verbose           Enable DEBUG-level logging\n");
    printf("  -h, --help              Show this help\n");
}

int main(int argc, char *argv[]) {
    /* Restrict permissions on every file the simulator creates (NVS,
     * SD card files, etc.) so they cannot be read by other users. */
    umask(077);

    /* Best-effort: keep pages out of swap and core dumps so a real seed
     * accidentally typed into the simulator does not hit disk.  Both calls
     * are non-fatal — typical desktops cap RLIMIT_MEMLOCK low. */
    (void)mlockall(MCL_CURRENT | MCL_FUTURE);
#if defined(__linux__)
    (void)prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
#endif

    fprintf(stderr,
        "\n"
        "  \x1b[1;31m================================================================\x1b[0m\n"
        "  \x1b[1;31m  Kern SIMULATOR - developer build, DO NOT USE WITH REAL FUNDS\x1b[0m\n"
        "  \x1b[1;31m================================================================\x1b[0m\n"
        "\n");

    /* Parse CLI arguments before any init */
    static const struct option long_opts[] = {
        { "qr-image",         required_argument, NULL, 'q' },
        { "qr-dir",           required_argument, NULL, 'Q' },
        { "data-dir",         required_argument, NULL, 'd' },
        { "width",            required_argument, NULL, 'W' },
        { "height",           required_argument, NULL, 'H' },
        { "webcam",           optional_argument, NULL, 'w' },
        { "screenshot",       required_argument, NULL, 's' },
        { "screenshot-delay", required_argument, NULL, 'S' },
        { "screenshot-tour",  required_argument, NULL, 't' },
        { "verbose",          no_argument,       NULL, 'v' },
        { "help",             no_argument,       NULL, 'h' },
        { NULL, 0, NULL, 0 }
    };
    int sim_width = SIM_LCD_H_RES;
    int sim_height = SIM_LCD_V_RES;
    int opt;
    while ((opt = getopt_long(argc, argv, "q:Q:d:W:H:w::s:S:t:vh", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'q':
                sim_video_set_qr_image(optarg);
                break;
            case 'Q':
                sim_video_set_qr_dir(optarg);
                break;
            case 'd': {
                char nvs_path[512];
                snprintf(nvs_path, sizeof(nvs_path), "%s/nvs", optarg);
                sim_nvs_set_data_dir(nvs_path);
                sim_sdcard_set_data_dir(optarg);
                break;
            }
            case 'W':
                sim_width = atoi(optarg);
                break;
            case 'H':
                sim_height = atoi(optarg);
                break;
            case 'w':
                sim_video_set_webcam(optarg);
                break;
            case 's':
                screenshot_path = optarg;
                break;
            case 'S': {
                char *end;
                long val = strtol(optarg, &end, 10);
                if (*end != '\0' || val <= 0) {
                    fprintf(stderr, "Invalid screenshot delay: %s\n", optarg);
                    return 1;
                }
                screenshot_delay_ms = (int)val;
                break;
            }
            case 't':
                snprintf(tour_output_dir, sizeof(tour_output_dir), "%s", optarg);
                tour_state = TOUR_CAPTURE_SPLASH;
                break;
            case 'v':
                esp_log_level_set("*", ESP_LOG_DEBUG);
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                fprintf(stderr,
                    "Usage: %s [--qr-image PATH] [--qr-dir DIR] [--data-dir DIR]"
                    " [--width N] [--height N] [--verbose]"
                    " [--screenshot PATH] [--screenshot-tour DIR]\n",
                    argv[0]);
                return 1;
        }
    }

    printf("Kern Simulator starting (%dx%d)\n", sim_width, sim_height);

    /* Initialize libwally-core */
    if (wally_init(0) != WALLY_OK) {
        fprintf(stderr, "wally_init failed\n");
        return 1;
    }

    /* Initialize LVGL */
    lv_init();

    /* Create SDL2 display */
    lv_display_t *disp = lv_sdl_window_create(sim_width, sim_height);
    if (!disp) {
        fprintf(stderr, "Failed to create SDL display\n");
        return 1;
    }

    lv_sdl_window_set_title(disp, "Kern Simulator");

    /* Create SDL2 mouse input */
    lv_indev_t *mouse = lv_sdl_mouse_create();
    (void)mouse;

    /* Initialize theme (copies Montserrat fonts, sets icon fallbacks) */
    theme_init();

    /* Apply dark background and default text style to screen */
    lv_obj_t *scr = lv_screen_active();
    theme_apply_screen(scr);

    /* Force initial render */
    lv_refr_now(NULL);

    /* -----------------------------------------------------------------------
     * Initialize NVS (file-backed storage for settings and PIN)
     * --------------------------------------------------------------------- */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        fprintf(stderr, "NVS init failed: 0x%x\n", ret);
        return 1;
    }

    /* Initialize persistent settings */
    settings_init();

    /* Initialize PMIC (simulated battery on wave_35; no-op on wave_4b) */
    bsp_pmic_init();

    /* -----------------------------------------------------------------------
     * Show animated Kern logo splash screen
     * --------------------------------------------------------------------- */
    kern_logo_animated(scr);

    /* -----------------------------------------------------------------------
     * Initialize application modules (while splash plays)
     * --------------------------------------------------------------------- */
    bip39_filter_init();
    pin_init();

    /* Register session expiry callback */
    session_set_expired_callback(session_expired_handler);

    /* -----------------------------------------------------------------------
     * Schedule transition to PIN gate after 3-second splash
     * (single-threaded: use one-shot LVGL timer instead of vTaskDelay)
     * --------------------------------------------------------------------- */
    lv_timer_t *splash_timer = lv_timer_create(splash_done_cb, 3000, NULL);
    lv_timer_set_repeat_count(splash_timer, 1);

    /* -----------------------------------------------------------------------
     * Schedule screenshot (if requested) and exit after the specified delay.
     * This is used by CI to capture UI screenshots for each board resolution.
     * --------------------------------------------------------------------- */
    if (screenshot_path) {
        lv_timer_t *ss_timer = lv_timer_create(screenshot_timer_cb,
                                               screenshot_delay_ms, NULL);
        lv_timer_set_repeat_count(ss_timer, 1);
    }

    /* -----------------------------------------------------------------------
     * Start screenshot tour (if requested via --screenshot-tour <dir>).
     * Navigates through every screen/menu, saves one PNG each, then exits.
     * --------------------------------------------------------------------- */
    if (tour_state == TOUR_CAPTURE_SPLASH) {
        tour_start();
    }

    /* -----------------------------------------------------------------------
     * Main loop
     * SDL_QUIT is handled by LVGL's SDL driver which calls exit(0)
     * --------------------------------------------------------------------- */
    while (1) {
        lvgl_port_lock(0);
        uint32_t ms_til_next = lv_timer_handler();
        lvgl_port_unlock();
        /* Cap delay to ~33ms (~30fps).  Background threads (camera stream)
         * update LVGL image sources via lv_img_set_src() which marks the
         * widget dirty, but SDL2 can only render from the main thread.
         * A short cap ensures lv_timer_handler() redraws promptly. */
        if (ms_til_next > 33) ms_til_next = 33;
        SDL_Delay(ms_til_next < 1 ? 1 : ms_til_next);
    }

    return 0;
}
