/** @file lib/bam.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: BAM processing functions
 *
 * @defgroup cbmimage_bam BAM processing functions
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"
#include "cbmimage/helper.h"

#include <assert.h>

enum { BAM_MASK_COUNT = 0x20 };

/** @brief contains the BAM of a specific track */
typedef
struct {
	/// @brief the mask of this BAM
	uint8_t mask[BAM_MASK_COUNT];
} bam_mask_t;

/** @brief @internal reverse the bit order in the byte
 * @ingroup cbmimage_bam
 *
 * @param[in] input
 *    byte value of which the bit order should be reversed
 *
 * @return
 *    the byte value with the reversed bits
 *
 * @remark
 *   - An extremly simple algorithm is used, just shifting out
 *     a bit from the input and shifting it in into the output
 *     in reverse order.
 *
 * @todo reversing can be optimized, for example by using a lookup table
 *
 */
static
uint8_t
reverse_bit_order(
		uint8_t input
		)
{
	uint8_t output = 0;

	for (int j = 0; j < 8; ++j) {
		output <<= 1;
		output |= (input & 1);
		input >>= 1;
	}
	return output;
}

/** @brief @internal count the number of "1" bits
 * @ingroup cbmimage_bam
 *
 * @param[in] value
 *    values of which to count the number of "1" bits
 *
 * @return
 *    The number of "1" bits in value
 *
 * @remark
 *   - An extremly simple algorithm is used, just counting the
 *     bits in a loop.
 */
static
int
cbmimage_i_countbits(
		bam_mask_t value
		)
{
	int result = 0;

	for (int i = 0; i < BAM_MASK_COUNT; ++i) {
		uint8_t bam_byte = value.mask[i];

		for (int j = 0; j < 8; ++j) {
			result += (bam_byte & 1) ? 1 : 0;
			bam_byte >>= 1;
		}
	}

	return result;
}


/** @brief @internal init a BAM selector
 * @ingroup cbmimage_bam
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @param[inout] selector
 *    array of the selectors to initialize
 *
 * @param[in] selector_count
 *    number of elements in the selector array
 *
 * @remark
 *   - The LBA part of the block address is initialized from the T/S values
 *   - The buffer pointer is set to the right location inside of the image
 */
void
cbmimage_i_init_bam_selector(
		cbmimage_image_settings * settings,
		cbmimage_i_bam_selector * selector,
		size_t                    selector_count
		)
{
	for (int i = 0; i < selector_count; ++i) {
		cbmimage_blockaddress_init_from_ts(settings->image, &selector[i].block);
		selector[i].buffer = cbmimage_i_get_address_of_block(settings->image, selector[i].block);
	}
}

/** @brief @internal get the BAM selector that matches a specific track
 * @ingroup cbmimage_bam
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @param[in] selector
 *    array of the selectors
 *
 * @param[in] selector_count
 *    number of elements in the selector array
 *
 * @param[in] track
 *    the track for which to find the BAM selector
 *
 * @return
 *    - The index of the selector to which the track belongs
 *    - If an error occurs (track does not exist), the return value is -1
 */
static int
cbmimage_i_get_right_selector(
		cbmimage_image_settings * settings,
		cbmimage_i_bam_selector * selector,
		size_t                    selector_count,
		uint8_t                   track
		)
{
	assert(track > 0);
	assert(track <= settings->maxtracks);

	if (track < 1 || track > settings->maxtracks) {
		return -1;
	}

	int selector_number;

	for (selector_number = 0; selector_number < selector_count - 1; ++selector_number) {
		if (track < selector[selector_number + 1].starttrack) {
			break;
		}
	}

	return selector_number;
}

/** @brief @internal output a BAM bitmap
 * @ingroup cbmimage_bam
 *
 * @param[in] mask
 *    the BAM map to output
 *
 */
static
void
cbmimage_i_bam_print(
		bam_mask_t mask
		)
{
	int i = BAM_MASK_COUNT - 1;
	while ((i > 0) && (mask.mask[i] == 0)) {
		--i;
	}

	for (i /* unchanged */ ; i >= 0; --i) {
		cbmimage_i_fmt_print("%02X", mask.mask[i]);
		if ((i % 4) == 0) {
			cbmimage_i_print(" ");
		}
	}
}

/** @brief @internal check the BAM bitmap of a specific track
 * @ingroup cbmimage_bam
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @param[in] track
 *    the track for which to find the BAM bitmap
 *
 * @param[in] mask
 *    the BAM map to check
 *
 * @return
 *    - 0 on success
 *    - -1 on error
 *
 * @remark
 *   The BAM bitmap is invalid if
 *   - the track does not exist, or
 *   - sectors that do not exist are marked are available
 *
 */
static
int
cbmimage_i_check_max_bam_of_track(
		cbmimage_image_settings * settings,
		uint16_t                  track,
		bam_mask_t                mask
		)
{
	if (track > cbmimage_get_max_track(settings->image)) {
		cbmimage_i_fmt_print("Track %u: invalid.\n", track);
		return -1;
	}
	uint16_t sectors_on_track = cbmimage_get_sectors_in_track(settings->image, track);
	uint16_t remaining_sectors_on_track = sectors_on_track;

	for (int i = 0; i < BAM_MASK_COUNT; ++i) {
		if (remaining_sectors_on_track >= 8) {
			remaining_sectors_on_track -= 8;
		}
		else if (remaining_sectors_on_track == 0) {
			if (mask.mask[i] != 0) {
				cbmimage_i_fmt_print("Track %u: Bits marked which are not allowed, no. of sectors is %u.\n",
						track, sectors_on_track);
				cbmimage_i_bam_print(mask);
				cbmimage_i_fmt_print("\n");
				return -1;
			}
		}
		else {
			uint8_t local_mask = 0xFFu << remaining_sectors_on_track;
			if ((mask.mask[i] & local_mask) != 0) {
				cbmimage_i_fmt_print("Track %u: Bits marked which are not allowed, no. of sectors is %u.\n",
						track, sectors_on_track);
				cbmimage_i_bam_print(mask);
				cbmimage_i_fmt_print("\n");
				return -1;
			}
		}
	}

	return 0;
}


/** @brief @internal get the BAM bitmap of a specific track
 * @ingroup cbmimage_bam
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @param[in] track
 *    the track for which to find the BAM bitmap
 *
 * @param[inout] mask
 *    pointer to a bam_mask_t that will contain the BAM map on exit
 *
 * @return
 *    - 0 on success
 *    - -1 on error
 *
 */
static
int
cbmimage_i_get_bam_of_track(
		cbmimage_image_settings * settings,
		uint8_t                   track,
		bam_mask_t *              mask
		)
{
	if (settings->bam_count == 0) {
		return -1;
	}

	int selector_number = cbmimage_i_get_right_selector(settings, settings->bam, settings->bam_count, track);

	assert(selector_number >= 0);

	if (selector_number < 0) {
		return -1;
	}

	uint8_t offset_bam = settings->bam[selector_number].startoffset + (track - settings->bam[selector_number].starttrack) * settings->bam[selector_number].multiplier; // offset to where the BAM is located in the block

	assert(settings->bam[selector_number].data_count > 0);
	assert(settings->bam[selector_number].data_count <= BAM_MASK_COUNT);

	for (int i = settings->bam[selector_number].data_count; i < BAM_MASK_COUNT; ++i) {
		mask->mask[i] = 0;
	}

	for (int i = settings->bam[selector_number].data_count - 1; i >= 0; --i) {
		uint8_t bam_byte = settings->bam[selector_number].buffer[offset_bam + i];
		if (settings->bam[selector_number].reverse_order) {
			mask->mask[i] = reverse_bit_order(bam_byte);
		}
		else {
			mask->mask[i] = bam_byte;
		}
	}

	return 0;
}

/** @brief @internal get the BAM counter of a specific track
 * @ingroup cbmimage_bam
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @param[in] track
 *    the track for which to find the BAM map
 *
 * @return
 *    - the number of free blocks on this track according to the BAM
 *
 * @remark
 *    - If an error occurs (track does not exist), the return value is 0.
 *      Note that this cannot be distinguished from an empty BAM map,
 *      so be careful that you provide a valid track.
 */
static
uint16_t
cbmimage_i_get_bam_counter_of_track(
		cbmimage_image_settings * settings,
		uint8_t                   track
		)
{
	if (settings->bam_counter == NULL) {
		// there is no specific BAM counter present (DNP images).
		// Because of this, calculate it on my own by adding
		// the free entries in the BAM for this track

		bam_mask_t bam_mask;
		if (cbmimage_i_get_bam_of_track(settings, track, &bam_mask) < 0) {
			return 0;
		}
		return cbmimage_i_countbits(bam_mask);
	}

	if (settings->bam_count == 0) {
		return 0;
	}
	else {
		int selector_number = cbmimage_i_get_right_selector(settings, settings->bam_counter, settings->bam_count, track);

		assert(selector_number >= 0);

		if (selector_number < 0) {
			return 0;
		}

		uint8_t offset_bam = settings->bam_counter[selector_number].startoffset + (track - settings->bam_counter[selector_number].starttrack) * settings->bam_counter[selector_number].multiplier; // offset to where the BAM is located in the block

		assert(settings->bam_counter[selector_number].data_count == 0);

		return settings->bam_counter[selector_number].buffer[offset_bam];
	}
}

/** @brief @internal check if a block is really unused, that is, it's value is
 * the same as after formatting
 *
 * @ingroup cbmimage_bam
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @param[in] block
 *    the address of the block to check
 *
 * @return
 *    - 0 if the block is used
 *    - != 0 otherwise
 *
 * @remark
 *    - If the block does not exist, the result is unspecified.
 *      Thus, be careful that you provide a valid track.
 *
 *    - It is tested if the block is all "0" (0x00, 0x00, 0x00, ..., 0x00)
 *      OR if it is all but the first value "1" (0x??, 0x01, 0x01, ..., 0x01)
 *      The second scheme is used in the 1541 for no apparant reason. The 0x??
 *      is often 0x4B (the result of the GCR conversion), but not on the first
 *      track!
 */
int
cbmimage_i_bam_check_really_unused(
		cbmimage_image_settings * settings,
		cbmimage_blockaddress     block
		)
{
	assert(settings != NULL);
	uint8_t * buffer = cbmimage_i_get_address_of_block(settings->image, block);

	assert(buffer != NULL);

	if (buffer == NULL) {
		return 0;
	}

	uint16_t bytes_in_block = cbmimage_get_bytes_in_block(settings->image);

	if (buffer[2] == 1) {
		// empty (1541 and above): 0x??, 0x01, 0x01, ..., with 0x?? often being 0x4B
		for (int i = 1; i < bytes_in_block; ++i) {
			if (buffer[i] != 1) {
				return 0;
			}
		}
		return 1;
	}
	else if (buffer[2] == 0) {
		// empty (1541 and above): all 0x00
		for (int i = 0; i < bytes_in_block; ++i) {
			if (buffer[i] != 0) {
				return 0;
			}
		}
		return 1;
	}

	return 0;
}

/** @brief get the unused/used state of a block in the BAM
 *
 * @ingroup cbmimage_bam
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] block
 *    the address of the block to check
 *
 * @return
 *    - the BAM state (BAM_REALLY_FREE, BAM_USED, BAM_FREE).
 *
 * @remark
 *    - If the block does not exist, the result is unspecified.
 *      Thus, be careful that you provide a valid track.
 *
 */
cbmimage_BAM_state
cbmimage_bam_get(
		cbmimage_fileimage *   image,
		cbmimage_blockaddress  block
		)
{
	assert(image);
	assert(image->settings);

	/// @todo: optimize. Getting the whole track just to extract one byte
	/// is a little bit of overhead. This can be optimized
	bam_mask_t bam_of_track;
	if (cbmimage_i_get_bam_of_track(image->settings, block.ts.track, &bam_of_track) < 0) {
		return -1;
	}

	unsigned int index = block.ts.sector / 8;
	unsigned int bitpos = block.ts.sector % 8;

	cbmimage_BAM_state bam_state = (bam_of_track.mask[index] & (1ull << bitpos)) ? BAM_FREE : BAM_USED;

	if (bam_state == BAM_FREE && cbmimage_i_bam_check_really_unused(image->settings, block)) {
		bam_state = BAM_REALLY_FREE;
	}

	return bam_state;
}

/** @brief check the consistency of a BAM
 * @ingroup cbmimage_bam
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
 *   - no track has free blocks for sectors that do not exist
 *   - the free block count of a track is the same as the number of free sectors \n
 *     (i.e., the number fits the bitmap)
 *
 * @todo How should details be given to the caller?
 *
 */
int
cbmimage_bam_check_consistency(
		cbmimage_fileimage * image
		)
{
	assert(image != NULL);
	cbmimage_image_settings * settings = image->settings;
	assert(settings != NULL);

	uint16_t maxtrack = cbmimage_get_max_track(image);

	for (uint16_t track = 1; track <= maxtrack; ++track) {

		bam_mask_t bam_mask;
		if (cbmimage_i_get_bam_of_track(settings, track, &bam_mask) < 0) {
			return -1;
		}
		uint16_t bam_counter_of_track = cbmimage_i_get_bam_counter_of_track(settings, track);
		uint16_t sectors_on_track = cbmimage_get_sectors_in_track(settings->image, track);

		cbmimage_i_check_max_bam_of_track(settings, track, bam_mask);

		uint16_t count_of_bam_blocks = cbmimage_i_countbits(bam_mask);

		if (bam_counter_of_track > sectors_on_track) {
			cbmimage_i_fmt_print("Track %u: Number of free blocks is reported as %u, but no. of sectors is %u.\n",
					track, bam_counter_of_track, sectors_on_track);
		}
		if (count_of_bam_blocks != bam_counter_of_track) {
			cbmimage_i_fmt_print("Track %u: Reported %u free blocks, but there are %u in %016llX.\n",
					track, bam_counter_of_track, count_of_bam_blocks, bam_mask);
		}
	}

	return 0;
}

/** @brief get the count of blocks free
 * @ingroup cbmimage_bam
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @return
 *    count of free blocks on this image
 *
 */
int
cbmimage_get_blocks_free(
		cbmimage_fileimage * image
		)
{
	uint16_t count = 0;
	uint8_t  dir_track_location = 0;
	uint8_t  maxtrack = cbmimage_get_max_track(image);

	assert(image != NULL);

	cbmimage_image_settings * settings = image->settings;
	assert(settings != NULL);

	for (uint8_t track = 1; track <= maxtrack; ++track) {

		// do not count directory tracks
		if (track == settings->dir_tracks[dir_track_location]) {
			// advance to next track location, ensuring that we do not move beyond the last one
			dir_track_location = (++dir_track_location) % CBMIMAGE_ARRAYSIZE(settings->dir_tracks);
			continue;
		}
		count += cbmimage_i_get_bam_counter_of_track(settings, track);
	}
	return count;
}

/** @brief get the blocks free on a specific track
 * @ingroup cbmimage_bam
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] track
 *    the track number for which the number of tracks is requested
 *
 * @return
 *    count of free blocks on this track
 *
 */
int
cbmimage_bam_get_free_on_track(
		cbmimage_fileimage * image,
		uint8_t              track
		)
{
	assert(image != NULL);

	cbmimage_image_settings * settings = image->settings;
	assert(settings != NULL);

	return cbmimage_i_get_bam_counter_of_track(settings, track);
}
