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

// Initialize the data module.
//
// This function is either idempotent or buggy: treat it as idempotent
// so that when the bugs are fixed it'll be correct.  Call it (and check
// the return value) at the beginning of any method that's going to be
// sending messages.
//
// Returns: true if and only if initialization is complete and
// successful, regardless whether this was the call that did so.
//
bool bsky_data_init(void);

// Shut down communications.
//
// Should be called from deinit() in main.c, or project equivalent.
//
void bsky_data_deinit(void);

// Perform periodic data synchronization tasks.
//
void bsky_data_update(void);

// Start and end times for an event as seconds relative to a custom epoch.
//
struct BSKY_data_event {
    int16_t rel_start;
    int16_t rel_end;
};

struct BSKY_data_receiver_args {
    const struct BSKY_data_event * agenda;
    int32_t agenda_length;
    bool agenda_changed;
    int32_t agenda_epoch;
};

typedef void (*BSKY_data_receiver) (
        void * context,
        struct BSKY_data_receiver_args args);

// Set the receiver for incoming data
//
// The existing receiver, if any, will be replaced.  A NULL receiver
// indicates that the app is not interested in receiving anything.  The
// implementation may decide not to request updates in this case.
//
void bsky_data_set_receiver (
        BSKY_data_receiver receiver,
        void * context);
