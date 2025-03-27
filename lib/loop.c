/** @file lib/loop.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: loop detector functions
 *
 * @defgroup cbmimage_loop Loop detector specific functions
 */
#include "cbmimage/internal.h"

#include "cbmimage/alloc.h"

#include <assert.h>

/** @brief create a loop detector data structure
 * @ingroup cbmimage_loop
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @return
 *    - pointer to an initialized cbmimage_loop structure in case of success
 *    - NULL on failure
 *
 * @remark
 *   - after usage, the loop detector must be freed with a call to cbmimage_loop_close().
 */
cbmimage_loop *
cbmimage_loop_create(
		cbmimage_fileimage * image
		)
{
	assert(image != NULL);

	uint16_t count_of_blocks = cbmimage_get_max_lba(image) + 1;

	// get the number of bytes needed to hold the structure
	size_t additional_data = (count_of_blocks + 7) / 8;

	cbmimage_loop * loop = cbmimage_i_xalloc(sizeof *loop + additional_data);

	if (loop) {
		loop->image = image;
		loop->map_size = additional_data;
		loop->map = &loop->bufferarray[0];
	}

	return loop;
}

/** @brief free a loop detector data structure
 * @ingroup cbmimage_loop
 *
 * @param[in] loop
 *    pointer to the loop detector data
 *
 * @remark
 *   - don't access the structure at *loop after this function has been called.
 */
void
cbmimage_loop_close(
		cbmimage_loop * loop
		)
{
	assert(loop != NULL);

	cbmimage_i_xfree(loop);
}

/** @brief mark a block as used/visited
 * @ingroup cbmimage_loop
 *
 * @param[in] loop
 *    pointer to the loop detector data
 *
 * @param[in] block
 *    the block to mark as used/visited
 *
 * @return
 *    - 1 if this block has already been used/visited/marked
 *    - 0 otherwise
 *    - -1 if an error occurred
 */
int
cbmimage_loop_mark(
		cbmimage_loop *       loop,
		cbmimage_blockaddress block
		)
{
	assert(loop != NULL);
	assert(loop->map != NULL);

	assert(block.lba > 0);
	assert(block.lba <= cbmimage_get_max_lba(loop->image));

	if (block.lba == 0 || block.lba > cbmimage_get_max_lba(loop->image)) {
		return -1;
	}

	uint16_t byte_of_block = block.lba / 8;
	uint8_t bit_of_block = block.lba % 8;

	uint8_t * map_typed = loop->map;

	assert(byte_of_block < loop->map_size);

	if (byte_of_block >= loop->map_size) {
		return -1;
	}

	int is_marked = map_typed[byte_of_block] & (1 << bit_of_block) ? 1 : 0;

	map_typed[byte_of_block] |= (1 << bit_of_block);

	if (is_marked) {
		cbmimage_i_fmt_print("Loop detected marking block %u/%u = %u.\n", block.ts.track, block.ts.sector, block.lba);
	}

	return is_marked;
}

/** @brief check if a block is marked as used/visited
 * @ingroup cbmimage_loop
 *
 * @param[in] loop
 *    pointer to the loop detector data
 *
 * @param[in] block
 *    the block to mark as used/visited
 *
 * @return
 *    - 1 if this block has already been used/visited/marked
 *    - 0 otherwise
 *    - -1 if an error occurred
 */
int
cbmimage_loop_check(
		cbmimage_loop *       loop,
		cbmimage_blockaddress block
		)
{
	assert(loop != NULL);
	assert(loop->map != NULL);

	assert(block.lba > 0);
	assert(block.lba <= cbmimage_get_max_lba(loop->image));

	if (block.lba == 0 || block.lba > cbmimage_get_max_lba(loop->image)) {
		return -1;
	}

	uint16_t byte_of_block = block.lba / 8;
	uint8_t bit_of_block = block.lba % 8;

	uint8_t * map_typed = loop->map;

	assert(byte_of_block < loop->map_size);

	if (byte_of_block >= loop->map_size) {
		return -1;
	}

	int is_marked = map_typed[byte_of_block] & (1 << bit_of_block) ? 1 : 0;

	return is_marked;
}
