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

static GRect jtt_rect_trim (const GRect rect, const int8_t trim) {
    const GRect result = {
        .origin = {
            .x = rect.origin.x + trim,
            .y = rect.origin.y + trim,
        },
        .size = {
            .w = rect.size.w - trim*2,
            .h = rect.size.h - trim*2,
        },
    };
    return result;
}

static void jtt_analog_layer_update (Layer *layer, GContext *ctx) {
    const GColor color_sun_fill = GColorYellow;
    const GColor color_sun_stroke = GColorOrange;
    const GColor color_sky_fill [] = {
        //GColorPictonBlue,
        //GColorVividCerulean,
        GColorCyan,
        GColorElectricBlue,
        GColorCeleste,
    };
    const int8_t color_sky_fill_len =
        sizeof(color_sky_fill) / sizeof(color_sky_fill[0]);
    const GColor color_sky_stroke = GColorBlueMoon;

    const JTT_AnalogData * const data = layer_get_data(layer);
    const GRect bounds = layer_get_bounds(layer);

    graphics_context_set_fill_color(ctx, GColorClear);
    graphics_fill_rect(ctx, bounds, 0, 0);

    // TODO: find value of inset_thickness based on available size, or
    //       set through public API of this module.
    const GRect sky_bounds = bounds;
    const int16_t sky_thickness =
        (sky_bounds.size.w > sky_bounds.size.h)
        ? sky_bounds.size.h / 6
        : sky_bounds.size.w / 6;
    for (int8_t sky_fill=0; sky_fill<color_sky_fill_len; ++sky_fill) {
        graphics_context_set_fill_color(ctx, color_sky_fill[sky_fill]);
        graphics_fill_radial(
                ctx,
                jtt_rect_trim(
                    sky_bounds,
                    sky_thickness * sky_fill / color_sky_fill_len),
                GOvalScaleModeFitCircle,
                sky_thickness / color_sky_fill_len,
                0,
                TRIG_MAX_ANGLE);
    }
    const int32_t midnight_angle = TRIG_MAX_ANGLE / 2;
    const GRect sky_inset = jtt_rect_trim(sky_bounds, sky_thickness);
    graphics_context_set_stroke_color(ctx, color_sky_stroke);
    graphics_context_set_antialiased(ctx, true);
    for (int32_t hour = 0; hour < 24; ++hour) {
        const int32_t hour_angle =
            (midnight_angle + hour * TRIG_MAX_ANGLE / 24)
            % TRIG_MAX_ANGLE;
        const GPoint p0 = gpoint_from_polar(
                sky_inset,
                GOvalScaleModeFitCircle,
                hour_angle);
        const GPoint p1 = gpoint_from_polar(
                bounds,
                GOvalScaleModeFitCircle,
                hour_angle);
        graphics_context_set_stroke_width(ctx, hour % 3 ? 1 : 3);
        graphics_draw_line(ctx, p0, p1);
    }
    const int32_t sun_angle = midnight_angle
        + TRIG_MAX_ANGLE * data->wall_time.tm_hour / 24
        + TRIG_MAX_ANGLE * data->wall_time.tm_min / (24 * 60);
    const int32_t sun_diameter = sky_thickness*2/3;
    const GRect sun_orbit = jtt_rect_trim(sky_bounds, sun_diameter/2);
    const GPoint sun_center = gpoint_from_polar(
            sun_orbit,
            GOvalScaleModeFitCircle,
            sun_angle);
    graphics_context_set_stroke_color(ctx, color_sun_stroke);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(
            ctx,
            sun_center,
            gpoint_from_polar(
                sky_inset,
                GOvalScaleModeFitCircle,
                sun_angle));
    graphics_context_set_fill_color(ctx, color_sun_fill);
    graphics_fill_circle(ctx, sun_center, sun_diameter/2);
    graphics_context_set_stroke_color(ctx, color_sun_stroke);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_circle(ctx, sun_center, sun_diameter/2);
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
    layer_mark_dirty(analog_layer->layer);
}
