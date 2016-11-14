CFLAGS=-g -Wall
LDFLAGS=-losmocore -losmogsm -losmovty -losmoabis -ltalloc

all: osmo-e1-recorder osmo-e1cap-dump hdlc-test

osmo-e1-recorder: e1_recorder.o storage.o vty.o
	$(CC) $(LDFLAGS) -o$@ $^

osmo-e1cap-dump: e1cap_dump.o storage.o flip_bits.o hdlc.o
	$(CC) $(LDFLAGS) -o$@ $^

hdlc-test: hdlc.o hdlc-test.o
	$(CC) $(LDFLAGS) -o$@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	@rm *.o osmo-e1-recorder osmo-e1cap-dump
