#include <signal.h>
#include <getopt.h>
#include <errno.h>

#include <osmocom/core/signal.h>
#include <osmocom/core/logging.h>
#include <osmocom/core/application.h>
#include <osmocom/vty/vty.h>
#include <osmocom/vty/telnet_interface.h>
#include <osmocom/vty/logging.h>

#include <osmocom/abis/abis.h>
#include <osmocom/abis/e1_input.h>

#include "storage.h"
#include "recorder.h"

static enum osmo_e1cap_capture_mode ts2cap_mode(struct e1inp_ts *ts)
{
	switch (ts->type) {
	case E1INP_TS_TYPE_RAW:
		return OSMO_E1CAP_MODE_RAW;
	case E1INP_TS_TYPE_HDLC:
		return OSMO_E1CAP_MODE_HDLC;
	case E1INP_TS_TYPE_TRAU:
		return OSMO_E1CAP_MODE_TRAU;
	default:
		OSMO_ASSERT(0);
	}
}

/* receive a raw message frome the E1 timeslot */
void e1ts_raw_recv(struct e1inp_ts *ts, struct msgb *msg)
{
	struct e1_recorder_line *rline = &g_recorder.line[ts->line->num];
	enum osmo_e1cap_capture_mode cap_mode = ts2cap_mode(ts);
	int rc;

	/* FIXME: special processing of TFP and PGSL */

	rc = e1frame_store(ts, msg, cap_mode);
	if (rc < 0) {
		LOGP(DMAIN, LOGL_FATAL, "Error writing E1/T1 frame to disk\n");
		exit(1);
	}

	if (rline->mirror.enabled) {
		struct e1inp_line *other_line =
				e1inp_line_find(rline->mirror.line_nr);
		struct e1inp_ts *other_ts;
		other_ts = &other_line->ts[ts->num-1];
		if (!other_ts) {
			msgb_free(msg);
			return;
		}
		/* forward data to destination line */
		OSMO_ASSERT(other_ts->type == ts->type);
		msgb_enqueue(&other_ts->raw.tx_queue, msg);
	} else
		msgb_free(msg);
}

static int inp_sig_cb(unsigned int subsys, unsigned int signal,
		      void *handler_data, void *signal_data)
{
	OSMO_ASSERT(subsys == SS_L_INPUT);

	/* FIXME */

	return 0;
}

static const struct log_info_cat recorder_categories[] = {
	[DMAIN] = {
		.name = "MAIN",
		.description = "Osmocom E1 Recorder",
		.enabled = 1, .loglevel = LOGL_DEBUG,
	},
};
static struct log_info info = {
	.cat = recorder_categories,
	.num_cat = ARRAY_SIZE(recorder_categories),
};

struct vty_app_info vty_info = {
	.name = "osmo-e1-recorder",
	.version = "0",
	.copyright = "(C) 2016 by Harald Welte <laforge@gnumonks.org>\n",
};

static void *rec_tall_ctx;
struct e1_recorder g_recorder;
static char *g_config_file = "osmo-e1-recorder.cfg";

static void signal_handler(int signo)
{
	switch (signo) {
	case SIGHUP:
		storage_close();
		break;
	case SIGUSR1:
		talloc_report(rec_tall_ctx, stderr);
		break;
	}
}

static void handle_options(int argc, char **argv)
{
	while (1) {
		int option_index = 0, c;
		static const struct option long_options[] = {
			{ "config-file", 1, 0, 'c' },
			{ 0, 0, 0, 0 }
		};

		c = getopt_long(argc, argv, "c:",
				long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'c':
			g_config_file = optarg;
			break;
		}
	}

	if (argc > optind) {
		fprintf(stderr, "Unsupported positional arguments on command line\n");
		exit(2);
	}
}

int main(int argc, char **argv)
{
	int rc;

	rec_tall_ctx = talloc_named_const(NULL, 0, "recorder");

	osmo_init_logging(&info);
	vty_init(&vty_info);
	logging_vty_add_cmds(&info);
	osmo_signal_register_handler(SS_L_INPUT, inp_sig_cb, NULL);
	libosmo_abis_init(rec_tall_ctx);
	e1inp_vty_init();
	recorder_vty_init();

	signal(SIGHUP, &signal_handler);
	signal(SIGUSR1, &signal_handler);

	handle_options(argc, argv);

	rc = vty_read_config_file(g_config_file, NULL);
	if (rc < 0) {
		fprintf(stderr, "Cannot parse configuration file '%s': %s\n", g_config_file,
			strerror(errno));
		exit(1);
	}

	/* start telne tafte reading config for vty_get_bind_adr() */
	telnet_init_dynif(rec_tall_ctx, NULL, vty_get_bind_addr(), 4444);

	while (1) {
		osmo_select_main(0);
	};
}
