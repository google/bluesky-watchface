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

static void bsky_agenda_notify ();

// Using the built-in qsort function to sort an array of indices by the values
// they index in another array requires making that other array available to
// the "compare" function passed to qsort.
//
// See cmp_agenda_events_by_height.
//
static const struct BSKY_DataEvent * cmp_agenda_events_by_height_agenda;

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
                "bsky_agenda_receive_data:"
                " calloc failed for agenda height index; continuing");
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

// Update static state according to received data.
//
// Matches function type BSKY_DataReceiver.
//
static void bsky_agenda_receive_data(
        void * context,
        struct BSKY_DataReceiverArgs args) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_agenda_receive_data:"
            " agenda=%p"
            ",agenda_length=%ld"
            ",agenda_changed=%s"
            ",agenda_epoch=%ld",
            args.agenda,
            args.agenda_length,
            (args.agenda_changed ? "true" : "false"),
            args.agenda_epoch);
    struct BSKY_Agenda * agenda = context;
    bool changed
        = args.agenda_changed
        || (args.agenda == NULL && agenda->events != NULL)
        || (args.agenda != NULL && agenda->events == NULL);
    agenda->events = args.agenda;
    agenda->events_length = args.agenda_length;
    agenda->epoch = args.agenda_epoch;
    struct tm * epoch_wall_time = localtime(&agenda->epoch);
    agenda->epoch_wall_time = *epoch_wall_time;
    if (agenda->epoch_wall_time.tm_sec) {
        // TODO: fix this in the remote code by always rounding epoch down to
        // the minute.
        agenda->epoch += (60 - agenda->epoch_wall_time.tm_sec);
        agenda->epoch_wall_time.tm_sec = 0;
    }
    if (args.agenda_changed) {
        bsky_agenda_update_events_by_height(agenda);
    }
    if (changed) {
        bsky_agenda_notify();
    }
}

struct BSKY_AgendaReceiverInfo {
    BSKY_AgendaReceiver receiver;
    void * context;
};

struct BSKY_AgendaReceiverInfo s_receivers [4];

static void bsky_agenda_notify () {
    for (size_t i=0; i<sizeof(s_receivers)/sizeof(s_receivers[0]); ++i) {
        if (s_receivers[i].receiver) {
            s_receivers[i].receiver(s_receivers[i].context);
        }
    }
}

bool bsky_agenda_subscribe (
        BSKY_AgendaReceiver receiver,
        void * context) {
    for (size_t i=0; i<sizeof(s_receivers)/sizeof(s_receivers[0]); ++i) {
        if (!s_receivers[i].receiver) {
            s_receivers[i].receiver = receiver;
            s_receivers[i].context = context;
            return true;
        }
    }
    return false;
}

void bsky_agenda_unsubscribe (
        BSKY_AgendaReceiver receiver,
        void * context) {
    for (size_t i=0; i<sizeof(s_receivers)/sizeof(s_receivers[0]); ++i) {
        if (s_receivers[i].receiver==receiver
                && s_receivers[i].context==context) {
            s_receivers[i].receiver = NULL;
            s_receivers[i].context = NULL;
        }
    }
}

static struct BSKY_Agenda s_agenda;

const struct BSKY_Agenda * bsky_agenda_read () { return &s_agenda; }

void bsky_agenda_init () {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_agenda_init()");
    bsky_data_set_receiver(
            bsky_agenda_receive_data,
            &s_agenda);
    for (size_t i=0; i<sizeof(s_receivers)/sizeof(s_receivers[0]); ++i) {
        s_receivers[i].receiver = NULL;
        s_receivers[i].context = NULL;
    }
}

void bsky_agenda_deinit () {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_agenda_deinit()");
    for (size_t i=0; i<sizeof(s_receivers)/sizeof(s_receivers[0]); ++i) {
        s_receivers[i].receiver = NULL;
        s_receivers[i].context = NULL;
    }
    bsky_data_set_receiver(NULL, NULL);
    if (s_agenda.events_by_height) {
        free (s_agenda.events_by_height);
        s_agenda.events_by_height = NULL;
    }
}
