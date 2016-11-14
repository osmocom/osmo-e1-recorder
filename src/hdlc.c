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

static const ubit_t five_ones[] = { 1,1,1,1,1 };

static int append_bit(struct hdlc_proc *hdlc, uint8_t bit, int ignore)
{
	/* we always add the bit to the history */
	memmove(hdlc->history+1, hdlc->history, sizeof(hdlc->history)-1);
	hdlc->history[0] = bit;

	/* if it is a to-be-discarded bit, we bail out */
	if (ignore)
		return -1;

	memmove(hdlc->next_outbyte+1, hdlc->next_outbyte, sizeof(hdlc->next_outbyte)-1);
	hdlc->next_outbyte[0] = bit;
	hdlc->num_bits++;

	if (hdlc->num_bits == 8) {
		pbit_t out;
		/* generate one output byte */
		osmo_ubit2pbit_ext(&out, 0, hdlc->next_outbyte, 0, 8, 0);
		hdlc->num_bits = 0;
		return out;
	}

	return -1;
}

static int process_hdlc_bit(struct hdlc_proc *hdlc, uint8_t bit)
{
	int ignore = 0;
	int out, flag = 0;

	DEBUGP("bit=%u, history_in = %s, ", bit, osmo_ubit_dump(hdlc->history, sizeof(hdlc->history)));

	switch (hdlc->state) {
	case STATE_FLAG_WAIT_ZERO:
		/* we've received 6 consecutive '1' just before and are
		 * waiting for the final '0' to end the flag character */
		if (bit == 0) {
			/* it was a zero, flag character detected */
			DEBUGP("F ");
			flag = 1;
			/* we're now inside the payload state */
			hdlc->state = STATE_PAYLOAD;
		} else {
			/* if we received yet another '1', we re-start
			 * from the beginning */
			hdlc->state = STATE_INIT;
		}
		ignore = 1;
		hdlc->num_bits = 0;
		break;
	case STATE_PAYLOAD:
	case STATE_INIT:
		if (!memcmp(five_ones, hdlc->history, sizeof(five_ones))) {
			/* five consecutive ones in the history */
			if (bit == 1) {
				/* one more '1' was received -> we wait
				 * for a zero at the end of the flag
				 * character 0x7E */
				hdlc->state = STATE_FLAG_WAIT_ZERO;
				/* discard bit */
				ignore = 1;
			} else {
				/* discard bit */
				ignore = 1;
			}
		}
		break;
	}
	out = append_bit(hdlc, bit, ignore);
	DEBUGP("history_out = %s", osmo_ubit_dump(hdlc->history, sizeof(hdlc->history)));
	if (out > 0)
		DEBUGP(", out 0x%02x\n", out);
	else
		DEBUGP("\n");

	if (flag)
		return -123;
	else
		return out;
}

int process_raw_hdlc(struct hdlc_proc *hdlc, uint8_t *data, unsigned int len)
{
	unsigned int i;
	int out;
	static int last_out;

	for (i = 0; i < len; i ++) {
		out = process_hdlc_bit(hdlc, data[i]);
		if (out == -123) {
			/* suppress repeating Flag characters */
			if (last_out != out)
				printf("\nF ");
			last_out = out;
		} else if (out >= 0) {
			/* suppress 0xAA and 0x55 bit pattern */
			if (out != 0xaa && out != 0x55)
				printf("%02x ", out);
			last_out = out;
		}
	}
	return 0;
}
