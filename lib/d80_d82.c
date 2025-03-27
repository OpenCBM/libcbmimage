/** @file lib/d80_d82.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: specific functions and data for D80 and D82 images
 *
 * @defgroup cbmimage_d80_d82 D80D82 specific functions
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"
#include "cbmimage/helper.h"

#include <assert.h>

/** @brief @internal The number of the sectors on a track in a D80 and D82 image
 * @ingroup cbmimage_d82
 *
 * @remark
 *   - The track number is used as index into this array.
 *     As the tracks are based by 1, the element "0" is left out
 *     and kept as zero. Additionally, the array length is one more than the
 *     number of tracks in the image.
 *   - This array is used both D80 and D82 images; for D80, only the first half
 *     (tracks 1 - 77) is used.
 */
static const uint8_t sectors_in_track_d82[154 + 1] = {
	0,                  // track 0
	29, 29, 29, 29, 29, //   1 -   5
	29, 29, 29, 29, 29, //   6 -  10
	29, 29, 29, 29, 29, //  11 -  15
	29, 29, 29, 29, 29, //  16 -  20
	29, 29, 29, 29, 29, //  21 -  25
	29, 29, 29, 29, 29, //  26 -  30
	29, 29, 29, 29, 29, //  31 -  35
	29, 29, 29, 29, 27, //  36 -  40
	27, 27, 27, 27, 27, //  41 -  45
	27, 27, 27, 27, 27, //  46 -  50
	27, 27, 27, 25, 25, //  51 -  55
	25, 25, 25, 25, 25, //  56 -  60
	25, 25, 25, 25, 23, //  61 -  65
	23, 23, 23, 23, 23, //  66 -  70
	23, 23, 23, 23, 23, //  71 -  75
	23, 23,             //  76 -  77

	// only D82:
	29, 29, 29, 29, 29, //  78 -  82
	29, 29, 29, 29, 29, //  83 -  87
	29, 29, 29, 29, 29, //  88 -  92
	29, 29, 29, 29, 29, //  93 -  97
	29, 29, 29, 29, 29, //  98 - 102
	29, 29, 29, 29, 29, // 103 - 107
	29, 29, 29, 29, 29, // 108 - 112
	29, 29, 29, 29, 27, // 113 - 117
	27, 27, 27, 27, 27, // 118 - 122
	27, 27, 27, 27, 27, // 123 - 127
	27, 27, 27, 25, 25, // 128 - 132
	25, 25, 25, 25, 25, // 133 - 137
	25, 25, 25, 25, 23, // 138 - 142
	23, 23, 23, 23, 23, // 143 - 147
	23, 23, 23, 23, 23, // 148 - 152
	23, 23,             // 153 - 154
};

/** @brief @internal get the number of sectors on a specific track of the D80 and D82 image
 * @ingroup cbmimage_d80_d82
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @param[in] track
 *    the track number for which to obtain the number of sectors
 *
 * @return
 *    Number of sectors on this specific track in the image
 *
 * @remark
 *  - This is the number of sectors on this specific track.
 *    Due to the nature of many CBM disk images, including D80 and D82
 *    images, the number of sectors can vary from track to track. \n
 *    To obtain the maximum number of sectors on any track, use
 *    cbmimage_get_max_sectors() instead!
 *  - As the CBM sector numbering goes, the sector range is from
 *    0 to the result of this call minus 1. \n
 *    For example, for D64 images on the first speed zone
 *    (i.e., tracks 1 to 17), this function will return 21, giving
 *    a range of 0 .. 20.
 *
 */
static uint16_t
cbmimage_i_d80_d82_get_sectors_in_track(
		cbmimage_image_settings * settings,
		uint16_t                  track
		)
{
	assert(settings);

	assert(track <= settings->maxtracks);

	if (track <= settings->maxtracks) {
		return settings->d80_d82.sectors_in_track[track];
	}
	return 0;
}

/** @brief @internal convert the T/S to LBA block address
 * @ingroup cbmimage_d80_d82
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @param[inout] block
 *    pointer to a block address witn an initialized T/S address
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - the block address is completely created from the ts parameter
 *   - an error occurs if the track/sector combination does not
 *     exist on this type of image
 *
 */
static int
cbmimage_i_d80_d82_ts_to_blockaddress(
	cbmimage_image_settings * settings,
	cbmimage_blockaddress *   block
	)
{
	assert(settings != 0);
	assert(block != 0);

	if (block->ts.track == 0 || block->ts.track > settings->maxtracks) {
		return 1;
	}

	assert(block->ts.track > 0);
	assert(block->ts.track <= settings->maxtracks);

	block->lba = settings->d40_d64_d71.track_lba_start[block->ts.track] + block->ts.sector;

	return 0;
}

/** @brief @internal convert the LBA to T/S block address
 * @ingroup cbmimage_d80_d82
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @param[inout] block
 *    pointer to a block address with an initialized LBA address
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - the block address is completely created from the lba parameter
 *   - an error occurs if the lba block does not exist on this type of image
 *
 */
static int
cbmimage_i_d80_d82_lba_to_blockaddress(
		cbmimage_image_settings * settings,
		cbmimage_blockaddress *   block
		)
{
	int ret = 0;

	assert(settings != 0);
	assert(block != 0);

	uint16_t track;

	for (track = 1; track <= settings->maxtracks; ++track) {
		if (settings->d40_d64_d71.track_lba_start[track] > block->lba) {
			break;
		}
	}

	--track;

	uint16_t sector = block->lba - settings->d40_d64_d71.track_lba_start[track];

	if (sector > settings->d40_d64_d71.sectors_in_track[track]) {
		sector = 0;
		track = 0;
		ret = 1;
	}

	CBMIMAGE_TS_SET(block->ts, track, sector);

	return ret;
}

/** @brief @internal create the "LBA start table"
 * @ingroup cbmimage_d80_d82
 *
 * In order to ease the calculation of LBA to T/S or
 * vice versa (cf. cbmimage_i_d80_d82_ts_to_blockaddress(),
 * cbmimage_i_d80_d82_lba_to_blockaddress()) an internal table of the
 * first LBA on a track is used internally. \n
 * This function initializes this table.
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 */
static void
cbmimage_i_d80_d82_calculate_track_lba_start_table(
	cbmimage_image_settings * settings
	)
{
	uint16_t block_number = 1;

	for (uint16_t track = 1; track <= settings->maxtracks; ++track) {
		settings->d80_d82.track_lba_start[track] = block_number;
		block_number += settings->d80_d82.sectors_in_track[track];
	}
}

/** @brief @internal template for the file image functions for D80 and D82 images
 * @ingroup cbmimage_d80_d82
 *
 * This is a template that is copied into the image specific data.
 */
static const cbmimage_fileimage_functions d80_d82_fileimage_functions = {
	.get_sectors_in_track = cbmimage_i_d80_d82_get_sectors_in_track,
	.ts_to_blockaddress = cbmimage_i_d80_d82_ts_to_blockaddress,
	.lba_to_blockaddress = cbmimage_i_d80_d82_lba_to_blockaddress,
};

/** @brief @internal create a D80 or D82 image
 * @ingroup cbmimage_d80_d82
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] imagetype
 *    The type of image that should be created
 *
 * @param[in] imagetype_name
 *    The name of the image type that should be stored
 *    in the images. \n
 *    This value is only informational and can be asked
 *    for by the user; it is not used internally.
 *
 * @param[in] maxtracks
 *    The maximum number of tracks of this image
 *
 */
static void
cbmimage_i_d80_d82_image_create(
		cbmimage_fileimage * image,
		cbmimage_imagetype   imagetype,
		unsigned char *      imagetype_name,
		uint8_t              maxtracks
		)
{
	static const cbmimage_image_settings settings_template = {
		.fct = d80_d82_fileimage_functions,
		.image = NULL,

		.info_offset_diskname = 0x06,

		.dir_tracks[0] = 39,
		.dir_tracks[1] = 38,

		.d80_d82.sectors_in_track = sectors_in_track_d82,

		CBMIMAGE_BAM_AND_BAM_COUNTER_CREATE(.d80_d82, 0,   1, 0x06, 5, 4, 38, 0),
		CBMIMAGE_BAM_AND_BAM_COUNTER_CREATE(.d80_d82, 1,  51, 0x06, 5, 4, 38, 3),
		CBMIMAGE_BAM_AND_BAM_COUNTER_CREATE(.d80_d82, 2, 101, 0x06, 5, 4, 38, 6),
		CBMIMAGE_BAM_AND_BAM_COUNTER_CREATE(.d80_d82, 3, 151, 0x06, 5, 4, 38, 9),

		.dir = CBMIMAGE_BLOCK_INIT_FROM_TS(39, 1),

		.maxsectors = 29,
		.bytes_in_block = 256,
	};

	if (image) {
		cbmimage_image_settings * settings = image->settings;
		*settings = settings_template;

		settings->maxtracks = maxtracks;
		settings->imagetype = imagetype;
		settings->imagetype_name = imagetype_name;
		settings->image = image;

		settings->bam = settings->d80_d82.bam;
		settings->bam_counter = settings->d80_d82.bam_counter;

		switch (imagetype) {
			case TYPE_D80:
				settings->bam_count = 2;
				break;

			case TYPE_D82:
				settings->bam_count = 4;
				break;

			default:
				assert("Unknown type" == 0);
				break;
		}

		cbmimage_i_d80_d82_calculate_track_lba_start_table(image->settings);

		cbmimage_i_create_last_block(image);

		settings->info = cbmimage_blockaccessor_create_from_ts(image, 39, 0);
		cbmimage_blockaddress_init_from_ts(image, &settings->dir);

		// Init BAM data
		cbmimage_i_init_bam_selector(settings, settings->d80_d82.bam, CBMIMAGE_ARRAYSIZE(settings->d80_d82.bam));
		cbmimage_i_init_bam_counter_selector(settings, settings->d80_d82.bam_counter, CBMIMAGE_ARRAYSIZE(settings->d80_d82.bam_counter));
	}
}

/** @brief @internal create structures for a D80 image
 * @ingroup cbmimage_d80_d82
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_d80_image_open(
		cbmimage_fileimage * image
		)
{
	cbmimage_i_d80_d82_image_create(
			image,
			TYPE_D80, "D80",
			77);
}

/** @brief @internal create structures for a D82 image
 * @ingroup cbmimage_d80_d82
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_d82_image_open(
		cbmimage_fileimage * image
		)
{
	cbmimage_i_d80_d82_image_create(
			image,
			TYPE_D82, "D82",
			154);
}
