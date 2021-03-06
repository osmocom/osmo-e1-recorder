#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>

#include <osmocom/core/bits.h>
#include <osmocom/core/signal.h>
#include <osmocom/core/logging.h>
#include <osmocom/core/msgb.h>
#include <osmocom/core/application.h>
#include <osmocom/abis/subchan_demux.h>
#include <osmocom/abis/lapd_pcap.h>

#include "storage.h"
#include "recorder.h"
#include "flip_bits.h"
#include "hdlc.h"

#include "config.h"

struct e1_recorder g_recorder;

enum mode {
	MODE_PRINT,
	MODE_BIN,
	MODE_SC,
};

#define MAX_TS	32
#define CHUNK_BYTES 160

/* Ericsson super-channel */
struct sc_state {
	uint8_t ts_data[MAX_TS][CHUNK_BYTES];
	uint8_t num_ts;
	uint32_t ts_mask;
	struct hdlc_proc hdlc;
};

static struct sc_state g_sc_state[2];

static enum mode g_mode = MODE_PRINT;
static int g_filter_line = -1;
static int g_filter_slot = -1;
static int g_filter_subslot = -1;
static struct osmo_e1cap_pkthdr *g_last_pkthdr;
static int g_pcap_fd = -1;
struct msgb *g_pcap_msg = NULL;

static char *timeval2str(struct timeval *tv)
{
	time_t nowtime;
	struct tm *nowtm;
	char tmbuf[64];
	static char buf[64+20];

	nowtime = tv->tv_sec;
	nowtm = localtime(&nowtime);
	strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
	snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tv->tv_usec);
	return buf;
}

#if 0
static int all_bytes_are(unsigned char ch, const uint8_t *data, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (data[i] != ch)
			return 0;
	}
	return 1;
}
#endif

static void handle_hdlc_frame_content(const uint8_t *data, unsigned int len,
				      void *priv)
{
	if (g_pcap_fd >= 0 && len >= 4) {
		uint8_t *cur = msgb_put(g_pcap_msg, len-2);
		memcpy(cur, data, len-2);
		printf("==> %s\n", msgb_hexdump(g_pcap_msg));
		OSMO_ASSERT(g_pcap_msg->len >= 2);
		/* FIXME: timestamp! */
		osmo_pcap_lapd_write(g_pcap_fd, OSMO_LAPD_PCAP_OUTPUT, g_pcap_msg);
		msgb_reset(g_pcap_msg);
	}
}

static void handle_sc_out(struct sc_state *scs)
{
	unsigned int num_bytes = scs->num_ts * CHUNK_BYTES;
	unsigned int num_bits = num_bytes * 8;
	uint8_t out[32*CHUNK_BYTES];
	ubit_t out_bits[32*CHUNK_BYTES*8];
	int i, j, k = 0;

	/* re-shuffle the data from columns to lines */
	for (i = 0; i < CHUNK_BYTES; i++) {
		for (j = 0; j < scs->num_ts; j++)
			out[k++] = scs->ts_data[j][i];
	}
	//printf("num_bytes=%u %s\n", num_bytes, osmo_hexdump_nospc(out, num_bytes));
	osmo_pbit2ubit_ext(out_bits, 0, out, 0, num_bits, 1);
	//for (i = 0; i < num_bits; i++) fputc(out_bits[i] ? '1' : '.', stdout); fputc('\n', stdout);
	process_raw_hdlc(&scs->hdlc, out_bits, num_bits);
}

static void handle_sc_in(struct osmo_e1cap_pkthdr *pkt, const uint8_t *data, unsigned int len)
{
	struct sc_state *scs;

	if (pkt->line_nr >= ARRAY_SIZE(g_sc_state)) {
		fprintf(stderr, "Line number out of range\n");
		exit(1);
	}

	scs = &g_sc_state[pkt->line_nr];
	if (pkt->ts_nr >= ARRAY_SIZE(scs->ts_data)) {
		fprintf(stderr, "Timeslot number out of range\n");
		exit(1);
	}

	if (len != sizeof(scs->ts_data[pkt->ts_nr])) {
		fprintf(stderr, "Insufficient data\n");
		exit(1);
	}

	if (pkt->ts_nr == 1) {
		scs->ts_mask = 0;
		memset(scs->ts_data, 0, sizeof(scs->ts_data));
	}

	/* copy over the data */
	memcpy(scs->ts_data[pkt->ts_nr-1], data, len);
	/* note that we have valid data for the given timeslot */
	scs->ts_mask |= (1 << (pkt->ts_nr-1));

	/* make sure we know what's the maximum timeslot number */
	if (pkt->ts_nr > scs->num_ts)
		scs->num_ts = pkt->ts_nr;

	/* check if we have data for all needed timeslots */
	uint32_t ts_mask = (1 << scs->num_ts) -1;
	//printf("num_ts=%u, ts_mask=0x%x, scs_ts_mask=0x%x\n", scs->num_ts, ts_mask, scs->ts_mask);
	if (scs->ts_mask == ts_mask) {
		handle_sc_out(scs);
	}
}


static void handle_data(struct osmo_e1cap_pkthdr *pkt, const uint8_t *idata, int len)
{
	uint8_t data[len];
	struct timeval tv;

	flip_buf_bits(data, idata, len);
#if 0
	/* filter out all-ff/all-fe/all-7f */
	if (all_bytes_are(0xff, data, len) ||
	    all_bytes_are(0x7f, data, len) ||
	    all_bytes_are(0x7e, data, len) ||
	    all_bytes_are(0xe7, data, len) ||
	    all_bytes_are(0x3f, data, len) ||
	    all_bytes_are(0xf3, data, len) ||
	    all_bytes_are(0x9f, data, len) ||
	    all_bytes_are(0xf9, data, len) ||
	    all_bytes_are(0xcf, data, len) ||
	    all_bytes_are(0xfc, data, len) ||
	    all_bytes_are(0xfe, data, len))
		return;
#endif

	switch (g_mode) {
	case MODE_PRINT:
		tv.tv_sec = pkt->ts.tv_sec;
		tv.tv_usec = pkt->ts.tv_usec;
		printf("%s %02u/%02u %u (%u): %s\n",
			timeval2str(&tv),
			pkt->line_nr, pkt->ts_nr, pkt->capture_mode,
			pkt->len,
			osmo_hexdump_nospc(data, len));
		break;
	case MODE_BIN:
		write(1, data, len);
		break;
	case MODE_SC:
		handle_sc_in(pkt, data, len);
		break;
	}
}

static int subch_demux_out_cb(struct subch_demux *dmx, int ch, const ubit_t *data,
			      int len, void *c)
{
	OSMO_ASSERT(ch == g_filter_subslot);

	handle_data(g_last_pkthdr, data, len);

	return 0;
}

static void print_help(void)
{
	printf(	"  -h 			Print this message\n"
		"  -V			Print version of the program\n"
		"  -l LINE_NR		Filter on line number\n"
		"  -s SLOT_NR		Filter on timeslot number\n"
		"  -b			Raw binary output mode (for piping stdout)\n"
		"  -S			Super-Channel mode\n"
		"  -u SUBSLOT_NR		Filter on 16k sub-slot number\n"
		"  -p PCAP_FILE		Write LAPD PCAP file\n"
	      );
};

static int handle_options(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "hVl:s:bSu:p:")) != -1) {
		switch (opt) {
		case 'h':
			print_help();
			exit(0);
		case 'V':
			printf("%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
			exit(0);
			break;
		case 'l': /* Filter on E1 Line Number */
			g_filter_line = atoi(optarg);
			break;
		case 's': /* Filter on E1 Slot Number */
			g_filter_slot = atoi(optarg);
			break;
		case 'b': /* Raw binary output mode (for piping) */
			g_mode = MODE_BIN;
			break;
		case 'S': /* Super Channel Mode */
			g_mode = MODE_SC;
			break;
		case 'u': /* 16k Sub-channel demux + filter */
			g_filter_subslot = atoi(optarg);
			if (g_filter_subslot < 0 || g_filter_subslot > 3)
				exit(2);
			break;
		case 'p': /* PCAP writing */
			g_pcap_fd = osmo_pcap_lapd_open(optarg, 0640);
			if (g_pcap_fd < 0) {
				perror("opening pcap file");
				exit(1);
			}
			g_pcap_msg = msgb_alloc(1024, "pcap");
			break;
		default:
			fprintf(stderr, "Unknown option '%c'\n", opt);
			exit(EXIT_FAILURE);
			break;
		}
	}

	return 0;
}

static struct log_info_cat log_categories[] = {
};

static struct log_info log_info = {
	.cat = log_categories,
	.num_cat = ARRAY_SIZE(log_categories),
};

int main(int argc, char **argv)
{
	struct osmo_e1cap_file *f;
	struct osmo_e1cap_pkthdr *pkt;
	unsigned long num_pkt = 0;
	struct subch_demux smux;
	int i;

	printf("sizeof(timeval) = %zu\n", sizeof(struct timeval));
	printf("sizeof(osmo_e1cap_pkthdr) = %zu\n", sizeof(*pkt));

	memset(g_sc_state, 0, sizeof(g_sc_state));
	for (i = 0; i < ARRAY_SIZE(g_sc_state); i++)
		g_sc_state[i].hdlc.out_cb = handle_hdlc_frame_content;
	init_flip_bits();

	osmo_init_logging2(NULL, &log_info);

	handle_options(argc, argv);

	if (optind >= argc) {
		fprintf(stderr, "Missing input file name\n");
		exit(2);
	}

	if (argc > optind+1) {
		fprintf(stderr, "Unsupported positional arguments on command line\n");
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

//	printf("hdlc=%s\n", osmo_hexdump(&g_sc_state[0].hdlc, sizeof(g_sc_state[0].hdlc)));

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
