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
#include "agenda.h"
#include "palette.h"
#include "sky_layer.h"

// Custom state per sky layer.
//
typedef struct {

    // The "absolute" moment to be displayed.
    //
    time_t unix_time;

    // The timezone-local moment to be displayed.
    //
    struct tm wall_time;

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

// Matches BSKY_DataReceiver
//
static void bsky_sky_layer_agenda_update(void * context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_sky_layer_agenda_update");
    layer_mark_dirty((Layer*)context);
}

// Get an easily human-readable representation of a time_t.
//
// TODO: refactor this to avoid potentially sharing and corrupting the static
// string buffer; maybe write a log message with a given prefix followed by the
// time.
//
static const char * bsky_debug_fmt_time (time_t t) {
    const struct tm *local_now = localtime(&t);
    static char s_time_buffer[20];
    if (0 == strftime(s_time_buffer,
                sizeof(s_time_buffer),
                "%F %T",
                local_now)) {
        return "(error while formatting time)";
    }
    return s_time_buffer;
}

// Pebble Layer callback to do the rendering work.
//
// TODO: split this up, maybe even going as far as creating separate layers.
//
static void bsky_sky_layer_update (Layer *layer, GContext *ctx) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_sky_layer_update(%p, %p)",
            layer,
            ctx);

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

    const int32_t circum_hours
        = bsky_data_int(BSKY_DATAKEY_FACE_HOURS)
        ? bsky_data_int(BSKY_DATAKEY_FACE_HOURS)
        : clock_is_24h_style() ? HOURS_PER_DAY : HOURS_PER_DAY/2;

    const enum BSKY_Data_FaceOrientation orientation = bsky_data_int(BSKY_DATAKEY_FACE_ORIENTATION);
    const int32_t midnight_angle
        = (orientation==BSKY_DATA_FACE_ORIENTATION_NOON_TOP && circum_hours==24)
        ? (TRIG_MAX_ANGLE/2)
        : 0;

    // Paint the sky blue
    graphics_context_set_fill_color(ctx, GColorVividCerulean);
    graphics_fill_rect(ctx, sky_bounds, 0, 0);

    // Update the hour markers
    const GRect sky_inset = bsky_rect_trim(sky_bounds, sky_diameter / 4);
    graphics_context_set_stroke_color(ctx, color_sky_stroke);
    graphics_context_set_antialiased(ctx, true);
    for (int32_t hour = 0; hour < circum_hours; ++hour) {
        const int32_t hour_angle =
            (midnight_angle + hour * TRIG_MAX_ANGLE / circum_hours)
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

    // Put a nice big white circular cloud in the center
    const GPoint center = {
        .x=bounds.origin.x+bounds.size.w/2,
        .y=bounds.origin.y+bounds.size.h/2,
    };
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, center, sky_diameter/2-(sky_diameter*3/13));

    // Update the Sun
    const int32_t sun_angle = midnight_angle
        + TRIG_MAX_ANGLE * data->wall_time.tm_hour / circum_hours
        + TRIG_MAX_ANGLE * data->wall_time.tm_min / (circum_hours * 60);
    const int32_t sun_diameter = sky_diameter / 7;
    const GRect sun_orbit
        = bsky_rect_trim(
                sky_bounds,
                (sky_diameter*3/13) - sun_diameter/2);
    const GPoint sun_center = gpoint_from_polar(
            sun_orbit,
            GOvalScaleModeFitCircle,
            sun_angle);
    const GPoint sun_beam = gpoint_from_polar(
            sky_bounds,
            GOvalScaleModeFitCircle,
            sun_angle);
    graphics_context_set_stroke_color(ctx, color_sun_stroke);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_context_set_fill_color(ctx, color_sun_fill);
    graphics_draw_line(ctx, sun_center, sun_beam);
    graphics_fill_circle(ctx, sun_center, sun_diameter/2);
    graphics_draw_circle(ctx, sun_center, sun_diameter/2);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_context_set_stroke_color(ctx, color_sun_fill);
    graphics_draw_line(ctx, sun_center, sun_beam);

    // Draw the Skyline as solid blocks
    const GRect skyline_bounds = sky_bounds;
    const uint16_t inset_min = sky_diameter/10;
    const uint16_t inset_max = sky_diameter/2-(sky_diameter*4/14);
    const uint16_t duration_min = 20*60;
    const uint16_t duration_max = 90*60;
    const struct BSKY_Agenda * agenda = bsky_agenda_read();
    const struct BSKY_AgendaEvent * events = agenda->events;
    time_t max_start_time = data->unix_time+circum_hours*60*60;
    time_t min_end_time = data->unix_time;
    for (int32_t index=0; index<agenda->events_length; ++index) {
        int32_t ievent
            = agenda->events_by_height
            ? agenda->events_by_height[index]
            : index;
        if (events[ievent].rel_start>=events[ievent].rel_end) {
            // Skip zero-length and negative-length events
            continue;
        }
        const time_t times [2] = {
            agenda->epoch + events[ievent].rel_start*60,
            agenda->epoch + events[ievent].rel_end*60,
        };
        if (times[0] > max_start_time) {
            continue;
        }
        if (times[1] <= min_end_time) {
            continue;
        }
        graphics_context_set_fill_color(ctx, GColorBlack);
        struct tm wall_times [2];
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
            wall_times[t] = *localtime(&cropped);
            angles[t]
                = midnight_angle
                + TRIG_MAX_ANGLE * wall_times[t].tm_hour / circum_hours
                + TRIG_MAX_ANGLE * wall_times[t].tm_min / (circum_hours * 60);
        }
        APP_LOG(APP_LOG_LEVEL_DEBUG, "event start=%s", bsky_debug_fmt_time(times[0]));
        APP_LOG(APP_LOG_LEVEL_DEBUG, "event end=%s", bsky_debug_fmt_time(times[1]));
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
            layer_set_update_proc(
                    sky_layer->layer,
                    bsky_sky_layer_update);
            if (!bsky_data_subscribe(
                    bsky_sky_layer_agenda_update,
                    sky_layer->layer,
                    BSKY_DATAKEY_AGENDA)) {
                // This should never happen on non-developer devices: the
                // number of subscribers supported by the Data module should be
                // hard-coded to a sufficient limit.
                //
                // TODO: log this error from within the bsky_data_subscribe
                // function instead and have it return void; there's nothing
                // smart for a caller to do in this case anyway.
                //
                APP_LOG(APP_LOG_LEVEL_ERROR,
                        "bsky_sky_layer_create:"
                        " failed to subscribe to agenda updates");
            }
        }
    }
    return sky_layer;
}

void bsky_sky_layer_destroy(BSKY_SkyLayer *sky_layer) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_sky_layer_destroy(%p)",
            sky_layer);
    bsky_data_unsubscribe(
            bsky_sky_layer_agenda_update,
            sky_layer->layer);
    layer_destroy(sky_layer->layer);
    sky_layer->layer = NULL;
    sky_layer->data = NULL;
    free(sky_layer);
}

Layer * bsky_sky_layer_get_layer(BSKY_SkyLayer *sky_layer) {
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
