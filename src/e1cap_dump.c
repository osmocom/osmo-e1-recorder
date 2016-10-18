#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <sys/time.h>

#include <osmocom/core/signal.h>
#include <osmocom/core/logging.h>
#include <osmocom/core/application.h>

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

static int handle_options(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "l:s:b")) != -1) {
		switch (opt) {
		case 'l':
			g_filter_line = atoi(optarg);
			break;
		case 's':
			g_filter_slot = atoi(optarg);
			break;
		case 'b':
			g_mode = MODE_BIN;
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

	while ((pkt = osmo_e1cap_read_next(f))) {
		num_pkt++;

		if (g_filter_line >= 0 && pkt->line_nr != g_filter_line)
			continue;
		if (g_filter_slot >= 0 && pkt->ts_nr != g_filter_slot)
			continue;

		switch (g_mode) {
		case MODE_PRINT:
			printf("%s %02u/%02u %u (%u): %s\n",
				timeval2str(&pkt->ts),
				pkt->line_nr, pkt->ts_nr, pkt->capture_mode,
				pkt->len,
				osmo_hexdump_nospc(pkt->data, pkt->len));
			break;
		case MODE_BIN:
			write(1, pkt->data, pkt->len);
			break;
		}
		talloc_free(pkt);
	}

	fprintf(stderr, "Processed a total of %lu packets\n", num_pkt);
}
