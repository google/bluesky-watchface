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
static int32_t s_agenda_needed;

// All known data keys, from appinfo.js#appKeys
//
// For each key, the most essential information is: in what direction is
// this key sent, and what is the maximum length of a value associated
// with that key.
//
enum BSKY_DataKey {
    BSKY_DATAKEY_AGENDA_NEEDED = 1,
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
        case BSKY_DATAKEY_AGENDA_NEEDED:
            APP_LOG(APP_LOG_LEVEL_INFO,
                    "bsky_data_sync_update:"
                    " agenda_needed=%ld"
                    ",s_agenda_needed=%ld",
                    new_tuple->value->int32,
                    s_agenda_needed);
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
}

static AppSync s_sync;
static uint8_t * s_buffer;

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
    const size_t buffer_size = dict_calc_buffer_size(
            3,
            sizeof(int32_t),
            sizeof(int32_t),
            sizeof(int32_t));
    if (!s_buffer) {
        s_buffer = malloc(buffer_size);
        if (!s_buffer) {
            APP_LOG(APP_LOG_LEVEL_ERROR,
                    "malloc failed for sync dict buffer");
            return false;
        }
    }

    // Rough calculation of inbound buffer size.
    const uint32_t size_inbound = dict_calc_buffer_size(
            4,
            sizeof(int32_t),
            sizeof(int32_t),
            sizeof(int32_t),
            sizeof(int32_t));

    // Rough calculation of outbound buffer size.
    const uint32_t size_outbound = dict_calc_buffer_size(
            4,
            sizeof(int32_t),
            sizeof(int32_t),
            sizeof(int32_t),
            sizeof(int32_t));

    // This should already be idempotent, but it might fail if inbox and
    // outbox are already "opened".  Therefore, do this step last and
    // count the entire operation a failure if it failed.
    //
    // Note: the only reason this should ever fail is "out of memory".
    // That is, it's not dependent on Bluetooth connection status or
    // anything similarly unpredictable.
    //
    AppMessageResult result = app_message_open(
            size_inbound,
            size_outbound);
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
    Tuplet initial_values[] = {
        TupletInteger(BSKY_DATAKEY_AGENDA_NEEDED, (int32_t) 0),
    };
    app_sync_init(
            &s_sync,
            s_buffer,
            buffer_size,
            initial_values,
            sizeof(initial_values)/sizeof(initial_values[0]),
            bsky_data_sync_tuple_changed,
            bsky_data_sync_error,
            NULL);

    // Set initial value for "agenda needed".  This represents the
    // watch's value, rather than the synchronized value, for this
    // parameter.
    //
    // TODO: only set this value through bsky_data_set_agenda_needed or
    // similar.
    //
    s_agenda_needed = 24*60*60;

    APP_LOG(APP_LOG_LEVEL_INFO, "bsky_data_init: successful!");
    s_bsky_data_inited_already = true;

    return s_bsky_data_inited_already;
}

void bsky_data_deinit(void) {
    app_sync_deinit(&s_sync);
    if (s_buffer) {
        free (s_buffer);
    }
}

void bsky_data_update(void) {
    if (!bsky_data_init()) {
        APP_LOG(APP_LOG_LEVEL_INFO,
                "bsky_data_update: not initialized");
        return;
    }

    Tuplet values[] = {
        TupletInteger(BSKY_DATAKEY_AGENDA_NEEDED, (int32_t) 24*60*60),
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
