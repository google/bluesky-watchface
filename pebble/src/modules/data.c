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

static BSKY_DataReceiver s_receiver;
static void *s_receiver_context;
static int32_t s_agenda_need_seconds;
static int32_t s_agenda_capacity_bytes;
static int32_t s_agenda_length_bytes;
static const uint8_t * s_agenda;
static int32_t s_agenda_version;
static int32_t s_agenda_epoch;
static bool s_update_in_progress = false;

#define MAX_AGENDA_CAPACITY_BYTES (1<<10)
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
    BSKY_DATAKEY_AGENDA_VERSION = 4,
    BSKY_DATAKEY_PEBBLE_NOW_UNIX_TIME = 5,
    BSKY_DATAKEY_AGENDA_EPOCH = 6,
};

static void bsky_data_call_receiver(bool changed) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_data_call_receiver()");
    if (!s_receiver || !bsky_data_init()) {
        APP_LOG(APP_LOG_LEVEL_DEBUG,
                "bsky_data_call_receiver: init failed or no receiver");
        return;
    }
    struct BSKY_DataReceiverArgs args = {
        .agenda = (struct BSKY_DataEvent*) s_agenda,
        .agenda_length = (s_agenda_version
                ? (s_agenda_length_bytes / sizeof(*args.agenda))
                : 0),
        .agenda_changed = changed,
        .agenda_epoch = s_agenda_epoch,
    };
    s_receiver(s_receiver_context, args);
}

void bsky_data_set_receiver (
        BSKY_DataReceiver receiver,
        void * context) {
    if (s_receiver == receiver && s_receiver_context == context) {
        return;
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_data_set_receiver(%p, %p)",
            receiver,
            context);
    s_receiver = receiver;
    s_receiver_context = context;
    bsky_data_call_receiver(false);
}

static void bsky_data_sync_tuple_changed (
        const uint32_t key,
        const Tuple *new_tuple,
        const Tuple *old_tuple,
        void *context) {
    s_update_in_progress = false;
    static bool call_receiver = false;
    static bool agenda_version_changed = false;
    switch (key) {
        case BSKY_DATAKEY_AGENDA_NEED_SECONDS:
            APP_LOG(APP_LOG_LEVEL_DEBUG,
                    "bsky_data_sync_update:"
                    " agenda_need_seconds=%ld"
                    ",expected=%ld",
                    new_tuple->value->int32,
                    s_agenda_need_seconds);
            break;
        case BSKY_DATAKEY_AGENDA_CAPACITY_BYTES:
            APP_LOG(APP_LOG_LEVEL_DEBUG,
                    "bsky_data_sync_update:"
                    " agenda_capacity_bytes=%ld"
                    ",expected=%ld",
                    new_tuple->value->int32,
                    s_agenda_capacity_bytes);
            break;
        case BSKY_DATAKEY_AGENDA:
            s_agenda = new_tuple->value->data;
            s_agenda_length_bytes = new_tuple->length;
            call_receiver = true;
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
        case BSKY_DATAKEY_AGENDA_VERSION:
            APP_LOG(APP_LOG_LEVEL_INFO,
                    "bsky_data_sync_update:"
                    " agenda_version=%ld"
                    ",s_agenda_version=%ld",
                    new_tuple->value->int32,
                    s_agenda_version);
            if (new_tuple->value->int32 != s_agenda_version) {
                // It's possible to receive an agenda version change out
                // of sync with the agenda update it's really about.
                // Therefore this approach may be a bit silly.
                agenda_version_changed = true;
                s_agenda_version = new_tuple->value->int32;
            }
            break;
        case BSKY_DATAKEY_PEBBLE_NOW_UNIX_TIME:
            APP_LOG(APP_LOG_LEVEL_DEBUG,
                    "bsky_data_sync_update:"
                    " pebble_now=%ld"
                    ",time=%ld",
                    new_tuple->value->int32,
                    (int32_t) time(NULL));
            break;
        case BSKY_DATAKEY_AGENDA_EPOCH:
            APP_LOG(APP_LOG_LEVEL_INFO,
                    "bsky_data_sync_update:"
                    " agenda_epoch=%ld",
                    new_tuple->value->int32);
            s_agenda_epoch = new_tuple->value->int32;
            break;
    }
    if (call_receiver) {
        APP_LOG(APP_LOG_LEVEL_DEBUG,
                "bsky_data_sync_update: persisting agenda");
        persist_write_data(
                BSKY_DATAKEY_AGENDA,
                s_agenda,
                s_agenda_length_bytes);
        persist_write_int(
                BSKY_DATAKEY_AGENDA_VERSION,
                s_agenda_version);
        persist_write_int(
                BSKY_DATAKEY_AGENDA_EPOCH,
                s_agenda_epoch);
        bsky_data_call_receiver(agenda_version_changed);
        call_receiver = false;
        agenda_version_changed = false;
    }
}

static void bsky_data_sync_error (
        DictionaryResult dict_error,
        AppMessageResult app_message_error,
        void *context) {
    s_update_in_progress = false;
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "bsky_data_sync_error(%u,%u)",
            dict_error,
            app_message_error);
    // TODO: switch on error to ignore for now (present behavior), retry
    // with another call to bsky_data_update, or disable bsky_data for
    // some time.
    const char * err_name = NULL;
    switch (dict_error) {
        case DICT_NOT_ENOUGH_STORAGE:
            err_name = "DICT_NOT_ENOUGH_STORAGE"; break;
        case DICT_INVALID_ARGS:
            err_name = "DICT_INVALID_ARGS"; break;
        case DICT_INTERNAL_INCONSISTENCY:
            err_name = "DICT_INTERNAL_INCONSISTENCY"; break;
        case DICT_MALLOC_FAILED:
            err_name = "DICT_MALLOC_FAILED"; break;
        case DICT_OK:
            break;
    }
    if (err_name) {
        APP_LOG(APP_LOG_LEVEL_ERROR,
                "bsky_data_sync_error: %s",
                err_name);
    }
}

static AppSync s_sync;
static uint8_t * s_buffer;

bool bsky_data_init(void) {
    // We use static bool values help to ensure this method is
    // idempotent and can safely be called many times over as a way to
    // ensure, whenever we want to, that this module is initialized.

    static bool s_bsky_data_inited_already = false;
    static bool s_app_message_opened_already = false;
    static bool s_app_sync_inited_already = false;

    if (s_bsky_data_inited_already) { return true; }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_data_init()");

    s_agenda_capacity_bytes = MAX_AGENDA_CAPACITY_BYTES;

    // Open the app_message inbox and outboxes.  Doing this before
    // registering app_message event handlers may result in dropping
    // some incoming messages, however we will be resilient to this.
    //
    if (!s_app_message_opened_already) {
        const uint32_t size_outbound = dict_calc_buffer_size(
                3,
                sizeof(int32_t),
                sizeof(int32_t),
                sizeof(int32_t));
        AppMessageResult result; // lint...
        const uint32_t size_inbound = dict_calc_buffer_size(
                2,
                s_agenda_capacity_bytes,
                sizeof(int32_t),
                sizeof(int32_t));
        APP_LOG(APP_LOG_LEVEL_DEBUG,
                "attempting app_message_open:"
                " size_inbound=%lu,size_outbound=%lu",
                size_inbound,
                size_outbound);
        result = app_message_open(
                size_inbound,
                size_outbound);
        if (result != APP_MSG_OK) {
            APP_LOG(APP_LOG_LEVEL_ERROR,
                    "app_message_open: %u",
                    result);
            return false;
        }
        s_app_message_opened_already = true;
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
    if (!s_app_sync_inited_already) {
        if (s_buffer) {
            free (s_buffer);
            s_buffer = NULL;
        }
        uint8_t * initial_agenda = NULL;
        size_t initial_agenda_length = 0;
        if (!persist_exists(BSKY_DATAKEY_AGENDA)) {
            initial_agenda = calloc (s_agenda_capacity_bytes, 1);
            if (!initial_agenda) {
                APP_LOG(APP_LOG_LEVEL_ERROR, "bsky_data_init: calloc failure");
                return false;
            }
            initial_agenda_length = s_agenda_capacity_bytes;
            APP_LOG(APP_LOG_LEVEL_DEBUG,
                    "bsky_data_init: starting with zero agenda");
        } else {
            size_t size = persist_get_size(BSKY_DATAKEY_AGENDA);
            initial_agenda = malloc (size);
            if (!initial_agenda) {
                APP_LOG(APP_LOG_LEVEL_ERROR, "bsky_data_init: malloc failure");
                return false;
            }
            initial_agenda_length = persist_read_data(
                    BSKY_DATAKEY_AGENDA,
                    initial_agenda,
                    size);
            APP_LOG(APP_LOG_LEVEL_DEBUG,
                    "bsky_data_init: starting with persisted agenda");
        }
        if (persist_exists(BSKY_DATAKEY_AGENDA_VERSION)) {
            s_agenda_version = persist_read_int(BSKY_DATAKEY_AGENDA_VERSION);
        }
        if (persist_exists(BSKY_DATAKEY_AGENDA_EPOCH)) {
            s_agenda_epoch = persist_read_int (BSKY_DATAKEY_AGENDA_EPOCH);
        }
        Tuplet initial_values[] = {
            TupletInteger(
                    BSKY_DATAKEY_AGENDA_NEED_SECONDS, (int32_t) 0),
            TupletInteger(
                    BSKY_DATAKEY_AGENDA_CAPACITY_BYTES, (int32_t) 0),
            TupletBytes(
                    BSKY_DATAKEY_AGENDA,
                    initial_agenda,
                    initial_agenda_length),
            TupletInteger(
                    BSKY_DATAKEY_AGENDA_VERSION,
                    s_agenda_version),
            TupletInteger(
                    BSKY_DATAKEY_PEBBLE_NOW_UNIX_TIME,
                    (int32_t) time(NULL)),
            TupletInteger(
                    BSKY_DATAKEY_AGENDA_EPOCH,
                    s_agenda_epoch),
        };
        uint32_t buffer_size
            = dict_calc_buffer_size_from_tuplets(
                initial_values,
                sizeof(initial_values)/sizeof(initial_values[0]))
            + (s_agenda_capacity_bytes - initial_agenda_length);
        s_buffer = malloc(buffer_size);
        if (!s_buffer) {
            free(initial_agenda);
            APP_LOG(APP_LOG_LEVEL_ERROR,
                    "bsky_data_init: malloc failed to allocate sync buffer");
            return false;
        }
        APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_data_init: app_sync_init()");
        app_sync_init(
                &s_sync,
                s_buffer,
                buffer_size,
                initial_values,
                sizeof(initial_values)/sizeof(initial_values[0]),
                bsky_data_sync_tuple_changed,
                bsky_data_sync_error,
                NULL);
        free (initial_agenda);
        s_app_sync_inited_already = true;
    }

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
    if (s_update_in_progress) {
        APP_LOG(APP_LOG_LEVEL_DEBUG,
                "bsky_data_update:"
                " update in progress, nothing to do");
        return;
    }
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
        agenda_capacity_bytes->value->int32 == s_agenda_capacity_bytes
        &&
        s_agenda_version != 0)
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
        TupletInteger(
                BSKY_DATAKEY_PEBBLE_NOW_UNIX_TIME,
                (int32_t) time(NULL)),
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
        s_update_in_progress = true;
    }
}
