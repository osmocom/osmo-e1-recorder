#include <osmocom/core/msgb.h>
#include <osmocom/abis/e1_input.h>

#include "storage.h"

int e1frame_store(struct e1inp_ts *ts, struct msgb *msg, enum osmo_e1cap_capture_mode mode)
{
	struct osmo_e1cap_pkthdr *h;
	uint32_t len = msg->len;

	h = (struct osmo_e1cap_pkthdr *) msgb_push(msg, sizeof(*h));
	h->len = htonl(len);
	h->line_nr = ts->line->num;
	h->ts_nr = ts->num;
	h->capture_mode = mode;
	h->flags = 0;

	/* FIXME: Write */
	return 0;
}
