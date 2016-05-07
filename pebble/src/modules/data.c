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

#include "skyline.h"
#include "data.h"

static BSKY_Skyline *s_skyline_current;
static BSKY_Skyline *s_skyline_incoming;
static BSKY_data_skyline_handler s_skyline_handler;
static void *s_skyline_handler_context;
static int32_t s_agenda_need_seconds;
static int32_t s_agenda_capacity_bytes;
static int32_t s_agenda_length_bytes;
static const uint8_t * s_agenda;

#define MAX_AGENDA_CAPACITY_BYTES (1<<12)
#define MIN_AGENDA_CAPACITY_BYTES (64)

// All known data keys, from appinfo.js#appKeys
//
// For each key, the most essential information is: in what direction is
// this key sent, and what is the maximum length of a value associated
// with that key.
//
enum BSKY_DataKey {
    BSKY_DATAKEY_AGENDA_NEED_SECONDS = 1,
    BSKY_DATAKEY_AGENDA_CAPACITY_BYTES = 2,
    BSKY_DATAKEY_AGENDA = 3,
};

void bsky_data_skyline_subscribe (
        BSKY_data_skyline_handler handler,
        void * context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_data_skyline_subscribe(%p, %p)",
            handler,
            context);
    s_skyline_handler = handler;
    s_skyline_handler_context = context;
    if (bsky_data_init() && s_skyline_handler) {
        s_skyline_handler(s_skyline_handler_context, s_skyline_current);
    }
}

void bsky_data_sync_tuple_changed (
        const uint32_t key,
        const Tuple *new_tuple,
        const Tuple *old_tuple,
        void *context) {
    switch (key) {
        case BSKY_DATAKEY_AGENDA_NEED_SECONDS:
            APP_LOG(APP_LOG_LEVEL_INFO,
                    "bsky_data_sync_update:"
                    " agenda_need_seconds=%ld"
                    ",expected=%ld",
                    new_tuple->value->int32,
                    s_agenda_need_seconds);
            break;
        case BSKY_DATAKEY_AGENDA_CAPACITY_BYTES:
            APP_LOG(APP_LOG_LEVEL_INFO,
                    "bsky_data_sync_update:"
                    " agenda_capacity_bytes=%ld"
                    ",expected=%ld",
                    new_tuple->value->int32,
                    s_agenda_capacity_bytes);
            break;
        case BSKY_DATAKEY_AGENDA:
            s_agenda = new_tuple->value->data;
            s_agenda_length_bytes = new_tuple->length;
            APP_LOG(APP_LOG_LEVEL_INFO,
                    "bsky_data_sync_update: agenda"
                    ",agenda_length_bytes=%ld"
                    ",agenda[0..3]=%02x%02x%02x%02x",
                    s_agenda_length_bytes,
                    s_agenda[0],
                    s_agenda[1],
                    s_agenda[2],
                    s_agenda[3]);
            break;
    }
}

void bsky_data_sync_error (
        DictionaryResult dict_error,
        AppMessageResult app_message_error,
        void *context) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "bsky_data_sync_error(%u,%u)",
            dict_error,
            app_message_error);
    // TODO: switch on error to ignore for now (present behavior), retry
    // with another call to bsky_data_update, or disable bsky_data for
    // some time.
}

static AppSync s_sync;
static uint8_t * s_buffer;
static size_t s_buffer_size;

bool bsky_data_init(void) {
    static bool s_bsky_data_inited_already = false;
    if (s_bsky_data_inited_already) { return true; }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_data_init()");

    // Try to be idempotent: don't allocate new skyline structs if
    // current pointers are non-NULL.
    s_skyline_current = s_skyline_current
        ? s_skyline_current
        : bsky_skyline_create();
    s_skyline_incoming = s_skyline_incoming
        ? s_skyline_incoming
        : bsky_skyline_create();

    // TODO: remove this line when real data starts arriving.
    bsky_skyline_rewrite_random (s_skyline_current);

    // Calculate and allocate memory for the sync dictionary.
    //
    if (!s_buffer) {
        s_agenda_capacity_bytes = MAX_AGENDA_CAPACITY_BYTES;
        do {
            s_buffer_size = dict_calc_buffer_size(
                3,
                sizeof(int32_t),
                sizeof(int32_t),
                s_agenda_capacity_bytes);
            s_buffer = malloc(s_buffer_size);
            if (s_buffer) {
                break;
            }
            s_agenda_capacity_bytes /= 2;
            if (s_agenda_capacity_bytes < MIN_AGENDA_CAPACITY_BYTES) {
                break;
            }
        } while (!s_buffer);
        if (!s_buffer) {
            APP_LOG(APP_LOG_LEVEL_ERROR,
                    "bsky_data_init: malloc(%u) failed",
                    s_buffer_size);
            return false;
        }
        APP_LOG(APP_LOG_LEVEL_DEBUG,
                "bsky_data_init: malloc(%u):"
                " s_agenda_capacity_bytes=%ld",
                s_buffer_size,
                s_agenda_capacity_bytes);
        return false;
    }

    // This is idempotent if and only if subsequent calls to
    // app_message_open just reset the inbound and outbound buffer
    // lengths.
    //
    const uint32_t size_outbound = dict_calc_buffer_size(
            2,
            sizeof(int32_t),
            sizeof(int32_t));
    AppMessageResult result; // lint...
    while (s_agenda_capacity_bytes >= MIN_AGENDA_CAPACITY_BYTES) {
        const uint32_t size_inbound = dict_calc_buffer_size(
                1,
                s_agenda_capacity_bytes);
        result = app_message_open(
                size_inbound,
                size_outbound);
        if (result != APP_MSG_OUT_OF_MEMORY) {
            APP_LOG(APP_LOG_LEVEL_DEBUG,
                    "app_message_open:"
                    " size_inbound=%lu,size_outbound=%lu",
                    size_inbound,
                    size_outbound);
            break;
        }
        s_agenda_capacity_bytes /= 2;
        if (s_agenda_capacity_bytes < MIN_AGENDA_CAPACITY_BYTES) {
            s_agenda_capacity_bytes = 0;
            APP_LOG(APP_LOG_LEVEL_INFO, "stopping at tried-everything");
            break;
        }
    }
    if (result != APP_MSG_OK) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "app_message_open: %u", result);
        return false;
    }

    // Setup initial tuple values and begin sync.
    //
    // This, too, does not involve any communication with the phone.
    // However, it will cause bsky_data_sync_tuple_changed to be called
    // for the given initial values.
    //
    // Since defaults are supposed to represent values that are
    // synchronized between the watch and the remote device, do not
    // choose values that indicate synchronization has already happened:
    // defaults here typically should be zero.
    //
    uint8_t * zero_agenda = calloc (s_agenda_capacity_bytes, 1);
    if (!zero_agenda) {
        // TODO: handle this differently, maybe by allocating
        // zero_agenda nearer the beginning of this function.
        APP_LOG(APP_LOG_LEVEL_ERROR,
                "bsky_data_init:"
                " out of memory at inopportune moment");
        return false;
    }
    Tuplet initial_values[] = {
        TupletInteger(BSKY_DATAKEY_AGENDA_NEED_SECONDS, (int32_t) 0),
        TupletInteger(BSKY_DATAKEY_AGENDA_CAPACITY_BYTES, (int32_t) 0),
        TupletBytes(
                BSKY_DATAKEY_AGENDA, zero_agenda, sizeof(zero_agenda)),
    };
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_data_init: app_sync_init()");
    app_sync_init(
            &s_sync,
            s_buffer,
            s_buffer_size,
            initial_values,
            sizeof(initial_values)/sizeof(initial_values[0]),
            bsky_data_sync_tuple_changed,
            bsky_data_sync_error,
            NULL);
    free (zero_agenda);

    // Set initial value for "agenda need seconds".  This represents
    // the watch's value, rather than the synchronized value, for this
    // parameter.
    //
    // TODO: only set this value through
    // bsky_data_set_agenda_need_seconds or similar.
    //
    s_agenda_need_seconds = 24*60*60;

    APP_LOG(APP_LOG_LEVEL_INFO, "bsky_data_init: successful!");
    s_bsky_data_inited_already = true;
    return s_bsky_data_inited_already;
}

void bsky_data_deinit(void) {
    app_sync_deinit(&s_sync);
    if (s_buffer) {
        free (s_buffer);
        s_buffer = NULL;
    }
}

void bsky_data_update(void) {
    if (!bsky_data_init()) {
        APP_LOG(APP_LOG_LEVEL_WARNING,
                "bsky_data_update:"
                " failed to initialize, nothing to do");
        return;
    }

    // Decide whether an update is necessary
    const Tuple * agenda_need_seconds
        = app_sync_get(&s_sync, BSKY_DATAKEY_AGENDA_NEED_SECONDS);
    const Tuple * agenda_capacity_bytes
        = app_sync_get(&s_sync, BSKY_DATAKEY_AGENDA_CAPACITY_BYTES);
    if (agenda_need_seconds->value->int32 == s_agenda_need_seconds
        &&
        agenda_capacity_bytes->value->int32 == s_agenda_capacity_bytes)
    {
        APP_LOG(APP_LOG_LEVEL_DEBUG,
                "bsky_data_update: nothing to do");
        return;
    }

    // Send update using app_sync_set
    Tuplet values[] = {
        TupletInteger(
                BSKY_DATAKEY_AGENDA_NEED_SECONDS,
                s_agenda_need_seconds),
        TupletInteger(
                BSKY_DATAKEY_AGENDA_CAPACITY_BYTES,
                s_agenda_capacity_bytes),
    };
    AppMessageResult result = app_sync_set(
            &s_sync, values, sizeof(values)/sizeof(values[0]));
    if (result != APP_MSG_OK) {
        APP_LOG(APP_LOG_LEVEL_WARNING,
                "bsky_data_update: app_sync_set: %u",
                result);
    } else {
        APP_LOG(APP_LOG_LEVEL_INFO,
                "bsky_data_update: app_sync_set: ok!");
    }
}
