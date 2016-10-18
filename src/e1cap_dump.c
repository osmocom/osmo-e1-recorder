#include <stdio.h>
#include <time.h>

#include <sys/time.h>

#include <osmocom/core/signal.h>
#include <osmocom/core/logging.h>
#include <osmocom/core/application.h>

#include "storage.h"
#include "recorder.h"

struct e1_recorder g_recorder;

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

int main(int argc, char **argv)
{
	struct osmo_e1cap_file *f;
	struct osmo_e1cap_pkthdr *pkt;

	printf("sizeof(timeval) = %zu\n", sizeof(struct timeval));
	printf("sizeof(osmo_e1cap_pkthdr) = %zu\n", sizeof(*pkt));

	if (argc < 2)
		exit(2);

	f = osmo_e1cap_open(NULL, argv[1]);
	if (!f)
		exit(1);

	while ((pkt = osmo_e1cap_read_next(f))) {
		printf("%s %02u/%02u %u (%u): %s\n",
			timeval2str(&pkt->ts),
			pkt->line_nr, pkt->ts_nr, pkt->capture_mode,
			pkt->len,
			osmo_hexdump_nospc(pkt->data, pkt->len));
		talloc_free(pkt);
	}
}
