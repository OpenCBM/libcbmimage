/** @file lib/blockaccessor.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: Structure to access specific blocks
 *
 * For CBM disks, the addresses are given by track and sector specifications.
 * On the other side, a so-called LBA (logical block address) is much easier
 * to handle.
 *
 * These functions specify how on address type is converted into the other
 *
 * @defgroup cbmimage_blockaccessor block accessor functions
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"

#include <assert.h>


/** @brief create a block accessor for a specific block
 * @ingroup cbmimage_blockaccessor
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] block
 *    the block address which we want to access
 *
 * @return
 *    - the block accessor structure for this accessor
 *    - NULL if an error occurred
 *
 * @remark
 *   - When the block accessor is not needed anymore,
 *     it must be freed with cbmimage_blockaccessor_close().
 *
 */
cbmimage_blockaccessor *
cbmimage_blockaccessor_create(
		cbmimage_fileimage *   image,
		cbmimage_blockaddress  block
		)
{
	cbmimage_blockaccessor * new_accessor = NULL;

	assert(image != NULL);
	new_accessor = cbmimage_i_xalloc(sizeof *new_accessor);

	if (new_accessor) {
		new_accessor->image = image;

		cbmimage_blockaccessor_set_to(new_accessor, block);
	}

	return new_accessor;
}

/** @brief create a block accessor for a specific T/S
 * @ingroup cbmimage_blockaccessor
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] track
 *    the track of the block which we want to access
 *
 * @param[in] sector
 *    the sector of the block which we want to access
 *
 * @return
 *    - the block accessor structure for this accessor
 *    - NULL if an error occurred
 *
 * @remark
 *   - When the block accessor is not needed anymore,
 *     it must be freed with cbmimage_blockaccessor_close().
 *
 */
cbmimage_blockaccessor *
cbmimage_blockaccessor_create_from_ts(
		cbmimage_fileimage * image,
		uint8_t              track,
		uint8_t              sector
		)
{
	cbmimage_blockaddress block;

	assert(image != NULL);

	CBMIMAGE_BLOCK_SET_FROM_TS(image, block, track, sector);

	return cbmimage_blockaccessor_create(image, block);
}

/** @brief create a block accessor for a specific LBA
 * @ingroup cbmimage_blockaccessor
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] lba
 *    the LBA of the block which we want to access
 *
 * @return
 *    - the block accessor structure for this accessor
 *    - NULL if an error occurred
 *
 * @remark
 *   - When the block accessor is not needed anymore,
 *     it must be freed with cbmimage_blockaccessor_close().
 *
 */
cbmimage_blockaccessor *
cbmimage_blockaccessor_create_from_lba(
		cbmimage_fileimage * image,
		uint16_t             lba
		)
{
	cbmimage_blockaddress block;

	assert(image != NULL);

	CBMIMAGE_BLOCK_SET_FROM_LBA(image, block, lba);

	return cbmimage_blockaccessor_create(image, block);
}


/** @brief close a block accessor, freeing its resources
 * @ingroup cbmimage_blockaccessor
 *
 * @param[in] accessor
 *    pointer to the block accessor
 *
 */
void
cbmimage_blockaccessor_close(
		cbmimage_blockaccessor * accessor
		)
{
	cbmimage_i_blockaccessor_release(accessor);
	cbmimage_i_xfree(accessor);
}

/** @brief set a block accessor for a specific block
 * @ingroup cbmimage_blockaccessor
 *
 * @param[in] accessor
 *    pointer to the block accessor
 *
 * @param[in] block
 *    the block address which we want to access
 *
 * @return
 *   - 0 on success
 *   - != 0 if an error occurred
 *
 */
int
cbmimage_blockaccessor_set_to(
		cbmimage_blockaccessor * accessor,
		cbmimage_blockaddress    block
		)
{
	assert(accessor != NULL);

	cbmimage_i_blockaccessor_release(accessor);

	assert(block.ts.track > 0);
	assert(block.ts.track <= cbmimage_get_max_track(accessor->image));
	assert(block.ts.sector < cbmimage_get_sectors_in_track(accessor->image, block.ts.track));
	assert(block.lba > 0);

	if ( (block.ts.track > 0) && (block.ts.track <= cbmimage_get_max_track(accessor->image))
	  && (block.ts.sector < cbmimage_get_sectors_in_track(accessor->image, block.ts.track))
	  && (block.lba > 0)
	   )
	{
		accessor->block = block;
		accessor->data = cbmimage_i_get_address_of_block(accessor->image, block);
	}

	return 0;
}

/** @brief set a block accessor for a specific T/S
 * @ingroup cbmimage_blockaccessor
 *
 * @param[in] accessor
 *    pointer to the block accessor
 *
 * @param[in] track
 *    the track of the block which we want to access
 *
 * @param[in] sector
 *    the sector of the block which we want to access
 *
 * @return
 *   - 0 on success
 *   - != 0 if an error occurred
 *
 */
int
cbmimage_blockaccessor_set_to_ts(
		cbmimage_blockaccessor * accessor,
		uint8_t                  track,
		uint8_t                  sector
		)
{
	assert(accessor != NULL);

	cbmimage_blockaddress block;

	CBMIMAGE_BLOCK_SET_FROM_TS(accessor->image, block, track, sector);

	return cbmimage_blockaccessor_set_to(accessor, block);
}

/** @brief set a block accessor for a specific LBA
 * @ingroup cbmimage_blockaccessor
 *
 * @param[in] accessor
 *    pointer to the block accessor
 *
 * @param[in] lba
 *    the LBA of the block which we want to access
 *
 * @return
 *   - 0 on success
 *   - != 0 if an error occurred
 *
 */
int
cbmimage_blockaccessor_set_to_lba(
		cbmimage_blockaccessor * accessor,
		uint16_t                 lba
		)
{
	assert(accessor != NULL);

	cbmimage_blockaddress block;

	CBMIMAGE_BLOCK_SET_FROM_LBA(accessor->image, block, lba);

	return cbmimage_blockaccessor_set_to(accessor, block);
}

/** @brief advance a block accessor to the next block
 *
 * @ingroup cbmimage_blockaccessor
 *
 * @param[in] accessor
 *    pointer to the block accessor
 *
 * @return
 *   - 0 on success
 *   - != 0 if an error occurred
 *
 * @remark
 *   - If you want to follow the chain of the block as given by the first two
 *     byte, use cbmimage_blockaccessor_follow() instead.
 *   - The block is changed to the next block on the current track.
 *   - If it is the last block of this track, the first block of the next track
 *     is used.
 *   - If this is the last block of this image, an error results and no
 *     block is returned. In this case, the return value is != 0.
 */
int
cbmimage_blockaccessor_advance(
		cbmimage_blockaccessor * accessor
		)
{
	assert(accessor != NULL);
	assert(accessor->image != NULL);

	cbmimage_blockaddress block = accessor->block;

	int ret = cbmimage_blockaddress_advance(accessor->image, &block);

	if (ret == 0) {
		cbmimage_blockaccessor_set_to(accessor, block);
	}
	else {
		cbmimage_i_blockaccessor_release(accessor);
	}

	return ret;
}


/** @brief get the next block of this accessor if we follow the chain
 *
 * @ingroup cbmimage_blockaccessor
 *
 * @param[in] accessor
 *    pointer to the block accessor
 *
 * @param[inout] block_next
 *    pointer to a valid block address. On termination, it will contain the next block address.
 *
 * @return
 *   - 0 on success
 *   - > 0: this is the last block, the return gives the number of valid byte in this block.
 *          This is, if the link of this last block is (0,x), it will return the value x.
 *   - -1 if an error occurred
 *
 * @remark
 *   - If this is the last block of this chain, return value is != 0. In this case,
 *     the parameter block is not modified.
 */
int
cbmimage_blockaccessor_get_next_block(
		cbmimage_blockaccessor * accessor,
		cbmimage_blockaddress *  block_next
		)
{
	int ret = -1;

	assert(accessor != NULL);
	assert(accessor->data != NULL);

	uint8_t track  = accessor->data[0];
	uint8_t sector = accessor->data[1];

	if (track == 0) {
		ret = (sector == 0) ? 256 : sector;
	}
	else if (track <= cbmimage_get_max_track(accessor->image)) {
		if (sector < cbmimage_get_sectors_in_track(accessor->image, track)) {
			if (block_next) {
				ret = cbmimage_blockaddress_init_from_ts_value(accessor->image, block_next, track, sector);
			}
			else {
				ret = 0;
			}
		}
	}

	return ret;
}


/** @brief follow the block chain of a block accessor to the next block
 *
 * @ingroup cbmimage_blockaccessor
 *
 * @param[in] accessor
 *    pointer to the block accessor
 *
 * @return
 *   - 0 on success
 *   - != 0 if an error occurred
 *
 * @remark
 *   - If order to advance to the next hardware block of this image, use
 *     cbmimage_blockaccessor_advance() instead.
 *   - This function follows the chain of the block as give by the first two
 *     byte of the current block.
 *   - If this is the last block of this chain, return value is != 0. In this case,
 *     the accessor cannot be used to access any data anymore.
 */
int
cbmimage_blockaccessor_follow(
		cbmimage_blockaccessor * accessor
		)
{
	assert(accessor != NULL);
	assert(accessor->image != NULL);

	cbmimage_blockaddress block_next = cbmimage_block_unused;

	int ret = cbmimage_blockaccessor_get_next_block(accessor, &block_next);

	if (ret == 0) {
		cbmimage_blockaccessor_set_to(accessor, block_next);
	}
	else {
		cbmimage_i_blockaccessor_release(accessor);
	}

	return ret;
}

/** @brief @internal release a block accessor, allowing to re-use it or to close it
 * @ingroup cbmimage_blockaccessor
 *
 * @param[in] accessor
 *    pointer to the block accessor
 *
 * @return
 *   - 0 on success
 *   - != 0 if an error occurred
 *
 * @remark
 *   This function releases the block that was accessed by the block accessor.
 *   Afterwards, the accessor can be re-used for another block, or it can be closed.
 *
 */
int
cbmimage_i_blockaccessor_release(
		cbmimage_blockaccessor * accessor
		)
{
	if (accessor != NULL) {
		accessor->data = NULL;
		accessor->block = cbmimage_block_unused;
	}

	return 0;
}
