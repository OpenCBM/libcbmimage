/** @file lib/dnp.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: specific functions for CMD native partitions (DNP)
 *
 * @defgroup cbmimage_dnp CMD native partition specific functions
 *
 * @todo The number of free blocks of a DNP partition is wrong. It seems that the first 64 blocks are reserved and not counted. How to be confirmed
 * @todo The partition sizes are not correct. For D1M, the last block is 12/127. For D2M, it is 25/255. For D4M, it is 50/255.
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"
#include "cbmimage/helper.h"

#include <assert.h>
#include <string.h>

/** @brief @internal function for chdir'ing in a DNP image
 * @ingroup cbmimage_dnp
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @param[in] dir_entry
 *    pointer to the dir_entry to which to chdir.
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - This function tests if the directory entry is actually a directory
 *     to which it can change. If it is not valid, it should return with an
 *     error value != 0. In this case, the chdir is aborted and returns with
 *     an error, too.
 *
 * @todo this function is a dummy
 *
 */
static
int
cbmimage_i_dnp_chdir(
		cbmimage_image_settings * settings,
		cbmimage_dir_entry *      dir_entry
		)
{
	assert(settings);

	cbmimage_fileimage * image = settings->image;

	assert(image);

	int ret = -1;

	cbmimage_blockaddress block_subdir_first = cbmimage_block_unused;
	cbmimage_blockaddress block_subdir_last = cbmimage_block_unused;

	size_t block_count = 0;

	if (cbmimage_i_dir_get_partition_data(dir_entry, &block_subdir_first, &block_subdir_last, &block_count)) {
		return -1;
	}

	settings->info                       = cbmimage_blockaccessor_create(settings->image, block_subdir_first);

	cbmimage_blockaccessor_get_next_block(settings->info, &settings->dir);

	ret = 0;

	return ret;
}

/** @brief @internal Occupy additional BAM entries for DNP sub-dirs
 * @ingroup cbmimage_dnp
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - This function is used after chdir'ing. In this case,
 *     the 1581 marks all blocks outside of the partition as used.
 */
static
int
cbmimage_i_dnp_set_bam(
		cbmimage_image_settings * settings
		)
{
	int ret = 0;

	cbmimage_fileimage * image = settings->image;

	// on a DNP partition, block 1/0 ("C128 boot block") is marked as used, too.
	// Additionally, the BAM blocks 1/3 - 1/33 have to be marked because these are not linked.
	//
	cbmimage_blockaddress block_current;
	cbmimage_blockaddress block_next = cbmimage_block_unused;
	cbmimage_blockaddress_init_from_ts_value(image, &block_current, 1, 0);

	if (cbmimage_fat_is_used(settings->fat, block_current)) {
		cbmimage_i_fmt_print("====> Marking already marked C128 boot block at %u/%u(%03X).\n",
				block_current.ts.track, block_current.ts.sector, block_current.lba);
		ret = -1;
	}
	cbmimage_fat_set(image->settings->fat, block_current, block_next);

	cbmimage_blockaddress_advance(image, &block_current);
	cbmimage_blockaddress_advance(image, &block_current);
	cbmimage_blockaddress_advance(image, &block_current);
	block_next = block_current;
	cbmimage_blockaddress_advance(image, &block_next);

	for (int i = 3; i < 34; ++i) {
		if (cbmimage_fat_is_used(settings->fat, block_current)) {
			cbmimage_i_fmt_print("====> Marking already marked BAM block at %u/%u(%03X).\n",
					block_current.ts.track, block_current.ts.sector, block_current.lba);
			ret = -1;
		}
		cbmimage_fat_set(image->settings->fat, block_current, block_next);

		if (block_next.lba > 0) {
			block_current = block_next;
			if (cbmimage_blockaddress_advance(image, &block_next) || block_next.ts.sector == 34) {
				block_next = cbmimage_block_unused;
			}
		}
	}

	return ret;
}

/** @brief @internal get the GEOS info block of this image has one
 * @ingroup cbmimage_dnp
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
cbmimage_i_dnp_get_geos_infoblock(
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

/** @brief @internal template for the file image functions for DNP images
 * @ingroup cbmimage_dnp
 *
 * This is a template that is copied into the image specific data.
 */
static const cbmimage_fileimage_functions dnp_fileimage_functions = {
	.chdir = cbmimage_i_dnp_chdir,
	.set_bam = cbmimage_i_dnp_set_bam,
};


/** @brief @internal create structures for a DNP image when chdir()ing to an image
 * @ingroup cbmimage_dnp
 *
 * @param[in] settings
 *    pointer to the image settings
 *
 * @return
 *   - 0 on success
 *   - -1 on error
 */
int
cbmimage_i_dnp_chdir_partition_init(
		cbmimage_image_settings * settings
		)
{
	cbmimage_fileimage * image = settings->image;

	settings->fct = dnp_fileimage_functions;
	settings->imagetype = TYPE_CMD_NATIVE;
	settings->imagetype_name = "DNP";

	settings->info_offset_diskname = 0x04;
	settings->dir_tracks[0] = 1;
	settings->dir_tracks[1] = 0;

	settings->maxtracks = 255; // for now, will be set correctly later
	settings->maxsectors = 256;
	settings->bytes_in_block = 256;

	settings->has_super_sidesector = 1;

	settings->bam_count = CBMIMAGE_ARRAYSIZE(settings->dnp.bam);
	settings->bam = settings->dnp.bam;
	settings->bam_counter = NULL; // DNP do not have a BAM counter, it must be calculated by hand

	cbmimage_i_bam_selector bam_template_first = CBMIMAGE_BAM_SELECTOR_INIT_REVERSE(1, 0x20, 0x20, 0x20, 1, 2);
	cbmimage_i_bam_selector bam_template = CBMIMAGE_BAM_SELECTOR_INIT_REVERSE(8, 0x00, 0x20, 0x20, 1, 3);

	settings->dnp.bam[0] = bam_template_first;

	for (int i = 1; i < CBMIMAGE_ARRAYSIZE(settings->dnp.bam); ++i) {
		settings->dnp.bam[i] = bam_template;
		bam_template.starttrack += 8;
		bam_template.block.ts.sector += 1;
		bam_template.block.lba += 1;
	}

	cbmimage_i_create_last_block(image);

	settings->info = cbmimage_blockaccessor_create_from_ts(image, 1, 1);

	settings->is_geos = cbmimage_i_dnp_get_geos_infoblock(settings);

	// Init BAM data
	cbmimage_i_init_bam_selector(settings, settings->dnp.bam, CBMIMAGE_ARRAYSIZE(settings->dnp.bam));

	// now, read the *real* maximum track from the info block
	uint8_t * block_bam_0 = cbmimage_i_get_address_of_block(settings->image, settings->bam[0].block);
	uint8_t max_track_from_info_block = block_bam_0 ? block_bam_0[8] : 0;

	if ( (block_bam_0 == NULL) || (max_track_from_info_block == 0) ) {
		return -1;
	}

	settings->maxtracks = max_track_from_info_block;

	cbmimage_i_create_last_block(image);

	CBMIMAGE_BLOCK_SET_FROM_TS(image, settings->dir, 1, 34);

	cbmimage_blockaddress_init_from_ts(image, &settings->dir);

	return 0;
}

#if 0
// DHD files are not CMD NATIVE partitions!
// A native partition exists inside of a D1M, D2M or D4M image.
// A DHD file is similar, but not identical to D1M, D2M and D4M.
// Look at the VICE doku for an explanation of DHD.
//

/** @brief @internal create structures for a DNP image
 * @ingroup cbmimage_dnp
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_dnp_image_open(
		cbmimage_fileimage * image
		)
{
	if (image) {
		cbmimage_image_settings * settings = image->settings;
		settings->image = image;
		cbmimage_i_dnp_chdir_partition_init(settings);
	}
}
#endif
