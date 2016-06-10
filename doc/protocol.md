# Blue Sky Protocol

Pebble provides a nicely abstracted communications protocol that lets each
message be processed as a map from keys (integers) to values (integers and byte
arrays).  How do the Blue Sky Watchface (BSW), running on Pebble, and the Blue
Sky Companion (BSC), running on Android, communicate with each other using this
protocol?

Keys must mean the same thing on both ends and, at present, need to be
hard-coded in multiple places.  Key definitions in the following files must be
kept in sync:

* `android/src/main/java/ca/joshuatacoma/bluesky/BlueSkyConstants.java`.
* `pebble/appinfo.json`
* `pebble/src/modules/data.h`

The current set of keys is as follows:

1. Integer.  How much time the watchface displays at once, in seconds.  "Agenda
   Need Seconds".

2. Integer.  Maximum usable length of the *Agenda* data, in bytes.  "Agenda
   Capacity Bytes".  Divide by 4 to get maximum number of events the watch face
   can remember at once.

3. Byte array.  A sequence of pairs of 16-bit signed integers, each
   representing a number of minutes relative to an epoch (key 6).  "Agenda".

4. Integer.  A unique version number for each *Agenda* value to help
   distinguish differences.  "Agenda Version".

5. Integer.  The current time according to the watch, as seconds since the Unix
   epoch.  "Pebble Now".

6. Integer.  The epoch for interpretation of *Agenda*, in seconds since Unix
   epoch.  "Agenda Epoch".

These are currently used to form two kinds of messages:

* BSW allocates a buffer for the agenda and is the authority on the size of
  that buffer.  It also, for now, decides how much time the watchface will
  displays at once even though this is always exactly 24 hours.  It sends this
  information, along with the current time according to Pebble, to Blue Sky
  Companion.

* BSC puts a unique number along with a sequence of pairs of 16-bit signed
  integers, each representing a number of minutes relative to an included epoch
  all together into a message to Blue Sky Watchface.

## Problem

BSC knows when a Pebble device is connected or not, can be notified whenever
there is a change in Android's Calendar Content Provider, and can present a UI
to gather intents from the person who is wearing the watch running BSW.  BSC
can send all the data, and only the data, that BSW needs.  All it needs to know
is the buffer length.  Why, then, is BSW responsible for sending requests to
keep itself up to date?  The current design is built on an idea that BSW knows
exactly what the user wants and only needs to call on BSC to get the details on
demand.

## Rethinking

* BSC can and should wait for "Pebble device is connected" and "Calendar is
  updated" before sending updates.

* BSW doesn't need to tell BSC anything aside from the size of its agenda
  buffer.  When should it do this?  If over-large agenda data triggers a
  recognizable error like "message was dropped from inbox due to buffer size
  constraints" then it can respond by sending its buffer size until it either
  gets an ACK or a new agenda value that fits within its buffer.

* If updates are infrequent, they might as well aim to fill the buffer rather
  than be conservative about the span of time.

* There's still no protocol for configuration settings.  Some day, there will
  be.

## The Future is Bright

* BSW will send no messages unless it needs to inform BSW that its agenda
  buffer is relatively small.

* BSC will keep track of when it last successfully sent an agenda update to
  BSW.  Typically it will send an update if and only if there is something new
  to say.
