AM_CPPFLAGS = \
	$(all_includes) \
	-I$(top_builddir) \
	$(NULL)

AM_CFLAGS = \
	-Wall \
	$(LIBOSMOCORE_CFLAGS) \
	$(LIBOSMOGSM_CFLAGS) \
	$(LIBOSMOVTY_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	$(LIBOSMOABIS_CFLAGS) \
	$(NULL)

AM_LDFLAGS = \
	$(COVERAGE_LDFLAGS) \
	$(NULL)

LDADD = \
	$(LIBOSMOCORE_LIBS) \
	$(LIBOSMOGSM_LIBS) \
	$(LIBOSMOVTY_LIBS) \
	$(COVERAGE_LDFLAGS) \
	$(LIBOSMOABIS_LIBS) \
	$(NULL)

noinst_HEADERS = \
	flip_bits.h \
	hdlc.h \
	recorder.h \
	storage.h

bin_PROGRAMS = \
	osmo-e1-recorder \
	osmo-e1cap-dump \
	hdlc-test \
	$(NULL)

osmo_e1_recorder_SOURCES = \
	e1_recorder.c \
	storage.c \
	vty.c \
	$(NULL)

osmo_e1cap_dump_SOURCES = \
	e1cap_dump.c \
	storage.c \
	flip_bits.c \
	hdlc.c \
	$(NULL)

hdlc_test_SOURCES = \
	hdlc-test.c \
	hdlc.c \
	$(NULL)
