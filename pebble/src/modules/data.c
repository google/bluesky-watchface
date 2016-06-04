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

static const char * s_key_name [BSKY_DATAKEY_MAX] = {
    [BSKY_DATAKEY_AGENDA_VERSION] = "BSKY_DATAKEY_AGENDA_VERSION",
    [BSKY_DATAKEY_AGENDA_EPOCH] = "BSKY_DATAKEY_AGENDA_EPOCH",
    [BSKY_DATAKEY_AGENDA_NEED_SECONDS] = "BSKY_DATAKEY_AGENDA_NEED_SECONDS",
    [BSKY_DATAKEY_AGENDA_CAPACITY_BYTES] = "BSKY_DATAKEY_AGENDA_CAPACITY_BYTES",
    [BSKY_DATAKEY_AGENDA] = "BSKY_DATAKEY_AGENDA",
    [BSKY_DATAKEY_PEBBLE_NOW_UNIX_TIME] = "BSKY_DATAKEY_PEBBLE_NOW_UNIX_TIME",
};

static const TupleType s_key_type [BSKY_DATAKEY_MAX] = {
    [BSKY_DATAKEY_AGENDA_NEED_SECONDS] = TUPLE_INT,
    [BSKY_DATAKEY_AGENDA_CAPACITY_BYTES] = TUPLE_INT,
    [BSKY_DATAKEY_AGENDA] = TUPLE_BYTE_ARRAY,
    [BSKY_DATAKEY_PEBBLE_NOW_UNIX_TIME] = TUPLE_INT,
    [BSKY_DATAKEY_AGENDA_VERSION] = TUPLE_INT,
    [BSKY_DATAKEY_AGENDA_EPOCH] = TUPLE_INT,
};

static const size_t s_key_size [BSKY_DATAKEY_MAX] = {
    [BSKY_DATAKEY_AGENDA_NEED_SECONDS] = sizeof(int32_t),
    [BSKY_DATAKEY_AGENDA_CAPACITY_BYTES] = sizeof(int32_t),
    [BSKY_DATAKEY_AGENDA] = 1024,
    [BSKY_DATAKEY_PEBBLE_NOW_UNIX_TIME] = sizeof(int32_t),
    [BSKY_DATAKEY_AGENDA_VERSION] = sizeof(int32_t),
    [BSKY_DATAKEY_AGENDA_EPOCH] = sizeof(int32_t),
};

static const bool s_key_incoming [BSKY_DATAKEY_MAX] = {
    [BSKY_DATAKEY_AGENDA] = true,
    [BSKY_DATAKEY_AGENDA_VERSION] = true,
    [BSKY_DATAKEY_AGENDA_EPOCH] = true,
};

static const bool s_key_outgoing [BSKY_DATAKEY_MAX] = {
    [BSKY_DATAKEY_AGENDA_NEED_SECONDS] = true,
    [BSKY_DATAKEY_AGENDA_CAPACITY_BYTES] = true,
    [BSKY_DATAKEY_PEBBLE_NOW_UNIX_TIME] = true,
};

static uint8_t s_agenda_buffer [1024] = {0};

union BSKY_Value {
    void * const ptr;
    int32_t int32;
};

static union BSKY_Value s_key_buffer [BSKY_DATAKEY_MAX] = {
    [BSKY_DATAKEY_AGENDA_NEED_SECONDS] = {.int32=24*60*60},
    [BSKY_DATAKEY_AGENDA_CAPACITY_BYTES] = {.int32=sizeof(s_agenda_buffer)},
    [BSKY_DATAKEY_AGENDA] = {.ptr=s_agenda_buffer},
};

static bool s_key_buffer_initialized [BSKY_DATAKEY_MAX] = {0};

// buffer_length is the number of bytes currently meaningful in each buffer.
static size_t s_key_buffer_length [BSKY_DATAKEY_MAX] = {0};

// Calculate the buffer size for an inbox or outbox.
//
// filter: an array of BSKY_DATAKEY_MAX bool values.
//
static size_t bsky_data_buffer_size(const bool * filter) {
    size_t buffer_size = 1;
    for (int key=0; key<BSKY_DATAKEY_MAX; ++key) {
        if (filter[key] && s_key_size[key]) {
            buffer_size += 7 + s_key_size[key];
        }
    }
    return buffer_size;
}

static void bsky_data_notify (const bool * key);

static void bsky_data_in_received(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_data_in_received");
    bool keys [BSKY_DATAKEY_MAX];
    Tuple * tuple = dict_read_first(iterator);
    while (tuple) {
        const uint32_t key = tuple->key;
        if (key >= BSKY_DATAKEY_MAX) {
            APP_LOG(APP_LOG_LEVEL_WARNING,
                    "bsky_data_in_received: ignoring unrecognized key %lu",
                    key);
        } else if (!s_key_incoming[key]) {
            APP_LOG(APP_LOG_LEVEL_WARNING,
                    "bsky_data_in_received:"
                    " ignoring key %s unexpected in inbox",
                    s_key_name[key]);
        } else if (tuple->type != s_key_type[key]) {
            APP_LOG(APP_LOG_LEVEL_WARNING,
                    "bsky_data_in_received:"
                    " ignoring bad value for key %s,"
                    " expected type %d, got %d",
                    s_key_name[key],
                    s_key_type[key],
                    tuple->type);
        } else if (tuple->length > s_key_size[key]) {
            APP_LOG(APP_LOG_LEVEL_WARNING,
                    "bsky_data_in_received:"
                    " ignoring oversized value for key %s,"
                    " expected max %u, got %d",
                    s_key_name[key],
                    s_key_size[key],
                    tuple->length);
        } else {
            switch (tuple->type) {
                case TUPLE_BYTE_ARRAY:
                case TUPLE_CSTRING:
                    persist_write_data (key, tuple->value->data, tuple->length);
                    memcpy (s_key_buffer[key].ptr, tuple->value->data, tuple->length);
                    s_key_buffer_length[key] = tuple->length;
                    s_key_buffer_initialized[key] = true;
                    APP_LOG(APP_LOG_LEVEL_INFO,
                            "bsky_data_in_received: %s (%d bytes)",
                            s_key_name[key],
                            tuple->length);
                    break;
                case TUPLE_UINT:
                case TUPLE_INT:
                    persist_write_int (key, tuple->value->int32);
                    s_key_buffer[key].int32 = tuple->value->int32;
                    s_key_buffer_initialized[key] = true;
                    APP_LOG(APP_LOG_LEVEL_INFO,
                            "bsky_data_in_received: %s = %ld",
                            s_key_name[key],
                            tuple->value->int32);
                    break;
            }
            keys[key] = true;
        }
        tuple = dict_read_next(iterator);
    }
    bsky_data_notify (keys);
}

static void bsky_data_in_dropped(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "bsky_data_in_dropped");
}

static void bsky_data_out_sent(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_data_out_sent");
    // TODO: remember that values flagged for sending have been sent
}

static void bsky_data_out_failed(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "bsky_data_out_failed");
    // TODO: remember that values flagged for sending have not been sent
}

bool bsky_data_init(void) {
    // We use static bool values to help ensure this method is
    // idempotent and can safely be called many times over as a way to
    // ensure, whenever we want to, that this module is initialized.

    static bool s_bsky_data_inited_already = false;
    static bool s_app_message_opened_already = false;

    if (s_bsky_data_inited_already) { return true; }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_data_init()");

    // Open the app_message inbox and outboxes.  Doing this before
    // registering app_message event handlers may result in dropping
    // some incoming messages, however we will be resilient to this.
    //
    if (!s_app_message_opened_already) {
        app_message_register_inbox_received(bsky_data_in_received);
        app_message_register_inbox_dropped(bsky_data_in_dropped);
        app_message_register_outbox_sent(bsky_data_out_sent);
        app_message_register_outbox_failed(bsky_data_out_failed);
        const uint32_t size_outbound = bsky_data_buffer_size(s_key_outgoing);
        const uint32_t size_inbound = bsky_data_buffer_size(s_key_incoming);
        AppMessageResult result = app_message_open(
                size_inbound,
                size_outbound);
        if (result != APP_MSG_OK) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "app_message_open: %u", result);
            return false;
        }
        s_app_message_opened_already = true;
    }

    APP_LOG(APP_LOG_LEVEL_INFO, "bsky_data_init: successful!");
    s_bsky_data_inited_already = true;
    return s_bsky_data_inited_already;
}

void bsky_data_deinit(void) {
}

int32_t bsky_data_int(uint32_t key) {
    const TupleType type = s_key_type[key];
    bool ok = key<BSKY_DATAKEY_MAX
        && type==TUPLE_INT
        && s_key_size[key]==sizeof(int32_t);
    if (!ok) {
        APP_LOG(APP_LOG_LEVEL_ERROR,
                "bsky_data_int: bad request for key %lu",
                key);
        return 0;
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_data_int: %s", s_key_name[key]);
    int32_t * const buffer = &s_key_buffer[key].int32;
    if (!s_key_buffer_initialized[key] && persist_exists(key)) {
        APP_LOG(APP_LOG_LEVEL_DEBUG,
                "bsky_data_int: loading from local storage");
        *buffer = persist_read_int(key);
        s_key_buffer_initialized[key] = true;
    }
    if (!s_key_buffer_initialized[key]) {
        APP_LOG(APP_LOG_LEVEL_DEBUG,
                "bsky_data_int: %s has no value",
                s_key_name[key]);
        return 0;
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_data_int: %s == %ld",
            s_key_name[key],
            s_key_buffer[key].int32);
    return *buffer;
}

const void * bsky_data_ptr(uint32_t key, size_t * length_bytes) {
    const TupleType type = s_key_type[key];
    bool ok = key<BSKY_DATAKEY_MAX
        && (type == TUPLE_BYTE_ARRAY || type == TUPLE_CSTRING)
        && s_key_buffer[key].ptr;
    if (!ok) {
        APP_LOG(APP_LOG_LEVEL_ERROR,
                "bsky_data_ptr: bad request for key %lu",
                key);
        *length_bytes = 0;
        return NULL;
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_data_ptr: %s", s_key_name[key]);
    void * const buffer = s_key_buffer[key].ptr;
    if (!s_key_buffer_initialized[key] && persist_exists(key)) {
        APP_LOG(APP_LOG_LEVEL_DEBUG,
                "bsky_data_ptr: attempting load from local storage");
        const size_t capacity = s_key_size[key];
        const size_t available = persist_get_size(key);
        if (available > capacity) {
            APP_LOG(APP_LOG_LEVEL_WARNING,
                    "bsky_data_ptr: local storage value is too large");
        } else {
            persist_read_data(key, buffer, available);
            s_key_buffer_initialized[key] = true;
            s_key_buffer_length[key] = available;
        }
    }
    if (!s_key_buffer_initialized[key]) {
        APP_LOG(APP_LOG_LEVEL_DEBUG,
                "bsky_data_ptr: %s has no value",
                s_key_name[key]);
        *length_bytes = 0;
        return NULL;
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_data_ptr: %s has value %u bytes long",
            s_key_name[key],
            s_key_buffer_length[key]);
    *length_bytes = s_key_buffer_length[key];
    return buffer;
}

void bsky_data_set_outgoing_int(uint32_t key, int32_t data) {
    const TupleType type = s_key_type[key];
    if (type != TUPLE_INT) {
        APP_LOG(APP_LOG_LEVEL_ERROR,
                "%s type is %d, got %d",
                s_key_name[key],
                type,
                TUPLE_INT);
    } else {
        s_key_buffer_initialized[key] = true;
        s_key_buffer[key].int32 = data;
    }
}

bool bsky_data_send_outgoing() {
    if (!bsky_data_init()) {
        APP_LOG(APP_LOG_LEVEL_WARNING,
                "bsky_data_send_outgoing:"
                " failed to initialize, nothing to do");
        return false;
    }
    DictionaryIterator * iterator;
    AppMessageResult result = app_message_outbox_begin(&iterator);
    if (result != APP_MSG_OK) {
        APP_LOG(APP_LOG_LEVEL_WARNING,
                "bsky_data_send_outgoing: dictionary error: %d",
                result);
        return false;
    }
    for (uint32_t key=0; key<BSKY_DATAKEY_MAX; ++key) {
        if (s_key_outgoing[key] && s_key_buffer_initialized[key]) {
            DictionaryResult dict_result = DICT_OK;
            switch (s_key_type[key]) {
                case TUPLE_INT:
                    APP_LOG(APP_LOG_LEVEL_DEBUG,
                            "writing %s to outbox dict",
                            s_key_name[key]);
                    dict_result = dict_write_int(
                            iterator,
                            key,
                            &s_key_buffer[key].int32,
                            sizeof(int32_t),
                            true);
                    break;
                default:
                    APP_LOG(APP_LOG_LEVEL_WARNING,
                            "skipping %s",
                            s_key_name[key]);
                    break;
            }
            if (dict_result != DICT_OK) {
                APP_LOG(APP_LOG_LEVEL_WARNING,
                        "dictionary error writing %s: %d",
                        s_key_name[key],
                        dict_result);
            }
        }
    }
    result = app_message_outbox_send();
    if (result!=APP_MSG_OK) {
        APP_LOG(APP_LOG_LEVEL_WARNING,
                "bsky_data_send_outgoing: AppMessageResult: %d",
                result);
        return false;
    }
    APP_LOG(APP_LOG_LEVEL_INFO,
            "bsky_data_send_outgoing: sent message");
    return true;
}

struct BSKY_DataReceiverInfo {
    BSKY_DataReceiver receiver;
    void * context;
    bool keys [BSKY_DATAKEY_MAX];
};

static struct BSKY_DataReceiverInfo s_subscribers [16];

static void bsky_data_notify (const bool * keys) {
    const size_t num_subscribers
        = sizeof(s_subscribers)
        / sizeof(s_subscribers[0]);
    for (size_t i=0; i<num_subscribers; ++i) {
        if (s_subscribers[i].receiver) {
            for (uint32_t key=0; key<BSKY_DATAKEY_MAX; ++key) {
                if (keys[key] && s_subscribers[i].keys[key]) {
                    s_subscribers[i].receiver(s_subscribers[i].context);
                    break;
                }
            }
        }
    }
}

bool bsky_data_subscribe (
        BSKY_DataReceiver receiver,
        void * context,
        uint32_t key) {
    if (key>BSKY_DATAKEY_MAX || !s_key_incoming[key]) {
        return true;
    }
    for (size_t i=0; i<sizeof(s_subscribers)/sizeof(s_subscribers[0]); ++i) {
        if (!s_subscribers[i].receiver) {
            s_subscribers[i].receiver = receiver;
            s_subscribers[i].context = context;
            s_subscribers[i].keys[key] = true;
            return true;
        }
    }
    return false;
}

void bsky_data_unsubscribe (
        BSKY_DataReceiver receiver,
        void * context) {
    for (size_t i=0; i<sizeof(s_subscribers)/sizeof(s_subscribers[0]); ++i) {
        if (s_subscribers[i].receiver==receiver
                && s_subscribers[i].context==context) {
            s_subscribers[i].receiver = NULL;
            s_subscribers[i].context = NULL;
            for (uint32_t key=0; key<BSKY_DATAKEY_MAX; ++key) {
                s_subscribers[i].keys[key] = 0;
            }
        }
    }
}
