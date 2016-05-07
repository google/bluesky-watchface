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
import android.widget.Toast;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.util.PebbleDictionary;

import java.util.UUID;

public class MainReceiver extends PebbleKit.PebbleDataReceiver
{
    static final java.util.UUID BLUESKY_UUID
        = new java.util.UUID(0xf205e9af41244829L, 0xa5bb53cc3f8b0362L);

    static final int BLUESKY_AGENDA_NEED_SECONDS_KEY = 1;
    static final int BLUESKY_AGENDA_CAPACITY_BYTES = 2;
    static final int BLUESKY_AGENDA_KEY = 3;

    public MainReceiver() {
        super(BLUESKY_UUID);
    }

    @Override
    public void receiveData(
            Context context,
            int id,
            PebbleDictionary data)
    {
        PebbleKit.sendAckToPebble(context, id);

        if (data.contains(BLUESKY_AGENDA_NEED_SECONDS_KEY)
                && data.contains(BLUESKY_AGENDA_CAPACITY_BYTES))
        {
            long agenda_need_seconds
                = data.getInteger(BLUESKY_AGENDA_NEED_SECONDS_KEY);
            long agenda_capacity_bytes
                = data.getInteger(BLUESKY_AGENDA_CAPACITY_BYTES);

            Toast
                .makeText(
                        context,
                        "Agenda is needed: "
                            + String.valueOf(
                                data.getInteger(BLUESKY_AGENDA_NEED_SECONDS_KEY)),
                        Toast.LENGTH_LONG)
                .show();

            PebbleDictionary message = new PebbleDictionary();
            message.addBytes(BLUESKY_AGENDA_KEY, new byte[] { 0x12, 0x34, 0x56, 0x78 });
            PebbleKit.sendDataToPebble(context, BLUESKY_UUID, message);
        }
    }
};
