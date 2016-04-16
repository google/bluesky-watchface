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

#include "modules/skyline.h"
#include "data.h"

static BSKY_Skyline *s_skyline_current;
static BSKY_Skyline *s_skyline_incoming;
static BSKY_data_skyline_handler s_skyline_handler;
static void *s_skyline_handler_context;

static void bsky_data_init(void) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bsky_data_init()");
    if (s_skyline_current == NULL) {
        s_skyline_current = bsky_skyline_create();
    }
    if (s_skyline_incoming == NULL) {
        s_skyline_incoming = bsky_skyline_create();
    }
    if (s_skyline_current != NULL) {
        bsky_skyline_rewrite_random (s_skyline_current);
    }
}

void bsky_data_skyline_subscribe (
        BSKY_data_skyline_handler handler,
        void * context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,
            "bsky_data_skyline_subscribe(%p, %p)",
            handler,
            context);
    bsky_data_init();
    s_skyline_handler = handler;
    s_skyline_handler_context = context;
    s_skyline_handler(s_skyline_handler_context, s_skyline_current);
}
