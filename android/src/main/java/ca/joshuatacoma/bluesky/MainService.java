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

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

import ca.joshuatacoma.bluesky.CalendarBridge;

public class MainService extends Service
{
    @Override
    public IBinder onBind(Intent intent) {
        // This service does not support binding.
        return null;
    }

    @Override
    public void onCreate() {
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // TODO: subscribe to calendar content provider updates?
        return 0;
    }

    @Override
    public void onDestroy() {
    }

}
