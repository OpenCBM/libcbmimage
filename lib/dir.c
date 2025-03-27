/** @file lib/dir.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: Directory processing functions
 *
 * @defgroup cbmimage_dir Directory processing functions
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"

#include "cbmimage/internal/dir.h"

#include <assert.h>
#include <string.h>

/** @brief get the header entry
 * @ingroup cbmimage_dir
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @return
 *    A pointer to the header entry.
 *    If there is no header (partition table), the pointer will be NULL.
 *
 * @remark
 *    - the structure obtained by this function has to be freed by
 *      cbmimage_dir_get_header_close() once the processing is done.
 */
cbmimage_dir_header *
cbmimage_dir_get_header(
		cbmimage_fileimage * image
		)
{
	if (image->settings->is_partition_table) {
		return NULL;
	}

	cbmimage_dir_header * dir_header = cbmimage_i_xalloc(sizeof * dir_header);

	cbmimage_image_settings * settings = image->settings;

	// get the info buffer
	memcpy((char*)dir_header->name.text, &settings->info->data[settings->info_offset_diskname], sizeof dir_header->name.text);

	dir_header->name.length = sizeof dir_header->name.text;
	dir_header->name.end_index = CBMIMAGE_HEADER_ENTRY_NAME_LENGTH;

	dir_header->free_block_count = cbmimage_get_blocks_free(image);

	dir_header->is_geos = image->settings->is_geos;

	return dir_header;
}

/** @brief free the resources from a cbmimage_dir_get_header()
 * @ingroup cbmimage_dir
 *
 * @param[in] header_entry
 *    ptr to a header entry that was the result of a previous call
 *    to cbmimage_dir_get_header().
 *
 * @remark
 *    - This function frees the associated resources to the given pointer.
 *    - After return, it is not allowed anymore to access the data pointed to
 *      by header_entry.
 */
void
cbmimage_dir_get_header_close(
		cbmimage_dir_header * header_entry
		)
{
	cbmimage_i_xfree(header_entry);
}

/** @brief internal store date and time of directory entry
 * @ingroup cbmimage_dir @internal
 *
 * @param[inout] dei
 *    ptr to a *internal* directory entry that will be initialized
 *
 * @return
 *    - 0 on success
 *    - -1 if there are no more directory entries
 *
 * @remark
 *    - If the file has a date and time, the values of the directory entry
 *      are modified to reflect these. \n
 *      If not, the values of the directory entry are cleared.
 *
 */
static
int
cbmimage_i_dir_entry_store_datetime(
		cbmimage_i_dir_entry_internal * dei
		)
{
	assert(dei != NULL);

	if (
			dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_YEAR]
			||
			dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_MONTH]
			||
			dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_DAY]
			||
			dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_HOUR]
			||
			dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_MINUTE]
		 )
	{
		dei->entry.has_datetime = 1;
		uint16_t year = dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_YEAR];
		if (year > 83) {
			year += 1900;
		}
		else {
			year += 2000;
		}
		dei->entry.year   = year;
		dei->entry.month  = dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_MONTH];
		dei->entry.day    = dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_DAY];
		dei->entry.hour   = dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_HOUR];
		dei->entry.minute = dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_MINUTE];
	}
	else {
		dei->entry.has_datetime = 0;
		dei->entry.year   = 0;
		dei->entry.month  = 0;
		dei->entry.day    = 0;
		dei->entry.hour   = 0;
		dei->entry.minute = 0;
	}

	return 0;
}

/** @brief create a directory entry
 * @ingroup cbmimage_dir @internal
 *
 * @param[inout] dei
 *    ptr to a *internal* directory entry that will be initialized
 *
 * @return
 *    - 0 on success
 *    - -1 if there are no more directory entries
 *
 * @remark
 *    - If needed, this function reads in the next block of the directory
 *    - After the call, the pointer to the directory entry is advanced to the
 *      next one
 */
static int
cbmimage_i_dir_get(
		cbmimage_i_dir_entry_internal * dei
		)
{
	if (dei->dir_block_offset >= cbmimage_get_bytes_in_block(dei->image)) {
		// advance to next block
		int v = cbmimage_blockaccessor_follow(dei->dir_block_accessor);

		if (v != 0) {
			return -1;
		}

		dei->dir_block_offset -= cbmimage_get_bytes_in_block(dei->image);
	}

	if (dei->dir_block_offset == 0) {
		if (cbmimage_loop_mark(dei->loop_detector, dei->dir_block_accessor->block)) {
			// we fell in a loop, we're done
			return -1;
		}
	}

	// get the directory data
	uint8_t type = dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_TYPE_OFFSET];

	dei->is_empty = type == 0;

	dei->entry.start_block = cbmimage_block_unused;
	dei->entry.rel_sidesector_block = cbmimage_block_unused;
	dei->entry.rel_recordlength = 0;
	dei->entry.is_geos = 0;
	dei->entry.geos_infoblock = cbmimage_block_unused;
	dei->entry.geos_filetype = GEOS_FILETYPE_NON_GEOS;
	dei->entry.geos_is_vlir = 0;

	if (dei->image->settings->is_partition_table) {
		dei->entry.type = type + DIR_TYPE_PART_OFFSET;
		dei->entry.is_locked = 0;
		dei->entry.is_closed = 1;

		unsigned int lba =
				 dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_PARTITION_START_LOW]
			| (dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_PARTITION_START_HIGH] << 8);

		CBMIMAGE_BLOCK_SET_FROM_LBA(
				dei->image,
				dei->entry.start_block,
				lba * 2 + 1
				);

		dei->entry.block_count =
				 dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_PARTITION_BLOCK_COUNT_LOW]
			| (dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_PARTITION_BLOCK_COUNT_HIGH] << 8);

		dei->entry.block_count *= 2;

		dei->entry.is_geos = 0;
	}
	else {
		dei->entry.type = type & CBMIMAGE_DIR_ENTRY_TYPE_MASK_TYPE;
		dei->entry.is_locked = type & CBMIMAGE_DIR_ENTRY_TYPE_MASK_LOCKED ? 1 : 0;
		dei->entry.is_closed = type & CBMIMAGE_DIR_ENTRY_TYPE_MASK_CLOSED ? 1 : 0;

		CBMIMAGE_BLOCK_SET_FROM_TS(
				dei->image,
				dei->entry.start_block,
				dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_TRACK_OFFSET],  // track
				dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_SECTOR_OFFSET]  // sector
				);

		// check if this is a GEOS file
		if (dei->entry.type < DIR_TYPE_REL) {
			uint8_t geos_filetype = dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_GEOS_FILETYPE];
			uint8_t is_vlir = dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_GEOS_FILESTRUCTURE];

			if (geos_filetype != 0 || is_vlir == 1) {
				dei->entry.is_geos = 1;
				dei->entry.geos_filetype = geos_filetype;
				dei->entry.geos_is_vlir = is_vlir;

				CBMIMAGE_BLOCK_SET_FROM_TS(
						dei->image,
						dei->entry.geos_infoblock,
						dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_GEOS_INFO_TRACK],  // track of GEOS info block
						dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_GEOS_INFO_SECTOR]  // sector of GEOS info block
						);
			}
		}

		if (!dei->entry.is_geos) {
			CBMIMAGE_BLOCK_SET_FROM_TS(
					dei->image,
					dei->entry.rel_sidesector_block,
					dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_SS_TRACK_OFFSET],  // track of side-sector
					dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_SS_SECTOR_OFFSET]  // sector of side-sector
					);

			dei->entry.rel_recordlength = dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_REL_RECORD_LENGTH];
		}

		dei->entry.block_count =
				 dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_BLOCK_COUNT_LOW]
			| (dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_BLOCK_COUNT_HIGH] << 8);

		cbmimage_i_dir_entry_store_datetime(dei);
	}

	memcpy(
			(char*)&dei->entry.name.text[0],
			&dei->dir_block_accessor->data[dei->dir_block_offset + CBMIMAGE_DIR_ENTRY_NAME_OFFSET],
			CBMIMAGE_DIR_ENTRY_NAME_LENGTH
			);

	const char * p = strchr(dei->entry.name.text, CBMIMAGE_DIR_ENTRY_NAME_SHIFTSPACE);
	if (p) {
		dei->entry.name.end_index = p - dei->entry.name.text;
	}
	else {
		dei->entry.name.end_index = 16;
	}
	dei->entry.name.length = CBMIMAGE_DIR_ENTRY_NAME_LENGTH;

	dei->dir_block_offset += CBMIMAGE_DIR_ENTRY_NEXT_ONE;

	return 0;
}

/** @brief get the next non-empty directory entry
 * @ingroup cbmimage_dir @internal
 *
 * @param[inout] dei
 *    ptr to a *internal* directory entry that will be initialized
 *
 * @return
 *    - 0 on success
 *    - -1 if there are no more directory entries
 *
 * @remark
 *    - If needed, this function reads in the next block or blocks of the
 *      directory until it finds a non-empty entry, or the last directory entry
 *      has been checked.
 *    - After the call, the pointer to the directory entry is advanced to the
 *      next one after the one returned.
 */
static int
cbmimage_i_dir_get_nonempty(
		cbmimage_i_dir_entry_internal * dei
		)
{
	int ret;

	do {
		ret = cbmimage_i_dir_get(dei);
		if (ret) {
			dei->entry.is_valid = 0;
			return ret;
		}

		if (
				(dei->entry.type == DIR_TYPE_DEL)
				&& (dei->entry.is_locked == 0)
				&& (dei->entry.is_closed == 0)
				&& (dei->entry.start_block.ts.track == 0)
				&& (dei->entry.name.text[0] == 0)
				) {
			continue;
		}

		break;

	} while (1);

	dei->entry.is_valid = (ret == 0);

	return ret;
}

/** @brief get the first (non-empty) directory entry
 * @ingroup cbmimage_dir
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @return
 *    A pointer to the directory entry. \n
 *    If there is none, it returns 0.
 *
 * @remark
 *    - the structure obtained by this function has to be freed by
 *      cbmimage_dir_get_close() once the processing is done.
 *    - If needed, this function reads in the next block or blocks of the
 *      directory until it finds a non-empty entry, or the last directory entry
 *      has been checked.
 *    - After the call, the pointer to the directory entry is advanced to the
 *      next one after the one returned.
 *    - Subsequent entries can be obtained with a call to cbmimage_dir_get_next().
 *    - If the directory processing is done, the returned pointer and
 *      associated resources must be freed with a call to cbmimage_dir_get_close().
 */
cbmimage_dir_entry *
cbmimage_dir_get_first(
		cbmimage_fileimage * image
		)
{
	cbmimage_i_dir_entry_internal * dei = cbmimage_i_xalloc(sizeof * dei);

	if (!dei) {
		return 0;
	}

	dei->image = image;

	// create a loop detector in order to not fall into a loop
	dei->loop_detector = cbmimage_loop_create(image);

	dei->dir_block_accessor = cbmimage_blockaccessor_create(image, image->settings->dir);
	dei->dir_block_offset = 0;

	int ret = cbmimage_i_dir_get_nonempty(dei);
	return &dei->entry;
}

/** @brief get the next (non-empty) directory entry
 * @ingroup cbmimage_dir
 *
 * @param[inout] dir_entry
 *    ptr to a directory entry that was the result of a previous
 *    cbmimage_dir_get_first() or cbmimage_dir_get_next() call. \n
 *    On return, this dir_entry contains the next directory entry
 *    if the return value is 0.
 *    If the return value is not 0, The result of this is unspecified.
 *
 * @return
 *    - 0 on success
 *    - -1 if there are no more directory entries
 *
 * @remark
 *    - If needed, this function reads in the next block or blocks of the
 *      directory until it finds a non-empty entry, or the last directory entry
 *      has been checked.
 *    - Subsequent entries can be obtained with another call to cbmimage_dir_get_next().
 *    - If the directory processing is done, the returned pointer and
 *      associated resources must be freed with a call to cbmimage_dir_get_close().
 */
int
cbmimage_dir_get_next(
		cbmimage_dir_entry * dir_entry
		)
{
	assert(dir_entry);

	cbmimage_i_dir_entry_internal * dei = (void*) dir_entry;

	assert(dei->image);

	int ret = cbmimage_i_dir_get_nonempty(dei);

	return ret;
}

/** @brief get the next (non-empty) directory entry
 * @ingroup cbmimage_dir
 *
 * @param[in] dir_entry
 *    ptr to a directory entry that was the result of a previous
 *    cbmimage_dir_get_first() or cbmimage_dir_get_next() call. \n
 *    On return, this dir_entry contains the next directory entry
 *    if the return value is 0.
 *    If the return value is not 0, The result of this is unspecified.
 *
 * @return
 *    - 0 on success
 *    - != 0 if there are no more directory entries
 */
int
cbmimage_dir_get_is_valid(
		cbmimage_dir_entry * dir_entry
		)
{
	return dir_entry->is_valid;
}

/** @brief free the resources from a cbmimage_dir_get_first()
 * @ingroup cbmimage_dir
 *
 * @param[in] dir_entry
 *    ptr to a directory entry that was the result of a previous
 *    cbmimage_dir_get_first() or cbmimage_dir_get_next() call.
 *
 * @remark
 *    - This function frees the associated resources to the given pointer.
 *    - After return, it is not allowed anymore to access the data pointed to
 *      by dir_entry.
 */
void
cbmimage_dir_get_close(
		cbmimage_dir_entry * dir_entry
		)
{
	cbmimage_i_dir_entry_internal * dei = (void*) dir_entry;

	if (dei) {
		cbmimage_blockaccessor_close(dei->dir_block_accessor);
		dei->dir_block_accessor = NULL;
		cbmimage_loop_close(dei->loop_detector);
	}

	cbmimage_i_xfree(dei);
}


/** @brief check if the directory entry points to a deleted file
 * @ingroup cbmimage_dir
 *
 * @param[in] dir_entry
 *    pointer to a directory entry that was the result of a previous
 *    cbmimage_dir_get_first() or cbmimage_dir_get_next() call.
 *
 * @return
 *  - 0 if this directory entry is empty
 *  - != 0 otherwise
 *
 * @remark
 *    - An entry is considered empty/deleted if the CBM DOS file type is 0. \n
 *      That is, not only does it have to have DIR_TYPE_DEL, but also is_locked
 *      and is_closed must not be set. \n
 *      This is exactly the same semantincs as applied by CBM DOS.
 */
int
cbmimage_dir_is_deleted(
		cbmimage_dir_entry * dir_entry
		)
{
	cbmimage_i_dir_entry_internal * dei = (void*) dir_entry;
	return dei->is_empty;
}

/** @brief extract the name of a directory entry as a C string
 * @ingroup cbmimage_dir
 *
 * @param[in] dir_name
 *    pointer to the name entry of a cbmimage_dir_entry or cbmimage_header_entry
 *
 * @param[inout] name_buffer
 *    pointer to a block of memory that acts as a buffer for the
 *    name as a C string
 *
 * @param[in] name_buffer_len
 *    the length of the buffer provided by the parameter name.
 *    This must be at least 18 for a regular directory entry,
 *    and at least 26 for the header
 *
 * @return
 *  pointer to the extra text after the name
 *
 * @remark
 *   - Example: A{SHIFT-SPACE},8,1 would be output as "A",8,1 by CBM DOS.
 *     This function would give the string A{NUL},8,1{NUL} in the buffer
 *     pointed to by name, and the return value would point to the ,8,1 part.
 */
char *
cbmimage_dir_extract_name(
		cbmimage_dir_header_name * dir_name,
		char *                     name_buffer,
		size_t                     name_buffer_len
		)
{
	assert(dir_name != NULL);
	assert(name_buffer != NULL);

	char * extra_name = NULL;

	assert(name_buffer_len >= dir_name->length + 2);
	assert(dir_name->length <= sizeof dir_name->text);

	memset(name_buffer, 0, name_buffer_len);

	memcpy(name_buffer, dir_name->text, name_buffer_len);
	name_buffer[dir_name->end_index] = 0;
	name_buffer[dir_name->length] = 0;

	extra_name = &name_buffer[dir_name->end_index + 1];

	for (int i = 0; i < name_buffer_len; ++i) {
		if ((uint8_t)name_buffer[i] == CBMIMAGE_DIR_ENTRY_NAME_SHIFTSPACE) {
			name_buffer[i] = ' ';
		}
	}

	return extra_name;
}

/** @brief clone a directory entry
 * @ingroup cbmimage_dir
 *
 * @param[in] dir_entry
 *    ptr to a directory entry that was the result of a previous
 *    cbmimage_dir_get_first() or cbmimage_dir_get_next() call. \n
 *
 * @return
 *    pointer to a copy of the dir_entry
 *
 * @remark
 *    - the structure obtained by this function has to be freed by
 *      cbmimage_dir_get_close() once the processing is done.
 */
cbmimage_dir_entry *
cbmimage_i_dir_get_clone(
		cbmimage_dir_entry * dir_entry
		)
{
	cbmimage_i_dir_entry_internal * dei_original = (void*) dir_entry;

	assert(dei_original->image);

	cbmimage_i_dir_entry_internal * dei_cloned = cbmimage_i_xalloc_and_copy(sizeof * dei_cloned, dei_original, sizeof *dei_original);

	// create a new loop detector in order to not fall into a loop
	dei_cloned->loop_detector = cbmimage_loop_create(dei_cloned->image);

	dei_cloned->dir_block_accessor = NULL;

	return &dei_cloned->entry;
}

/** @brief perform a chdir to a partition marked by a directory entry
 * @ingroup cbmimage_dir
 *
 * @param[in] dir_entry
 *    pointer to a directory entry that was the result of a previous
 *    cbmimage_dir_get_first() or cbmimage_dir_get_next() call.
 *
 * @return
 *  - 0 on success
 *  - != 0 otherwise
 */
int
cbmimage_dir_chdir(
		cbmimage_dir_entry * dir_entry
		)
{
	cbmimage_i_dir_entry_internal * dei = (cbmimage_i_dir_entry_internal *) dir_entry;

	cbmimage_image_settings * new_settings = NULL;

	cbmimage_fileimage * image = dei ? dei->image : NULL;

	int error = 1;

	if (image && image->settings && image->settings->fct.chdir) {
		new_settings = cbmimage_i_xalloc_and_copy(sizeof *new_settings, dei->image->settings, sizeof *new_settings);
	}

	if (new_settings) {
		new_settings->next_settings = image->settings;
		new_settings->fat = 0;

		new_settings->info = NULL;

		image->settings = new_settings;
		new_settings = NULL;
		if (image->settings->fct.chdir(image->settings, dir_entry) == 0) {
			error = 0;
		}
		else {
			cbmimage_dir_chdir_close(image);
		}
	}

	if (new_settings) {
		// an error occurred, free the memory block
		cbmimage_i_xfree(new_settings);
	}

	return error;
}

/** @brief "close a chdir"; that is, go back to the parent directory
 * @ingroup cbmimage_dir
 *
 * @param[in] image
 *    pointer to an image for which to return to the parent directory
 *
 * @return
 *  - 0 on success
 *  - != 0 otherwise
 *
 * @remark
 *   If no parent exists, this functions returns with an error
 */
int
cbmimage_dir_chdir_close(
		cbmimage_fileimage * image
		)
{
	assert(image);

	cbmimage_image_settings * settings_to_pop = NULL;

	int error = 1;

	if (image->settings && image->settings->next_settings) {
		settings_to_pop = image->settings;
		image->settings = settings_to_pop->next_settings;

		if (settings_to_pop->fat != image->settings->fat) {
			cbmimage_fat_close(settings_to_pop->fat);
			settings_to_pop->fat = NULL;
		}

		if (image->settings->info != settings_to_pop->info) {
			cbmimage_blockaccessor_close(settings_to_pop->info);
			settings_to_pop->info = NULL;
		}
		settings_to_pop->next_settings = NULL;
		cbmimage_i_xfree(settings_to_pop);

		error = 0;
	}

	return error;
}


/** @brief @internal read the partition data (start and end block) from a directory entry
 * @ingroup cbmimage_dir
 *
 * @param[in] dir_entry
 *    pointer to a entry
 *
 * @param[inout] block_first
 *    pointer to a block address that will contain the first block of the partition on exit
 *
 * @param[inout] block_last
 *    pointer to a block address that will contain the last block of the partition on exit.\n
 *    Can be NULL if this info is not needed.
 *
 * @param[inout] block_count
 *    pointer to a size_t that will contain the number of blocks of this partition on exit.\n
 *    Can be NULL if this info is not needed.
 *
 * @return
 *  - 0 on success
 *  - != 0 otherwise
 *
 * @remark
 *   The info block_last and block_count is somewhat redundant.
 *   That is, block_first + block_count = block_last.
 *   However, for different purposes, the one or the other info is needed, so this function
 *   returns both variants.
 */
int
cbmimage_i_dir_get_partition_data(
		cbmimage_dir_entry *    dir_entry,
		cbmimage_blockaddress * block_first,
		cbmimage_blockaddress * block_last,
		size_t *                block_count
		)
{
	assert(dir_entry != NULL);
	assert(block_first != NULL);
	assert(block_last != NULL);
	assert(block_count != NULL);

	cbmimage_i_dir_entry_internal * dei = (cbmimage_i_dir_entry_internal *) dir_entry;

	if (!dei) {
		return -1;
	}

	unsigned int lba = dei->entry.start_block.lba;

	if (block_count) {
		*block_count = dei->entry.block_count;
	}

	lba += dei->entry.block_count - 1;
	assert(lba < cbmimage_get_max_lba(dei->image));

	if (lba >= cbmimage_get_max_lba(dei->image)) {
		return -1;
	}

	*block_first = dei->entry.start_block;

	if (block_last) {
		CBMIMAGE_BLOCK_SET_FROM_LBA(
				dei->image,
				*block_last,
				lba
				);
	}

	return 0;
}

/** @brief @internal change to a (global) subpartition
 * @ingroup cbmimage_dir
 *
 * @param[in] settings
 *    pointer to image settings
 *
 * @param[in] block_subdir_first
 *    the block that is the first block of the subdir/subpartition
 *
 * @param[inout] block_count
 *    the number of blocks that form this subdir/subpartition
 *
 * @return
 *  - 0 on success
 *  - -1 if an error occurred
 *
 * @remark
 *   A global subpartition/subdir is a subdir where the first block
 *   inside of the subpartition has the address T/S 1/0 (LBA 1).
 *   That is, the whole addressing schema is changed.
 *
 *   This is the type of partitions that was introduced with the CMD devices
 */
int
cbmimage_i_dir_set_subpartition_global(
		cbmimage_image_settings * settings,
		cbmimage_blockaddress     block_subdir_first,
		size_t                    block_count
		)
{
#if 0
	/// @todo: introduce a verbosity level for debugging

	cbmimage_i_fmt_print("Setting directory to (global) %u/%u(%03X) of length %u.\n",
			block_subdir_first.ts.track,
			block_subdir_first.ts.sector,
			block_subdir_first.lba,
			block_count
			);
#endif

	settings->block_subdir_first = cbmimage_block_unused;
	settings->block_subdir_first.ts.track = 1;
	settings->block_subdir_first.lba = 1;
	settings->block_subdir_last = settings->image->global_settings->lastblock;

	settings->subdir_data_offset = 0;
	uint8_t * newdata = cbmimage_i_get_address_of_block(settings->image, block_subdir_first);

	settings->subdir_data_offset = newdata - settings->image->parameter->buffer;

	return 0;
}

/** @brief @internal change to a (relative) subpartition
 * @ingroup cbmimage_dir
 *
 * @param[in] settings
 *    pointer to image settings
 *
 * @param[in] block_subdir_first
 *    the block that is the first block of the subdir/subpartition
 *
 * @param[inout] block_subdir_last
 *    the last block of the subdir/subpartition
 *
 * @return
 *  - 0 on success
 *  - -1 if an error occurred
 *
 * @remark
 *   A relative subpartition/subdir is a subdir where the first block
 *   inside of the subpartition has the same address as it has in the
 *   "surrounding" image. That is, block x/y is also addresses as x/y
 *   inside of the subpartition.
 *
 *   This is the type of partitions that was introduced with the 1581 drive.
 *
 * @todo Change the input parameter block_subdir_last to block_count to be consistent with cbmimage_i_dir_set_subpartition_global()
 */
int
cbmimage_i_dir_set_subpartition_relative(
		cbmimage_image_settings * settings,
		cbmimage_blockaddress     block_subdir_first,
		cbmimage_blockaddress     block_subdir_last
		)
{
	int ret = 0;

	cbmimage_blockaddress block_subdir_first_adjusted = settings->block_subdir_first;
	cbmimage_blockaddress block_subdir_last_adjusted = settings->block_subdir_first;

	ret = cbmimage_blockaddress_add(settings->image, &block_subdir_first_adjusted, block_subdir_first);

	if (ret == 0) {
		ret = cbmimage_blockaddress_add(settings->image, &block_subdir_last_adjusted, block_subdir_last);
	}

	if (ret == 0) {
		settings->block_subdir_first = block_subdir_first_adjusted;
		settings->block_subdir_last = block_subdir_last_adjusted;
	}

#if 0
	/// @todo: introduce a verbosity level for debugging

	cbmimage_i_fmt_print("Setting directory to: %u/%u(%03X) till %u/%u(%03X).\n",
			block_subdir_first_adjusted.ts.track,
			block_subdir_first_adjusted.ts.sector,
			block_subdir_first_adjusted.lba,
			block_subdir_last_adjusted.ts.track,
			block_subdir_last_adjusted.ts.sector,
			block_subdir_last_adjusted.lba
			);
#endif

	return ret;
}
