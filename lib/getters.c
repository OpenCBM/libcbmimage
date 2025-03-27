/** @file lib/getters.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: getting image specific data
 *
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"

#include <assert.h>

/** @brief get a pointer to the raw image contents
 * @ingroup cbmimage_image
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @return
 *    - Pointer to the raw image data
 *    - 0 if an error occurred
 *
 * @remark
 *  - Be careful! Use the pointer only for reading!
 *    libcbmimage can act strangely if essential data
 *    is changed without libcbmimage knowing about it!
 *  - The size of the buffer can be obtained with
 *    cbmimage_image_get_raw_size().
 */
const void *
cbmimage_image_get_raw(
		cbmimage_fileimage * image
		)
{
	assert(image != NULL);
	cbmimage_image_parameter * parameter = image->parameter;
	assert(parameter != NULL);

	return parameter->buffer;
}

/** @brief get the size of the raw image contents
 * @ingroup cbmimage_image
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @return
 *    - size of the raw image contents (in byte)
 *    - 0 if there is no raw image content
 *
 * @remark
 *  - The buffer itself can be obtained with cbmimage_image_get_raw().
 *
 */
size_t
cbmimage_image_get_raw_size(
		cbmimage_fileimage * image
		)
{
	assert(image != NULL);
	cbmimage_image_parameter * parameter = image->parameter;
	assert(parameter != NULL);

	return parameter->size;
}

/** @brief get the image type as string
 * @ingroup cbmimage_image
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @return
 *    Pointer to a string that contains the name of the image type.
 *
 * @remark
 *  - The pointer points inside of the image structure. Do not write to it, as
 *    it will break subsequent processes.
 *  - When the image is closed, the pointer gets invalid.
 *
 */
const char *
cbmimage_get_imagetype_name(
		cbmimage_fileimage * image
		)
{
	assert(image != NULL);
	return image->settings->imagetype_name;
}

/** @brief get the file name of the image
 * @ingroup cbmimage_image
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @return
 *    Pointer to a string that contains the file name of the image.
 *
 * @remark
 *  - The pointer points inside of the image structure. Do not write to it, as
 *    it will break subsequent processes.
 *  - When the image is closed, the pointer gets invalid.
 *
 */
const char *
cbmimage_get_filename(
		cbmimage_fileimage * image
		)
{
	assert(image != NULL);
	cbmimage_image_parameter * parameter = image->parameter;
	assert(parameter != NULL);

	return parameter->filename;
}

/** @brief get the maximum tracks of the image
 * @ingroup cbmimage_image
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @return
 *    Number of tracks in the image
 *
 * @remark
 *  - As the CBM track numbering goes, the track range is from
 *    1 to the result of this call. \n
 *    For example, for D64 images, this function will return 35,
 *    giving a range of 1 .. 35.
 *
 */
uint16_t
cbmimage_get_max_track(
		cbmimage_fileimage * image
		)
{
	assert(image != NULL);
	return image->settings->maxtracks;
}

/** @brief get the maximum sectors on a track of the image
 * @ingroup cbmimage_image
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @return
 *    Maximum number of sectors on a track in the image
 *
 * @remark
 *  - This is the *maximum* number of sectors on a track.
 *    Please not that for a specific track, the number of sectors
 *    can be lower! \n
 *    To obtain the number of sectors on a specific track, use
 *    cbmimage_get_sectors_in_track() instead!
 *  - As the CBM sector numbering goes, the sector range is from
 *    0 to the result of this call minus 1. \n
 *    For example, for D64 images, this function will return 21,
 *    giving a range of 0 .. 20.
 *
 */
uint16_t
cbmimage_get_max_sectors(
		cbmimage_fileimage * image
		)
{
	assert(image != NULL);
	return image->settings->maxsectors;
}

/** @brief get the maximum LBA of the image
 * @ingroup cbmimage_image
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @return
 *    the biggest possible LBA of this image
 */
uint16_t
cbmimage_get_max_lba(
		cbmimage_fileimage * image
		)
{
	assert(image != NULL);
	return image->settings->lastblock.lba;
}

/** @brief get the number of blocks in a block of the image
 * @ingroup cbmimage_image
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @return
 *    Number of bytes in a block in the image
 *
 * @remark
 *  - Most CBM images contain 256 (= 0x100) bytes in each block.
 *    Note however that the functions are generic, so thus assumption
 *    does not need to hold true. \n
 *
 *    In fact, some image types (namely, D81, D1M, D2M, D4M) only give
 *    the illusion that the floppy handles 256 byte blocks.
 *    The physical blocks have different sizes (512 and 1024 byte blocks)!
 *
 *    In the future, some image types might present
 *    options to address the blocks in these other numberings.
 *
 */
uint16_t
cbmimage_get_bytes_in_block(
		cbmimage_fileimage * image
		)
{
	assert(image != NULL);
	return image->settings->bytes_in_block;
}

/** @brief get the number of sectors on a specific track of the image
 * @ingroup cbmimage_image
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] track
 *    the track number for which to obtain the number of sectors
 *
 * @return
 *    Number of sectors on this specific track in the image
 *
 * @remark
 *  - This is the number of sectors on this specific track.
 *    Due to the nature of many CBM disk images, the number of
 *    sectors can vary from track to track. \n
 *    To obtain the maximum number of sectors on any track, use
 *    cbmimage_get_max_sectors() instead!
 *  - As the CBM sector numbering goes, the sector range is from
 *    0 to the result of this call minus 1. \n
 *    For example, for D64 images on the first speed zone
 *    (i.e., tracks 1 to 17), this function will return 21, giving
 *    a range of 0 .. 20.
 *
 */
uint16_t
cbmimage_get_sectors_in_track(
		cbmimage_fileimage * image,
		uint16_t track
		)
{
	assert(image != NULL);
	cbmimage_image_settings * settings = image->settings;
	assert(settings != NULL);

	if (settings->fct.get_sectors_in_track != 0) {
		return settings->fct.get_sectors_in_track(image->settings, track);
	}
	else {
		return settings->maxsectors;
	}
}
