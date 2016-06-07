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
import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.provider.CalendarContract.Instances;
import android.util.Log;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.util.PebbleDictionary;

import java.util.Arrays;
import java.util.Date;

import ca.joshuatacoma.bluesky.BlueSkyConstants;

public class CalendarBridge
{
    static final String TAG = "BlueSky";

    static public void sendAgenda(
            Context context,
            Date start_date,
            Date end_date,
            int agenda_capacity_bytes)
    {
        Log.d(TAG,
                "sendAgenda start_date="
                +String.valueOf(start_date)
                +",end_date="
                +String.valueOf(end_date)
                +",agenda_capacity_bytes="
                +String.valueOf(agenda_capacity_bytes));

        ContentResolver cr = context.getContentResolver();

        Uri.Builder builder = Instances.CONTENT_URI.buildUpon();
        ContentUris.appendId(builder, start_date.getTime());
        ContentUris.appendId(builder, end_date.getTime());

        // Include only events from calendars the user has selected as
        // visible-by-default in the Calendar app.  Ignore all-day events just
        // because there's no obviously meaningful way to display them in this
        // system.  Finally, also ignore canceled events.
        String filter
            = Instances.VISIBLE+" = 1"
            + " AND "
            + Instances.ALL_DAY+" = 0"
            + " AND "
            + Instances.STATUS+" <> "+String.valueOf(Instances.STATUS_CANCELED);

        // Sort events by their beginning time so that, if there are too many,
        // the current and imminent events will get preference.
        String sort_order = Instances.BEGIN + " ASC";

        // Make sure we're dealing with a multiple of 4 bytes to hold pairs of 2
        // byte integers.
        agenda_capacity_bytes -= agenda_capacity_bytes % 4;
        byte[] agenda = new byte[agenda_capacity_bytes];

        // "short time" is time, in minutes, from an epoch.  This is not the
        // Unix epoch, it is defined and kept in context with "short time"
        // values so that they can be converted to absolute time later.  In this
        // case, the epoch will be start_date.

        Cursor cursor = cr.query(
                builder.build(),
                INSTANCE_PROJECTION,
                filter,
                null,
                sort_order);

        int iagenda = 0;
        try {
            while (iagenda<agenda_capacity_bytes && cursor.moveToNext()) {
                long[] as_unix_milliseconds = new long[] {
                    cursor.getLong(PROJECTION_BEGIN_INDEX),
                    cursor.getLong(PROJECTION_END_INDEX),
                };
                Log.d(TAG,
                        "begin="+String.valueOf(as_unix_milliseconds[0])
                        +",end="+String.valueOf(as_unix_milliseconds[1]));
                short[] as_short_time = new short[2];
                boolean overflowed = false;
                for (int i=0; i<2; ++i) {
                    long short_time
                        = (as_unix_milliseconds[i] - start_date.getTime())
                        / (60*1000);
                    overflowed
                        = short_time < Short.MIN_VALUE
                        || Short.MAX_VALUE < short_time;
                    if (overflowed) {
                        Log.d(TAG,
                                "short_overflow="+String.valueOf(short_time));
                        break;
                    }
                    as_short_time[i] = (short) short_time;
                }
                if (overflowed) {
                    continue;
                }
                for (int i=0; i<2; ++i) {
                    agenda[iagenda++] = (byte) (as_short_time[i] & 0x00ff);
                    agenda[iagenda++] = (byte) ((as_short_time[i] & 0xff00) >> 8);
                }
                Log.d(TAG,
                        "begin_short="+String.valueOf(as_short_time[0])
                        +",end_short="+String.valueOf(as_short_time[1]));
            }
        } finally {
            cursor.close();
        }

        Log.d(TAG,
                "collected iagenda="+String.valueOf(iagenda)
                +",n="+String.valueOf(iagenda/4));

        int version = (int) (new Date().getTime() % 0xffffffffL);

        PebbleDictionary message = new PebbleDictionary();
        message.addBytes(
                BlueSkyConstants.AGENDA_KEY,
                Arrays.copyOfRange(agenda, 0, iagenda));
        message.addInt32(
                BlueSkyConstants.AGENDA_VERSION_KEY,
                version);
        message.addInt32(
                BlueSkyConstants.AGENDA_EPOCH_KEY,
                (int) (start_date.getTime()/1000));
        PebbleKit.sendDataToPebble(
                context,
                BlueSkyConstants.APP_UUID,
                message);
        Log.d(TAG, "sent agenda to Pebble");
    }

    private static final String[] INSTANCE_PROJECTION = new String[] {
        Instances.BEGIN,
        Instances.END,
    };
    private static final int PROJECTION_BEGIN_INDEX = 0;
    private static final int PROJECTION_END_INDEX = 1;
};
