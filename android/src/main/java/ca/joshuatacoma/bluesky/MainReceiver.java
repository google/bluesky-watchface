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

import android.content.Context;
import android.content.ContentUris;
import android.util.Log;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.util.PebbleDictionary;

import java.util.Date;

import ca.joshuatacoma.bluesky.BlueSkyConstants;
import ca.joshuatacoma.bluesky.CalendarBridge;

public class MainReceiver extends PebbleKit.PebbleDataReceiver
{
    static final String TAG = "BlueSky";

    public MainReceiver() {
        super(BlueSkyConstants.APP_UUID);
    }

    @Override
    public void receiveData(
            Context context,
            int id,
            PebbleDictionary data)
    {
        Log.d(TAG, "receiveData");

        PebbleKit.sendAckToPebble(context, id);

        Log.d(TAG, "ACK");

        if (data.contains(BlueSkyConstants.AGENDA_NEED_SECONDS_KEY)
                && data.contains(BlueSkyConstants.AGENDA_CAPACITY_BYTES_KEY)
                && data.contains(BlueSkyConstants.PEBBLE_NOW_UNIX_TIME_KEY))
        {
            try {

                // Gather values from incoming data
                long agenda_need_seconds
                    = data.getInteger(BlueSkyConstants.AGENDA_NEED_SECONDS_KEY);
                long agenda_capacity_bytes
                    = data.getInteger(BlueSkyConstants.AGENDA_CAPACITY_BYTES_KEY);
                long pebble_now_unix_time
                    = data.getInteger(BlueSkyConstants.PEBBLE_NOW_UNIX_TIME_KEY);

                // Convert to local data types and add fudge factor for end time
                long extra_time_multiplier = 4;
                long end_unix_time
                    = pebble_now_unix_time
                    + (agenda_need_seconds * extra_time_multiplier);
                Date start_date
                    = new Date((long)pebble_now_unix_time*1000L);
                Date end_date
                    = new Date(end_unix_time*1000L);

                // Send a response synchronously, which is fine because this
                // code is not running on the UI thread anyway
                CalendarBridge.sendAgenda(
                        context,
                        start_date,
                        end_date,
                        (int) agenda_capacity_bytes);

            } catch(Exception e) {
                Log.e(TAG, e.toString());
            }
        }
        Log.d(TAG, "done");
    }
};
