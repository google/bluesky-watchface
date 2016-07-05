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

// The next time it would be acceptable to request an update.
//
static time_t s_next_attempt_update;

// Using the built-in qsort function to sort an array of indices by the values
// they index in another array requires making that other array available to
// the "compare" function passed to qsort.
//
// See cmp_agenda_events_by_height.
//
static const struct BSKY_AgendaEvent * cmp_agenda_events_by_height_agenda;

// Compare two indices in a list of indices based on the values they index in
// the cmp_agenda_events_by_height_agenda array.
//
// See bsky_agenda_update_events_by_height.
//
static int cmp_agenda_events_by_height(const void * a, const void * b) {
    uint16_t index [2] = { *((uint16_t*)a), *((uint16_t*)b) };
    int32_t duration[2];
    for (int i=0; i<2; ++i) {
        int32_t start = cmp_agenda_events_by_height_agenda[index[i]].rel_start;
        int32_t end = cmp_agenda_events_by_height_agenda[index[i]].rel_end;
        duration[i] = end-start;
    }
    return duration[0]-duration[1];
}

// Re-generate the events_by_height index.
//
static void bsky_agenda_update_events_by_height(struct BSKY_Agenda * agenda) {
    if (agenda->events_by_height) {
        free(agenda->events_by_height);
    }
    agenda->events_by_height
        = calloc(agenda->events_length,
                 sizeof(agenda->events_by_height[0]));
    if (!agenda->events_by_height) {
        APP_LOG(APP_LOG_LEVEL_ERROR,
                "bsky_agenda_update_events_by_height:"
                " calloc failed for %lu x %u bytes",
                agenda->events_length,
                sizeof(agenda->events_by_height[0]));
    } else {
        for (int16_t i=0; i<agenda->events_length; ++i) {
            agenda->events_by_height[i] = i;
        }
        cmp_agenda_events_by_height_agenda = agenda->events;
        qsort (agenda->events_by_height,
                agenda->events_length,
                sizeof(agenda->events_by_height[0]),
                cmp_agenda_events_by_height);
        cmp_agenda_events_by_height_agenda = NULL;
    }
}

static void bsky_agenda_reload(struct BSKY_Agenda * agenda) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_agenda_reload");

    int32_t epoch = bsky_data_int(BSKY_DATAKEY_AGENDA_EPOCH);

    size_t num_bytes;
    const void * bytes = bsky_data_ptr(BSKY_DATAKEY_AGENDA, &num_bytes);
    if (!bytes || num_bytes==0) {
        APP_LOG(APP_LOG_LEVEL_INFO,
                "bsky_agenda_reload: no agenda data available yet");
        agenda->events_length = 0;
        return;
    }

    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_agenda_reload: %u bytes",
            num_bytes);
    agenda->epoch = epoch;
    struct tm * epoch_wall_time = localtime(&agenda->epoch);
    agenda->epoch_wall_time = *epoch_wall_time;
    agenda->events_length = num_bytes / sizeof(agenda->events[0]);
    agenda->events = bytes;
    if (agenda->epoch_wall_time.tm_sec) {
        // TODO: fix this in the remote code by always rounding epoch down to
        // the minute.
        agenda->epoch += (60 - agenda->epoch_wall_time.tm_sec);
        agenda->epoch_wall_time.tm_sec = 0;
    }
    bsky_agenda_update_events_by_height(agenda);
    APP_LOG(APP_LOG_LEVEL_INFO,
            "bsky_agenda_reload: finished loading agenda data");
}

// Update static state according to received data.
//
// Matches function type BSKY_DataReceiver.
//
static void bsky_agenda_receive_data(void * context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_agenda_receive_data");

    // Don't request another update for at least a few hours.
    s_next_attempt_update = time(NULL) + 6*60*60;

    bsky_agenda_reload(context);
}

static struct BSKY_Agenda s_agenda;

const struct BSKY_Agenda * bsky_agenda_read () {
    // Take this opportunity to trigger an update?
    time_t now = time(NULL);
    if (now >= s_next_attempt_update) {
        APP_LOG(APP_LOG_LEVEL_DEBUG,
                "bsky_agenda_read: update-hungry for at least %ld seconds",
                now-s_next_attempt_update);
        s_next_attempt_update = now + 30;
        bsky_data_set_outgoing_int(BSKY_DATAKEY_AGENDA_NEED_SECONDS, 24*60*60);
        bsky_data_set_outgoing_int(BSKY_DATAKEY_AGENDA_CAPACITY_BYTES, 1024);
        bsky_data_set_outgoing_int(BSKY_DATAKEY_PEBBLE_NOW_UNIX_TIME, now);
        //bsky_data_send_outgoing();
    }
    return &s_agenda;
}

void bsky_agenda_init () {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_agenda_init()");
    bsky_agenda_reload(&s_agenda);
    bsky_data_subscribe(
            bsky_agenda_receive_data,
            &s_agenda,
            BSKY_DATAKEY_AGENDA);
}

void bsky_agenda_deinit () {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_agenda_deinit()");
    bsky_data_unsubscribe(bsky_agenda_receive_data, &s_agenda);
    if (s_agenda.events_by_height) {
        free (s_agenda.events_by_height);
        s_agenda.events_by_height = NULL;
    }
}
