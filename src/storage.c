#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include <osmocom/core/msgb.h>
#include <osmocom/core/talloc.h>
#include <osmocom/abis/e1_input.h>

#include "storage.h"
#include "recorder.h"

static int g_out_fd = -1;;
static uint64_t g_written_bytes;

static const char *storage_gen_filename(void)
{
	static char buf[32];
	time_t t;
	struct tm *tmp;

	t = time(NULL);
	tmp = localtime(&t);
	strftime(buf, sizeof(buf), "%Y%m%d%H%M%S.e1cap", tmp);
	return buf;
}

static int storage_reopen_if_needed(void)
{
	if (g_written_bytes / (1024*1024) >= g_recorder.max_file_size_mb) {
		close(g_out_fd);
		g_out_fd = -1;
	}

	if (g_out_fd < 0) {
		int rc;
		const char *fname = storage_gen_filename();
		rc = chdir(g_recorder.storage_path);
		if (rc < 0) {
			LOGP(DMAIN, LOGL_ERROR, "Unable to chdir(%s): %s\n",
				g_recorder.storage_path, strerror(errno));
			return -1;
		}
		g_out_fd = open(fname, O_WRONLY|O_CREAT, 0664);
		if (g_out_fd < 0) {
			LOGP(DMAIN, LOGL_ERROR, "Unable to open(%s): %s\n",
				fname, strerror(errno));
		}
		g_written_bytes = 0;
	}

	return g_out_fd;
}

int e1frame_store(struct e1inp_ts *ts, struct msgb *msg, enum osmo_e1cap_capture_mode mode)
{
	struct osmo_e1cap_pkthdr _h, *h = &_h;
	int rc;
	struct iovec iov[2] = {
		{
			.iov_base = h,
			.iov_len = sizeof(*h),
		}, {
			.iov_base = msg->data,
			.iov_len = msg->len,
		}
	};

	h->len = htonl(msg->len);
	gettimeofday(&h->ts, NULL);
	h->line_nr = ts->line->num;
	h->ts_nr = ts->num;
	h->capture_mode = mode;
	h->flags = 0;

	storage_reopen_if_needed();

	rc = writev(g_out_fd, iov, ARRAY_SIZE(iov));
	if (rc < 0)
		return rc;

	g_written_bytes += rc;

	return 0;
}

/* reading */


struct osmo_e1cap_file {
	int fd;
};

#define OSMO_E1REC_ALLOC_SIZE	1024

struct osmo_e1cap_file *osmo_e1cap_open(void *ctx, const char *path)
{
	struct osmo_e1cap_file *f = talloc_zero(ctx, struct osmo_e1cap_file);
	if (!f)
		return NULL;

	f->fd = open(path, O_RDONLY);
	if (f->fd < 0) {
		talloc_free(f);
		return NULL;
	}
	return f;
}

struct osmo_e1cap_pkthdr *osmo_e1cap_read_next(struct osmo_e1cap_file *f)
{
	struct osmo_e1cap_pkthdr *pkt = talloc_zero_size(f, OSMO_E1REC_ALLOC_SIZE);
	int rc;

	if (!pkt)
		return NULL;

	/* read header */
	rc = read(f->fd, (uint8_t *)pkt, sizeof(*pkt));
	if (rc < sizeof(*pkt)) {
		talloc_free(pkt);
		return NULL;
	}
	pkt->len = ntohl(pkt->len);
	/* read data */
	if (pkt->len > OSMO_E1REC_ALLOC_SIZE - sizeof(*pkt)) {
		pkt = talloc_realloc_size(f, pkt, sizeof(*pkt) + pkt->len);
		if (!pkt)
			return NULL;
	}
	rc = read(f->fd, (uint8_t*)(pkt+1), pkt->len);
	if (rc < pkt->len) {
		talloc_free(pkt);
		return NULL;
	}
	return pkt;
}
