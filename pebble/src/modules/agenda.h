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

// Start and end times for an event as minutes relative to a custom epoch.
//
struct BSKY_AgendaEvent {
    int16_t rel_start;
    int16_t rel_end;
};

struct BSKY_Agenda {
    const struct BSKY_AgendaEvent * events;
    int32_t events_length;
    int32_t epoch;
    struct tm epoch_wall_time;
    int16_t * events_by_height;
};

void bsky_agenda_init ();

void bsky_agenda_deinit ();

const struct BSKY_Agenda * bsky_agenda_read ();
