#include "cbmimage.h"

#include "cbmimage/testhelper.h"

#include <stdio.h>

#define SIZE_D64         (174848)
#define SIZE_D64_ERROR   (174848 + 683)
#define SIZE_D64_40TRACK (174848 + 17 * 5 * 256)
#define SIZE_D64_42TRACK (174848 + 17 * 7 * 256)
#define SIZE_D40         (174848 + 7 * 256)
#define SIZE_D71         (2 * SIZE_D64)
#define SIZE_D71_ERROR   (2 * SIZE_D64_ERROR)

#define SIZE_D_MAX       SIZE_D71_ERROR

static void
check_sectors_d40(
	cbmimage_fileimage * image
	)
{
	TEST_ASSERT(cbmimage_get_max_track(image) == 35);
	TEST_ASSERT(cbmimage_get_max_sectors(image) == 21);

	for (int i = 1; i < 18; i++) {
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i) == 21);
	}
	for (int i = 18; i < 25; i++) {
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i) == 20);
	}
	for (int i = 25; i < 31; i++) {
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i) == 18);
	}
	for (int i = 31; i <= 35; i++) {
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i) == 17);
	}
}

static void
check_sectors_d64(
	cbmimage_fileimage * image,
	uint16_t             tracks
	)
{
	TEST_ASSERT((cbmimage_get_max_track(image) == 35) || (cbmimage_get_max_track(image) == 40) || (cbmimage_get_max_track(image) == 42));
	TEST_ASSERT(cbmimage_get_max_track(image) == tracks);
	TEST_ASSERT(cbmimage_get_max_sectors(image) == 21);

	for (int i = 1; i < 18; i++) {
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i) == 21);
	}
	for (int i = 18; i < 25; i++) {
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i) == 19);
	}
	for (int i = 25; i < 31; i++) {
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i) == 18);
	}
	for (int i = 31; i <= tracks; i++) {
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i) == 17);
	}
}

static void
check_sectors_d71(
	cbmimage_fileimage * image
	)
{
	TEST_ASSERT(cbmimage_get_max_track(image) == 70);
	TEST_ASSERT(cbmimage_get_max_sectors(image) == 21);

	for (int i = 1; i < 18; i++) {
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i) == 21);
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i + 35) == 21);
	}
	for (int i = 18; i < 25; i++) {
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i) == 19);
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i + 35) == 19);
	}
	for (int i = 25; i < 31; i++) {
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i) == 18);
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i + 35) == 18);
	}
	for (int i = 31; i <= 35; i++) {
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i) == 17);
		TEST_ASSERT(cbmimage_get_sectors_in_track(image, i + 35) == 17);
	}
}

static void
check_lba(
	cbmimage_fileimage * image
	)
{
	uint16_t maxtrack       = cbmimage_get_max_track(image);
	size_t   bytes_in_block = cbmimage_get_bytes_in_block(image);

	fprintf(stdout, "Testing image %s:\n", cbmimage_get_imagetype_name(image));

	cbmimage_blockaddress block;
	cbmimage_blockaddress_init_from_ts_value(image, &block, 1, 0);

	do {
		char buffer[bytes_in_block];

		cbmimage_read_block(image, block, buffer, sizeof buffer);
		buffer[0] = block.ts.track;
		buffer[1] = block.ts.sector;
		buffer[2] = 0xFFu;
		buffer[3] = 0xFFu;
		buffer[4] = block.lba & 0xFFu;
		buffer[5] = (block.lba >> 8) & 0xFFu;
		cbmimage_write_block(image, block, buffer, sizeof buffer);
	} while (!cbmimage_blockaddress_advance(image, &block));

	// now, check if everything is where it should be
	const uint8_t * buffer = cbmimage_image_get_raw(image);
	size_t          buffer_size = cbmimage_image_get_raw_size(image);
	size_t          buffer_offset = 0;
	uint16_t        lba_count = 1;

	for (uint16_t track = 1; track <= maxtrack; ++track) {
		uint16_t cnt_sectors = cbmimage_get_sectors_in_track(image, track);
		for (uint16_t sector = 0; sector < cnt_sectors; ++sector) {
			TEST_ASSERT (buffer[buffer_offset    ] == track);
			TEST_ASSERT (buffer[buffer_offset + 1] == sector);
			TEST_ASSERT (buffer[buffer_offset + 2] == 0xFFu);
			TEST_ASSERT (buffer[buffer_offset + 3] == 0xFFu);
			TEST_ASSERT (buffer[buffer_offset + 4] == (lba_count & 0xFFu));
			TEST_ASSERT (buffer[buffer_offset + 5] == ((lba_count >> 8) & 0xFFu));

			for (int16_t i = 6; i < bytes_in_block; ++i) {
				TEST_ASSERT (buffer[buffer_offset + i] == 0x00u);
			}

			buffer_offset += bytes_in_block;
			++lba_count;
		}
	}
}

static void
check_bam(
	cbmimage_fileimage * image
	)
{
	uint16_t maxtrack = cbmimage_get_max_track(image);

	cbmimage_blockaddress block;
	cbmimage_blockaddress_init_from_ts_value(image, &block, 1, 0);

	do {
		if (block.ts.sector == 0) {
			fprintf(stdout, "\n%02u: ", block.ts.track);
		}
		cbmimage_BAM_state bam_state = cbmimage_bam_get(image, block);

		switch (bam_state) {
			case BAM_UNKNOWN:
				fprintf(stdout, "?");
				break;
			case BAM_REALLY_FREE:
				fprintf(stdout, ",");
				break;
			case BAM_FREE:
				fprintf(stdout, ".");
				break;
			case BAM_USED:
				fprintf(stdout, "*");
				break;
			case BAM_DOES_NOT_EXIST:
				break;
		}
	} while (!cbmimage_blockaddress_advance(image, &block));

	fprintf(stdout, "\n");
}

static void
dump_file(
		cbmimage_fileimage * image,
		uint8_t              track,
		uint8_t              sector
		)
{
	static char blockbuffer[256] = { 0 };

	cbmimage_blockaddress block;

	if (cbmimage_blockaddress_init_from_ts_value(image, &block, track, sector)) {
		return;
	}

	fprintf(stdout, "Reading %u/%u\n", block.ts.track, block.ts.sector);
	cbmimage_read_block(image, block, blockbuffer, sizeof blockbuffer);

	dump(blockbuffer, sizeof blockbuffer);

	int v;
	while (0 <= (v = cbmimage_read_next_block(image, &block, blockbuffer, sizeof blockbuffer))) {
		fprintf(stdout, "Read %u/%u\n", block.ts.track, block.ts.sector);
		dump(blockbuffer, sizeof blockbuffer);
	}
}

int
main(
		void
		)
{
	cbmimage_fileimage * image = NULL;

	static char buffer[SIZE_D_MAX];

	TEST_ASSERT(image == NULL);

	image = cbmimage_image_open(buffer, SIZE_D64, TYPE_D64);
	TEST_ASSERT(image != NULL);
	check_sectors_d64(image, 35);
	check_lba(image);
	check_bam(image);
	cbmimage_image_close(image);

	image = cbmimage_image_open(buffer, SIZE_D64_40TRACK, TYPE_D64_40TRACK);
	TEST_ASSERT(image != NULL);
	check_sectors_d64(image, 40);
	check_lba(image);
	check_bam(image);
	cbmimage_image_close(image);

	image = cbmimage_image_open(buffer, SIZE_D64_40TRACK, TYPE_D64_40TRACK_DOLPHIN);
	TEST_ASSERT(image != NULL);
	check_sectors_d64(image, 40);
	check_lba(image);
	check_bam(image);
	cbmimage_image_close(image);

	image = cbmimage_image_open(buffer, SIZE_D64_40TRACK, TYPE_D64_40TRACK_PROLOGIC);
	TEST_ASSERT(image != NULL);
	check_sectors_d64(image, 40);
	check_lba(image);
	check_bam(image);
	cbmimage_image_close(image);

	image = cbmimage_image_open(buffer, SIZE_D64_40TRACK, TYPE_D64_40TRACK_SPEEDDOS);
	TEST_ASSERT(image != NULL);
	check_sectors_d64(image, 40);
	check_lba(image);
	check_bam(image);
	cbmimage_image_close(image);

	image = cbmimage_image_open(buffer, SIZE_D64_42TRACK, TYPE_D64_42TRACK);
	TEST_ASSERT(image != NULL);
	check_sectors_d64(image, 42);
	check_lba(image);
	check_bam(image);
	cbmimage_image_close(image);

	image = cbmimage_image_open(buffer, SIZE_D40, TYPE_D40);
	TEST_ASSERT(image != NULL);
	check_sectors_d40(image);
	check_lba(image);
	check_bam(image);
	cbmimage_image_close(image);

	image = cbmimage_image_open(buffer, SIZE_D71, TYPE_D71);
	TEST_ASSERT(image != NULL);
	check_sectors_d71(image);
	check_lba(image);
	check_bam(image);
	cbmimage_image_close(image);

	return 0;
}
