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

static Window *s_main_window;
static TextLayer *s_time_layer;

static void update_time() {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    static char s_buffer[8];
    strftime(s_buffer,
            sizeof(s_buffer),
            clock_is_24h_style() ? "%H:%M" : "%I:%M",
            tick_time);

    text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(
        struct tm *tick_time,
        TimeUnits units_changed) {
    update_time(); // TODO: find out whether to forward tick_time here
}

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_time_layer = text_layer_create(
            GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));

    text_layer_set_background_color(
            s_time_layer,
            GColorClear);
    text_layer_set_font(
            s_time_layer,
            fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(
            s_time_layer,
            GTextAlignmentCenter);
    text_layer_set_text_color(
            s_time_layer,
            GColorBlack);

    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
}

static void main_window_unload(Window *window) {
    text_layer_destroy(s_time_layer);
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
