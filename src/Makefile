CFLAGS=-g -Wall
LDFLAGS=-losmocore -losmogsm -losmovty -losmoabis -ltalloc

all: osmo-e1-recorder

osmo-e1-recorder: e1_recorder.o storage.o vty.o
	$(CC) $(LDFLAGS) -o$@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	@rm *.o osmo-e1-recorder
