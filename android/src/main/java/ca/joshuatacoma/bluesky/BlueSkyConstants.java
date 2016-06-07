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
package ca.joshuatacoma.bluesky;

import java.util.UUID;

public class BlueSkyConstants
{
    static final UUID APP_UUID
        = new UUID(0xf205e9af41244829L, 0xa5bb53cc3f8b0362L);

    static final int AGENDA_NEED_SECONDS_KEY = 1;
    static final int AGENDA_CAPACITY_BYTES_KEY = 2;
    static final int AGENDA_KEY = 3;
    static final int AGENDA_VERSION_KEY = 4;
    static final int PEBBLE_NOW_UNIX_TIME_KEY = 5;
    static final int AGENDA_EPOCH_KEY = 6;
};
