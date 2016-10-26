#pragma once

#include <osmocom/core/bits.h>

enum hdlc_proc_state {
	STATE_INIT,
	STATE_FLAG_WAIT_ZERO,
	STATE_PAYLOAD,
};

struct hdlc_proc {
	ubit_t history[8];
	ubit_t next_outbyte[8];
	enum hdlc_proc_state state;
	uint8_t num_bits;
};

int process_raw_hdlc(struct hdlc_proc *hdlc, uint8_t *data, unsigned int len);
