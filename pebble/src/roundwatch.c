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
#include "./analog_layer.h"

static Window *s_main_window;
static BSKY_AnalogLayer *s_analog_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;

static void update_time() {
    const time_t now = time(NULL);

    bsky_analog_layer_set_time(s_analog_layer, now);

    const struct tm *local_now = localtime(&now);

    static char s_time_buffer[8];
    strftime(s_time_buffer,
            sizeof(s_time_buffer),
            clock_is_24h_style() ? "%H:%M" : "%I:%M",
            local_now);
    text_layer_set_text(s_time_layer, s_time_buffer);

    static char s_date_buffer[20]; // 10 should suffice, but..
    strftime(s_date_buffer,
            sizeof(s_date_buffer),
            "%a %m-%d",
            local_now);
    text_layer_set_text(s_date_layer, s_date_buffer);
}

static void tick_handler(
        struct tm *tick_time,
        TimeUnits units_changed) {
    update_time(); // TODO: find out whether to forward tick_time here
}

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_analog_layer = bsky_analog_layer_create(bounds);
    layer_add_child(
            window_layer,
            bsky_analog_layer_get_layer(s_analog_layer));

    s_time_layer = text_layer_create(
            GRect(0, 52, bounds.size.w, 40));
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
            GColorBlack);
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

    s_date_layer = text_layer_create(GRect(0, 92, bounds.size.w, 26));
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
    text_layer_destroy(s_date_layer);
    text_layer_destroy(s_time_layer);
    bsky_analog_layer_destroy(s_analog_layer);
}

static void init() {
    s_main_window = window_create();

    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload,
    });

    window_stack_push(s_main_window, true);

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    update_time();
}

static void deinit() {
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
