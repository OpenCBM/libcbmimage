/** @file lib/d1m_d2m_d4m.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: specific functions and data for D1M, D2M and D4M images
 *
 * @defgroup cbmimage_d1m_d2m_d4m CMD D1M/D2M/D4M specific functions
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"
#include "cbmimage/testhelper.h"

#include <assert.h>
#include <string.h>

/** @brief occupy the blocks for the partition table of a CMD disk
 * @ingroup cbmimage_validate
 *
 * @param[in] settings
 *    pointer to the image data
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - special handling for validating D1M, D2M and D4M partition tables
 *   - the blocks of a partition are treated as single file
 */
static
int
cbmimage_i_d1m_d2m_d4m_set_bam(
		cbmimage_image_settings * settings
		)
{
	int ret = 0;

	cbmimage_fileimage * image = settings->image;

	assert(image != NULL);

	// iterate through all files and mark their blocks as used
	cbmimage_dir_entry * dir_entry;

	for (dir_entry = cbmimage_dir_get_first(image);
			 cbmimage_dir_get_is_valid(dir_entry);
			 cbmimage_dir_get_next(dir_entry)
			)
	{
		if (cbmimage_dir_is_deleted(dir_entry)) {
			continue;
		}

		// as the partitions in the partition table are numerated
		// with "global" block addresses, temporarily disable the relative
		// addressing for the processing of the BAM

		int tmp_subdir_relative_addressing = settings->subdir_relative_addressing;
		settings->subdir_relative_addressing = 0;

		// mark the partition as used
		ret = cbmimage_i_validate_1581_partition(image, dir_entry->start_block, dir_entry->block_count) || ret;

		// restore the relative addressing
		settings->subdir_relative_addressing = tmp_subdir_relative_addressing;
	}

	cbmimage_dir_get_close(dir_entry);
	return ret;
}


/** @brief @internal function for chdir'ing in a D1M, D2M or D4M image
 * @ingroup cbmimage_d1m_d2m_d4m
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
 */
static
int
cbmimage_i_d1m_d2m_d4m_chdir(
		cbmimage_image_settings * settings,
		cbmimage_dir_entry *      dir_entry
		)
{
	int ret = 0;

	assert(settings);

	if (!settings->is_partition_table) {
		return -1;
	}

	cbmimage_fileimage * image = settings->image;

	assert(image);

	settings->is_partition_table = 0;

	cbmimage_blockaddress block_subdir_first = cbmimage_block_unused;
	cbmimage_blockaddress block_subdir_last = cbmimage_block_unused;

	size_t block_count = 0;

	if (cbmimage_i_dir_get_partition_data(dir_entry, &block_subdir_first, &block_subdir_last, &block_count)) {
		return -1;
	}

#if 0
	cbmimage_i_fmt_print(
			"Partition at %u/%u(%03X) till %u/%u(%03X).\n",
			block_subdir_first.ts.track, block_subdir_first.ts.sector, block_subdir_first.lba,
			block_subdir_last.ts.track, block_subdir_last.ts.sector, block_subdir_last.lba
			);
#endif

	settings->subdir_relative_addressing = 1;
	settings->subdir_global_addressing = 0;

	ret = cbmimage_i_dir_set_subpartition_global(settings, block_subdir_first, block_count);

	if (ret == 0) {
		// initialize the image specific data
		switch (dir_entry->type) {
			case DIR_TYPE_PART_CMD_NATIVE:
				ret = cbmimage_i_dnp_chdir_partition_init(settings);
				break;
			case DIR_TYPE_PART_D64:
				ret = cbmimage_i_d64_chdir_partition_init(settings);
				break;
			case DIR_TYPE_PART_D71:
				ret = cbmimage_i_d71_chdir_partition_init(settings);
				break;
			case DIR_TYPE_PART_D81:
				ret = cbmimage_i_d81_chdir_partition_init(settings);
				break;

			default:
				ret = -1;
				break;
		}
	}

	return ret;
}

/** @brief @internal get the GEOS info block of this image has one
 * @ingroup cbmimage_d1m_d2m_d4m
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
cbmimage_i_d1m_d2m_d4m_get_geos_infoblock(
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

/** @brief @internal template for the file image functions for D1M, D2M and D4M images
 * @ingroup cbmimage_d1m_d2m_d4m
 *
 * This is a template that is copied into the image specific data.
 */
static const cbmimage_fileimage_functions d1m_d2m_d4m_fileimage_functions = {
	.chdir = cbmimage_i_d1m_d2m_d4m_chdir,
	.set_bam = cbmimage_i_d1m_d2m_d4m_set_bam,
};

/** @brief @internal create structures for a D1M, D2M and D4M image
 * @ingroup cbmimage_d1m_d2m_d4m
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
 * @param[in] maxsectors
 *    The number of sectors of this image
 *
 * @todo IMPLEMENT
 * @bug NOT IMPLEMENTED YET
 */
void
cbmimage_i_d1m_d2m_d4m_image_create(
		cbmimage_fileimage * image,
		cbmimage_imagetype   imagetype,
		unsigned char *      imagetype_name,
		uint8_t              maxsectors
		)
{
	static const cbmimage_image_settings settings_template = {
		.fct = d1m_d2m_d4m_fileimage_functions,
		.image = NULL,

		.info_offset_diskname = 0xF0u,

		.dir = CBMIMAGE_BLOCK_INIT_FROM_TS(1, 0),

		.maxtracks = 81,
		.maxsectors = 40,
		.bytes_in_block = 256,
	};

	if (image) {
		cbmimage_image_settings * settings = image->settings;
		*settings = settings_template;

		settings->maxsectors = maxsectors;
		settings->imagetype = imagetype;
		settings->imagetype_name = imagetype_name;

		settings->is_partition_table = 1;

		settings->image = image;

		cbmimage_i_create_last_block(image);

		settings->info = cbmimage_blockaccessor_create_from_ts(image, 1, 0); // @@@ 81, 5);

		cbmimage_blockaddress_init_from_ts(image, &settings->dir);

		switch (imagetype) {
			case TYPE_CMD_D1M:
				break;

			case TYPE_CMD_D2M:
				break;

			case TYPE_CMD_D4M:
				break;

			default:
				TEST_ASSERT(0);
				break;
		}

		settings->is_geos = cbmimage_i_d1m_d2m_d4m_get_geos_infoblock(settings);

		// Init BAM data
		cbmimage_i_init_bam_selector(settings, settings->d1m_d2m_d4m.bam, settings->bam_count);
		cbmimage_i_init_bam_counter_selector(settings, settings->d1m_d2m_d4m.bam_counter, settings->bam_count);

		CBMIMAGE_BLOCK_SET_FROM_TS(image, image->settings->block_subdir_first, 81, 8);
		CBMIMAGE_BLOCK_SET_FROM_TS(image, image->settings->block_subdir_last,  81, 39);
		image->settings->subdir_relative_addressing = 1;
		image->settings->subdir_global_addressing = 0;
	}
}

/** @brief @internal create structures for a D1M image
 * @ingroup cbmimage_d1m_d2m_d4m
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_d1m_image_open(
		cbmimage_fileimage * image
		)
{
	cbmimage_i_d1m_d2m_d4m_image_create(
			image,
			TYPE_CMD_D1M, "D1M",
			40);
}

/** @brief @internal create structures for a D2M image
 * @ingroup cbmimage_d1m_d2m_d4m
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_d2m_image_open(
		cbmimage_fileimage * image
		)
{
	cbmimage_i_d1m_d2m_d4m_image_create(
			image,
			TYPE_CMD_D2M, "D2M",
			80);
}

/** @brief @internal create structures for a D4M image
 * @ingroup cbmimage_d1m_d2m_d4m
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_d4m_image_open(
		cbmimage_fileimage * image
		)
{
	cbmimage_i_d1m_d2m_d4m_image_create(
			image,
			TYPE_CMD_D4M, "D4M",
			160);
}
