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

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.widget.Toast;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.util.PebbleDictionary;

import java.util.UUID;

public class MainActivity extends Activity
{
    static final java.util.UUID BLUESKY_UUID
        = new java.util.UUID(0xf205e9af41244829L, 0xa5bb53cc3f8b0362L);

    static final int BLUESKY_AGENDA_NEEDED_KEY = 1;

    private PebbleKit.PebbleDataReceiver mReceiver;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
    }

    @Override
    public void onResume()
    {
        super.onResume();
        boolean isConnected = PebbleKit.isWatchConnected(this);
        boolean isAppMessageSupported = PebbleKit.areAppMessagesSupported(this);

        if (mReceiver == null)
        {
            mReceiver = new PebbleKit.PebbleDataReceiver(BLUESKY_UUID)
            {
                @Override
                public void receiveData(
                        Context context,
                        int id,
                        PebbleDictionary data)
                {
                    PebbleKit.sendAckToPebble(getApplicationContext(), id);

                    if (data.contains(BLUESKY_AGENDA_NEEDED_KEY))
                    {
                        Toast
                            .makeText(
                                    context,
                                    "Agenda is needed: "
                                        + String.valueOf(
                                            data.getInteger(
                                                BLUESKY_AGENDA_NEEDED_KEY)),
                                    Toast.LENGTH_LONG)
                            .show();
                    }
                }
            };
        }

        PebbleKit.registerReceivedDataHandler(this, mReceiver);
    }

    @Override
    protected void onPause()
    {
        super.onPause();

        if (mReceiver != null)
        {
            unregisterReceiver(mReceiver);
            mReceiver = null;
        }
    }
}
