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

int bsky_skyline_index_from_time(int hour, int minute) {
    return (hour % 24) * 60/5 + (minute % 60) / 5;
}

struct BSKY_Skyline {
    int8_t heights [BSKY_SKYLINE_LEN];
};

BSKY_Skyline * bsky_skyline_create() {
    BSKY_Skyline * skyline = malloc(sizeof(*skyline));
    if (skyline) {
        memset(skyline, 0, sizeof(skyline));
    }
    return skyline;
}

void bsky_skyline_destroy(BSKY_Skyline * skyline) {
    free (skyline);
}

void bsky_skyline_copy(BSKY_Skyline * dst, const BSKY_Skyline * src) {
    memcpy(dst, src, sizeof(*src));
}

void bsky_skyline_set_heights(
        BSKY_Skyline * skyline,
        int start,
        int end,
        int8_t heights) {
    for (int i=start; i<end; ++i) {
        skyline->heights[i]=heights;
    }
}

const int8_t * bsky_skyline_get_heights(const BSKY_Skyline * skyline) {
    return skyline->heights;
}

void bsky_skyline_rewrite_random(BSKY_Skyline * skyline) {
    for (size_t i=0; i<BSKY_SKYLINE_LEN; ++i) {
        skyline->heights[i] = ((i/60) % 4) * 40;
    }
}
