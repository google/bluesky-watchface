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
import android.content.Context;
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

    public static void maybeSendAgendaUpdate(Context context) {
        if (PebbleState.getIsTransactionInProgress(context)) {
            Log.d(TAG,
                    "not attempting agenda update because another transaction"
                    +" is in progress");
            return;
        }

        Date lastAttempt = PebbleState.getAttemptTime(context);
        int nackCount = PebbleState.getNackCount(context);

        // If more than 5 NACKs since last ACKk and less than 10 seconds since
        // last attempt, don't bother.
        if (nackCount > 5 && lastAttempt.getTime() >= new Date().getTime()-10*1000) {
            Log.d(TAG,
                    "not attempting agenda update because:"
                    +" nack count="+String.valueOf(nackCount)
                    +", last attempt="+String.valueOf(lastAttempt));
            return;
        }

        try {
            // Create an intent that can cause MainService to send a
            // response.
            Intent sendAgendaIntent
                = new Intent(context, MainService.class)
                .setAction(BlueSkyConstants.ACTION_SEND_AGENDA)
                .putExtra(
                        BlueSkyConstants.EXTRA_CAPACITY_BYTES,
                        PebbleState.getAgendaCapacityBytes(context));

            if (context.startService(sendAgendaIntent)==null) {
                // TODO: supposing this happens in real use, what could it mean?  A
                // problem with permissions, maybe?  A reason to stop wasting
                // battery trying to do something that can't be done?  See also
                // the catch block below.
                Log.e(TAG, "failed to send intent to service");
            }
        } catch(Exception e) {
            Log.e(TAG, e.toString());
        }
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        Log.d(TAG, "service received intent");
        if (intent.getAction()==BlueSkyConstants.ACTION_SEND_AGENDA) {
            Log.i(TAG, "service received intent to send agenda");
            Date start = new Date();
            Date end = new Date(start.getTime()+7*24*60*60*1000);
            int capacityBytes
                = intent.getIntExtra(
                        BlueSkyConstants.EXTRA_CAPACITY_BYTES,
                        64);
            CalendarBridge.sendAgenda(this, start, end, capacityBytes);
        }
    }
}
