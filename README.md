Osmocom E1 recorder
===================
(C) 2016 by Harald Welte <laforge@gnumonks.org>

The idea of this program is to be able to build a "poor mans E1/T1
recorder" purposes of data analysis, *without* any special
equipment.

This approach is more risky than a purely passive, hardware based
approach as that of the Osmocom e1-tracer released at
https://osmocom.org/projects/e1-t1-adapter/wiki/E1_tracer

If you can, it is strongly recommended to use the purely passive,
high-impedance tap approach of e1-tracer and not the poor-man's
software proxy approach presented in osmo-e1-recorder.

Setup
-----

To do so, two E1 cards are used as some kind of proxy for the E1
communication.  Recording of a single E1 link always requires two E1
interface cards, one for each direction.

Recording can be performed either

* passively, using a E1 Tap adapter
* as a proxy / man-in-the-middle

All timeslots will be opened in "raw" mode, making sure the recording
will work whether or not there is HDLC-based signalling (MTP or LAPD),
PCM voice, TRAU frames or anything else on the line.

Recording will be done on a per-timeslot basis, dumping the raw bytes
read for this timeslot into a file.

New files are started regularly, after reaching a pre-determined file
size limit.  File names contain RTC time stamping and timeslot number.

Later possible extensions could include automatic detection of the
payload and a more intelligent storage format (e.g. in case of HDLC
based signalling).
