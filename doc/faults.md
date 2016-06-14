# Faults

This is a list of potential problems along with plans for dealing with these
problems.

These plans are not necessarily implemented yet.

# When Everything Is Fine

The companion application running on a smart-phone sends an agenda update to
the watch face whenever there is a change in the Android Calendar Content
Provider that obsoletes the last update that was successfully sent.

If the watch face receives an update that is too large to fit in its receiving
buffer, it will send a message to the companion application describing the size
of that buffer.

So that the companion application and the watch face can recognize that
everything is fine even when there are no agenda updates that need to be sent,
the companion application will occasionally, after some period of silence, send
an otherwise unnecessary agenda update.  This may also be called a "heartbeat"
update.

# Priorities

In the watch face,

1. Present up to date information if it is available.

2. Preserve battery life: never respond to an error by re-attempting the same
   action.

3. Inform the companion application about the agenda buffer size if necessary.

In the companion application,

1. Send up to date information if there is any to send.

2. Preserve battery life: never respond to an error by re-attempting the same
   action.

3. Send an occasional "heartbeat" update.

# Pebble: When Things Go Wrong

Only communication issues are addressed here.

## Pebble: Failure To Send

In this case, the watch face has discovered that it is failing to send
messages, either because an earlier attempt to send is still waiting in the
out-box or because an error occurred and an error callback has been invoked.
For example,

* The watch and phone went out of range from each other after a message was
  sent but before acknowledgement could be received.

* The companion application is not installed.

Assumptions:

* Communication failure due to an "out-box is busy" condition can be detected at
  the time that a message is being prepared to be sent.

* Communication failure can otherwise reliably be detected by paying attention
  to error callbacks.

* If a message needs to be sent later then the conditions that led to this
  attempt in the first place will also arise again later and cause that
  re-attempt.

When such an error is detected,

1. Discard it; do not respond by retrying.

## Pebble: Failure to Receive

In this case, agenda updates are not being received.  For example,

* The phone is powered off.

* The companion application is not installed.

* The companion application is sending agenda updates that are too large, and
  information about these failures are not reaching the watch face.

Each time the watch face receives an agenda update, it also receives a span of
time during which that update is relevant.  While some events may cross either
end of that span, the end of the span is the end of the relevance of the agenda
data.

1. Time outside the span of relevance should be displayed in a way that
   indicates something is wrong.

Note:

* The watch face will know at what time it last received updated agenda data,
  and therefore the age of agenda it currently has.  However, if there is a
  need to display that this age is excessive, it can be done by reducing the
  span of relevance.

* There is no real error handling here.  There is nothing for the watch face to
  do to get in touch with the companion application.

* In the case that no agenda updates have ever been received, maybe because the
  companion application was never installed, this can be treated just as though
  the last agenda update was received some time ago and its span of relevance,
  along with the last of its events, are long past.

# Android: When Things Go Wrong

The companion application will attempt to send an agenda update whenever there
is a change in the calendar content provider that might obsolete the last
update that was successfully sent to the watch.  It will will

## Android: Failure to Send

Assumption:

* Communication failure can reliably be detected by paying attention to error
  callbacks.

When errors are detected,

1. If the agenda update is a "heartbeat" update, ignore it; do not respond by
   retrying.

2. Otherwise, if it looks like the Pebble is connected, the message should be
   retried with exponential back-off.

3. Otherwise, the message should be queued for later, when it looks like the
   Pebble is connected.
