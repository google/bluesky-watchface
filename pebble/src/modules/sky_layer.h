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

// A sky layer, displaying the yellow sun against 24 hours of blue sky.
typedef struct BSKY_SkyLayer BSKY_SkyLayer;

// Create and return a new sky layer, or NULL.
BSKY_SkyLayer * bsky_sky_layer_create(GRect frame);

// Deallocate a non-NULL sky layer.
void bsky_sky_layer_destroy(BSKY_SkyLayer * sky_layer);

// Get the managed Pebble layer.
Layer * bsky_sky_layer_get_layer(BSKY_SkyLayer * sky_layer);

// Set the position of the sun.
void bsky_sky_layer_set_time(
        BSKY_SkyLayer * sky_layer,
        time_t time);
