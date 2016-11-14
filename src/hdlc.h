#pragma once

#include <osmocom/core/bits.h>

/* structure representing a HDLC decoder process */
struct hdlc_proc {
	/* 8 bits history of most recently received bits */
	ubit_t history[8];
	/* 8 bit buffer for assembling the next output byte */
	ubit_t next_outbyte[8];
	/* number of bits currently in use in next_outbyte */
	uint8_t num_bits;

	/* output buffer for re-aligned frame bytes */
	struct {
		uint8_t buf[1024];
		unsigned int len;
	} out;

	/* call-back ot be called at end of frame */
	void (*out_cb)(const uint8_t *data, unsigned int len, void *priv);
	/* private data passed to out_cb */
	void *priv;
};

/* input given number of raw bits from the bit stream into HDLC */
int process_raw_hdlc(struct hdlc_proc *hdlc, ubit_t *bits, unsigned int len);
