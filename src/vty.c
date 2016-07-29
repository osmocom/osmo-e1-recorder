
#include <osmocom/abis/e1_input.h>
#include <osmocom/vty/command.h>
#include <osmocom/vty/vty.h>

#include "recorder.h"

#define LINE_STR "Configure Recording for given Line\nE1/T1 Line Number\n"

DEFUN(cfg_recorder, cfg_recorder_cmd,
	"recorder",
	"Configuration of E1 Recorder\n")
{
	vty->node = RECORDER_NODE;
	return CMD_SUCCESS;
}

static const struct e1inp_line_ops dummy_e1i_line_ops = {
	.sign_link_up = NULL,
	.sign_link_down = NULL,
	.sign_link = NULL,
};

DEFUN(cfg_rec_line_ts_mode, cfg_rec_line_ts_mode_cmd,
	"line <0-255> ts <1-31> mode (none|signalling|trau|raw)",
	LINE_STR
	"E1/T1 Timeslot Number\n"
	"E1/T1 Timeslot Number\n"
	"Recording Mode\n"
	"No recording\n"
	"Signalling Data (HDLC)\n"
	"TRAU Frames\n"
	"Raw Data\n")
{
	int line_nr = atoi(argv[0]);
	int ts_nr = atoi(argv[1]);
	int mode = get_string_value(e1inp_ts_type_names, argv[2]);
	struct e1inp_line *line;
	struct e1inp_ts *ts;

	if (mode < 0) {
		vty_out(vty, "Cannot parse mode %s%s", argv[2], VTY_NEWLINE);
		return CMD_WARNING;
	}

	line = e1inp_line_find(line_nr);
	if (!line) {
		vty_out(vty, "Cannot find line %d%s", line_nr, VTY_NEWLINE);
		return CMD_WARNING;
	}
	if (ts_nr >= line->num_ts) {
		vty_out(vty, "Timeslot %d is too large%s", ts_nr, VTY_NEWLINE);
		return CMD_WARNING;
	}
	ts = &line->ts[ts_nr-1];
	e1inp_line_bind_ops(line, &dummy_e1i_line_ops);

	vty_out(vty, "Line %u TS %u mode %u%s", line_nr, ts_nr, mode, VTY_NEWLINE);
	switch (mode) {
	case E1INP_TS_TYPE_NONE:
		/* TOOD: have eqinp_ts_config_none ? */
		ts->type = E1INP_TS_TYPE_NONE;
		break;
	case E1INP_TS_TYPE_SIGN:
		e1inp_ts_config_sign(ts, line);
		break;
	case E1INP_TS_TYPE_RAW:
		e1inp_ts_config_raw(ts, line, &e1ts_raw_recv);
		break;
	default:
		vty_out(vty, "Unknown mode %u ?!?%s", mode, VTY_NEWLINE);
		break;
	}

	/* notify driver of change */
	e1inp_line_update(line);

	return CMD_SUCCESS;
}

DEFUN(cfg_rec_line_mirror, cfg_rec_line_mirror_cmd,
	"line <0-255> mirror <0-255>",
	LINE_STR "Mirror this line to another line\n"
	"E1/T1 Line Number\n")
{
	uint8_t line_nr = atoi(argv[0]);
	uint8_t peer_nr = atoi(argv[1]);
	struct e1_recorder_line *line = &g_recorder.line[line_nr];
	struct e1_recorder_line *peer = &g_recorder.line[peer_nr];
	/* look up morror peer and enable mirror flag on peer */
	if (peer->mirror.enabled &&
	    peer->mirror.line_nr != line_nr) {
		vty_out(vty, "Peer line %u already part of another mirror%s",
			peer_nr, VTY_NEWLINE);
		return CMD_WARNING;
	}
	peer->mirror.enabled = true;
	peer->mirror.line_nr = line_nr;
	/* enable mirror flag of current line */
	if (line->mirror.enabled &&
	    line->mirror.line_nr != peer_nr) {
		vty_out(vty, "Line %u already part of another mirror%s",
			line_nr, VTY_NEWLINE);
		return CMD_WARNING;
	}
	line->mirror.enabled = true;
	line->mirror.line_nr = peer_nr;
	return CMD_SUCCESS;
}

DEFUN(cfg_rec_no_line_mirror, cfg_rec_no_line_mirror_cmd,
	"no line <0-255> mirror",
	LINE_STR "Mirror this line to another line\n"
	"E1/T1 Line Number\n")
{
	uint8_t line_nr = atoi(argv[0]);
	struct e1_recorder_line *line = &g_recorder.line[line_nr];
	struct e1_recorder_line *peer;

	if (!line->mirror.enabled)
		return CMD_WARNING;
	/* look up morror peer (if any) and disable mirror flag on peer */
	peer = &g_recorder.line[line->mirror.line_nr];
	if (peer->mirror.enabled) {
		peer->mirror.enabled = false;
		peer->mirror.line_nr = 0;
	}
	/* dsiable mirror flag of current line */
	if (line->mirror.enabled){
		line->mirror.enabled = false;
		line->mirror.line_nr = 0;
	}
	return CMD_SUCCESS;
}


DEFUN(cfg_rec_save_path, cfg_rec_save_path_cmd,
	"storage-path PATH",
	"Configure the directory for storing recordings\n"
	"Directory to which recordings are stored\n")
{
	osmo_talloc_replace_string(NULL, &g_recorder.storage_path, argv[0]);
	return CMD_SUCCESS;
}

DEFUN(cfg_rec_file_size, cfg_rec_file_size_cmd,
	"file-size-mb <1-9999999>",
	"Configure the maximum file size before starting new file\n"
	"Megabytes\n")
{
	g_recorder.max_file_size_mb = atoi(argv[0]);
	return CMD_SUCCESS;
}

static void config_write_recorder_line(struct vty *vty, unsigned int lnr)
{
	struct e1inp_line *line = e1inp_line_find(lnr);
	struct e1_recorder_line *rline = &g_recorder.line[lnr];
	unsigned int i;

	if (rline->mirror.enabled) {
		vty_out(vty, " line %u mirror %u%s",
			lnr, rline->mirror.line_nr, VTY_NEWLINE);
	}

	if (!line)
		return;

	for (i = 0; i < line->num_ts; i++) {
		struct e1inp_ts *ts = &line->ts[i];
		const char *mode_str;

		mode_str = get_value_string(e1inp_ts_type_names, ts->type);

		vty_out(vty, " line %u ts %u mode %s%s",
			lnr, ts->num, mode_str, VTY_NEWLINE);
	}
}

static int config_write_recorder(struct vty *vty)
{
	unsigned int i;

	vty_out(vty, "recorder%s", VTY_NEWLINE);
	vty_out(vty, " file-size-mb %u%s", g_recorder.max_file_size_mb,
		VTY_NEWLINE);
	vty_out(vty, " storage-path %s%s", g_recorder.storage_path,
		VTY_NEWLINE);
	for (i = 0; i < 255; i++) {
		config_write_recorder_line(vty, i);
	}

	return 0;
}

static struct cmd_node cfg_recorder_node = {
	RECORDER_NODE,
	"%s(config-recorder)# ",
	1,
};

void recorder_vty_init(void)
{
	install_element(CONFIG_NODE, &cfg_recorder_cmd);

	install_node(&cfg_recorder_node, config_write_recorder);
	install_element(RECORDER_NODE, &cfg_rec_line_ts_mode_cmd);
	install_element(RECORDER_NODE, &cfg_rec_line_mirror_cmd);
	install_element(RECORDER_NODE, &cfg_rec_no_line_mirror_cmd);
	install_element(RECORDER_NODE, &cfg_rec_save_path_cmd);
	install_element(RECORDER_NODE, &cfg_rec_file_size_cmd);
}
