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

typedef struct BSKY_Skyline BSKY_Skyline;

// The precision of the skyline, in seconds per data point.
//
#define BSKY_SKYLINE_PRECISION_SECONDS (5*60)

// The resolution of the skyline is one point per 5 minutes for 24 hours
// for a total of 288 points.  Given a height map allowing up to 256
// possible values per point, that's also 288 bytes, or a little over
// one eigth of total stack space!
//
#define BSKY_SKYLINE_LEN (24*60*60/BSKY_SKYLINE_PRECISION_SECONDS)

// Compute from a non-negative hour and minute to a skyline index.
//
// It is the caller's responsibility to ensure that the hour and minute
// are not negative.
//
int bsky_skyline_index_from_time(int hour, int minute);

// Create and return a new skyline, or NULL.
//
BSKY_Skyline * bsky_skyline_create();

// Destroy a non-NULL skyline.
//
void bsky_skyline_destroy(BSKY_Skyline * skyline);

// Copy all data from one skyline into another.
//
void bsky_skyline_copy(BSKY_Skyline * dst, const BSKY_Skyline * src);

// Set the height of the skyline from start to end.
//
// If the start time is after the end time, then the range of time will
// span smoothly along the 24 hour circle, simply jumping the midnight
// divide.
//
// Both start and end should be computed using
// bsky_skyline_index_from_time().
//
void bsky_skyline_set_heights(
        BSKY_Skyline * skyline,
        int start,
        int end,
        int8_t heights);

// Height of skyline beginning at midnight.
//
const int8_t * bsky_skyline_get_heights(const BSKY_Skyline * skyline);

// Rewrite the content of a skyline to be something interesting.
//
// This should only be used for testing, to generate non-zero skylines
// for display.  It's probably not actually random, but it's also not
// supposed to actually mean anything.
//
void bsky_skyline_rewrite_random(BSKY_Skyline * skyline);
