#pragma once
#include <stdint.h>

enum osmo_e1cap_capture_mode {
	OSMO_E1CAP_MODE_RAW,
	OSMO_E1CAP_MODE_HDLC,
	OSMO_E1CAP_MODE_TRAU,
	OSMO_E1CAP_MODE_PGSL,
};

/* header for each frame we store */
struct osmo_e1cap_pkthdr {
	/* Timestamp at which frame was received */
	struct timeval ts;
	/* length of frame data after this header */
	uint32_t len;
	/* line/span number on which frame was received */
	uint8_t line_nr;
	/* timeslot number on which frame was received */
	uint8_t ts_nr;
	/* see osmo_e1cap_capture_mode */
	uint8_t capture_mode;
	/* any optional future flags */
	uint8_t flags;
} __attribute__((aligned));

int e1frame_store(struct e1inp_ts *ts, struct msgb *msg, enum osmo_e1cap_capture_mode mode);
