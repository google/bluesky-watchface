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

struct ConcentricRing {
    int32_t duration; // seconds
    GColor8 color;
};

/* Cycles:
 *  Regular numeric cycles:
 *   Minute of 60 seconds
 *   Hour of 60 minutes
 *   Day of 24 hours (but daylight savings...?)
 *  Regular named cycles
 *   Week of 7 named days
 *  Mostly-constant astronomical cycles:
 *   Moon phase cycle of about 29.53 days
 *   Solar year of about 365.25 days
 *  Irregular but well-defined cycles:
 *   Gregorian Month
 *   Gregorian Year
 */

#define DURATION_SECOND (1)
#define DURATION_MINUTE (60*DURATION_SECOND)
#define DURATION_HOUR   (60*DURATION_MINUTE)
#define DURATION_DAY    (24*DURATION_HOUR)
#define DURATION_LUNE   (2551392*DURATION_SECOND)

typedef struct {
    time_t unix_time;
    struct tm wall_time;
} JTT_AnalogData;

static void jtt_analog_layer_update (Layer *layer, GContext *ctx) {
    const JTT_AnalogData * const data = layer_get_data(layer);
    const GRect bounds = layer_get_bounds(layer);

    graphics_context_set_fill_color(ctx, GColorClear);
    graphics_fill_rect(ctx, bounds, 0, 0);

    // TODO: find value of inset_thickness based on available size, or
    //       set through public API of this module.
    const int16_t inset_thickness =
        (bounds.size.w > bounds.size.h)
        ? bounds.size.h / 8
        : bounds.size.w / 8;
    graphics_context_set_fill_color(ctx, GColorBlack);
    const int32_t angle_start = TRIG_MAX_ANGLE / 2;
    const int32_t angle_delta =
        TRIG_MAX_ANGLE * data->wall_time.tm_hour / 24
        +
        TRIG_MAX_ANGLE * data->wall_time.tm_min / (24 * 60);
    graphics_fill_radial(
            ctx,
            bounds,
            GOvalScaleModeFitCircle,
            inset_thickness,
            angle_start,
            angle_start + angle_delta);
}

struct JTT_AnalogLayer {
    Layer *layer;
    JTT_AnalogData *data;
};

JTT_AnalogLayer * jtt_analog_layer_create(GRect frame) {
    JTT_AnalogLayer *analog_layer = malloc(sizeof(*analog_layer));
    if (analog_layer) {
        analog_layer->layer = layer_create_with_data(
                frame,
                sizeof(JTT_AnalogData));
        if (!analog_layer->layer) {
            free(analog_layer);
            analog_layer = NULL;
        } else {
            analog_layer->data = layer_get_data(analog_layer->layer);
            layer_set_update_proc(
                    analog_layer->layer,
                    jtt_analog_layer_update);
        }
    }
    return analog_layer;
}

void jtt_analog_layer_destroy(JTT_AnalogLayer *analog_layer) {
    layer_destroy(analog_layer->layer);
    free(analog_layer);
}

Layer * jtt_analog_layer_get_layer(JTT_AnalogLayer *analog_layer) {
    return analog_layer->layer;
}

void jtt_analog_layer_set_time(
        JTT_AnalogLayer *analog_layer,
        time_t time) {
    analog_layer->data->unix_time = time;
    struct tm * wall_time = localtime(&time);
    analog_layer->data->wall_time = *wall_time;
}
