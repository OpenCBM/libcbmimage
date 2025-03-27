/** @file lib/file.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: file handling functions
 *
 * @defgroup cbmimage_file File handling functions
 */
#include "cbmimage/internal.h"

#include "cbmimage/alloc.h"
#include "cbmimage/helper.h"

#include <assert.h>
#include <string.h>

/** @brief open a file on the cbmimage where the name is known
 * @ingroup cbmimage_file
 *
 * @param[in] filename
 *    pointer to the file name of the file to open on the image.
 *
 * @return
 *    - pointer to a cbmimage_file object than can be used to access the file
 *    - NULL if an error occurred
 *
 * @remark
 *    - After the processing of the file has finished, the cbmimage_file must
 *      be closed with a call to cbmimage_file_close()
 *    - If the file has been already enumated in the directory,
 *      cbmimage_file_open_by_dir_entry() will be the better choice.
 */
cbmimage_file *
cbmimage_file_open_by_name(
		const char * filename
		)
{
	/// @todo implement cbmimage_file_open_by_name()
	/// @bug cbmimage_file_open_by_name() not yet implemented
	return NULL;
}

/** @brief open a file on the cbmimage that has already een enumerated
 * @ingroup cbmimage_file
 *
 * @param[in] dir_entry
 *    ptr to a directory entry for the file that should be opened. \n
 *    That dir_entry must have been the result of a previous
 *    cbmimage_dir_get_first() or cbmimage_dir_get_next() call. \n
 *
 * @return
 *    - pointer to a cbmimage_file object than can be used to access the file
 *    - NULL if an error occurred
 *
 * @remark
 *    - After the processing of the file has finished, the cbmimage_file must
 *      be closed with a call to cbmimage_file_close()
 *    - If the file name is known but the directory entry has not been
 *      enumerated, cbmimage_file_open_by_name() can be the better choice.
 */
cbmimage_file *
cbmimage_file_open_by_dir_entry(
		cbmimage_dir_entry * dir_entry
		)
{
	assert(dir_entry != NULL);

	cbmimage_i_dir_entry_internal * dei_original = (void*) dir_entry;
	assert(dei_original != NULL);

	cbmimage_file * file = cbmimage_i_xalloc(sizeof *file + cbmimage_get_bytes_in_block(dei_original->image));

	if (file) {
		file->dir_entry = cbmimage_i_dir_get_clone(dir_entry);

		if (file->dir_entry) {
			cbmimage_i_dir_entry_internal * dei = (void*) file->dir_entry;
			file->image = dei->image;

			file->bytes_in_block = cbmimage_get_bytes_in_block(file->image);

			file->loop_detector = cbmimage_loop_create(file->image);
			file->chain = cbmimage_chain_start(dei->image, dei->entry.start_block);
			int ret = cbmimage_chain_last_result(file->chain);
			// ret < 0: an error occurred
			// ret = 1: the number of used bytes in the block is 1. This does not
			//          make sense, because we need at least 2 byte for the chain.
			//          Thus, assume this to be an error, too.
			if (ret == 0) {
				file->block_current_offset = 2;
				file->block_current_remain = 0x100 - file->block_current_offset;
			}
			else if (ret > 1) {
				file->block_current_offset = 2;
				file->block_current_remain = ret - file->block_current_offset + 1;
			}
		}
	}

	return file;
}

/** @brief close a file
 * @ingroup cbmimage_file
 *
 * @param[in] file
 *    pointer to a cbmimage_file that was obtained by a previous
 *    cbmimage_file_open_by_name() or cbmimage_file_open_by_dir_entry() call.
 *
 * @remark
 *    - Call this function after the proceessing of the file has finished.
 */
void
cbmimage_file_close(
		cbmimage_file * file
		)
{
	if (file) {
		cbmimage_dir_get_close(file->dir_entry);

		cbmimage_loop_close(file->loop_detector);

		cbmimage_chain_close(file->chain);

		cbmimage_i_xfree(file);
	}
}

/** @brief read a block from the file
 * @ingroup cbmimage_file
 *
 * @param[in] file
 *    pointer to a cbmimage_file that was obtained by a previous
 *    cbmimage_file_open_by_name() or cbmimage_file_open_by_dir_entry() call.
 *
 * @param[inout] buffer
 *    the buffer where the file contents are written to
 *
 * @param[in] buffer_size
 *    the size of the buffer.
 *
 * @return
 *    - > 0: the number of valid byte in the buffer
 *    - = 0: EOF, no more byte are available
 *    - < 0: an error occurred
 *
 * @remark
 *    - If you want to read in the whole file, call this function repeatedly until
 *      the return value is 0.
 */
int
cbmimage_file_read_next_block(
		cbmimage_file * file,
		uint8_t *       buffer,
		size_t          buffer_size
		)
{
	assert(file != NULL);
	assert(buffer != NULL);

	uint8_t * write_pointer = buffer;
	size_t    write_remaining = buffer_size;

	int bytes_return = 0;

	if (cbmimage_chain_is_done(file->chain)) {
		return -1;
	}

	/*
	 * Outline and vars used:
	 *
	 * buffer_size:   max. number of bytes to read in
	 * bytes_return:  number of bytes actually read in (until now). This is also the offset where to store the next bytes
	 *
	 * file->block_current_remain: no. of remaining (that is, unread) bytes in the current block
	 * file->block_current_offset: offset the the first unread byte in the current block
	 */
	while (write_remaining > 0 && !cbmimage_chain_is_done(file->chain)) {

		size_t bytes_to_copy = min(write_remaining, file->block_current_remain);

		if (bytes_to_copy > 0) {
			memcpy(buffer, &cbmimage_chain_get_data(file->chain)[file->block_current_offset], bytes_to_copy);
			file->block_current_offset += bytes_to_copy;
			file->block_current_remain -= bytes_to_copy;
			buffer          += bytes_to_copy;
			write_remaining -= bytes_to_copy;

			bytes_return    += bytes_to_copy;
		}

		if (write_remaining > 0) {
			assert(file->block_current_remain == 0);


			// there was not enough data, read in the next block
			int ret = cbmimage_chain_advance(file->chain);
			if (ret == 0) {
				file->block_current_offset = 2;
				file->block_current_remain = 0x100 - file->block_current_offset;
			}
			else if (ret == 1) {
				write_remaining = 0;
			}
			else if (ret > 1) {
				file->block_current_offset = 2;
				file->block_current_remain = ret - file->block_current_offset + 1;
			}
			else {
				write_remaining = 0;
			}
		}
	}

	return bytes_return;
}
