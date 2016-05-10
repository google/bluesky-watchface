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

#include "data.h"
#include "palette.h"
#include "sky_layer.h"

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
 *   Gregorian Month of 28, 29, 30, or 31 days
 *   Gregorian Year of 365 or 366 days
 */

// Custom state per sky layer.
//
typedef struct {

    // The "absolute" moment to be displayed.
    //
    time_t unix_time;

    // The timezone-local moment to be displayed.
    //
    struct tm wall_time;

    // Agenda data to be rendered as a skyline.
    //
    const int16_t * agenda;
    int32_t agenda_length;
    int32_t agenda_epoch;
    struct tm agenda_epoch_wall_time;

} BSKY_SkyLayerData;

// Make a smaller rect by trimming the edges of a larger one.
//
static GRect bsky_rect_trim (const GRect rect, const int8_t trim) {
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

// Callback for bsky_data_set_receiver.
//
static void bsky_sky_layer_receive_data(
        void * context,
        struct BSKY_data_receiver_args args) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_sky_layer_receive_data:"
            " agenda=%p"
            ",agenda_length_bytes=%ld"
            ",agenda_changed=%s"
            ",agenda_epoch=%ld",
            args.agenda,
            args.agenda_length_bytes,
            (args.agenda_changed ? "true" : "false"),
            args.agenda_epoch);
    BSKY_SkyLayerData * data = layer_get_data((Layer*)context);
    bool mark_dirty
        = args.agenda_changed
        || (args.agenda == NULL && data->agenda != NULL)
        || (args.agenda != NULL && data->agenda == NULL);
    data->agenda = (int16_t*) args.agenda;
    data->agenda_length = args.agenda_length_bytes/2;
    data->agenda_epoch = args.agenda_epoch;
    struct tm * epoch_wall_time = localtime(&data->agenda_epoch);
    data->agenda_epoch_wall_time = *epoch_wall_time;
    if (mark_dirty) {
        layer_mark_dirty((Layer*)context);
    }
}

static void bsky_sky_layer_update (Layer *layer, GContext *ctx) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_sky_layer_update(%p, %p)",
            layer,
            ctx);

    // It'd make sense to subscribe earlier, but then we might get our
    // first callback before we're ready to process it, and then we'd
    // have to hold on to it somehow... but that's what the data module
    // is for.  Therefore, don't subscribe until we're good and ready!
    bsky_data_set_receiver(
            bsky_sky_layer_receive_data,
            layer);

    const GColor color_sun_fill = BSKY_PALETTE_SUN_LIGHT;
    const GColor color_sun_stroke = BSKY_PALETTE_SUN_DARK;
    const GColor color_sky_stroke = BSKY_PALETTE_SKY_STROKE;

    const BSKY_SkyLayerData * const data = layer_get_data(layer);
    const GRect bounds = layer_get_bounds(layer);

    const GRect sky_bounds = bounds;
    const int16_t sky_diameter = 
        (sky_bounds.size.w > sky_bounds.size.h)
        ? sky_bounds.size.h
        : sky_bounds.size.w;

    // Paint the sky blue
    graphics_context_set_fill_color(ctx, GColorVividCerulean);
    graphics_fill_rect(ctx, sky_bounds, 0, 0);

    // Update the 24 hour markers
    const int32_t midnight_angle = TRIG_MAX_ANGLE / 2;
    const GRect sky_inset = bsky_rect_trim(sky_bounds, sky_diameter / 4);
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

    // Update the Sun
    const int32_t sun_angle = midnight_angle
        + TRIG_MAX_ANGLE * data->wall_time.tm_hour / 24
        + TRIG_MAX_ANGLE * data->wall_time.tm_min / (24 * 60);
    const int32_t sun_diameter = sky_diameter / 6;
    const GRect sun_orbit = bsky_rect_trim(sky_bounds, sun_diameter/2+2);
    const GPoint sun_center = gpoint_from_polar(
            sun_orbit,
            GOvalScaleModeFitCircle,
            sun_angle);
    graphics_context_set_fill_color(ctx, color_sun_fill);
    graphics_fill_circle(ctx, sun_center, sun_diameter/2);
    graphics_context_set_stroke_color(ctx, color_sun_stroke);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_circle(ctx, sun_center, sun_diameter/2);

    // Put a nice big white circular cloud in the center
    const GPoint center = {
        .x=bounds.origin.x+bounds.size.w/2,
        .y=bounds.origin.y+bounds.size.h/2,
    };
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, center, sky_diameter/2-(sky_diameter*3/13));

    // Draw the Skyline as solid blocks
    const GRect skyline_bounds = sky_bounds;
    const uint16_t inset_min = sky_diameter/10;
    const uint16_t inset_max = sky_diameter/2-(sky_diameter*3/13);
    const uint16_t duration_min = 20*60;
    const uint16_t duration_max = 90*60;
    const int16_t * agenda = data->agenda;
    time_t max_start_time = data->unix_time+24*60*60;
    time_t min_end_time = data->unix_time;
    for (int32_t i=0; i<data->agenda_length; i+=2) {
        if (agenda[i]==agenda[i+1]) {
            // Skip zero-length events
            continue;
        }
        time_t times [2];
        for (int t=0; t<2; ++t) {
            times[t] = data->agenda_epoch + agenda[i+t]*60;
        }
        if (times[0] > max_start_time) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "out of range");
            continue;
        }
        if (times[1] <= min_end_time) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "out of range");
            continue;
        }
        GColor color = GColorBlack;
        if (times[0] <= data->unix_time) {
            color = color_sun_stroke;
        }
        graphics_context_set_fill_color(ctx, color);
        int32_t angles [2];
        bool cropped_highlight = false;
        for (int t=0; t<2; ++t) {
            time_t cropped
                = times[t] < min_end_time ? min_end_time
                : times[t] > max_start_time ? max_start_time
                : times[t];
            if (t==0 && cropped!=times[t]) {
                cropped_highlight = true;
            }
            struct tm * wall_time = localtime(&cropped);
            angles[t]
                = midnight_angle
                + TRIG_MAX_ANGLE * wall_time->tm_hour / 24
                + TRIG_MAX_ANGLE * wall_time->tm_min / (24 * 60);
        }
        uint16_t duration_scale
            = (times[1]-times[0]) < duration_min
            ? 0
            : (times[1]-times[0]) > duration_max
            ? (duration_max-duration_min)
            : (times[1]-times[0]) - duration_min;
        uint16_t inset_offset
            = duration_scale
            * (inset_max-inset_min)
            / (duration_max-duration_min);
        graphics_fill_radial(
                ctx,
                skyline_bounds,
                GOvalScaleModeFitCircle,
                inset_max - inset_offset,
                angles[0],
                angles[1]);
        GColor highlight;
        if (angles[1]-angles[0] < TRIG_MAX_ANGLE/360) {
            // Towers too thin to show contrast against the blue sky
            // should keep a dark color even when highlighted.
            highlight = GColorDarkGray;
        } else if (inset_offset > (inset_max+inset_min)*2/5) {
            // Tall towers tend to be made of more grey/blue material so
            // highlight them that way.
            highlight = GColorLiberty;
        } else {
            // Otherwise let's say it's made of traditional red-orange
            // brick, and the highlight can be wider.
            highlight = GColorRoseVale;
        }
        if (!cropped_highlight) {
            int32_t highlight_start_angle = TRIG_MAX_ANGLE*4/(360*3);
            int32_t highlight_end_angle = (angles[1]-angles[0])*1/3;
            graphics_context_set_fill_color(ctx, highlight);
            graphics_fill_radial(
                    ctx,
                    skyline_bounds,
                    GOvalScaleModeFitCircle,
                    inset_max - inset_offset - 1,
                    angles[0]+highlight_start_angle,
                    angles[0]+highlight_end_angle);
        }
    }
}

struct BSKY_SkyLayer {

    // The real Pebble layer, of course.
    //
    Layer *layer;

    // A conveniently typed pointer to custom state data, stored in the
    // layer itself.
    //
    BSKY_SkyLayerData *data;
};

BSKY_SkyLayer * bsky_sky_layer_create(GRect frame) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_sky_layer_create({%d,%d,%d,%d})",
            frame.origin.x,
            frame.origin.y,
            frame.size.w,
            frame.size.h);
    BSKY_SkyLayer *sky_layer = malloc(sizeof(*sky_layer));
    if (sky_layer) {
        // Allocate Pebble layer
        sky_layer->layer = layer_create_with_data(
                frame,
                sizeof(BSKY_SkyLayerData));
        if (!sky_layer->layer) {
            APP_LOG(APP_LOG_LEVEL_ERROR,
                    "bsky_sky_layer_create: out of memory at"
                    " layer_create_with_data");
            free(sky_layer);
            sky_layer = NULL;
        } else {
            sky_layer->data = layer_get_data(sky_layer->layer);
            sky_layer->data->agenda = NULL;
            layer_set_update_proc(
                    sky_layer->layer,
                    bsky_sky_layer_update);
        }
    }
    return sky_layer;
}

void bsky_sky_layer_destroy(BSKY_SkyLayer *sky_layer) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_sky_layer_destroy(%p)",
            sky_layer);
    layer_destroy(sky_layer->layer);
    sky_layer->layer = NULL;
    sky_layer->data = NULL;
    free(sky_layer);
}

Layer * bsky_sky_layer_get_layer(BSKY_SkyLayer *sky_layer) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_sky_layer_get_layer(%p)",
            sky_layer);
    return sky_layer->layer;
}

void bsky_sky_layer_set_time(
        BSKY_SkyLayer *sky_layer,
        time_t time) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_sky_layer_set_time(%p, ...)",
            sky_layer);
    sky_layer->data->unix_time = time;
    struct tm * wall_time = localtime(&time);
    sky_layer->data->wall_time = *wall_time;
    layer_mark_dirty(sky_layer->layer);
}
