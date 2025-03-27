#include "cbmimage/internal.h"

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
	cbmimage_image_settings * settings
	)
{
	TEST_ASSERT(settings->maxtracks == 35);
	TEST_ASSERT(settings->maxsectors == 21);

	for (int i = 1; i < 18; i++) {
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i] == 21);
	}
	for (int i = 18; i < 25; i++) {
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i] == 20);
	}
	for (int i = 25; i < 31; i++) {
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i] == 18);
	}
	for (int i = 31; i <= 35; i++) {
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i] == 17);
	}
}

static void
check_sectors_d64(
	cbmimage_image_settings * settings,
	uint16_t                  tracks
	)
{
	TEST_ASSERT((settings->maxtracks == 35) || (settings->maxtracks == 40) || (settings->maxtracks == 42));
	TEST_ASSERT(settings->maxtracks == tracks);
	TEST_ASSERT(settings->maxsectors == 21);

	for (int i = 1; i < 18; i++) {
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i] == 21);
	}
	for (int i = 18; i < 25; i++) {
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i] == 19);
	}
	for (int i = 25; i < 31; i++) {
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i] == 18);
	}
	for (int i = 31; i <= tracks; i++) {
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i] == 17);
	}
}

static void
check_sectors_d71(
		cbmimage_image_settings * settings
		)
{
	TEST_ASSERT(settings->maxtracks == 70);
	TEST_ASSERT(settings->maxsectors == 21);

	for (int i = 1; i < 18; i++) {
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i] == 21);
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i + 35] == 21);
	}
	for (int i = 18; i < 25; i++) {
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i] == 19);
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i + 35] == 19);
	}
	for (int i = 25; i < 31; i++) {
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i] == 18);
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i + 35] == 18);
	}
	for (int i = 31; i <= 35; i++) {
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i] == 17);
		TEST_ASSERT(settings->d40_d64_d71.sectors_in_track[i + 35] == 17);
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
	check_sectors_d64(image->settings, 35);
	cbmimage_image_close(image);

	image = cbmimage_image_open(buffer, SIZE_D64_40TRACK, TYPE_D64_40TRACK);
	TEST_ASSERT(image != NULL);
	check_sectors_d64(image->settings, 40);
	cbmimage_image_close(image);

	image = cbmimage_image_open(buffer, SIZE_D64_40TRACK, TYPE_D64_40TRACK_DOLPHIN);
	TEST_ASSERT(image != NULL);
	check_sectors_d64(image->settings, 40);
	cbmimage_image_close(image);

	image = cbmimage_image_open(buffer, SIZE_D64_40TRACK, TYPE_D64_40TRACK_PROLOGIC);
	TEST_ASSERT(image != NULL);
	check_sectors_d64(image->settings, 40);
	cbmimage_image_close(image);

	image = cbmimage_image_open(buffer, SIZE_D64_40TRACK, TYPE_D64_40TRACK_SPEEDDOS);
	TEST_ASSERT(image != NULL);
	check_sectors_d64(image->settings, 40);
	cbmimage_image_close(image);

	image = cbmimage_image_open(buffer, SIZE_D64_42TRACK, TYPE_D64_42TRACK);
	TEST_ASSERT(image != NULL);
	check_sectors_d64(image->settings, 42);
	cbmimage_image_close(image);

	image = cbmimage_image_open(buffer, SIZE_D40, TYPE_D40);
	TEST_ASSERT(image != NULL);
	check_sectors_d40(image->settings);
	cbmimage_image_close(image);

	image = cbmimage_image_open(buffer, SIZE_D71, TYPE_D71);
	TEST_ASSERT(image != NULL);
	check_sectors_d71(image->settings);
	cbmimage_image_close(image);

	return 0;
}
