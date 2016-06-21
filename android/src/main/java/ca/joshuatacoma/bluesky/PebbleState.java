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
import android.content.SharedPreferences;
import android.util.Log;

import com.getpebble.android.kit.util.PebbleDictionary;

import java.util.Date;

/** Track important Pebble state insofar as it is known.
 */
public class PebbleState
{
    private static final String TAG = "BlueSky";

    private static SharedPreferences getSharedPreferences(Context context) {
        return context.getSharedPreferences("PebbleStatePrefs", 0);
    }

    /** When the last ACK was received.
     */
    public static Date getAckTime(Context context) {
        long time = getSharedPreferences(context).getLong("agenda.ack_time", 0);
        return new Date(time);
    }

    /** When the last attempt was made.
     */
    public static Date getAttemptTime(Context context) {
        long time = getSharedPreferences(context).getLong("agenda.attempt_time", 0);
        return new Date(time);
    }

    /** How many NACKs have been received since the last ACK.
     *
     * If no ACKs have been received, then this is the total number of NACKs
     * that have been received.
     */
    public static int getNackCount(Context context) {
        return getSharedPreferences(context).getInt("agenda.nack_count", 0);
    }

    /** When the last NACK was received.
     */
    public static Date getNackTime(Context context) {
        long time = getSharedPreferences(context).getLong("agenda.nack_time", 0);
        return new Date(time);
    }

    /** Record the outgoing Pebble message for the identified transaction.
     * 
     * Record the given agenda update message and transaction id so that
     * sending the same message can be re-attempted later with the same
     * transaction id.
     */
    public static void recordAttempt(Context context, int transactionId) {
        SharedPreferences.Editor editor = getSharedPreferences(context).edit();
        editor.putInt("agenda.transaction", transactionId);
        editor.putLong("agenda.attempt_time", new Date().getTime());
        editor.apply();
        Log.i(TAG, "beginning attempted send to Pebble");
    }

    /** Record an ACK received from the Pebble.
     *
     * Has no effect if transactionId does not match the transactionId most
     * recently passed to recordAttempt.
     */
    public static void recordAck(Context context, int transactionId) {
        SharedPreferences storage = getSharedPreferences(context);
        Log.i(TAG, "received ACK from Pebble");
        if (storage.getInt("agenda.transaction", 0) == transactionId) {
            SharedPreferences.Editor editor = storage.edit();
            editor.putLong("agenda.ack_time", new Date().getTime());
            editor.putInt("agenda.nack_count", 0);
            editor.apply();
        }
    }

    /** Record a NACK received from the Pebble.
     *
     * Has no effect if transactionId does not match the transactionId most
     * recently passed to recordAttempt.
     *
     * This method is not thread-safe.
     */
    public static void recordNack(Context context, int transactionId) {
        SharedPreferences storage = getSharedPreferences(context);
        Log.i(TAG, "received NACK from Pebble");
        if (storage.getInt("agenda.transaction", 0) == transactionId) {
            SharedPreferences.Editor editor = storage.edit();
            editor.putLong("agenda.nack_time", new Date().getTime());
            // The following line is not thread-safe because two threads
            // attempting the same update at about the same time can cause
            // one nack to be skipped:
            int nackCount = storage.getInt("agenda.nack_count", 0);
            editor.putInt("agenda.nack_count", nackCount+1);
            editor.apply();
            Log.w(TAG, "agenda.nack_count: "+String.valueOf(nackCount)
                    + " agenda.ack_time: "+String.valueOf(getAckTime(context)));
        }
    }
}
