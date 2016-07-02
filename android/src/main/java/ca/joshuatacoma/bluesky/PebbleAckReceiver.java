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
import android.util.Log;

import com.getpebble.android.kit.PebbleKit;

import ca.joshuatacoma.bluesky.BlueSkyConstants;

public class PebbleAckReceiver extends PebbleKit.PebbleAckReceiver
{
    static final String TAG = "BlueSky";

    public PebbleAckReceiver() {
        super(BlueSkyConstants.APP_UUID);
    }

    @Override
    public void receiveAck(
            Context context,
            int transactionId)
    {
        Log.d(TAG, "receiveAck(" + String.valueOf(transactionId) + ")");
        PebbleState.recordAck(context, transactionId);
    }
};
