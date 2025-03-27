/** @file lib/blockaddress.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: specificy "addresses" into images
 *
 * For CBM disks, the addresses are given by track and sector specifications.
 * On the other side, a so-called LBA (logical block address) is much easier
 * to handle.
 *
 * These functions specify how on address type is converted into the other
 *
 * @defgroup blockaddress block address functions
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"

#include <assert.h>

/** @brief Definition of an empty cbmimage_blockaddress
 *
 * Whenever you need to use an empty or unused block address, you can use
 * this one instead of defining it on your own.
 */
const cbmimage_blockaddress cbmimage_block_unused = CBMIMAGE_BLOCK_INIT(0, 0, 0);

/** @brief check if T/S is valid
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] track
 *    the track of the block to be tested for existance
 *
 * @param[in] sector
 *    the sector of the block to be tested for existance
 *
 * @return
 *    - 0 if the block T/S is not valid
 *    - != 0 if the block T/S is valid
 *
 * @remark
 */
int
cbmimage_blockaddress_ts_exists(
		cbmimage_fileimage * image,
		uint8_t              track,
		uint8_t              sector
		)
{
	assert(image != NULL);

	cbmimage_image_settings * settings = image->settings;
	assert(settings != NULL);

	return (track > 0
			&& track <= cbmimage_get_max_track(image)
			&& sector < cbmimage_get_max_sectors(image)
			&& sector < cbmimage_get_sectors_in_track(image, track)
			);
}

/** @brief check if LBA is valid
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] lba
 *    the LBA of the block to be tested for existance
 *
 * @return
 *    - 0 if the block T/S is not valid
 *    - != 0 if the block T/S is valid
 *
 * @remark
 */
int
cbmimage_blockaddress_lba_exists(
		cbmimage_fileimage * image,
		uint16_t             lba
		)
{
	assert(image != NULL);

	return (lba > 0
			&& lba <= cbmimage_get_max_lba(image)
			);
}

/** @brief @internal convert the T/S to LBA block address, generic version
 * @ingroup blockaddress
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[inout] block
 *    pointer to a block address with an initialized T/S address
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - the block address is completely created from the ts parameter
 *   - an error occurs if the track/sector combination does not
 *     exist on this type of image
 *   - this version is used if the image type does not specify its own
 *     variant. It assumes that the number of sectors is the same on
 *     all tracks.
 *
 */
static int
cbmimage_i_generic_ts_to_blockaddress(
		cbmimage_fileimage *    image,
		cbmimage_blockaddress * block
		)
{
	assert(image != NULL);
	assert(block != NULL);

	cbmimage_image_settings * settings = image->settings;
	assert(settings != NULL);

	assert(block->ts.track <= settings->maxtracks);
	assert(block->ts.sector < settings->maxsectors);

	if ( !cbmimage_blockaddress_ts_exists(image, block->ts.track, block->ts.sector) )
	{
		block->lba = 0;
		return 1;
	}

	block->lba = (block->ts.track - 1) * settings->maxsectors + block->ts.sector + 1;

	assert(cbmimage_blockaddress_lba_exists(image, block->lba));

	return 0;
}

/** @brief initialize a block address from its T/S specification
 * @ingroup blockaddress
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[inout] block
 *    pointer to the block address that has already track and sector set
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - the block address is updated in the LBA part
 *   - an error occurs if the track/sector combination does not
 *     exist on this type of image
 *
 */
int
cbmimage_blockaddress_init_from_ts(
		cbmimage_fileimage *    image,
		cbmimage_blockaddress * block
		)
{
	assert(image != NULL);
	assert(block != NULL);

	cbmimage_image_settings * settings = image->settings;
	assert(settings != NULL);

	assert(block->ts.track == 0 || cbmimage_blockaddress_ts_exists(image, block->ts.track, block->ts.sector));

	if (settings->fct.ts_to_blockaddress != NULL) {
		return settings->fct.ts_to_blockaddress(settings, block);
	}
	else {
		return cbmimage_i_generic_ts_to_blockaddress(image, block);
	}
}

/** @brief @internal convert the LBA to T/S block address, generic version
 * @ingroup blockaddress
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[inout] block
 *    pointer to a block address that has an initialized LBA address
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - the block address is completely created from the lba parameter
 *   - an error occurs if the lba block does not exist on this type of image
 *   - this version is used if the image type does not specify its own
 *     variant. It assumes that the number of sectors is the same on
 *     all tracks.
 *
 */
static int
cbmimage_i_generic_lba_to_blockaddress(
		cbmimage_fileimage *    image,
		cbmimage_blockaddress * block
		)
{
	assert(image != NULL);
	assert(block != NULL);

	cbmimage_image_settings * settings = image->settings;
	assert(settings != NULL);

	assert(cbmimage_blockaddress_lba_exists(image, block->lba));

	if (block->lba == 0) {
		return 1;
	}

	uint16_t track = ((block->lba - 1) / settings->maxsectors) + 1;
	uint16_t sector = (block->lba - 1) - (track - 1) * settings->maxsectors;

	assert(track <= settings->maxtracks);
	assert(sector<= settings->maxsectors);

	if (track > settings->maxtracks || sector >= settings->maxsectors) {
		CBMIMAGE_TS_CLEAR(block->ts);
		return 1;
	}

	CBMIMAGE_TS_SET(block->ts, track, sector);

	return 0;
}


/** @brief initialize a block address from its T/S specification
 * @ingroup blockaddress
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[inout] block
 *    pointer to the block address that has already the LBA set
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - the block address is updated in the track/sector part
 *   - an error occurs if the LBA does not exist as track/sector
 *     combination on this type of image
 *
 */
int
cbmimage_blockaddress_init_from_lba(
		cbmimage_fileimage *    image,
		cbmimage_blockaddress * block
		)
{
	assert(image != NULL);
	assert(block != NULL);

	cbmimage_image_settings * settings = image->settings;
	assert(settings != NULL);

	assert(cbmimage_blockaddress_lba_exists(image, block->lba));

	if (settings->fct.lba_to_blockaddress != NULL) {
		return settings->fct.lba_to_blockaddress(settings, block);
	}
	else {
		return cbmimage_i_generic_lba_to_blockaddress(image, block);
	}
}

/** @brief initialize a block address when T/S are given
 * @ingroup blockaddress
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[out] block
 *    pointer to a block address. It does not need to be initialized in any way
 *
 * @param[in] track
 *    the track to write into the block address
 *
 * @param[in] sector
 *    the sector to write into the block address
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - the block address is updated in the T/S and in the LBA part
 *   - an error occurs if the track/sector combination does not
 *     exist on this type of image
 *
 */
int
cbmimage_blockaddress_init_from_ts_value(
		cbmimage_fileimage *    image,
		cbmimage_blockaddress * block,
		uint8_t                 track,
		uint8_t                 sector
		)
{
	assert(image != NULL);
	assert(block != NULL);

	assert(cbmimage_blockaddress_ts_exists(image, track, sector));

	CBMIMAGE_TS_SET(block->ts, track, sector);

	return cbmimage_blockaddress_init_from_ts(image, block);
}

/** @brief initialize a block address when LBA is given
 * @ingroup blockaddress
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[out] block
 *    pointer to a block address. It does not need to be initialized in any way
 *
 * @param[in] lba
 *    the LBA to write into the block address
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - the block address is updated in the T/S and in the LBA part
 *   - an error occurs if the LBA does not exist on this type of image
 *
 */
int
cbmimage_blockaddress_init_from_lba_value(
		cbmimage_fileimage *    image,
		cbmimage_blockaddress * block,
		uint16_t                lba
		)
{
	assert(image != NULL);
	assert(block != NULL);

	assert(cbmimage_blockaddress_lba_exists(image, lba));

	block->lba = lba;

	return cbmimage_blockaddress_init_from_lba(image, block);
}

/** @brief \internal advance a block address, going to the next block of this image of in the same track
 * @ingroup blockaddress
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[inout] block
 *    pointer to a valid block address. On termination, it will contain the next block address.
 *
 * @param[in] do_not_advance_in_track
 *    - if 0, the next block of the image is determined. If needed, the block address advances to the next track
 *    - if not 0, the next block of this track is determined. That is, the block address does not advance to the next track
 *
 * @return
 *    - 0 if the block address has been advanced, no error occurred
 *    - 1 if an error occurred, the block address was already
 *       the last one of this image (if do_not_advance_in_track is 0) or of
 *       this track (if do_not_advance_in_track is not 0)
 *
 * @remark
 *   - the block address is updated in the T/S and in the LBA part
 *   - an error occurs if there is no next block as specified
 *   - this is an internal helper for both cbmimage_blockaddress() and cbmimage_blockaddress_advance()
 *   - cf. cbmimage_blockaddress(), cbmimage_blockaddress_advance()
 *
 */
static int
cbmimage_i_blockaddress_advance(
	cbmimage_fileimage *    image,
	cbmimage_blockaddress * block,
	int                     do_not_advance_in_track
	)
{
	assert(image != NULL);
	assert(block != NULL);

	uint16_t track = block->ts.track;
	uint16_t sector = block->ts.sector;
	uint16_t sector_count = cbmimage_get_sectors_in_track(image, track);
	uint16_t maxtrack = cbmimage_get_max_track(image);

	assert(cbmimage_blockaddress_ts_exists(image, track, sector));

	assert(block->lba > 0);

	if (!cbmimage_blockaddress_lba_exists(image, block->lba)) {
		return -1;
	}

	if (image->settings->subdir_relative_addressing) {
		if ((block->lba + image->settings->block_subdir_first.lba - 1) >= cbmimage_get_max_lba(image)) {
			return -1;
		}
	}

	if (++sector >= sector_count) {
		if (do_not_advance_in_track) {
			return -1;
		}
		else {
			sector = 0;
			if (++track > maxtrack) {
				return -1;
			}
		}
	}
	CBMIMAGE_TS_SET(block->ts, track, sector);
	block->lba++;

	assert(cbmimage_blockaddress_ts_exists(image, track, sector));

	assert(cbmimage_blockaddress_lba_exists(image, block->lba));

	return 0;
}

/** @brief advance a block address, going to the next block
 * @ingroup blockaddress
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[inout] block
 *    pointer to a valid block address. On termination, it will contain the next block address.
 *
 * @return
 *    - 0 if the block address has been advanced, no error occurred
 *    - 1 if an error occurred, the block address was already
 *       the last one of the image
 *
 * @remark
 *   - the block address is updated in the T/S and in the LBA part
 *   - an error occurs if there is no next block; that is, the block is
 *     already the last block of the image
 *   - cf. cbmimage_blockaddress_advance_in_track()
 *
 */
int
cbmimage_blockaddress_advance(
	cbmimage_fileimage *    image,
	cbmimage_blockaddress * block
	)
{
	assert(image != NULL);
	assert(block != NULL);

	return cbmimage_i_blockaddress_advance(image, block, 0);
}

/** @brief advance a block address, going to the next block in the same track
 * @ingroup blockaddress
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[inout] block
 *    pointer to a valid block address. On termination, it will contain the next block address.
 *
 * @return
 *    - 0 if the block address has been advanced, no error occurred
 *    - 1 if an error occurred, the block address was already
 *       the last one of this track
 *
 * @remark
 *   - the block address is updated in the T/S and in the LBA part
 *   - an error occurs if there is no next block in this track;
 *   - cf. cbmimage_blockaddress_advance()
 *
 */
int
cbmimage_blockaddress_advance_in_track(
	cbmimage_fileimage *    image,
	cbmimage_blockaddress * block
	)
{
	assert(image != NULL);
	assert(block != NULL);

	return cbmimage_i_blockaddress_advance(image, block, 1);
}

/** @brief add two block address together
 * @ingroup blockaddress
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[inout] blockresult
 *    pointer to a valid block address. On termination, it will contain the sum of the two block addresses.
 *
 * @param[in] block_adder
 *    a valid block address. It is the block to add to the one in *blockresult.
 *
 * @return
 *    - 0 if the add was successsfull.
 *    - 1 if an error occurred
 *
 * @remark
 *    To understand the nature of this function, it is best to think about using a "base" for a block.
 *    The block in *blockresult will be modified as if the block in block_adder was block 1/0 (LBA 1) of this
 *    image. Thus, this function is mostly useful for calculating the addresses of blocks that are part
 *    of a partition on the image.
 */
int
cbmimage_blockaddress_add(
		cbmimage_fileimage *    image,
		cbmimage_blockaddress * blockresult,
		cbmimage_blockaddress   block_adder
		)
{
	if (block_adder.lba == 0) {
	}
	else if (blockresult->lba == 0) {
		*blockresult = block_adder;
	}
	else {
		cbmimage_blockaddress result = cbmimage_block_unused;

		CBMIMAGE_BLOCK_SET_FROM_LBA(image, result, blockresult->lba + block_adder.lba - 1);

		*blockresult = result;
	}

	return 0;
}
