/** @file lib/fat.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: FAT processing functions
 *
 * @defgroup cbmimage_fat FAT processing functions
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"

#include <assert.h>

// #define CBMIMAGE_FAT_DEBUG 1

/** @brief @internal mark for the last block of a FAT
 */
#define CBMIMAGE_FAT_LASTBLOCK 0xFFFFu

/** @brief @internal mark for an unused block in a FAT
 */
#define CBMIMAGE_FAT_UNUSED    0x0000u

/** @brief create a FAT structure
 * @ingroup cbmimage_fat
 *
 * @param[in] image
 *    pointer to the image data internal settings
 *
 * @return
 *    pointer to an initialized fat structure
 *
 * @remark
 *  - after processing is done, close the FAT structure with a call to
 *    cbmimage_fat_close().
 */
cbmimage_fat *
cbmimage_fat_create(
		cbmimage_fileimage * image
		)
{
	assert(image != NULL);

	cbmimage_image_settings * settings = image->settings;
	assert(settings != NULL);

	size_t elements = cbmimage_get_max_lba(settings->image) + 1;

	cbmimage_fat * fat = cbmimage_i_xalloc(sizeof * fat + elements * sizeof(fat->entry[0]) );

	if (fat != NULL) {
		fat->image = image;
		fat->entry = (cbmimage_fat_entry *) &fat->bufferarray[0];
		fat->elements = elements;
	}

	return fat;
}

/** @brief close a FAT structure
 * @ingroup cbmimage_fat
 *
 * @param[in] fat
 *    pointer to a fat structure
 */
void
cbmimage_fat_close(
		cbmimage_fat * fat
		)
{
	cbmimage_i_xfree(fat);
}


/** @brief @internal set a FAT block to a specific LBA target
 * @ingroup cbmimage_fat
 *
 * @param[in] fat
 *    pointer to a fat structure
 *
 * @param[in] block
 *    The address of the block that is to be marked
 *
 * @param[in] target_lba
 *    The LBA to which the block links to. \n
 *    This can also be one of the some special values (cf. remarks)
 *
 * @return
 *    - 0 if everything is ok
 *    - != 0 if an error occurred
 *
 * @remarks
 *   For the LBA, there are some special values:
 *   - CBMIMAGE_FAT_UNUSED:    FAT marks an empty block
 *   - CBMIMAGE_FAT_LASTBLOCK: This is the last block of a chain
 */
int
cbmimage_i_fat_set(
		cbmimage_fat *        fat,
		cbmimage_blockaddress block,
		uint16_t              target_lba
		)
{
	assert(fat != NULL);

	assert(block.lba < fat->elements);

	assert((target_lba == CBMIMAGE_FAT_LASTBLOCK) || (target_lba < fat->elements));

#if CBMIMAGE_FAT_DEBUG
	cbmimage_i_fmt_print("--> Setting FAT from %u/%u(%03X) to %03X.\n",
			block.ts.track, block.ts.sector, block.lba,
			target_lba);
#endif

	fat->entry[block.lba].lba = target_lba;

	return 0;
}

/** @brief set a block in the FAT to a target in the FAT
 * @ingroup cbmimage_fat
 *
 * @param[in] fat
 *    pointer to a fat structure
 *
 * @param[in] block
 *    The address of the block that is to be marked
 *
 * @param[in] target
 *    The target to which the block links to
 *
 * @return
 *    - 0 if everything is ok
 *    - != 0 if an error occurred
 */
int
cbmimage_fat_set(
		cbmimage_fat *        fat,
		cbmimage_blockaddress block,
		cbmimage_blockaddress target
		)
{
	assert(target.lba < fat->elements);

	uint16_t lba = target.lba;

#if CBMIMAGE_FAT_DEBUG
	cbmimage_i_fmt_print("Setting FAT from %u/%u(%03X) to %u/%u(%03X).\n",
			block.ts.track, block.ts.sector, block.lba,
			target.ts.track, target.ts.sector, target.lba);
#endif

	if (lba == 0) {
		lba = CBMIMAGE_FAT_LASTBLOCK;
	}

	return cbmimage_i_fat_set(fat, block, lba);
}

/** @brief set a block in the FAT to unused
 * @ingroup cbmimage_fat
 *
 * @param[in] fat
 *    pointer to a fat structure
 *
 * @param[in] block
 *    The address of the block that is to be marked
 *
 * @return
 *    - 0 if everything is ok
 *    - != 0 if an error occurred
 */
int
cbmimage_fat_clear(
		cbmimage_fat *        fat,
		cbmimage_blockaddress block
		)
{
#if CBMIMAGE_FAT_DEBUG
	cbmimage_i_fmt_print("Clearing FAT entry of %u/%u(%03X).\n",
			block.ts.track, block.ts.sector, block.lba);
#endif

	return cbmimage_i_fat_set(fat, block, CBMIMAGE_FAT_UNUSED);
}

/** @brief @internal get the target of a block in the FAT
 * @ingroup cbmimage_fat
 *
 * @param[in] fat
 *    pointer to a fat structure
 *
 * @param[in] block
 *    The address of the block to be examined
 *
 * @return
 *    The target LBA of the block as written in the FAT. \n
 *    Note that the special values (cf. remark) can be returned, too.
 *
 * @remark
 *   The following special values can be returned:
 *   - CBMIMAGE_FAT_UNUSED:    FAT marks an empty block
 *   - CBMIMAGE_FAT_LASTBLOCK: This is the last block of a chain
 */
int
cbmimage_i_fat_get_target_lba(
		cbmimage_fat *        fat,
		cbmimage_blockaddress block
		)
{
	assert(fat != NULL);

	assert(block.lba < fat->elements);

	return fat->entry[block.lba].lba;
}

/** @brief get the target of a block in the FAT
 * @ingroup cbmimage_fat
 *
 * @param[in] fat
 *    pointer to a fat structure
 *
 * @param[in] block
 *    The address of the block to be examined
 *
 * @return
 *    The target to which the block links to
 *    - If the block is not used, the return will have lba = 0.
 *    - If the block is used, but there is no target, the return value will have lba = 0xFFu.
 */
cbmimage_blockaddress
cbmimage_fat_get(
		cbmimage_fat *        fat,
		cbmimage_blockaddress block
		)
{
	int lba = cbmimage_i_fat_get_target_lba(fat, block) != CBMIMAGE_FAT_UNUSED;

	cbmimage_blockaddress target = cbmimage_block_unused;

	switch (lba) {
		case CBMIMAGE_FAT_UNUSED:
			// not needed, this is already set: target = cbmimage_block_unused;
			break;

		case CBMIMAGE_FAT_LASTBLOCK:
			target.lba = CBMIMAGE_FAT_LASTBLOCK;
			break;

		default:
			CBMIMAGE_BLOCK_SET_FROM_LBA(fat->image, block, lba);
			break;
	}

	return target;
}

/** @brief check if a block in the FAT is marked as used
 * @ingroup cbmimage_fat
 *
 * @param[in] fat
 *    pointer to a fat structure
 *
 * @param[in] block
 *    The address of the block to be examined
 *
 * @return
 *    - 0 if the block is not used
 *    - != 0 if it is marked as used
 */
int
cbmimage_fat_is_used(
		cbmimage_fat *        fat,
		cbmimage_blockaddress block
		)
{
	return cbmimage_i_fat_get_target_lba(fat, block) != CBMIMAGE_FAT_UNUSED;
}


/** @brief dump a FAT structure
 * @ingroup cbmimage_fat
 *
 * @param[in] fat
 *    pointer to a fat structure
 *
 * @param[in] trackformat
 *    - if 0, show the FAT as linear following of LBAs.
 *    - else, show the FAT in the structure that the disk layout defines (track/sector)
 *      The value of trackformat defines how many values at most are output into one line
 */
void
cbmimage_fat_dump(
		cbmimage_fat * fat,
		int            trackformat
		)
{
	assert(fat != NULL);

	cbmimage_i_fmt_print("Dumping FAT:\nWe have %u=0x%04X elements.\n", fat->elements, fat->elements);

	if (trackformat) {
		cbmimage_fileimage * image = fat->image;

		cbmimage_blockaddress block;
		CBMIMAGE_BLOCK_SET_FROM_LBA(image, block, 1);

		cbmimage_i_fmt_print("\n%3u (%04X): %04X ", 0, 0, fat->entry[0]);
		int count = 0;
		do {
			if (block.ts.sector == 0) {
				cbmimage_i_fmt_print("\n%3u (%04X): ", block.ts.track, block.lba);
				count = 0;
			}
			else if (++count >= trackformat) {
				cbmimage_i_fmt_print("\n            ");
				count = 0;
			}
			cbmimage_i_fmt_print("%04X ", fat->entry[block.lba]);
		}
		while (cbmimage_blockaddress_advance(image, &block) == 0);
		cbmimage_i_print("\n");
	}
	else {
		for (int i = 0; i < fat->elements; ++i) {
			if (i % 16 == 0) {
				cbmimage_i_fmt_print("\n%04X: ", i);
			}
			cbmimage_i_fmt_print("%04X ", fat->entry[i]);
		}
		cbmimage_i_print("\n");
	}
}
