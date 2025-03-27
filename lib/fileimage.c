/** @file lib/fileimage.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: image specific processing
 *
 * @defgroup cbmimage_image image handling functions
 */
#include "cbmimage/internal.h"
#include "cbmimage/alloc.h"
#include "cbmimage/helper.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/** @brief @internal get the number of blocks of an image type
 *
 * @param[in] imagetype
 *   The image type for which to determine the number of blocks
 *
 * @return
 *   The number of blocks of this image type
 */
static
int
cbmimage_i_get_number_of_blocks(
		cbmimage_imagetype imagetype
		)
{
	static uint16_t count_of_blocks[TYPE_LAST] = {
		0,             // TYPE_UNKNOWN
		683 + 7,       // TYPE_D40
		683,           // TYPE_D64
		683 + 5 * 17,  // TYPE_D64_40TRACK
		683 + 5 * 17,  // TYPE_D64_40TRACK_SPEEDDOS
		683 + 5 * 17,  // TYPE_D64_40TRACK_DOLPHIN
		683 + 5 * 17,  // TYPE_D64_40TRACK_PROLOGIC
		683 + 7 * 17,  // TYPE_D64_42TRACK
		683 * 2,       // TYPE_D71
		3200,          // TYPE_D81
		2083,          // TYPE_D80
		2083 * 2,      // TYPE_D82
		3240,          // TYPE_CMD_D1M
		6480,          // TYPE_CMD_D2M
		12960,         // TYPE_CMD_D2M
	};

	if (imagetype < TYPE_UNKNOWN || imagetype >= TYPE_LAST) {
		return 0;
	}

	return count_of_blocks[imagetype];
}


/** @brief @internal store the last block into the image settings structure
 * @ingroup cbmimage_image
 *
 * It is important to know the last block for some internal
 * tests.
 *
 * @param[in] image
 *    pointer to the image data
 *
 */
void
cbmimage_i_create_last_block(
		cbmimage_fileimage * image
		)
{
	uint8_t last_track = cbmimage_get_max_track(image);
	uint8_t last_sector = cbmimage_get_sectors_in_track(image, last_track);
	CBMIMAGE_BLOCK_SET_FROM_TS(image, image->settings->lastblock, last_track, last_sector - 1);
}

/** @brief @internal create an image data structure
 * @ingroup cbmimage_image
 *
 * Internal helper for cbmimage_image_open() and cbmimage_image_openfile().
 * Create image data structure after all (filename and file data or memory data)
 * are known.
 *
 * @param[in] buffer
 *    pointer to the raw image data
 *
 * @param[in] size
 *    the size of the buffer
 *
 * @param[in] filename
 *    pointer to the name of the file that was opened
 *
 * @param[in] imagetype_hint
 *    If the type of the image is known, specify it here; otherwise, use
 *    TYPE_UNKNOWN.
 *
 * @return
 *    pointer to the image data
 *
 * @todo add error handling and an error result
 */
static cbmimage_fileimage *
cbmimage_i_fileimage_create(
		const uint8_t *    buffer,
		size_t             size,
		const char *       filename,
		cbmimage_imagetype imagetype_hint
		)
{
	cbmimage_fileimage *       image = NULL;
	cbmimage_image_settings *  settings;
	cbmimage_image_parameter * parameter;
	int                        extra_errormap = 1;

	size_t filename_len = filename ? strlen(filename + 1) : 1;

	if (imagetype_hint == TYPE_UNKNOWN) {
		imagetype_hint = cbmimage_image_guesstype(buffer, size, &extra_errormap);
	}

	if (extra_errormap) {
		extra_errormap = cbmimage_i_get_number_of_blocks(imagetype_hint);
	}

	image = cbmimage_i_xalloc(sizeof *image + sizeof *settings + sizeof *parameter + size + extra_errormap + filename_len);

	if (image) {
		parameter = (cbmimage_image_parameter*) &image->bufferarray[filename_len + size + extra_errormap + sizeof *settings];
		parameter->filename = &image->bufferarray[0];
		if (filename) {
			strcpy(parameter->filename, filename);
		}
		else {
			parameter->filename[0] = 0;
		}
		parameter->buffer = &image->bufferarray[filename_len];

		image->parameter = parameter;
		image->settings = (cbmimage_image_settings *) &image->bufferarray[filename_len + size + extra_errormap];
		image->global_settings = image->settings;

		memcpy(parameter->buffer, buffer, size);

		if (!extra_errormap) {
			size -= cbmimage_i_get_number_of_blocks(imagetype_hint);
		}
		parameter->errormap = &parameter->buffer[size];
		parameter->size = size;

		switch (imagetype_hint) {
			case TYPE_D40:
				cbmimage_i_d40_image_open(image);
				break;

			case TYPE_D71:
				cbmimage_i_d71_image_open(image);
				break;

			case TYPE_D64:
				cbmimage_i_d64_image_open(image);
				break;

			case TYPE_D64_40TRACK:
				cbmimage_i_d64_40track_image_open(image);
				break;

			case TYPE_D64_40TRACK_SPEEDDOS:
				cbmimage_i_d64_40track_speeddos_image_open(image);
				break;

			case TYPE_D64_40TRACK_DOLPHIN:
				cbmimage_i_d64_40track_dolphin_image_open(image);
				break;

			case TYPE_D64_40TRACK_PROLOGIC:
				cbmimage_i_d64_40track_prologic_image_open(image);
				break;

			case TYPE_D64_42TRACK:
				cbmimage_i_d64_42track_image_open(image);
				break;

			case TYPE_D81:
				cbmimage_i_d81_image_open(image);
				break;

			case TYPE_D80:
				cbmimage_i_d80_image_open(image);
				break;

			case TYPE_D82:
				cbmimage_i_d82_image_open(image);
				break;

			case TYPE_CMD_D1M:
				cbmimage_i_d1m_image_open(image);
				break;

			case TYPE_CMD_D2M:
				cbmimage_i_d2m_image_open(image);
				break;

			case TYPE_CMD_D4M:
				cbmimage_i_d4m_image_open(image);
				break;

#if 0
			// DHD files are not CMD NATIVE partitions!
			// A native partition exists inside of a D1M, D2M or D4M image.
			// A DHD file is similar, but not identical to D1M, D2M and D4M.
			// Look at the VICE doku for an explanation of DHD.
			//
			case TYPE_CMD_NATIVE:
				cbmimage_i_dnp_image_open(image);
				break;
#endif
			default:
				break;
		}

		cbmimage_i_create_last_block(image);
	}

	return image;
}

/** @brief @internal structure for mapping of image type and their respective
 * file size
 */
typedef
struct cbmimage_mapping_size_imagetype_s {
	const char *       name;          ///< the name (as string) of the image type
	cbmimage_imagetype imagetype;     ///< the image type
	size_t             size;          ///< size of such an image
	uint16_t           blocks;        ///< number of blocks in such an image.
	                                  ///  This determines the number of error map bytes
} cbmimage_mapping_size_imagetype;

/** @brief @internal mapping of image type and their respective file size
 * Note that image types can have multiple entries.
 * This happens because there are variants, for example, with and without an
 * error map.
 */
static cbmimage_mapping_size_imagetype cbmimage_mapping_from_size_to_imagetype[] = {
	{ "D64",        TYPE_D64,                  174848,                               683          },
	{ "D64_40",     TYPE_D64_40TRACK,          174848 + 5 * 17 * 256,                683 + 5 * 17 },
	{ "D64_42",     TYPE_D64_42TRACK,          174848 + 7 * 17 * 256,                683 + 7 * 17 },
	{ "D40",        TYPE_D40,                  174848 + 7 * 256,                     683 + 7      },
	{ "D71",        TYPE_D71,                  174848 * 2,                           683 * 2      },
	{ "D81",        TYPE_D81,                  819200,                               3200         },
	{ "D80",        TYPE_D80,                  533248,                               2083         },
	{ "D82",        TYPE_D82,                  533248 * 2,                           2083 * 2     },
	{ "D1M",        TYPE_CMD_D1M,              3240 * 256,                           3240         },
	{ "D2M",        TYPE_CMD_D2M,              3240 * 256 * 2,                       3240 * 2     },
	{ "D4M",        TYPE_CMD_D4M,              3240 * 256 * 4,                       3240 * 4     },
};

/** @brief @internal guess the image type of an image
 * @ingroup cbmimage_image
 *
 * Internal helper for cbmimage_image_open() and cbmimage_image_openfile().
 *
 * @param[in] buffer
 *    pointer to the raw image data
 *
 * @param[in] size
 *    the size of the buffer
 *
 * @param[in] extra_errormap
 *    - 0 if the image already has an error map
 *    - != 0 otherwise
 *
 * @return
 *    the guessed image type
 *
 * @todo currently, only the file size is considered. Add more thorough tests
 */
cbmimage_imagetype
cbmimage_image_guesstype(
		const uint8_t * buffer,
		size_t          size,
		int *           extra_errormap
		)
{
	*extra_errormap = 0;

	for (int i = 0; i < CBMIMAGE_ARRAYSIZE(cbmimage_mapping_from_size_to_imagetype); ++i) {
		if (cbmimage_mapping_from_size_to_imagetype[i].size == size) {
			*extra_errormap = 1;
			return cbmimage_mapping_from_size_to_imagetype[i].imagetype;
		}
		if (cbmimage_mapping_from_size_to_imagetype[i].size + cbmimage_mapping_from_size_to_imagetype[i].blocks == size) {
			return cbmimage_mapping_from_size_to_imagetype[i].imagetype;
		}
	}

	return TYPE_UNKNOWN;
}

/** @brief @internal guess the image type of an image file
 * @ingroup cbmimage_image
 *
 * Internal helper for cbmimage_image_open() and cbmimage_image_openfile().
 *
 * @param[in] filename
 *    pointer to the name of the file that should be opened
 *
 * @return
 *    the guessed image type
 *
 * @todo currently, only the file size is considered. Add more thorough tests
 */
cbmimage_imagetype
cbmimage_image_file_guesstype(
		const char * filename
		)
{
	cbmimage_imagetype guessed_type = TYPE_UNKNOWN;

	assert("cbmimage_image_file_guesstype() is unimplemented!" == 0);

	return guessed_type;
}

/** @brief open an in-memory CBM image
 * @ingroup cbmimage_image
 *
 * @param[in] buffer
 *    pointer to the raw image data
 *
 * @param[in] size
 *    the size of the buffer
 *
 * @param[in] imagetype_hint
 *    If the type of the image is known, specify it here; otherwise, use
 *    TYPE_UNKNOWN.
 *
 * @return
 *    pointer to the image data
 *
 * @remark
 *    - When the work is done, the image must be freed by cbmimage_image_close()
 *
 */
cbmimage_fileimage *
cbmimage_image_open(
		const uint8_t *    buffer,
		size_t             size,
		cbmimage_imagetype imagetype_hint
		)
{
	return cbmimage_i_fileimage_create(buffer, size, NULL, imagetype_hint);
}

/** @brief open a CBM image from a file
 * @ingroup cbmimage_image
 *
 * @param[in] filename
 *    pointer to the name of the file that should be opened
 *
 * @param[in] imagetype_hint
 *    If the type of the image is known, specify it here; otherwise, use
 *    TYPE_UNKNOWN.
 *
 * @return
 *    pointer to the image data
 *
 * @remark
 *    - When the work is done, the image must be freed by cbmimage_image_close()
 *
 */
cbmimage_fileimage *
cbmimage_image_openfile(
		const char *       filename,
		cbmimage_imagetype imagetype_hint
		)
{
	cbmimage_fileimage * image = NULL;

	FILE * f = fopen(filename, "rb");
	if (f) {
		fseek(f, 0l, SEEK_END);
		size_t size = ftell(f);

		rewind(f);

		void * buffer = cbmimage_i_xalloc(size);

		if (buffer && fread(buffer, size, 1, f) == 1) {
			image = cbmimage_i_fileimage_create(buffer, size, filename, imagetype_hint);
		}

		cbmimage_i_xfree(buffer);
		fclose(f);
	}

	return image;
}

/** @brief read a file and store it in a given CBM image
 * @ingroup cbmimage_image
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] filename
 *    pointer to the name of the file that should be opened
 *
 * @todo add error handling and an error result
 */
void
cbmimage_image_readfile(
		cbmimage_fileimage * image,
		const char *         filename
		)
{
	FILE * f = fopen(filename, "rb");
	if (f) {
		cbmimage_image_parameter * parameter = image->parameter;

		if (fread(parameter->buffer, parameter->size, 1, f) == 1) {
			// success
			cbmimage_i_print("successfully read!\n");
		}
		fclose(f);
	}
}

/** @brief write the CBM image to a file
 * @ingroup cbmimage_image
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] filename
 *    pointer to the name of the file that should be opened
 *
 * @todo add error handling and an error result
 */
void
cbmimage_image_writefile(
		cbmimage_fileimage * image,
		const char *         filename
		)
{
	FILE * f = fopen(filename, "wb");
	if (f) {
		cbmimage_image_parameter * parameter = image->parameter;

		if (fwrite(parameter->buffer, parameter->size, 1, f) == 1) {
			// success
			cbmimage_i_print("successfully written!\n");
		}
		fclose(f);
	}
}

/** @brief close a CBM image
 * @ingroup cbmimage_image
 *
 * Upon close, all resources are freed
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @remark
 *   - This is the counterpart to cbmimage_image_open() and
 *     cbmimage_image_openfile().
 */
void
cbmimage_image_close(
		cbmimage_fileimage * image
		)
{
	while (cbmimage_dir_chdir_close(image) == 0) {
	}

	cbmimage_image_settings * settings = image ? image->settings : NULL;

	if (settings) {
		cbmimage_fat_close(settings->fat);
		cbmimage_blockaccessor_close(settings->info);
	}

	cbmimage_i_xfree(image);
}


/** @brief dump a FAT structure of the image
 * @ingroup cbmimage_image
 *
 * @param[in] image
 *    pointer to the image data
 *
 * @param[in] trackformat
 *    - if 0, show the FAT as linear following of LBAs.
 *    - else, show the FAT in the structure that the disk layout defines (track/sector)
 *      The value of trackformat defines how many values at most are output into one line
 */
void
cbmimage_image_fat_dump(
		cbmimage_fileimage * image,
		int                  trackformat
		)
{
	assert(image != NULL);

	cbmimage_image_settings * settings = image->settings;

	if (!settings->fat) {
		cbmimage_validate(image);
	}
	if (settings->fat) {
		cbmimage_fat_dump(settings->fat, trackformat);
	}
}
