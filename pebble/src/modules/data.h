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

typedef void (*BSKY_data_skyline_handler) (
        void * context,
        const BSKY_Skyline * skyline);

// Set the handler for skyline updates.
//
// The existing handler, if any, will be replaced.  A NULL handler
// indicates that the app is not interested in skyline updates.  The
// implementation may decide not to request updates in this case.
//
void bsky_data_skyline_subscribe (
        BSKY_data_skyline_handler handler,
        void * context);
