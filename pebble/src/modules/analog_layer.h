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

// A Blue Sky analog layer.
typedef struct BSKY_AnalogLayer BSKY_AnalogLayer;

// Create and return a new Blue Sky analog layer, or NULL.
BSKY_AnalogLayer * bsky_analog_layer_create(GRect frame);

// Deallocate a non-NULL Blue Sky analog layer.
void bsky_analog_layer_destroy(BSKY_AnalogLayer * analog_layer);

// Get the managed Pebble layer.
Layer * bsky_analog_layer_get_layer(BSKY_AnalogLayer * analog_layer);

// Set the moment in time that will be displayed.
void bsky_analog_layer_set_time(
        BSKY_AnalogLayer * analog_layer,
        time_t time);
