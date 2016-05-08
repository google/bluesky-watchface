/*
 * Copyright 2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <pebble.h>

#include "modules/data.h"
#include "modules/palette.h"
#include "modules/sky_layer.h"

static Window *s_main_window;

// Layers are ordered here as they are in the window's stack.
// Operations over layers are always in the same order except while
// unloading the main window, when the order is reversed.
static BSKY_SkyLayer *s_sky_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;

static void sprint_error(char * buffer, size_t n) {
    static const char generic_error[] = "!ERROR";
    memset(buffer, 0, n);
    for (size_t i=0; i<n-1; ++i) {
        buffer[i] = generic_error[i];
    }
}

static void update_time() {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "update_time()");
    const time_t now = time(NULL);
    const struct tm *local_now = localtime(&now);

    bsky_sky_layer_set_time(s_sky_layer, now);

    // 6 characaters is enough in my locale, but 7 are necessary to hold
    // the generic_error message that will be displayed in case the
    // buffer isn't large enough after all.
    static char s_time_buffer[7];
    if (0 == strftime(s_time_buffer,
                sizeof(s_time_buffer),
                clock_is_24h_style() ? "%H:%M" : "%I:%M",
                local_now)) {
        sprint_error(s_time_buffer, sizeof(s_time_buffer));
    }
    text_layer_set_text(s_time_layer, s_time_buffer);

    static char s_date_buffer[7];
    if (0 == strftime(s_date_buffer,
                sizeof(s_date_buffer),
                "%a %d",
                local_now)) {
        sprint_error(s_date_buffer, sizeof(s_date_buffer));
    }
    text_layer_set_text(s_date_layer, s_date_buffer);
}

static void tick_handler(
        struct tm *tick_time,
        TimeUnits units_changed) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "tick_handler()");
    // Here we could forward tick_time to avoid having to recompute the
    // local time in update_time, but we may very well want to call
    // update_time in other contexts where we don't have an appropriate
    // value to pass in.  Therefore, update_time will ultimately have to
    // be reponsible for computing the local time anyway.
    update_time();

    bsky_data_update();
}

static void main_window_load(Window *window) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load(%p)", window);
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_sky_layer = bsky_sky_layer_create(bounds);
    layer_add_child(
            window_layer,
            bsky_sky_layer_get_layer(s_sky_layer));

    // TODO: I'm not sure what I'm doing with fonts here.  How should a
    // font be selected?  How should the layers be positioned and sized?
    // At this point it's all just guess work and "good enough" results.

    s_time_layer = text_layer_create(
            GRect(0, bounds.size.h/2-23, bounds.size.w, 40));
    text_layer_set_background_color(
            s_time_layer,
            GColorClear);
    text_layer_set_font(
            s_time_layer,
            fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
    text_layer_set_text_alignment(
            s_time_layer,
            GTextAlignmentCenter);
    text_layer_set_text_color(
            s_time_layer,
            BSKY_PALETTE_SUN_DARK);
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

    s_date_layer = text_layer_create(GRect(0, bounds.size.h/2-38, bounds.size.w, 26));
    text_layer_set_background_color(
            s_date_layer,
            GColorClear);
    text_layer_set_font(
            s_date_layer,
            fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
    text_layer_set_text_alignment(
            s_date_layer,
            GTextAlignmentCenter);
    text_layer_set_text_color(
            s_date_layer,
            GColorBlack);
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
}

static void main_window_unload(Window *window) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load(%p)", window);
    text_layer_destroy(s_date_layer);
    s_date_layer = NULL;
    text_layer_destroy(s_time_layer);
    s_time_layer = NULL;
    bsky_sky_layer_destroy(s_sky_layer);
    s_sky_layer = NULL;
    window_destroy(s_main_window);
    s_main_window = NULL;
}

void main_window_push() {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_push()");
    if (!s_main_window) {
        s_main_window = window_create();

        window_set_window_handlers(s_main_window, (WindowHandlers) {
            .load = main_window_load,
            .unload = main_window_unload,
        });

        tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    }

    window_stack_push(s_main_window, true);

    update_time();
}
