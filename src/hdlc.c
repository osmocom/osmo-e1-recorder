#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <osmocom/core/utils.h>
#include <osmocom/core/bits.h>

#include "hdlc.h"

#if 0
#define DEBUGP(x, args ...) fprintf(stderr, x, ## args)
#else
#define DEBUGP(x, args ...) do {} while (0)
#endif

static const ubit_t flag_octet[] = { 0,1,1,1,1,1,1,0 };
static const ubit_t five_ones_zero[] = { 0,1,1,1,1,1 };

static void append_bit_history(struct hdlc_proc *hdlc, uint8_t bit)
{
	/* we always add the bit to the history */
	memmove(hdlc->history+1, hdlc->history, sizeof(hdlc->history)-1);
	hdlc->history[0] = bit;
}

static int append_bit_out(struct hdlc_proc *hdlc, uint8_t bit)
{
	memmove(hdlc->next_outbyte+1, hdlc->next_outbyte, sizeof(hdlc->next_outbyte)-1);
	hdlc->next_outbyte[0] = bit;
	hdlc->num_bits++;

	if (hdlc->num_bits == 8) {
		pbit_t out;
		hdlc->num_bits = 0;
		/* generate one output byte */
		osmo_ubit2pbit_ext(&out, 0, hdlc->next_outbyte, 0, 8, 0);
		/* append to output buffer */
		OSMO_ASSERT(hdlc->out.len < sizeof(hdlc->out.buf));
		hdlc->out.buf[hdlc->out.len++] = out;
		return out;
	}

	return -1;
}

static int process_hdlc_bit(struct hdlc_proc *hdlc, uint8_t bit)
{
	int out = -1, flag = 0;

	DEBUGP("bit=%u, history_in = %s, ", bit, osmo_ubit_dump(hdlc->history, sizeof(hdlc->history)));

	/* always append bit to history */
	append_bit_history(hdlc, bit);

	if (!memcmp(flag_octet, hdlc->history, sizeof(flag_octet))) {
		hdlc->num_bits = 0;
		DEBUGP("S ");
		flag = 1;
		if (hdlc->out.len) {
			/* call output function if any frame was
			 * received before the flag octet */
			if (hdlc->out_cb)
				hdlc->out_cb(hdlc->out.buf, hdlc->out.len,
					     hdlc->priv);
			hdlc->out.len = 0;
		}
	} else if (!memcmp(five_ones_zero, hdlc->history, sizeof(five_ones_zero))) {
		/* 4.3.1 Synchronous transmission: receiver shall
		 * discard any "0" bit after five contiguous ones */
		DEBUGP("I ");
	} else {
		out = append_bit_out(hdlc, bit);
	}

	DEBUGP("history_out = %s", osmo_ubit_dump(hdlc->history, sizeof(hdlc->history)));
	if (out >= 0)
		DEBUGP(", out 0x%02x\n", out);
	else
		DEBUGP("\n");

	if (flag)
		return -123;
	else
		return out;
}

int process_raw_hdlc(struct hdlc_proc *hdlc, ubit_t *bits, unsigned int len)
{
	unsigned int i;
	int out;
	static int last_out;

	DEBUGP("process_raw_hdlc(%s)\n", osmo_hexdump(bits, len));

	for (i = 0; i < len; i ++) {
		out = process_hdlc_bit(hdlc, bits[i]);
		if (out == -123) {
			/* suppress repeating Flag characters */
			if (last_out != out)
				printf("\nF ");
			last_out = out;
		} else if (out >= 0) {
			/* suppress 0xAA and 0x55 bit pattern */
			//if (out != 0xaa && out != 0x55)
				printf("%02x ", out);
			last_out = out;
		}
	}
	return 0;
}
