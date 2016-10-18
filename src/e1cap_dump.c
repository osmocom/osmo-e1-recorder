#include <stdio.h>
#include <sys/time.h>

#include <osmocom/core/signal.h>
#include <osmocom/core/logging.h>
#include <osmocom/core/application.h>

#include "storage.h"
#include "recorder.h"

struct e1_recorder g_recorder;

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
		printf("%lu:%lu %02u/%02u %u (%u): %s\n",
			pkt->ts.tv_sec, pkt->ts.tv_usec,
			pkt->line_nr, pkt->ts_nr, pkt->capture_mode,
			pkt->len,
			osmo_hexdump_nospc(pkt->data, pkt->len));
		talloc_free(pkt);
	}
}
