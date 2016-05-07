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

#include "modules/data.h"
#include "windows/main_window.h"

static void init() {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "init()");
    bsky_data_init();
    main_window_push();
}

static void deinit() {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "deinit()");
    bsky_data_deinit();
}

int main(void) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "main()");
    init();
    app_event_loop();
    deinit();
}
