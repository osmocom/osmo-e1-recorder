#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <sys/time.h>

#include <osmocom/core/signal.h>
#include <osmocom/core/logging.h>
#include <osmocom/core/application.h>
#include <osmocom/abis/subchan_demux.h>

#include "storage.h"
#include "recorder.h"

struct e1_recorder g_recorder;

enum mode {
	MODE_PRINT,
	MODE_BIN,
};

static enum mode g_mode = MODE_PRINT;
static int g_filter_line = -1;
static int g_filter_slot = -1;
static int g_filter_subslot = -1;
static struct osmo_e1cap_pkthdr *g_last_pkthdr;

static char *timeval2str(struct timeval *tv)
{
	time_t nowtime;
	struct tm *nowtm;
	char tmbuf[64];
	static char buf[64];

	nowtime = tv->tv_sec;
	nowtm = localtime(&nowtime);
	strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
	snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tv->tv_usec);
	return buf;
}

static void handle_data(struct osmo_e1cap_pkthdr *pkt, const uint8_t *data, int len)
{
	switch (g_mode) {
	case MODE_PRINT:
		printf("%s %02u/%02u %u (%u): %s\n",
			timeval2str(&pkt->ts),
			pkt->line_nr, pkt->ts_nr, pkt->capture_mode,
			pkt->len,
			osmo_hexdump_nospc(data, len));
		break;
	case MODE_BIN:
		write(1, data, len);
		break;
	}
}

static int subch_demux_out_cb(struct subch_demux *dmx, int ch, uint8_t *data,
			      int len, void *c)
{
	OSMO_ASSERT(ch == g_filter_subslot);
	handle_data(g_last_pkthdr, data, len);

	return 0;
}

static int handle_options(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "l:s:bu:")) != -1) {
		switch (opt) {
		case 'l': /* Filter on E1 Line Number */
			g_filter_line = atoi(optarg);
			break;
		case 's': /* Filter on E1 Slot Number */
			g_filter_slot = atoi(optarg);
			break;
		case 'b': /* Raw binary output mode (for piping) */
			g_mode = MODE_BIN;
			break;
		case 'u': /* 16k Sub-channel demux + filter */
			g_filter_subslot = atoi(optarg);
			if (g_filter_subslot < 0 || g_filter_subslot > 3)
				exit(2);
			break;
		default:
			fprintf(stderr, "Unknown option '%c'\n", opt);
			exit(EXIT_FAILURE);
			break;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct osmo_e1cap_file *f;
	struct osmo_e1cap_pkthdr *pkt;
	unsigned long num_pkt = 0;
	struct subch_demux smux;

	printf("sizeof(timeval) = %zu\n", sizeof(struct timeval));
	printf("sizeof(osmo_e1cap_pkthdr) = %zu\n", sizeof(*pkt));

	handle_options(argc, argv);

	if (optind >= argc) {
		fprintf(stderr, "Missing input file name\n");
		exit(2);
	}

	f = osmo_e1cap_open(NULL, argv[optind++]);
	if (!f) {
		fprintf(stderr, "Unable to open input file\n");
		exit(1);
	}

	if (g_filter_subslot >= 0) {
		smux.out_cb = subch_demux_out_cb;
		subch_demux_init(&smux);
		subch_demux_activate(&smux, g_filter_subslot);
	}

	while ((pkt = osmo_e1cap_read_next(f))) {
		num_pkt++;
		g_last_pkthdr = pkt;

		if (g_filter_line >= 0 && pkt->line_nr != g_filter_line)
			continue;
		if (g_filter_slot >= 0 && pkt->ts_nr != g_filter_slot)
			continue;

		if (g_filter_subslot >= 0) {
			subch_demux_in(&smux, pkt->data, pkt->len);
			continue;
		}

		handle_data(pkt, pkt->data, pkt->len);

		talloc_free(pkt);
	}

	fprintf(stderr, "Processed a total of %lu packets\n", num_pkt);
}
