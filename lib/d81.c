/** @file lib/d81.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: specific functions and data for D81 images
 *
 * @defgroup cbmimage_d81 D81 specific functions
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"
#include "cbmimage/helper.h"

#include <assert.h>
#include <string.h>

/** @brief @internal function for chdir'ing in a 1581 image
 * @ingroup cbmimage_d81
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
cbmimage_i_d81_chdir(
		cbmimage_image_settings * settings,
		cbmimage_dir_entry *      dir_entry
		)
{
	assert(settings);

	cbmimage_fileimage * image = settings->image;

	cbmimage_blockaddress block_subdir_first;
	cbmimage_blockaddress block_subdir_last;

	assert(image);

	size_t block_count;

	if (cbmimage_i_dir_get_partition_data(dir_entry, &block_subdir_first, &block_subdir_last, &block_count)) {
		return -1;
	}

	if (block_subdir_first.ts.sector != 0) {
		cbmimage_i_fmt_print("Partition does not start on track boundary but at %u/%u(%03X).\n",
				block_subdir_first.ts.track,
				block_subdir_first.ts.sector,
				block_subdir_first.lba
				);
		return -1;
	}

	if (block_subdir_last.ts.sector != settings->maxsectors - 1) {
		cbmimage_i_fmt_print("Partition does not end on track boundary but at %u/%u(%03X).\n",
				block_subdir_last.ts.track,
				block_subdir_last.ts.sector,
				block_subdir_last.lba
				);
		return -1;
	}

	int track_start = block_subdir_first.ts.track;
	int track_last  = block_subdir_last.ts.track;
	int track_dir   = settings->dir_tracks[0];

	if ( (track_start == track_dir)
	  || (track_last == track_dir)
	  || ( (track_start < track_dir) && (track_last > track_dir) )
	   )
	{
		cbmimage_i_fmt_print("Partition from %u/%u(%03X) to %u/%u(%03X) crosses directory track!\n",
				block_subdir_first.ts.track,
				block_subdir_first.ts.sector,
				block_subdir_first.lba,
				block_subdir_last.ts.track,
				block_subdir_last.ts.sector,
				block_subdir_last.lba
				);
		return -1;
	}

#if 0
	/// @todo: introduce a verbosity level for debugging

	cbmimage_i_fmt_print("Setting directory to: %u/%u(%03X) till %u/%u(%03X).\n",
			block_subdir_first.ts.track,
			block_subdir_first.ts.sector,
			block_subdir_first.lba,
			block_subdir_last.ts.track,
			block_subdir_last.ts.sector,
			block_subdir_last.lba
			);
#endif

	int ret = cbmimage_i_dir_set_subpartition_relative(settings, block_subdir_first, block_subdir_last);

	cbmimage_blockaddress address_tmp = block_subdir_first;

	settings->info = cbmimage_blockaccessor_create(image, address_tmp);

	cbmimage_blockaddress_advance(image, &address_tmp);
	settings->d81.bam[0].block = address_tmp;
	settings->d81.bam_counter[0].block = address_tmp;
	cbmimage_blockaddress_advance(image, &address_tmp);
	settings->d81.bam[1].block = address_tmp;
	settings->d81.bam_counter[1].block = address_tmp;

	cbmimage_blockaddress_advance(image, &address_tmp);
	settings->dir = address_tmp;

	settings->subdir_global_addressing = 1;

	// Init BAM data
	settings->bam = settings->d81.bam;
	settings->bam_counter = settings->d81.bam_counter;
	cbmimage_i_init_bam_selector(settings, settings->bam, settings->bam_count);
	cbmimage_i_init_bam_counter_selector(settings, settings->bam_counter, settings->bam_count);

	/* we do not have a specific directory track */
	settings->dir_tracks[0] = 0;
	settings->dir_tracks[1] = 0;

	return ret;
}

/** @brief @internal Occupy additional BAM entries for 1581 sub-dirs
 * @ingroup cbmimage_helper
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
cbmimage_i_d81_set_bam(
		cbmimage_image_settings * settings
		)
{
	int ret = 0;

	if (settings->subdir_global_addressing && settings->block_subdir_first.lba > 0) {
		cbmimage_fileimage * image = settings->image;

		// we are in a sub-dir, process the BAM entries
		cbmimage_blockaddress block_current;
		cbmimage_blockaddress_init_from_ts_value(image, &block_current, 1, 0);

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
			if (cbmimage_blockaddress_advance(image, &block_next)) {
				block_next = cbmimage_block_unused;
				last_run = 1;
				continue;
			}

			// check if we will reach the partition with the next step.
			if (block_next.lba == settings->block_subdir_first.lba) {
				// skip the partition area
				block_next = settings->block_subdir_last;
				cbmimage_blockaddress_advance(image, &block_next);
			}
		}
	}

	return ret;
}

/** @brief @internal get the GEOS info block of this image has one
 * @ingroup cbmimage_d81
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
cbmimage_i_d81_get_geos_infoblock(
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

/** @brief @internal template for the file image functions for D81 images
 * @ingroup cbmimage_d81
 *
 * This is a template that is copied into the image specific data.
 */
static const cbmimage_fileimage_functions d81_fileimage_functions = {
	.chdir = cbmimage_i_d81_chdir,
	.set_bam = cbmimage_i_d81_set_bam,
};


/** @brief @internal create data structures for the BAM of a D81 image
 * @ingroup cbmimage_d81
 *
 */
static const
cbmimage_i_d81_image_settings i_d81 = {
		CBMIMAGE_BAM_AND_BAM_COUNTER_CREATE(, 0,  1, 0x10, 6, 5, 40, 1),
		CBMIMAGE_BAM_AND_BAM_COUNTER_CREATE(, 1, 41, 0x10, 6, 5, 40, 2),
};

/** @brief @internal create structures for a D81 image when chdir()ing to an image
 * @ingroup cbmimage_d81
 *
 * @param[in] settings
 *    pointer to the image settings
 *
 * @return
 *   - 0 on success
 *   - -1 on error
 */
int
cbmimage_i_d81_chdir_partition_init(
		cbmimage_image_settings * settings
		)
{
	cbmimage_fileimage * image = settings->image;

	settings->fct = d81_fileimage_functions;
	settings->imagetype = TYPE_D81;
	settings->imagetype_name = "D81";

	settings->info_offset_diskname = 0x04;
	settings->dir_tracks[0] = 40;
	settings->dir_tracks[1] = 0;

	settings->d81 = i_d81;

	settings->maxtracks = 80;
	settings->maxsectors = 40;
	settings->bytes_in_block = 256;

	settings->has_super_sidesector = 1;

	settings->bam_count = 2;
	settings->bam = settings->d81.bam;
	settings->bam_counter = settings->d81.bam_counter;

	cbmimage_i_create_last_block(image);

	CBMIMAGE_BLOCK_SET_FROM_TS(image, settings->dir, 40, 3);

	settings->info = cbmimage_blockaccessor_create_from_ts(image, 40, 0);
	cbmimage_blockaddress_init_from_ts(image, &settings->dir);

	settings->is_geos = cbmimage_i_d81_get_geos_infoblock(settings);

	// Init BAM data
	cbmimage_i_init_bam_selector(settings, settings->d81.bam, CBMIMAGE_ARRAYSIZE(settings->d81.bam));
	cbmimage_i_init_bam_counter_selector(settings, settings->d81.bam_counter, CBMIMAGE_ARRAYSIZE(settings->d81.bam_counter));

	return 0;
}

/** @brief @internal create structures for a D81 image
 * @ingroup cbmimage_d81
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_d81_image_open(
		cbmimage_fileimage * image
		)
{
	if (image) {
		cbmimage_image_settings * settings = image->settings;
		settings->image = image;
		cbmimage_i_d81_chdir_partition_init(settings);
	}
}
