/** @file lib/validate.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: validate disk
 *
 * @defgroup cbmimage_validate Function to validate a disk
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"

#include <assert.h>
#include <string.h>

/*
 * Set this to something else but 0 to give debugging output
 */
// #define CBMIMAGE_VALIDATE_DEBUG 1

/** @brief @internal mark blocks as used
 * @ingroup cbmimage_validate
 *
 * This function marks a block in to loop detectors.
 *
 * There is a first, global loop detector. It is used to find out if
 * files (or other link chains) "overlap" each other, that is, one link chain
 * and another link chain end in the same blocks. Thus, it is not really a loop,
 * but some chain error that is detected.
 *
 * The second, local loop detector, is used to find out if a link chain has an
 * intertial loop. This loop detector has to be newly initialized for each link
 * chain.
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] loop_detector
 *    pointer to the local loop detector
 *
 * @param[in] block_start
 *    The starting block of this chain.
 *    This data is not used internally. It is helpful in the output,
 *    telling what went wrong and where a loop was detected.
 *
 * @param[in] block_current
 *    The current block of this chain.
 *    This block is marked as used.
 *
 * @param[in] block_target
 *    The next block/the target of the current block in this chain.
 *    This block is set as target for the current block
 *
 * @return
 *    - 0 if there is no loop or sharing of links.
 *    - != 0 if not
 *
 */
static
int
cbmimage_i_mark_global_and_local(
		cbmimage_fileimage *  image,
		cbmimage_loop *       loop_detector,
		cbmimage_blockaddress block_start,
		cbmimage_blockaddress block_current,
		cbmimage_blockaddress block_target
		)
{
	int ret = 0;

#if CBMIMAGE_VALIDATE_DEBUG
	cbmimage_i_fmt_print("Marking %u/%u(%03X).\n", block_current.ts.track, block_current.ts.sector, block_current.lba);
#endif

	assert(block_start.lba > 0);
	assert(block_current.lba > 0);

	if ((loop_detector != NULL) && (cbmimage_loop_mark(loop_detector, block_current))) {
		cbmimage_i_fmt_print("====> Found loop following from %u/%u(%03X) at %u/%u(%03X).\n",
				block_start.ts.track, block_start.ts.sector, block_start.lba,
				block_current.ts.track, block_current.ts.sector, block_current.lba);

		ret = -1;
	}

	if (cbmimage_fat_is_used(image->settings->fat, block_current)) {
		cbmimage_i_fmt_print("====> Marking already marked block following from %u/%u(%03X) at %u/%u(%03X).\n",
				block_start.ts.track, block_start.ts.sector, block_start.lba,
				block_current.ts.track, block_current.ts.sector, block_current.lba);
		ret = -1;
	}
	cbmimage_fat_set(image->settings->fat, block_current, block_target);

	return ret;
}

/** @brief @internal follow a chain and check if it is consistent
 * @ingroup cbmimage_validate
 *
 * This function follows a link chain. Doing this, it determines
 * if there is a loop, or if this link chain shares blocks with another link chain.
 * If any of this occurrs, this function reports an error.
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] block_start
 *    The starting block of this chain.
 *    From this block, the chain is followed and all blocks are marked as used.
 *
 * @param[inout] count_blocks
 *    Pointer to a variable. On return, this function will add the number of blocks it
 *    followed in the chain. \n
 *    Can be NULL; in this case, nothing will be returned.
 *
 * @return
 *    - 0 if there is no loop or sharing of links.
 *    - != 0 if not
 *
 */
static
int
cbmimage_i_validate_follow_chain(
		cbmimage_fileimage *  image,
		cbmimage_blockaddress block_start,
		size_t *              count_blocks
		)
{
	int ret = 0;
	size_t count = 0;

	assert(image != NULL);

	cbmimage_loop * loop_detector = cbmimage_loop_create(image);

	cbmimage_chain * chain;
	for (chain = cbmimage_chain_start(image, block_start);
			!cbmimage_chain_is_done(chain);
			cbmimage_chain_advance(chain))
	{
		if (cbmimage_i_mark_global_and_local(image, loop_detector, block_start, cbmimage_chain_get_current(chain), cbmimage_chain_get_next(chain))) {
			ret = -1;
		}
		++count;
	}
	if (ret > 0) {
		ret = 0;
	}

	cbmimage_chain_close(chain);
	cbmimage_loop_close(loop_detector);

	if (count_blocks) {
		*count_blocks += count;
	}

	return ret;
}

/** @brief @internal check a 1581 partition
 * @ingroup cbmimage_validate
 *
 * A 1581 partition is just a contiguous area of the disk, without any implied
 * structure. \n
 * Thus, this function just marks all the blocks in the partition, nothing more.
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] block_start
 *    The starting block of this partion.
 *    From this block, the chain is followed and all blocks are marked as used.
 *
 * @param[in] count
 *    The count of blocks that are occupied by this partition
 *
 * @return
 *    - 0 if everything is ok
 *    - != 0 if not
 *
 */
int
cbmimage_i_validate_1581_partition(
		cbmimage_fileimage *  image,
		cbmimage_blockaddress block_start,
		int                   count
		)
{
	int ret = 0;
	cbmimage_blockaddress block_current = block_start;
	cbmimage_blockaddress block_next = block_current;
	cbmimage_blockaddress_advance(image, &block_next);

	for (; count > 0; --count) {
		if (count == 1) {
			block_next = cbmimage_block_unused;
		}

		if (cbmimage_i_mark_global_and_local(image, NULL, block_start, block_current, block_next)) {
			ret = -1;
		}

		if (cbmimage_blockaddress_advance(image, &block_current)) {
			cbmimage_i_fmt_print("Partition at the end of the image that exceeds the end of disk by %u blocks.\n",
					count);
			ret = -1;
		}

		if (count != 1) {
			cbmimage_blockaddress_advance(image, &block_next);
		}
	}

	return ret;
}

/** @brief @internal check if the loop detector and the BAM are identical
 * @ingroup cbmimage_validate
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @return
 *    - 0 if the BAM and the loop detector are identical
 *    - != 0 if not
 *
 * @remark
 *    - this function goes through all blocks and tests if the BAM is free and
 *      not marked in the loop detector, or not free and marked in the loop
 *      detector. If there is an unequality, it reports an error.
 */
static
int
cbmimage_i_bam_check_equality(
		cbmimage_fileimage * image
		)
{
	int ret = 0;

	cbmimage_blockaddress block;

	cbmimage_blockaddress_init_from_ts_value(image, &block, 1, 0);

	do {
		int is_marked_in_loop = cbmimage_fat_is_used(image->settings->fat, block);
		int is_used_in_bam = cbmimage_bam_get(image, block) == BAM_USED;

		cbmimage_BAM_state bam_state = cbmimage_bam_get(image, block);

		if (is_marked_in_loop && !is_used_in_bam) {
			cbmimage_i_fmt_print("Block %u/%u(%03X) is marked as used, but the BAM tells us it is empty.\n",
					block.ts.track, block.ts.sector, block.lba);
		}
		else if (!is_marked_in_loop && is_used_in_bam) {
			cbmimage_i_fmt_print("Block %u/%u(%03X) is not marked as used, but the BAM tells us it is used.\n",
					block.ts.track, block.ts.sector, block.lba);
		}
	} while (!cbmimage_blockaddress_advance(image, &block));

	return ret;
}

enum {
	SUPER_SIDESECTOR_OFFSET_LINK_TRACK    = 0x00,
	SUPER_SIDESECTOR_OFFSET_LINK_SECTOR   = 0x01,
	SUPER_SIDESECTOR_OFFSET_LINK_COUNT    = 0x02,
	SUPER_SIDESECTOR_OFFSET_GROUP0_TRACK  = 0x03,
	SUPER_SIDESECTOR_OFFSET_GROUP0_SECTOR = 0x04
};

enum {
	SUPER_SIDESECTOR_LINK_COUNT_FIXED = 0xFEu
};

enum {
	SIDESECTOR_OFFSET_LINK_TRACK    = 0x00,
	SIDESECTOR_OFFSET_LINK_SECTOR   = 0x01,
	SIDESECTOR_OFFSET_LINK_COUNT    = 0x02,
	SIDESECTOR_OFFSET_RECORD_SIZE   = 0x03,
	SIDESECTOR_OFFSET_SS0_TRACK     = 0x04,
	SIDESECTOR_OFFSET_SS0_SECTOR    = 0x05,
	SIDESECTOR_OFFSET_CHAIN_TRACK   = 0x10,
	SIDESECTOR_OFFSET_CHAIN_SECTOR  = 0x11,
};

enum {
	SIDESECTOR_MAX_COUNT = 6
};

/** @brief @internal check plausibility of a super side-sector
 * @ingroup cbmimage_validate
 *
 * @param[in] chain
 *    pointer to a link chain that points to the super side-sector
 *
 * @return
 *    - 0 if the super side-sector is plausible
 *    - != 0 if not
 *
 * @remark
 *    The super side-sector is plausible if the following applies:
 *    - the link chain and the first side-sector group are identical
 *    - the number of the side-sector block is 0xFE.
 */
static
int
cbmimage_i_validate_super_sidesector_plausibility(
		cbmimage_chain * chain
		)
{
	int ret = 0;

	uint8_t * ptr_super_sidesector = cbmimage_chain_get_data(chain);

	cbmimage_blockaddress sss = cbmimage_chain_get_current(chain);

	if ( (ptr_super_sidesector[SUPER_SIDESECTOR_OFFSET_LINK_TRACK] != ptr_super_sidesector[SUPER_SIDESECTOR_OFFSET_GROUP0_TRACK])
			|| (ptr_super_sidesector[SUPER_SIDESECTOR_OFFSET_LINK_SECTOR] != ptr_super_sidesector[SUPER_SIDESECTOR_OFFSET_GROUP0_SECTOR])
	   )
	{
		cbmimage_i_fmt_print("Super side-sector at %u/%u(%03X) links to %u/%u, but gives the first group at %u/%u!\n",
			sss.ts.track, sss.ts.sector, sss.lba,
			ptr_super_sidesector[SUPER_SIDESECTOR_OFFSET_LINK_TRACK], ptr_super_sidesector[SUPER_SIDESECTOR_OFFSET_LINK_SECTOR],
			ptr_super_sidesector[SUPER_SIDESECTOR_OFFSET_GROUP0_TRACK], ptr_super_sidesector[SUPER_SIDESECTOR_OFFSET_GROUP0_SECTOR]
			);
		ret = -1;
	}

	if (ptr_super_sidesector[SUPER_SIDESECTOR_OFFSET_LINK_COUNT] != SUPER_SIDESECTOR_LINK_COUNT_FIXED) {
		cbmimage_i_fmt_print("Super side-sector block at %u/%u(%03X) is not marked as such, it has number 0x%02X instead of 0x%02X\n",
			sss.ts.track, sss.ts.sector, sss.lba,
			ptr_super_sidesector[SUPER_SIDESECTOR_OFFSET_LINK_COUNT], SUPER_SIDESECTOR_LINK_COUNT_FIXED
			);
		ret = -1;
	}

	return ret;
}

/** @brief @internal check if the super side-sector does not contain extra data
 * @ingroup cbmimage_validate
 *
 * @param[in] chain
 *    pointer to a link chain that points to the super side-sector
 *
 * @param[in] offset
 *    the offset from which to check if the super side-sector is empty
 *
 * @return
 *    - 0 if the super side-sector is empty after the offset
 *    - != 0 if not
 */
static
int
cbmimage_i_validate_super_sidesector_end(
		cbmimage_chain * chain,
		uint8_t          offset
		)
{
	int ret = 0;

	uint8_t * ptr_super_sidesector = cbmimage_chain_get_data(chain);
	cbmimage_blockaddress sss = cbmimage_chain_get_current(chain);


	for (int i = offset; i < 0x100; ++i) {
		if (ptr_super_sidesector[i] != 0) {
			cbmimage_i_fmt_print("Super side-sector at %u/%u contains data after end at offset 0x%02X.\n",
					sss.ts.track, sss.ts.sector, sss.lba,
					i
					);
			ret = -1;
			break;
		}
	}

	return ret;
}

/** @brief @internal check plausibility of a (non-super) side-sector
 * @ingroup cbmimage_validate
 *
 * @param[in] chain_sidesector
 *    pointer to a link chain that points to the side-sector
 *
 * @param[in] first_sidesector
 *    pointer to a buffer that contains the data of the first side-sector in
 *    this side-sector group
 *
 * @param[in] count_sidesector
 *    the number of this side-sector
 *
 * @param[in] recordlength
 *
 * @return
 *    - 0 if the side-sector is plausible
 *    - != 0 if not
 *
 * @remark
 *    The side-sector is plausible if the following applies:
 *    - the addresses of all side-sectors in this side-sector group are
 *      identical to the first side-sector of this group.
 *    - the address of this side-sector is at the right place
 *    - the recordlength is the same as the record-length in the directory
 */
static
int
cbmimage_i_validate_sidesector_plausibility(
		cbmimage_chain * chain_sidesector,
		uint8_t *        first_sidesector,
		uint8_t          count_sidesector,
		int              recordlength
		)
{
	int ret = 0;

	uint8_t * data = cbmimage_chain_get_data(chain_sidesector);

	cbmimage_blockaddress block_this_sidesector = cbmimage_chain_get_current(chain_sidesector);

	count_sidesector %= SIDESECTOR_MAX_COUNT;

	// first, check if the common data for all side-sectors is identical.
	// We check this by comparing the data with the first side-sector.
	// If there are differences, we know the side-sectors are not consistent
	for (int i = 0; i < SIDESECTOR_MAX_COUNT; ++i) {
		if ( (data[SIDESECTOR_OFFSET_SS0_TRACK + 2 * i] != first_sidesector[SIDESECTOR_OFFSET_SS0_TRACK + 2 * i])
		  || (data[SIDESECTOR_OFFSET_SS0_SECTOR + 2 * i] != first_sidesector[SIDESECTOR_OFFSET_SS0_SECTOR + 2 * i])
			)
		{
			cbmimage_i_fmt_print("Side-sector %u differes from 1st in data of side-sector %u:\n"
					"In 1st, it is %u/%u, but it is %u/%u here.\n",
					count_sidesector, i,
					first_sidesector[SIDESECTOR_OFFSET_SS0_TRACK + 2 * i], first_sidesector[SIDESECTOR_OFFSET_SS0_SECTOR + 2 * i],
					data[SIDESECTOR_OFFSET_SS0_TRACK + 2 * i], data[SIDESECTOR_OFFSET_SS0_SECTOR + 2 * i]
					);
			ret = -1;
		}
	}

	// Now, check if this side-sector is correctly mentioned in its place.
	// If it is, we can be sure it is correctly mentioned in all side-sectors.

	if ( (data[SIDESECTOR_OFFSET_SS0_TRACK + 2 * count_sidesector] != block_this_sidesector.ts.track)
	  || (data[SIDESECTOR_OFFSET_SS0_SECTOR + 2 * count_sidesector] != block_this_sidesector.ts.sector)
	   )
	{
		cbmimage_i_fmt_print("Side-sector %u is not correctly mentioned in the side-sector common area!\n"
				"Should be %u/%u, but is %u/%u.\n",
				count_sidesector,
				block_this_sidesector.ts.track, block_this_sidesector.ts.sector,
				data[SIDESECTOR_OFFSET_SS0_TRACK + 2 * count_sidesector], data[SIDESECTOR_OFFSET_SS0_SECTOR + 2 * count_sidesector]
				);
		ret = -1;
	}

	// Check if the record-length is correct
	if (data[SIDESECTOR_OFFSET_RECORD_SIZE] != recordlength) {
		cbmimage_i_fmt_print("Record-length in side-sector %u is wrong! Should be %u, but is %u.\n",
				count_sidesector,
				recordlength,
				data[SIDESECTOR_OFFSET_RECORD_SIZE]
				);
		ret = -1;
	}

	return ret;
}

/** @brief @internal validate the side-sector chain
 * @ingroup cbmimage_validate
 *
 * @param[in] chain_sidesector
 *    pointer to a link chain that points to the side-sector
 *
 * @param[in] chain_file
 *    pointer to a link chain that points to the file contents
 *
 * @return
 *    - 0 if the side-sector chain is correct
 *    - != 0 if not
 *
 * @remark
 *   This function goes through all the links in the side-sector
 *   block and checks that it matches the corresponding block
 *   in the file chain.
 */
static
int
cbmimage_i_validate_sidesector_chain(
		cbmimage_chain * chain_sidesector,
		cbmimage_chain * chain_file
		)
{
	int ret = 0;

	uint8_t * sidesector = cbmimage_chain_get_data(chain_sidesector);

	int offset_block;

	for (offset_block = SIDESECTOR_OFFSET_CHAIN_TRACK; offset_block < 0x100; offset_block += 2) {
		cbmimage_blockaddress block_current = cbmimage_chain_get_current(chain_file);

		if ((sidesector[offset_block] | sidesector[offset_block + 1]) != 0) {
			// there is a link; check if it exists in the file chain, too!
			if (cbmimage_chain_is_done(chain_file)) {
				cbmimage_i_fmt_print("End of file, but link in side-sector to %u/%u.\n",
						sidesector[offset_block], sidesector[offset_block + 1]
						);
				ret = -1;
			}

			if ( (block_current.ts.track != sidesector[offset_block])
			  || (block_current.ts.sector != sidesector[offset_block + 1])
			   )
			{
				cbmimage_i_fmt_print("File has block %u/%u, but the side-sector links to %u/%u.\n",
						block_current.ts.track, block_current.ts.sector,
						sidesector[offset_block], sidesector[offset_block + 1]
						);
				ret = -1;
			}

			cbmimage_chain_advance(chain_file);
		}
		else {
			// end of link chain in side-sector block

			if (!cbmimage_chain_is_done(chain_file)) {
				cbmimage_i_fmt_print("Link in side-sector is done, but the file continues at %u/%u.\n",
						block_current.ts.track, block_current.ts.sector
						);
				ret = -1;
			}

			// check if all the other links are filled with 0.
			for (; offset_block < 0x100; offset_block += 2) {
				if ((sidesector[offset_block] | sidesector[offset_block + 1]) != 0) {
					// there is some data; should not be there!
					cbmimage_i_fmt_print("Extra data after end in side-sector block at %u/%u.\n",
							block_current.ts.track, block_current.ts.sector
							);
					ret = -1;
				}
			}
		}
	}

	return ret;
}

/** @brief @internal validate the side sector block of a REL file
 * @ingroup cbmimage_validate
 *
 * This function checks if the link chain of a REL file is represented correctly
 * by the side-sector block(s) of it.
 *
 * Additionally, it also checks if the side-sector blocks are consistent and
 * do not share blocks with other files or structures.
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] dir_entry
 *    The directory entry of this file
 *
 * @param[inout] count_blocks
 *    Pointer to a variable. On return, this function will add the number of blocks it
 *    followed in the chain. \n
 *    Can be NULL; in this case, nothing will be returned.
 *
 * @return
 *    - 0 if there is no loop or sharing of links.
 *    - != 0 if not
 *
 */
static
int
cbmimage_i_validate_rel_file(
		cbmimage_fileimage * image,
		cbmimage_dir_entry * dir_entry,
		size_t *             count_blocks
		)
{
	int ret = 0;
	size_t block_count = 0;

	assert(image != NULL);
	assert(dir_entry != NULL);

	cbmimage_image_settings * settings = image->settings;
	assert(settings);

	cbmimage_blockaddress block_super_sidesector = cbmimage_block_unused;
	cbmimage_blockaddress block_sidesector = cbmimage_block_unused;

	cbmimage_chain * chain_super_sidesector = NULL;
	uint8_t super_sidesector_offset = 0;

	// check if we have a super side-sector. If so, process it first and do some initial
	// plausibiliy tests
	if (!settings->has_super_sidesector) {
		block_sidesector = dir_entry->rel_sidesector_block;
	}
	else {
		block_super_sidesector = dir_entry->rel_sidesector_block;

		chain_super_sidesector = cbmimage_chain_start(image, block_super_sidesector);

		if (cbmimage_i_mark_global_and_local(image, NULL, block_super_sidesector, block_super_sidesector, cbmimage_chain_get_next(chain_super_sidesector))) {
			ret = -1;
		}

		++block_count;

		if (cbmimage_i_validate_super_sidesector_plausibility(chain_super_sidesector)) {
			ret = -1;
		}

		block_sidesector = cbmimage_chain_get_next(chain_super_sidesector);

		super_sidesector_offset = SUPER_SIDESECTOR_OFFSET_GROUP0_TRACK;
	}

	cbmimage_loop * loop_detector = cbmimage_loop_create(image);

	cbmimage_chain * chain_sidesector;
	cbmimage_chain * chain_file;

	chain_file = cbmimage_chain_start(image, dir_entry->start_block);

	uint8_t first_sidesector[0x100];
	cbmimage_blockaddress block_first_sidesector;

	int count_sidesector = 0;
	for (chain_sidesector = cbmimage_chain_start(image, block_sidesector);
			!cbmimage_chain_is_done(chain_sidesector);
			cbmimage_chain_advance(chain_sidesector), ++count_sidesector)
	{
		++block_count;

		if (cbmimage_i_mark_global_and_local(image, loop_detector, dir_entry->rel_sidesector_block, cbmimage_chain_get_current(chain_sidesector), cbmimage_chain_get_next(chain_sidesector))) {
			ret = -1;
		}

		if (count_sidesector % SIDESECTOR_MAX_COUNT == 0) {
			// this is the first side-sector of a six-group of side-sector blocks, remember it
			block_first_sidesector = cbmimage_chain_get_current(chain_sidesector);
			memcpy(first_sidesector, cbmimage_chain_get_data(chain_sidesector), sizeof first_sidesector);

			if (chain_super_sidesector == NULL) {
				// there is no super side-sector, make sure we do not have too many side-sectors
				if (count_sidesector != 0) {
					cbmimage_i_fmt_print("We have side-sector no. %u at %u/%u(%03X)!\n",
							count_sidesector,
							block_first_sidesector.ts.track, block_first_sidesector.ts.sector, block_first_sidesector.lba
							);
					ret = -1;
				}
			}
			else {
				// there is a super side-sector, make sure it points to this side-sector
				if (super_sidesector_offset >= 0xFF) {
					cbmimage_i_print("Super side-sector block is overflowed!\n");
					ret = -1;
				}
				else {
					uint8_t * ptr_super_sidesector = cbmimage_chain_get_data(chain_super_sidesector);
					if ( (ptr_super_sidesector[super_sidesector_offset] != block_first_sidesector.ts.track)
							|| (ptr_super_sidesector[super_sidesector_offset + 1] != block_first_sidesector.ts.sector)
						 )
					{
						cbmimage_i_fmt_print("Super side-sector says block is at %u/%u, but it is at %u/%u!\n",
							ptr_super_sidesector[super_sidesector_offset], ptr_super_sidesector[super_sidesector_offset + 1],
							block_first_sidesector.ts.track, block_first_sidesector.ts.sector
								);
						ret = -1;
					}
					super_sidesector_offset += 2;
				}
			}
		}

		if (cbmimage_i_validate_sidesector_plausibility(chain_sidesector, first_sidesector, count_sidesector, dir_entry->rel_recordlength)) {
			ret = -1;
		}

		if (cbmimage_i_validate_sidesector_chain(chain_sidesector, chain_file)) {
			ret = -1;
		}

	}

	if (chain_super_sidesector != NULL) {
		ret = cbmimage_i_validate_super_sidesector_end(chain_super_sidesector, super_sidesector_offset) | ret;
	}

	if (ret > 0) {
		ret = 0;
	}

	cbmimage_chain_close(chain_file);
	cbmimage_chain_close(chain_sidesector);
	cbmimage_chain_close(chain_super_sidesector);
	cbmimage_loop_close(loop_detector);

	if (count_blocks) {
		*count_blocks += block_count;
	}

	return ret;
}

/** @brief @internal validate the specialities of a GEOS file
 * @ingroup cbmimage_validate
 *
 * This functions checks that the info block and the chains from the VLIR table
 * (if it is a VLIR file) do not share blocks with other files or structures.
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] dir_entry
 *    The directory entry of this file
 *
 * @param[inout] count_blocks
 *    Pointer to a variable. On return, this function will add the number of blocks it
 *    followed in the chain. \n
 *    Can be NULL; in this case, nothing will be returned.
 *
 * @return
 *    - 0 if there is no loop or sharing of links.
 *    - != 0 if not
 *
 */
static
int
cbmimage_i_validate_geos_file(
		cbmimage_fileimage * image,
		cbmimage_dir_entry * dir_entry,
		size_t *             count_blocks
		)
{
	int ret = 0;
	size_t block_count = 0;

	if (dir_entry->is_geos) {
		if (dir_entry->geos_is_vlir) {
			assert((count_blocks == NULL) || (*count_blocks == 1));

			cbmimage_chain * chain_recordblock = cbmimage_chain_start(image, dir_entry->start_block);
			if (chain_recordblock) {
				// go throught the record block and check each record
				char * recordblock_data = cbmimage_chain_get_data(chain_recordblock);
				int i;
				for (i = 2; i < 0x100; i += 2) {
					uint8_t track = recordblock_data[i];
					uint8_t sector = recordblock_data[i + 1];

					// check if we reached the end of the chain
					if (track == 0 && sector == 0) {
						break;
					}

					// check if the record does not exist
					if (track == 0 && sector == 0xFFu) {
						continue;
					}

					cbmimage_blockaddress block_record;
					CBMIMAGE_BLOCK_SET_FROM_TS(image, block_record, track, sector);
					ret = cbmimage_i_validate_follow_chain(image, block_record, &block_count) | ret;
				}
				for (i; i < 0x100; i += 2) {
					uint8_t track = recordblock_data[i];
					uint8_t sector = recordblock_data[i + 1];

					if (track != 0 || sector != 0) {
						cbmimage_i_fmt_print("VLIR record block at %u/%u(%03X) contains data after offset %02X.\n",
								dir_entry->start_block.ts.track, dir_entry->start_block.ts.sector, dir_entry->start_block.lba, i
								);
						ret = -1;
						break;
					}
				}
			}

			cbmimage_chain_close(chain_recordblock);
		}

		if (dir_entry->geos_infoblock.lba > 0) {
			cbmimage_blockaddress block_info = dir_entry->geos_infoblock;
			if (cbmimage_i_mark_global_and_local(image, NULL, block_info, block_info, cbmimage_block_unused)) {
				ret = -1;
			}
			++block_count;
		}

	}
	else {
		ret = 0;
	}

	if (count_blocks) {
		*count_blocks += block_count;
	}

	return ret;
}

/** @brief validate a specific file
 * @ingroup cbmimage_validate
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] dir_entry
 *   pointer to the directory entry of the file to validate
 *
 * @return
 *    - 0 if the file is consistent
 *    - != 0 if not
 *
 * @remark
 *   A file is consistent if the following applies:
 *   - the file chain does not conflict with any other
 *     file chain that was processed before this file
 *
 * @todo implement GEOS support
 * @bug GEOS is not supported yet!
 *
 */
int
cbmimage_i_validate_process_file(
		cbmimage_fileimage * image,
		cbmimage_dir_entry * dir_entry
		)
{
	assert(image != NULL);
	assert(dir_entry != NULL);

	int ret = 0;
	size_t block_count = 0;

#if CBMIMAGE_VALIDATE_DEBUG
	{
		char name_buffer[26];
		cbmimage_dir_extract_name(&dir_entry->name, name_buffer, sizeof name_buffer);
		cbmimage_i_fmt_print("\nFile \"%s\" at %u/%u(%03X):\n", name_buffer, dir_entry->start_block.ts.track, dir_entry->start_block.ts.sector, dir_entry->start_block.lba);
	}
#endif

	switch (dir_entry->type) {
		case DIR_TYPE_PART_D64:
		case DIR_TYPE_PART_D71:
		case DIR_TYPE_PART_D81:

		case DIR_TYPE_PART1581:
			// special handling of a 1581 partition:
			// There is no special meaning in the blocks, just mark the number of
			// blocks as specified in the dir_entry, as used. Especially, do *not*
			// follow the chain!
			ret = cbmimage_i_validate_1581_partition(image, dir_entry->start_block, dir_entry->block_count) || ret;

			// as we cannot count the number of blocks other than what we are given, accept it.
			block_count = dir_entry->block_count;
			break;

		case DIR_TYPE_CMD_NATIVE:
			/// @todo implement CMD native partition
			/// @bug CMD native partition not yet implemented
			break;

		default:
			ret = cbmimage_i_validate_follow_chain(image, dir_entry->start_block, &block_count) || ret;
			break;
	}

	if (dir_entry->type == DIR_TYPE_REL) {
		// this is a REL file, special handling is due
		ret = cbmimage_i_validate_rel_file(image, dir_entry, &block_count) || ret;
	}

	if (dir_entry->is_geos) {
		ret = cbmimage_i_validate_geos_file(image, dir_entry, &block_count) || ret;
	}

	if (dir_entry->block_count != block_count) {
		char name_buffer[26];
		cbmimage_dir_extract_name(&dir_entry->name, name_buffer, sizeof name_buffer);
		cbmimage_i_fmt_print("\nFile \"%s\" reports %u blocks, but occupies %u blocks.\n", name_buffer, dir_entry->block_count, block_count);
		ret = 1;
	}
}

/** @brief validate the disk (and the bam)
 * @ingroup cbmimage_validate
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @return
 *    - 0 if the BAM is consistent
 *    - != 0 if not
 *
 * @remark
 *   A BAM is consistent if the following applies:
 *   - cbmimage_bam_check_consistency() reports success
 *   - all reachable blocks from files are marked as used
 *
 * @todo How should details be given to the caller?
 *
 * @todo Implement GEOS support
 * @bug GEOS is not supported yet!
 *
 */
int
cbmimage_validate(
		cbmimage_fileimage * image
		)
{
	assert(image != NULL);

	cbmimage_image_settings * settings = image->settings;

	assert(settings->fat == NULL);

	if (settings->fat == NULL) {
		settings->fat = cbmimage_fat_create(image);
	}

	int ret = 0;

	if (!settings->is_partition_table) {
		ret = cbmimage_bam_check_consistency(image);

		// now, iterate through all internal data structures and mark the blocks as used

		if (ret == 0) {
			ret = cbmimage_i_validate_follow_chain(image, settings->info->block, NULL);
		}

		if (ret == 0 && settings->bam != NULL) {
			// look if the BAM block is already marked as used. If it is, leave it out.
			// This is because the info and the BAM block are the same on some drives
			// (i.e., D64, D40, ...)
			if (!cbmimage_fat_is_used(image->settings->fat, settings->bam[0].block)) {
				ret = cbmimage_i_validate_follow_chain(image, settings->bam[0].block, NULL);
			}
		}

		if (ret == 0 && settings->geos_border.lba != 0) {
			// mark the GEOS border block as used
				ret = cbmimage_i_validate_follow_chain(image, settings->geos_border, NULL);
		}

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

			ret = cbmimage_i_validate_process_file(image, dir_entry) || ret;

		}

		cbmimage_dir_get_close(dir_entry);
	}

	if (settings->fct.set_bam) {
		ret = settings->fct.set_bam(settings);
	}

	if (!settings->is_partition_table) {
		// check if the BAM is the same as the data we just followed
		ret = cbmimage_i_bam_check_equality(image) | ret;
	}

	return ret;
}
