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

import android.app.IntentService;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import java.util.Date;

import ca.joshuatacoma.bluesky.CalendarBridge;

/** Translate intents into potentially blocking I/O operations.
 */
public class MainService extends IntentService
{
    static final String TAG = "BlueSky";

    public MainService() {
        super("MainService");
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        Log.d(TAG, "service received intent");
        if (intent.getAction()==BlueSkyConstants.ACTION_SEND_AGENDA) {
            Log.i(TAG, "service received intent to send agenda");
            Date start
                = new Date(intent.getLongExtra(
                            BlueSkyConstants.EXTRA_START_TIME,
                            new Date().getTime()));
            Date end
                = new Date(intent.getLongExtra(
                            BlueSkyConstants.EXTRA_END_TIME,
                            start.getTime()+3*24*60*60*1000));
            int capacityBytes
                = intent.getIntExtra(
                        BlueSkyConstants.EXTRA_CAPACITY_BYTES,
                        64);
            CalendarBridge.sendAgenda(this, start, end, capacityBytes);
        }
    }
}
