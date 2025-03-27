/** @file lib/d40_d64_d71.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: specific functions and data for D40, D64 and D71 images
 *
 * @defgroup cbmimage_d40_d64_d71 D64/D71/D40 specific functions
 * @defgroup cbmimage_d40_d64     D64/D40 specific functions
 * @defgroup cbmimage_d40         D40 specific functions
 * @defgroup cbmimage_d64         D64 specific functions
 * @defgroup cbmimage_d71         D71 specific functions
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"

#include <assert.h>
#include <string.h>

/** @brief @internal The number of the sectors on a track in a D40 image
 * @ingroup cbmimage_d40
 *
 * @remark
 *   - The track number is used as index into this array.
 *     As the tracks are based by 1, the element "0" is left out
 *     and kept as zero. Additionally, the array length is one more than the
 *     number of tracks in the image.
 *   - The number of sectors on a track in speed zone 1 (track 18 to 24) is
 *     one more than for a D64 image
 */
static const uint8_t sectors_in_track_d40[35 + 1] = {
	0,                  // track 0
	21, 21, 21, 21, 21, //  1 -  5
	21, 21, 21, 21, 21, //  6 - 10
	21, 21, 21, 21, 21, // 11 - 15
	21, 21, 20, 20, 20, // 16 - 20
	20, 20, 20, 20, 18, // 21 - 25
	18, 18, 18, 18, 18, // 26 - 30
	17, 17, 17, 17, 17  // 31 - 35
};

/** @brief @internal The number of the sectors on a track in a D64 image
 * @ingroup cbmimage_d64
 *
 * @remark
 *   - The track number is used as index into this array.
 *     As the tracks are based by 1, the element "0" is left out
 *     and kept as zero. Additionally, the array length is one more than the
 *     number of tracks in the image.
 *   - This array is used for "normal" 35 track images, for 40 track images
 *     as well as for 42 track images. The arraay is the name, only the
 *     highest track number differs.
 */
static const uint8_t sectors_in_track_d64[42 + 1] = {
	0,                  // track 0
	21, 21, 21, 21, 21, //  1 -  5
	21, 21, 21, 21, 21, //  6 - 10
	21, 21, 21, 21, 21, // 11 - 15
	21, 21, 19, 19, 19, // 16 - 20
	19, 19, 19, 19, 18, // 21 - 25
	18, 18, 18, 18, 18, // 26 - 30
	17, 17, 17, 17, 17, // 31 - 35
	17, 17, 17, 17, 17, // 36 - 40
	17, 17							// 41 - 42
};

/** @brief @internal The number of the sectors on a track in a D71 image
 * @ingroup cbmimage_d71
 *
 * @remark
 *   - The track number is used as index into this array.
 *     As the tracks are based by 1, the element "0" is left out
 *     and kept as zero. Additionally, the array length is one more than the
 *     number of tracks in the image.
 *   - As the D64 image has differing settings for tracks 36 to 42,
 *     that array cannot be reused. That's why we need a separate array
 *     for D71.
 */
static const uint8_t sectors_in_track_d71[70 + 1] = {
	0,                  // track 0
	21, 21, 21, 21, 21, //  1 -  5
	21, 21, 21, 21, 21, //  6 - 10
	21, 21, 21, 21, 21, // 11 - 15
	21, 21, 19, 19, 19, // 16 - 20
	19, 19, 19, 19, 18, // 21 - 25
	18, 18, 18, 18, 18, // 26 - 30
	17, 17, 17, 17, 17, // 31 - 35
	21, 21, 21, 21, 21, // 36 - 40
	21, 21, 21, 21, 21, // 41 - 45
	21, 21, 21, 21, 21, // 46 - 50
	21, 21, 19, 19, 19, // 51 - 55
	19, 19, 19, 19, 18, // 56 - 60
	18, 18, 18, 18, 18, // 61 - 65
	17, 17, 17, 17, 17  // 66 - 70
};

/** @brief @internal get the number of sectors on a specific track of the D64 image
 * @ingroup cbmimage_d40_d64_d71
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
 *    Due to the nature of many CBM disk images, including D64, D71 and D40
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
static
uint16_t
cbmimage_i_d40_d64_d71_get_sectors_in_track(
		cbmimage_image_settings * settings,
		uint16_t                  track
		)
{
	assert(settings);

	assert(track <= settings->maxtracks);

	if (track <= settings->maxtracks) {
		return settings->d40_d64_d71.sectors_in_track[track];
	}
	return 0;
}

/** @brief @internal convert the T/S to LBA block address
 * @ingroup cbmimage_d40_d64_d71
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
static
int
cbmimage_i_d40_d64_d71_ts_to_blockaddress(
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
 * @ingroup cbmimage_d40_d64_d71
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
static
int
cbmimage_i_d40_d64_d71_lba_to_blockaddress(
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
 * @ingroup cbmimage_d40_d64_d71
 *
 * In order to ease the calculation of LBA to T/S or
 * vice versa (cf. cbmimage_i_d40_d64_d71_ts_to_blockaddress(),
 * cbmimage_i_d40_d64_d71_lba_to_blockaddress()) an internal table of the
 * first LBA on a track is used internally. \n
 * This function initializes this table.
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 */
static
void
cbmimage_i_d40_d64_d71_calculate_track_lba_start_table(
	cbmimage_image_settings * settings
	)
{
	uint16_t block_number = 1;

	for (uint16_t track = 1; track <= settings->maxtracks; ++track) {
		settings->d40_d64_d71.track_lba_start[track] = block_number;
		block_number += settings->d40_d64_d71.sectors_in_track[track];
	}
}

/** @brief @internal Occupy additional BAM entries for 1571 second directory track
 * @ingroup cbmimage_d40_d64_d71
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - The 1571 marks the 2nd dir track as used. Mimic that behaviour.
 */
static
int
cbmimage_i_d71_set_bam(
		cbmimage_image_settings * settings
		)
{
	int ret = 0;

	cbmimage_fileimage * image = settings->image;

	cbmimage_blockaddress block_current;
	cbmimage_blockaddress_init_from_ts_value(image, &block_current, 18 + 35, 0);

	cbmimage_blockaddress block_next = block_current;
	cbmimage_blockaddress_advance(image, &block_next);

	for (int last_run = 0; 1; /* nothing */) {
		if (cbmimage_fat_is_used(settings->fat, block_current)) {
			cbmimage_i_fmt_print("====> Marking already marked block following from %u/%u(%03X) at %u/%u(%03X).\n",
					settings->block_subdir_first.ts.track, settings->block_subdir_first.ts.sector, settings->block_subdir_first.lba,
					block_current.ts.track, block_current.ts.sector, block_current.lba);
			ret = -1;
		}
		cbmimage_fat_set(image->settings->fat, block_current, block_next);

		if (last_run) {
			break;
		}

		block_current = block_next;

		if (cbmimage_blockaddress_advance_in_track(image, &block_next)) {
			block_next = cbmimage_block_unused;
			last_run = 1;
		}
	};

	return ret;
}

/** @brief @internal get the GEOS info block of this image has one
 * @ingroup cbmimage_d40_d64_d71
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @return
 *    - 0 if this is a non-GEOS image
 *    - != 0 if this is a GEOS image
 *
 */
static
int
cbmimage_i_d40_d64_d71_get_geos_infoblock(
		cbmimage_image_settings * settings
		)
{
	const static char geos_text[] = "GEOS format V1.";

	if (strncmp(geos_text, &settings->info->data[0xADu], sizeof geos_text -1) == 0) {
		uint16_t border_track = settings->info->data[0xABu];
		uint16_t border_sector = settings->info->data[0xACu];

		CBMIMAGE_BLOCK_SET_FROM_TS(settings->image, settings->geos_border, border_track, border_sector);
		cbmimage_blockaddress_init_from_ts(settings->image, &settings->geos_border);

		return 1;
	}

	return 0;
}

/** @brief @internal template for the file image functions for D40 or D64 images
 * @ingroup cbmimage_d40_d64_d71
 *
 * This is a template that is copied into the image specific data.
 * This template defines the functions for D40 and D64 images.
 */
static const cbmimage_fileimage_functions d64_fileimage_functions = {
	.get_sectors_in_track = cbmimage_i_d40_d64_d71_get_sectors_in_track,
	.ts_to_blockaddress = cbmimage_i_d40_d64_d71_ts_to_blockaddress,
	.lba_to_blockaddress = cbmimage_i_d40_d64_d71_lba_to_blockaddress,
};

/** @brief @internal template for the file image functions for D71 images
 * @ingroup cbmimage_d40_d64_d71
 *
 * This is a template that is copied into the image specific data.
 * This template defines the functions for D71 images.
 */
static const cbmimage_fileimage_functions d71_fileimage_functions = {
	.get_sectors_in_track = cbmimage_i_d40_d64_d71_get_sectors_in_track,
	.ts_to_blockaddress = cbmimage_i_d40_d64_d71_ts_to_blockaddress,
	.lba_to_blockaddress = cbmimage_i_d40_d64_d71_lba_to_blockaddress,
	.set_bam = cbmimage_i_d71_set_bam,
};

/** @brief @internal create data structures for the BAM of a D64 image
 * @ingroup cbmimage_d64
 *
 */
static const
cbmimage_i_d40_d64_d71_image_settings i_d40_d64 = {
		CBMIMAGE_BAM_AND_BAM_COUNTER_CREATE(, 0, 1, 0x04, 4, 3, 18, 0)
};

/** @brief @internal create data structures for the BAM of a D71 image
 * @ingroup cbmimage_d71
 *
 */
static const
cbmimage_i_d40_d64_d71_image_settings i_d71 = {
		CBMIMAGE_BAM_AND_BAM_COUNTER_CREATE(, 0, 1, 0x04, 4, 3, 18, 0),
		.bam[1]         = CBMIMAGE_BAM_SELECTOR_INIT(        36, 0x00, 3, 3, 18 + 35, 0),
		.bam_counter[1] = CBMIMAGE_BAM_COUNTER_SELECTOR_INIT(36, 0xDD, 1,    18,      0)
};

/** @brief @internal create structures for a D40, D64 or D71 image
 * @ingroup cbmimage_d40_d64_d71
 *
 * @param[in] settings
 *    pointer to the image settings
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
 * @return
 *   - 0 on success
 *   - -1 on error
 */
static
int
cbmimage_i_d40_d64_d71_chdir_partition_init(
		cbmimage_image_settings * settings,
		cbmimage_imagetype        imagetype,
		unsigned char *           imagetype_name,
		uint8_t                   maxtracks
		)
{
	cbmimage_fileimage * image = settings->image;

	settings->maxtracks = maxtracks;
	settings->imagetype = imagetype;
	settings->imagetype_name = imagetype_name;

	settings->info_offset_diskname = 0x90;

	settings->dir_tracks[0] = 18;

	settings->maxsectors = 21;
	settings->bytes_in_block = 256;

	settings->has_super_sidesector = 0;

	switch (imagetype) {
		case TYPE_D40:
			settings->fct = d64_fileimage_functions;
			settings->dir_tracks[1] = 0;
			settings->bam_count = 1;
			settings->d40_d64_d71 = i_d40_d64;
			settings->d40_d64_d71.sectors_in_track = sectors_in_track_d40;
			break;

		case TYPE_D64_40TRACK:
		case TYPE_D64_40TRACK_SPEEDDOS:
		case TYPE_D64_40TRACK_DOLPHIN:
		case TYPE_D64_40TRACK_PROLOGIC:
		case TYPE_D64_42TRACK:
		case TYPE_D64:
			settings->fct = d64_fileimage_functions;
			settings->dir_tracks[1] = 0;
			settings->bam_count = 1;
			settings->d40_d64_d71 = i_d40_d64;
			settings->d40_d64_d71.sectors_in_track = sectors_in_track_d64;
			break;

		case TYPE_D71:
			settings->fct = d71_fileimage_functions;
			settings->dir_tracks[1] = 18 + 35;
			settings->bam_count = 2;
			settings->d40_d64_d71 = i_d71;
			settings->d40_d64_d71.sectors_in_track = sectors_in_track_d71;
			break;
	}

	settings->bam = settings->d40_d64_d71.bam;
	settings->bam_counter = settings->d40_d64_d71.bam_counter;

	cbmimage_i_d40_d64_d71_calculate_track_lba_start_table(image->settings);
	cbmimage_i_create_last_block(image);

	CBMIMAGE_BLOCK_SET_FROM_TS(image, settings->dir, 18, 1);

	settings->info = cbmimage_blockaccessor_create_from_ts(image, 18, 0);
	cbmimage_blockaddress_init_from_ts(image, &settings->dir);

	settings->is_geos = cbmimage_i_d40_d64_d71_get_geos_infoblock(settings);

	// Init BAM data
	cbmimage_i_init_bam_selector(settings, settings->d40_d64_d71.bam, settings->bam_count);
	cbmimage_i_init_bam_counter_selector(settings, settings->d40_d64_d71.bam_counter, settings->bam_count);

	return 0;
}

/** @brief @internal create structures for a D64 image when chdir()ing to an image
 * @ingroup cbmimage_d64
 *
 * @param[in] settings
 *    pointer to the image settings
 *
 * @return
 *   - 0 on success
 *   - -1 on error
 */
int
cbmimage_i_d64_chdir_partition_init(
		cbmimage_image_settings * settings
		)
{
	return cbmimage_i_d40_d64_d71_chdir_partition_init(settings, TYPE_D64, "D64", 35);
}

/** @brief @internal create structures for a D71 image when chdir()ing to an image
 * @ingroup cbmimage_d71
 *
 * @param[in] settings
 *    pointer to the image settings
 *
 * @return
 *   - 0 on success
 *   - -1 on error
 */
int
cbmimage_i_d71_chdir_partition_init(
		cbmimage_image_settings * settings
		)
{
	return cbmimage_i_d40_d64_d71_chdir_partition_init(settings, TYPE_D71, "D71", 70);
}

/** @brief @internal create a D64, D71 or D40 image
 * @ingroup cbmimage_d40_d64_d71
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
static
void
cbmimage_i_d40_d64_d71_image_create(
		cbmimage_fileimage * image,
		cbmimage_imagetype   imagetype,
		unsigned char *      imagetype_name,
		uint8_t              maxtracks
		)
{
	if (image) {
		cbmimage_image_settings * settings = image->settings;
		settings->image = image;
		cbmimage_i_d40_d64_d71_chdir_partition_init(settings, imagetype, imagetype_name, maxtracks);
	}
}

/** @brief @internal create structures for a D40 image
 * @ingroup cbmimage_d40
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_d40_image_open(
		cbmimage_fileimage * image
		)
{
	cbmimage_i_d40_d64_d71_image_create(
			image,
			TYPE_D40, "D40",
			35);
}

/** @brief @internal create structures for a D64 image
 * @ingroup cbmimage_d64
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_d64_image_open(
		cbmimage_fileimage * image
		)
{
	if (image) {
		cbmimage_image_settings * settings = image->settings;
		settings->image = image;
		cbmimage_i_d64_chdir_partition_init(settings);
	}
}

/** @brief @internal create structures for a D64 image with 40 tracks
 * @ingroup cbmimage_d64
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_d64_40track_image_open(
		cbmimage_fileimage * image
		)
{
	cbmimage_i_d40_d64_d71_image_create(
			image,
			TYPE_D64_40TRACK, "D64_40TRACK",
			40);
}

/** @brief @internal create structures for a SpeedDOS D64 image with 40 tracks
 * @ingroup cbmimage_d64
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_d64_40track_speeddos_image_open(
		cbmimage_fileimage * image
		)
{
	cbmimage_i_d40_d64_d71_image_create(
			image,
			TYPE_D64_40TRACK_SPEEDDOS, "D64_40TRACK_SPEEDDOS",
			40);
}

/** @brief @internal create structures for a Dolphin DOS D64 image with 40 tracks
 * @ingroup cbmimage_d64
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_d64_40track_dolphin_image_open(
		cbmimage_fileimage * image
		)
{
	cbmimage_i_d40_d64_d71_image_create(
			image,
			TYPE_D64_40TRACK_DOLPHIN, "D64_40TRACK_DOLPHIN",
			40);
}

/** @brief @internal create structures for a Prologic DOS D64 image with 40 tracks
 * @ingroup cbmimage_d64
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_d64_40track_prologic_image_open(
		cbmimage_fileimage * image
		)
{
	cbmimage_i_d40_d64_d71_image_create(
			image,
			TYPE_D64_40TRACK_PROLOGIC, "D64_40TRACK_PROLOGIC",
			40);
}

/** @brief @internal create structures for a D64 image with 42 tracks
 * @ingroup cbmimage_d64
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_d64_42track_image_open(
		cbmimage_fileimage * image
		)
{
	cbmimage_i_d40_d64_d71_image_create(
			image,
			TYPE_D64_42TRACK, "D64_42TRACK",
			42);
}

/** @brief @internal create structures for a D71 image
 * @ingroup cbmimage_d71
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_d71_image_open(
		cbmimage_fileimage * image
		)
{
	if (image) {
		cbmimage_image_settings * settings = image->settings;
		settings->image = image;
		cbmimage_i_d71_chdir_partition_init(settings);
	}
}
