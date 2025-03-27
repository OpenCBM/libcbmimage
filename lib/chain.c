/** @file lib/chain.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: follow a file chain
 *
 * @defgroup cbmimage_chain Chain following functions
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"

#include <assert.h>

/** @brief @internal read the current block of this chain
 * @ingroup cbmimage_chain
 *
 * @param[in] chain
 *    chain structure which contains all relevant info
 *
 * @param[in] block
 *    the block that is to be read
 *
 * @return
 *    - 0 if the block is available and full (that is, it contains another link, so all byte are relevant)
 *    - > 0: this is the last block, the return gives the number of valid byte in this block.
 *      For example, if the link of this last block is (0,x), it will return the value x.
 *    - -1 if an error occurred
 */
static
int
cbmimage_i_chain_readblock(
		cbmimage_chain *      chain,
		cbmimage_blockaddress block
		)
{
	int ret = 0;

	assert(chain != NULL);
	assert(chain->image != NULL);
	assert(chain->loop_detector != NULL);

	/* mark the link to find out if we already have a loop */
	if (cbmimage_loop_mark(chain->loop_detector, block)) {
		// we fell in a loop, we're done
		chain->is_loop = 1;
		chain->is_done = 1;
		ret = -1;
	}

	if (ret == 0) {
		ret = cbmimage_blockaccessor_set_to(chain->block_accessor, block);
	}

	if (ret == 0) {
		ret = cbmimage_blockaccessor_get_next_block(chain->block_accessor, NULL);
	}

	return ret;
}

/** @brief start the chain processing for a file chain
 * @ingroup cbmimage_chain
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] block_start
 *    the starting block, that is, where this chain processing should start
 *
 * @return chain
 *    chain structure which contains all relevant info
 *
 * @remark
 *  - after processing is done, close the chain structure with a call to
 *    cbmimage_chain_close().
 */
cbmimage_chain *
cbmimage_chain_start(
		cbmimage_fileimage *  image,
		cbmimage_blockaddress block_start
		)
{
	assert(image != NULL);

	cbmimage_chain * chain = NULL;

	uint16_t buffersize = cbmimage_get_bytes_in_block(image);

	chain = cbmimage_i_xalloc(sizeof *chain + buffersize);

	assert(chain);

	if (chain) {
		chain->image = image;
		chain->block_start = block_start;
		chain->block_accessor = cbmimage_blockaccessor_create(image, block_start);

		chain->loop_detector = cbmimage_loop_create(image);

		int ret = cbmimage_i_chain_readblock(chain, block_start);
	}

	return chain;
}

/** @brief close the chain structure
 * @ingroup cbmimage_chain
 *
 * @param[in] chain
 *    chain structure which contains all relevant info
 *
 * @remark
 *  - Closes the chain structure that was obtained by cbmimage_chain_start()
 *  after processing is done
 */
void
cbmimage_chain_close(
		cbmimage_chain * chain
		)
{
	if (chain) {
		/* intentionally do *not* close the global loop detector! This is the
		 * reponsibility of the caller.
		 */

		cbmimage_loop_close(chain->loop_detector);
		cbmimage_blockaccessor_close(chain->block_accessor);
		cbmimage_i_xfree(chain);
	}
}

/** @brief advance to the next block of this chain
 * @ingroup cbmimage_chain
 *
 * @param[in] chain
 *    chain structure which contains all relevant info
 *
 * @return
 *    - 0 if the block is available and full (that is, it contains another link, so all byte are relevant)
 *    - > 0: this is the last block, the return gives the number of valid byte in this block.
 *      For example, if the link of this last block is (0,x), it will return the value x.
 *    - -1 if an error occurred
 *
 * @remark
 *   If this is already the last block, then the return value is the same as
 *   with the last call to cbmimage_chain_advance() or cbmimage_chain_start(),
 *   and nothing is done at all.
 */
int
cbmimage_chain_advance(
		cbmimage_chain * chain
		)
{
	assert(chain != NULL);
	assert(chain->image != NULL);

	cbmimage_blockaddress block_next;
	int ret = cbmimage_blockaccessor_get_next_block(chain->block_accessor, &block_next);

	if (ret == 0) {
		// go to next block
		ret = cbmimage_i_chain_readblock(chain, block_next);
	}
	else {
		chain->is_done = 1;
	}

	return ret;
}

/** @brief check if this chain has been processed competely
 * @ingroup cbmimage_chain
 *
 * @param[in] chain
 *    chain structure which contains all relevant info
 *
 * @return
 *    1 if all blocks have been read already.
 */
int
cbmimage_chain_last_result(
		cbmimage_chain * chain
		)
{
	assert(chain != NULL);
	assert(chain->image != NULL);

	return cbmimage_blockaccessor_get_next_block(chain->block_accessor, NULL);
}

/** @brief check if this chain has been processed competely
 * @ingroup cbmimage_chain
 *
 * @param[in] chain
 *    chain structure which contains all relevant info
 *
 * @return
 *   - 1 if all blocks of the the chain have already been read
 *   - 0 otherwise
 */
int
cbmimage_chain_is_done(
		cbmimage_chain * chain
		)
{
	assert(chain != NULL);
	assert(chain->image != NULL);

	return chain->is_done;
}

/** @brief check if this chain has fallen into a loop
 * @ingroup cbmimage_chain
 *
 * @param[in] chain
 *    chain structure which contains all relevant info
 *
 * @return
 *   - 1 if the chain has fallen into a loop
 *   - 0 otherwise
 */
int
cbmimage_chain_is_loop(
		cbmimage_chain * chain
		)
{
	assert(chain != NULL);
	assert(chain->image != NULL);

	return chain->is_loop;
}

/** @brief get the address of the current block in this chain
 * @ingroup cbmimage_chain
 *
 * @param[in] chain
 *    chain structure which contains all relevant info
 *
 * @return
 *   the block address of the current block in this chain.
 */
cbmimage_blockaddress
cbmimage_chain_get_current(
		cbmimage_chain * chain
		)
{
	assert(chain != NULL);
	assert(chain->image != NULL);

	return chain->block_accessor->block;
}

/** @brief get the address of the next block in this chain
 * @ingroup cbmimage_chain
 *
 * @param[in] chain
 *    chain structure which contains all relevant info
 *
 * @return
 *   the block address of the current block in this chain.
 *   If there is no next block, then the block has lba = 0.
 */
cbmimage_blockaddress
cbmimage_chain_get_next(
		cbmimage_chain * chain
		)
{
	assert(chain != NULL);
	assert(chain->image != NULL);

	cbmimage_blockaddress block_next = cbmimage_block_unused;

	cbmimage_blockaccessor_get_next_block(chain->block_accessor, &block_next);
	return block_next;
}

/** @brief get a pointer to the data of the current block in this chain
 * @ingroup cbmimage_chain
 *
 * @param[in] chain
 *    chain structure which contains all relevant info
 *
 * @return
 *   pointer to the data of this block
 */
uint8_t *
cbmimage_chain_get_data(
		cbmimage_chain * chain
		)
{
	assert(chain != NULL);
	assert(chain->image != NULL);

	assert(chain->block_accessor != NULL);
	assert(chain->block_accessor->data != NULL);
	return chain->block_accessor->data;
}
