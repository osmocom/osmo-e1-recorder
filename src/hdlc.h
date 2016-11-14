#pragma once

#include <osmocom/core/bits.h>

struct hdlc_proc {
	ubit_t history[8];
	ubit_t next_outbyte[8];
	uint8_t num_bits;
};

int process_raw_hdlc(struct hdlc_proc *hdlc, uint8_t *data, unsigned int len);
