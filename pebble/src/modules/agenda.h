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
#pragma once

struct BSKY_Agenda {
    const struct BSKY_DataEvent * agenda;
    int32_t agenda_length;
    int32_t agenda_epoch;
    struct tm agenda_epoch_wall_time;
    int16_t * agenda_height_index;
};

void bsky_agenda_init ();

void bsky_agenda_deinit ();

const struct BSKY_Agenda * bsky_agenda_read ();

// Function type for Agenda update receivers.
//
typedef void (*BSKY_AgendaReceiver) (void * context);

// Subscribe to Agenda updates.
//
// Returns true if subscription was successful, otherwise false.
//
bool bsky_agenda_subscribe (
        BSKY_AgendaReceiver receiver,
        void * context);

// Unsubscribe to Agenda updates.
//
// Undoes the effect of calling bsky_agenda_subscribe with exactly the same
// arguments i.e. context must match too.
//
void bsky_agenda_unsubscribe (
        BSKY_AgendaReceiver receiver,
        void * context);
