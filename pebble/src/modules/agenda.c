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

static const struct BSKY_DataEvent * cmp_agenda_height_index_agenda;

static int cmp_agenda_height_index(const void * a, const void * b) {
    uint16_t index [2] = { *((uint16_t*)a), *((uint16_t*)b) };
    int32_t duration[2];
    for (int i=0; i<2; ++i) {
        int32_t start = cmp_agenda_height_index_agenda[index[i]].rel_start;
        int32_t end = cmp_agenda_height_index_agenda[index[i]].rel_end;
        duration[i] = end-start;
    }
    return duration[0]-duration[1];
}

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
    struct BSKY_Agenda * data = context;
    bool changed
        = args.agenda_changed
        || (args.agenda == NULL && data->agenda != NULL)
        || (args.agenda != NULL && data->agenda == NULL);
    data->agenda = args.agenda;
    data->agenda_length = args.agenda_length;
    data->agenda_epoch = args.agenda_epoch;
    struct tm * epoch_wall_time = localtime(&data->agenda_epoch);
    data->agenda_epoch_wall_time = *epoch_wall_time;
    if (args.agenda_changed) {
        if (data->agenda_height_index) {
            free(data->agenda_height_index);
        }
        data->agenda_height_index
            = calloc(data->agenda_length,
                     sizeof(data->agenda_height_index[0]));
        if (!data->agenda_height_index) {
            APP_LOG(APP_LOG_LEVEL_ERROR,
                    "bsky_agenda_receive_data:"
                    " calloc failed for agenda height index");
        } else {
            for (int16_t i=0; i<data->agenda_length; ++i) {
                data->agenda_height_index[i] = i*2;
            }
            cmp_agenda_height_index_agenda = data->agenda;
            qsort (data->agenda_height_index,
                    data->agenda_length,
                    sizeof(data->agenda_height_index[0]),
                    cmp_agenda_height_index);
            cmp_agenda_height_index_agenda = NULL;
        }
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
    if (s_agenda.agenda_height_index) {
        free (s_agenda.agenda_height_index);
        s_agenda.agenda_height_index = NULL;
    }
}
