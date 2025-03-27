/** @file lib/readwriteblock.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: read and write blocks into/from an image
 *
 * @defgroup blockaccess block access functions
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"

#include <assert.h>
#include <string.h>

/** @brief @internal adjust a relative address to a global address
 * @ingroup blockaccess
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] block
 *    pointer to the block address for the block that is of interest
 *
 * @return
 *    - 0 if the block lies in the current subdir
 *    - -1 if an error occurred
 */
static
int
cbmimage_i_adjust_relative_address(
		cbmimage_fileimage *    image,
		cbmimage_blockaddress * block
		)
{
	assert(image);
	assert(block);

	cbmimage_image_settings * settings = image->settings;
	assert(settings);

	if (settings->subdir_relative_addressing) {
		/* we have a subdir, and it's addressing is performed with
		 * relative addressing: fix the address so we have the global address
		 */

		cbmimage_blockaddress block_old = *block;

		assert(block->lba < settings->block_subdir_last.lba);
		if (block->lba >= settings->block_subdir_last.lba) {
			return -1;
		}

		block->lba += settings->block_subdir_first.lba - 1;
		cbmimage_blockaddress_init_from_lba(image, block);

#if 0
		cbmimage_i_fmt_print("Converting relative address %u/%u(%03X) to absolute address %u/%u(%03X).\n",
				block_old.ts.track, block_old.ts.sector, block_old.lba,
				block->ts.track, block->ts.sector, block->lba
				);
#endif
	}

	return 0;
}


/** @brief @internal get the address of a block
 * @ingroup blockaccess
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] block
 *    pointer to the block address for the block that is of interest
 *
 * @return
 *    pointer to the beginning of the block inside of the image memory
 *
 * @remark
 *    - This function is for internal purposes only. \n
 *      Do not access from outside of the library, as it is subject to change!
 *
 * @bug
 *    - There is no error checking on the provided block. \n
 *      That is, if the block does not exist, this function will happily provide
 *      an undefined pointer!
 */
uint8_t *
cbmimage_i_get_address_of_block(
		cbmimage_fileimage *  image,
		cbmimage_blockaddress block
		)
{
	uint8_t * ret = NULL;

	assert(image != NULL);
	cbmimage_image_parameter * parameter = image->parameter;

	assert(parameter != NULL);
	assert(block.lba > 0);
	assert(block.ts.track > 0);

	uint16_t bytes_in_block = cbmimage_get_bytes_in_block(image);

	if (cbmimage_i_adjust_relative_address(image, &block)) {
		return NULL;
	}

	if (block.lba > 0 && block.lba <= cbmimage_get_max_lba(image)) {
		size_t buffer_offset = (block.lba - 1) * bytes_in_block;
		size_t size          = parameter->size;

		assert(buffer_offset <= size - bytes_in_block);

		if (buffer_offset <= size - bytes_in_block) {
			ret = &parameter->buffer[buffer_offset + image->settings->subdir_data_offset];
		}
	}
	return ret;
}

/** @brief read a block from the image and copy it into the provided buffer
 * @ingroup blockaccess
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] block
 *    pointer to the block address for the block that is of interest
 *
 * @param[out] buffer
 *    pointer to the buffer that will contain the block contents on termination
 *
 * @param[in] buffersize
 *    the size of the buffer pointed to by the parameter buffer
 *
 * @return
 *    - 0 if the block is available and full (that is, it contains another link, so all byte are relevant)
 *    - > 0: this is the last block, the return gives the number of valid byte in this block.
 *      For example, if the link of this last block is (0,x), it will return the value x.
 *    - -1 if an error occurred
 *
 * @remark
 *    - If the buffersize is not big enough, that is, it is not at least
 *      cbmimage_get_bytes_in_block() byte long, this function does nothing.
 *      It does not even provide a partial copy of the data in this case!
 *
 */
int
cbmimage_read_block(
		cbmimage_fileimage *  image,
		cbmimage_blockaddress block,
		void *                buffer,
		size_t                buffersize
		)
{
	int ret = -1;

	assert(image);

	cbmimage_image_settings * settings = image->settings;
	assert(settings);

	uint16_t bytes_in_block = cbmimage_get_bytes_in_block(image);

	assert(buffersize >= bytes_in_block);
	assert(buffer);

	uint8_t * block_in_buffer_to_copy = cbmimage_i_get_address_of_block(image, block);

	if (buffer && buffersize >= bytes_in_block && block_in_buffer_to_copy) {
		memcpy(buffer, block_in_buffer_to_copy, bytes_in_block);

		if (block_in_buffer_to_copy[0] == 0) {
			ret = block_in_buffer_to_copy[1];
		}
		else {
			ret = 0;
		}
	}

	return ret;
}

/** @brief write a block to the image by copying it from the provided buffer
 * @ingroup blockaccess
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] block
 *    pointer to the block address for the block that is of interest
 *
 * @param[in] buffer
 *    pointer to the buffer that has the contents that are to be written into
 *    the image
 *
 * @param[in] buffersize
 *    the size of the buffer pointed to by the parameter buffer
 *
 * @return
 *    - 0 if the block is available
 *    - -1 if an error occurred
 *
 * @remark
 *    - If the buffersize is not big enough, that is, it is not at least
 *      cbmimage_get_bytes_in_block() byte long, this function does nothing.
 *      It does not even partially copy the data in this case!
 *
 */
int
cbmimage_write_block(
		cbmimage_fileimage *  image,
		cbmimage_blockaddress block,
		void *                buffer,
		size_t                buffersize
		)
{
	int ret = -1;

	assert(image);

	cbmimage_image_settings * settings = image->settings;
	assert(settings);

	uint16_t bytes_in_block = cbmimage_get_bytes_in_block(image);

	assert(buffersize >= bytes_in_block);
	assert(buffer);

	uint8_t * block_in_buffer_to_copy = cbmimage_i_get_address_of_block(image, block);

	if (buffer && buffersize >= bytes_in_block && block_in_buffer_to_copy) {
		memcpy(block_in_buffer_to_copy, buffer, bytes_in_block);
		ret = 0;
	}

	return ret;
}


/** @brief read the next block from the image and copy it into the provided buffer
 * @ingroup blockaccess
 *
 * The next block is the next one according to the chain at the beginning of
 * the block.
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[out] block
 *    pointer to the block address that will get the block address
 *    that has been read on termination
 *
 * @param[inout] buffer
 *    pointer to the buffer that has the link address, and that will contain
 *    the contents of the new block on termination
 *
 * @param[in] buffersize
 *    the size of the buffer pointed to by the parameter buffer
 *
 * @return
 *    - 0 if the next block is available and full (that is, it contains another link, so all byte are relevant)
 *    - > 0: this is the last block, the return gives the number of valid byte in this block.
 *      For example, if the link of this last block is (0,x), it will return the value x.
 *    - -1 if an error occurred, for example, there is no next block
 *
 * @remark
 *    - If the buffersize is not big enough, that is, it is not at least
 *      cbmimage_get_bytes_in_block() byte long, this function does nothing.
 *      It does not even provide a partial copy of the data in this case!
 *    - If you want to read all the blocks of a file, call this function
 *      repeatedly until you get a -1 as return value.
 *
 * @bug
 *    - There is no error checking on the provided link in the block. \n
 *      That is, if the linked to block does not exist, this function will
 *      happily provide an undefined pointer!
 */
int
cbmimage_read_next_block(
		cbmimage_fileimage *    image,
		cbmimage_blockaddress * block,
		void *                  buffer,
		size_t                  buffersize
		)
{
	uint8_t * buffer_ui8 = buffer;
	int ret = 0;

	cbmimage_blockaddress block_next = CBMIMAGE_BLOCK_INIT_FROM_TS(buffer_ui8[0], buffer_ui8[1]);

	if (block_next.ts.track == 0) {
		return -1;
	}

	if (cbmimage_blockaddress_init_from_ts(image, &block_next)) {
		return buffer_ui8[1];
	}

	ret = cbmimage_read_block(image, block_next, buffer, buffersize);
	*block = block_next;
	return ret;
}
