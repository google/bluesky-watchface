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

// All known data keys, synchronized with ../../appinfo.js#appKeys.
//
enum BSKY_DataKey {
    BSKY_DATAKEY_AGENDA_NEED_SECONDS = 1,
    BSKY_DATAKEY_AGENDA_CAPACITY_BYTES = 2,
    BSKY_DATAKEY_AGENDA = 3,
    BSKY_DATAKEY_AGENDA_VERSION = 4,
    BSKY_DATAKEY_PEBBLE_NOW_UNIX_TIME = 5,
    BSKY_DATAKEY_AGENDA_EPOCH = 6,
    BSKY_DATAKEY_FACE_HOURS = 7,
    BSKY_DATAKEY_FACE_ORIENTATION = 8,
    BSKY_DATAKEY_MAX = 9, // largest key + 1
};

enum BSKY_Data_FaceOrientation {
    BSKY_DATA_FACE_ORIENTATION_MIDNIGHT_TOP = 0,
    BSKY_DATA_FACE_ORIENTATION_NOON_TOP = 1,
};

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

// Retrieve a copy of the int32 value of a key.
//
// Returns: the value of the key if that value is an int, otherwise zero.
//
int32_t bsky_data_int(uint32_t key);

// Retrieve a pointer to the value of a key where that value is a byte array.
//
// key: a key whose value is known to be a byte array.
// length_bytes: the address where the length of the array will be written.
//
// Returns: if the key has a value that is an array, then a pointer to the
// array; otherwise NULL.
//
const void * bsky_data_ptr(uint32_t key, size_t * length_bytes);

// Set an int value to be sent the next time bsky_data_send_outgoing is called.
//
// TODO: refactor this API to just "set_int" or similar;
// bsky_data_send_outgoing can become something more like "synchronize" similar
// to AppSync API.
//
void bsky_data_set_outgoing_int(uint32_t key, int32_t data);

bool bsky_data_send_outgoing();

// Function type for data update subscriber callback functions.
//
typedef void (*BSKY_DataReceiver) (void * context);

// Subscribe to data updates.
//
// May be called many times with different contexts and keys.
//
// Returns true if subscription was successful, otherwise false.
//
bool bsky_data_subscribe (
        BSKY_DataReceiver receiver,
        void * context,
        uint32_t key);

// Unsubscribe from all data updates for a given receiver and context.
// 
// Has no effect unless bsky_data_subscribe was previously called with the same
// receiver and context.
//
void bsky_data_unsubscribe (BSKY_DataReceiver receiver, void * context);
