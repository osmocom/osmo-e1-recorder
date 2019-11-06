#pragma once
#include <stdbool.h>

#include <osmocom/core/msgb.h>
#include <osmocom/core/logging.h>
#include <osmocom/vty/command.h>
#include <osmocom/abis/e1_input.h>

/* logging */
enum {
	DMAIN,
};

/* vty */
enum rec_vty_node {
	RECORDER_NODE = _LAST_OSMOVTY_NODE + 1,
};

struct e1_recorder_line {
	bool has_alarm;
	struct {
		bool enabled;
		uint8_t line_nr;
	} mirror;
};

struct e1_recorder {
	char *storage_path;
	unsigned int max_file_size_mb;
	struct e1_recorder_line line[256];
};

extern struct e1_recorder g_recorder;

/* e1_recorder.c */
void e1ts_raw_recv(struct e1inp_ts *ts, struct msgb *msg);

/* vty.c */
void recorder_vty_init(void);
