#include <string.h>
#include "hdlc.h"

#include <osmocom/core/utils.h>
#include <osmocom/core/bits.h>

const char *tdata = "7e7e7e7e7a76f20609167d3cfcfcfc";

static void hdlc_process_hex_str(struct hdlc_proc *hdlc, const char *hex)
{
	uint8_t *bytes, *bits;
	int string_len = strlen(hex);
	int byte_len = string_len/2;
	int bit_len = byte_len*8;
	int rc;

	printf("hex string   = %s\n", hex);
	bytes = alloca(byte_len);
	bits = alloca(bit_len);
	rc = osmo_hexparse(hex, bytes, byte_len);
	printf("parsed bytes = %s\n", osmo_hexdump(bytes, byte_len));

	printf("MSB mode\n");
	osmo_pbit2ubit(bits, bytes, bit_len);
	process_raw_hdlc(hdlc, bits, bit_len);

	printf("LSB mode\n");
	memset(hdlc, 0, sizeof(*hdlc));
	osmo_pbit2ubit_ext(bits, 0, bytes, 0, bit_len, 1);
	process_raw_hdlc(hdlc, bits, bit_len);
}

int main(int argc, char **argv)
{
	struct hdlc_proc hdlc;
	memset(&hdlc, 0, sizeof(hdlc));

	if (argc < 2)
		exit(1);

	hdlc_process_hex_str(&hdlc, argv[1]);
}
