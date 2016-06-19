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

import java.util.Date;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class MainActivity extends Activity
{
    static final String TAG = "BlueSkyActivity";

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
    }

    public void onResume()
    {
        super.onResume();
        try {
            // Gather values from incoming data.
            long agenda_need_seconds = 24*60*60;
            long agenda_capacity_bytes = 1024;
            long pebble_now_unix_time = new Date().getTime()/1000;

            // Convert to local data types and add fudge factor for end
            // time.
            long extra_time_multiplier = 4;
            long end_unix_time
                = pebble_now_unix_time
                + (agenda_need_seconds * extra_time_multiplier);

            // Create an intent that can cause MainService to send a
            // response.
            Intent sendAgendaIntent
                = new Intent(this, MainService.class)
                .setAction(BlueSkyConstants.ACTION_SEND_AGENDA)
                .putExtra(
                        BlueSkyConstants.EXTRA_START_TIME,
                        (long)pebble_now_unix_time*1000L)
                .putExtra(
                        BlueSkyConstants.EXTRA_END_TIME,
                        (long)end_unix_time*1000L)
                .putExtra(
                        BlueSkyConstants.EXTRA_CAPACITY_BYTES,
                        (int)agenda_capacity_bytes);

            if (startService(sendAgendaIntent)==null) {
                Log.e(TAG, "failed to send intent to service");
            }
        } catch(Exception e) {
            Log.e(TAG, e.toString());
        }
    }
}
