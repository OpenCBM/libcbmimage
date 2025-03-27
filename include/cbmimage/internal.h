/** @file include/cbmimage/internal.h \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage internals
 *
 * @defgroup cbmimage_helper internal helper
 */
#ifndef CBMIMAGE_INTERNAL_H
#define CBMIMAGE_INTERNAL_H 1

#include "cbmimage.h"

/** @brief internal data for directory entry
 * @ingroup cbmimage_dir
 *
 * This type contains the internal data that is needed to specify a directory
 * entry.
 *
 * @remark
 *  - see also cbmimage_dir_entry, cbmimage_dir_entry_s
 */
typedef
struct cbmimage_i_dir_entry_internal_s {

	/// the caller visible cbmimage_dir_entry
	cbmimage_dir_entry entry;

	/// the fileimage on which this dir_entry operates
	cbmimage_fileimage * image;

	/// accessor to the directory block where this entry is located
	cbmimage_blockaccessor * dir_block_accessor;

	/// the offset into the directory block where this entry is located
	uint16_t dir_block_offset;

	/// if this directory entry is empty, this is != 0
	int is_empty;

	/// loop detector
	cbmimage_loop * loop_detector;

} cbmimage_i_dir_entry_internal;


/* forward definition; will be specified later */
typedef struct cbmimage_image_settings_s cbmimage_image_settings;

/** @brief @internal type for a function that gets the number of sectors on a specific track of the image
 * @ingroup cbmimage_helper
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @param[in] track
 *    the track number for which to obtain the number of sectors
 *
 * @return
 *    Number of sectors on this specific track in the image
 *
 * @remark
 *  - This is the number of sectors on this specific track.
 *    Due to the nature of many CBM disk images, including D64, D71 and D40
 *    images, the number of sectors can vary from track to track. \n
 *    To obtain the maximum number of sectors on any track, use
 *    cbmimage_get_max_sectors() instead!
 *  - As the CBM sector numbering goes, the sector range is from
 *    0 to the result of this call minus 1. \n
 *    For example, for D64 images on the first speed zone
 *    (i.e., tracks 1 to 17), this function will return 21, giving
 *    a range of 0 .. 20.
 *
 */
typedef
uint16_t
cbmimage_get_sectors_in_track_fct(
		cbmimage_image_settings * settings,
		uint16_t                  track
		);

/** @brief @internal type for a function that converts the T/S to LBA block address
 * @ingroup cbmimage_helper
 *
 * @param[in] settings
 *    pointer to the image data internal settings
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
 *
 */
typedef
int
cbmimage_ts_to_blockaddress_fct(
		cbmimage_image_settings * settings,
		cbmimage_blockaddress *   block
		);

/** @brief @internal type for a function that converts the LBA to T/S block address
 * @ingroup cbmimage_helper
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @param[inout] block
 *    pointer to a block address with an initialized LBA address
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - the block address is completely created from the lba parameter
 *   - an error occurs if the lba block does not exist on this type of image
 *
 */
typedef
int
cbmimage_lba_to_blockaddress_fct(
		cbmimage_image_settings * settings,
		cbmimage_blockaddress *   block
		);

/** @brief @internal type for a function that helps in chdir'ing
 * @ingroup cbmimage_helper
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @param[in] dir_entry
 *    pointer to the dir_entry to which to chdir.
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - This function must test if the directory entry is actually a directory
 *     to which it can change. If it is not valid, it should return with an
 *     error value != 0. In this case, the chdir is aborted and returns with
 *     an error, too.
 *
 */
typedef
int
cbmimage_chdir_fct(
		cbmimage_image_settings * settings,
		cbmimage_dir_entry *      dir_entry
		);

/** @brief @internal type for a function that occupies additional BAM entries
 * @ingroup cbmimage_helper
 *
 * @param[in] settings
 *    pointer to the image data internal settings
 *
 * @return
 *    - 0 if no error occurred
 *    - != 0 if an error occurred
 *
 * @remark
 *   - This function is used for D71 images, which mark track 53 (18 + 35, 2nd
 *     directory track) as occupied.
 *   - This function is also used for D81 images after chdir'ing. In this case,
 *     the 1581 marks all blocks outside of the partition as used.
 */
typedef
int
cbmimage_set_bam_fct(
		cbmimage_image_settings * settings
		);

/** @brief BAM Selector
 * @ingroup cbmimage_bam @internal
 *
 * This type describes where the BAM entries (that is, the bitmap part of the
 * BAM) are located on disk.  This way, the code that handles BAM entries is
 * generic, and only the initialization of the BAM selector has to be image
 * specific.
 *
 * In case the BAM is stored in multiple blocks (or in the same block, but at
 * different locations), an array of BAM selectors has to be used, with every
 * selector describing one location of the BAM entries
 *
 * @remark
 *  - see also cbmimage_i_bam_counter_selector
 */
typedef
struct cbmimage_i_bam_selector_s {

	/// pointer to the block data where this BAM entry is located
	uint8_t * buffer;

	/// the number of the first track that is described in this BAM selector
	uint8_t starttrack;

	/// the block where this BAM entry is located
	cbmimage_blockaddress block;

	/// the offset inside of the block where this BAM entry is located
	uint8_t startoffset;

	/** multiplier for the BAM entries; that is, how many byte are the different
	 * tracks aways from each other?
	 */
	uint8_t multiplier;

	/** the number of byte that form one BAM entry. This is restricted to a
	 * maximum of BAM_MASK_COUNT. In case of a cbmimage_i_bam_counter_selector,
	 * this must be 0.
	 */
	uint8_t data_count;

	/** @brief reverse the order of the bits of the BAM
	 *
	 * If this value is 0, the BAM uses the Commodore ordering.
	 * If this value is != 0, the BAM uses the CMD / DNP ordering.
	 *
	 * @remark
	 * The DNP image format used a reverse order of the BAM bits w.r.t. the Commodore drives.
	 * For Commodore, bit 0 corresponds to the (numerically) lowest block, and bit 7 to the (numerically) highest block.
	 * Take, or example, the byte 0xFE for the first block of the BAM of a track. In a Commodore drives, this means that block 1/0 is occupied.
	 *
	 * For DNP, bit 0 corresponds to the (numerically) *highest* block, and bit 7 to the (numerically) lowest block.
	 * That is, the example 0xFE for the first block of the BAM of a track
	 * corresponds to block 1/7 being occupied in a DNP partition.
	 */
	uint8_t reverse_order;

} cbmimage_i_bam_selector;

/** @brief @internal initializer for a cbmimage_i_bam_selector, used as a BAM entry
 *
 * This macro can be used like \n
 * cbmimage_i_bam_selector selector = CBMIMAGE_BAM_SELECTOR_INIT(starttrack, startoffset, multiplier, data_count, track, sector) \n
 * in order to initialize the selector
 *
 * @param[in] _starttrack
 *   the number of the first track which is described in this BAM selector
 *
 * @param[in] _startoffset
 *   the offset inside of the block where this BAM entry is located
 *
 * @param[in] _multiplier
 *    multiplier for the BAM entries; that is, how many byte are the different tracks aways from each other?
 *
 * @param[in] _data_count
 *    the number of byte that form one BAM entry. This is restricted to a maximum of BAM_MASK_COUNT.
 *    In case of a cbmimage_i_bam_counter_selector, this must be 0.
 *
 * @param[in] _track
 *   the track where this BAM is located
 *
 * @param[in] _sector
 *   the sector where this BAM is located
 *
 * @param[in] _reverse_order
 *   set to != 0 if the BAM uses the reverse (a.k.a. CMD or DNP) ordering.
 *
 * @remark
 *   This macro is only used if the BAM does not have the regular outline. \n
 *   Regularly, the BAM looks as follows: [number of free blocks on this track] [bitmap for this track] \n
 *   \n
 *   If the BAM looks that way, use CBMIMAGE_BAM_AND_BAM_COUNTER_CREATE() instead!
 *
 */
#define CBMIMAGE_I_BAM_SELECTOR_INIT(_starttrack, _startoffset, _multiplier, _data_count, _track, _sector, _reverse_order) \
	{ .starttrack = _starttrack, .startoffset = _startoffset, .multiplier = _multiplier, .data_count = _data_count, .reverse_order = _reverse_order, .block = CBMIMAGE_BLOCK_INIT_FROM_TS(_track, _sector) }

/** @brief @internal initializer for a cbmimage_i_bam_selector, used as a BAM entry
 *
 * This macro can be used like \n
 * cbmimage_i_bam_selector selector = CBMIMAGE_BAM_SELECTOR_INIT(starttrack, startoffset, multiplier, data_count, track, sector) \n
 * in order to initialize the selector
 *
 * @param[in] _starttrack
 *   the number of the first track which is described in this BAM selector
 *
 * @param[in] _startoffset
 *   the offset inside of the block where this BAM entry is located
 *
 * @param[in] _multiplier
 *    multiplier for the BAM entries; that is, how many byte are the different tracks aways from each other?
 *
 * @param[in] _data_count
 *    the number of byte that form one BAM entry. This is restricted to a maximum of BAM_MASK_COUNT.
 *    In case of a cbmimage_i_bam_counter_selector, this must be 0.
 *
 * @param[in] _track
 *   the track where this BAM is located
 *
 * @param[in] _sector
 *   the sector where this BAM is located
 *
 * @remark
 *   This macro is only used if the BAM does not have the regular outline. \n
 *   Regularly, the BAM looks as follows: [number of free blocks on this track] [bitmap for this track] \n
 *   \n
 *   If the BAM looks that way, use CBMIMAGE_BAM_AND_BAM_COUNTER_CREATE() instead!
 *
 */
#define CBMIMAGE_BAM_SELECTOR_INIT(_starttrack, _startoffset, _multiplier, _data_count, _track, _sector) \
	CBMIMAGE_I_BAM_SELECTOR_INIT(_starttrack, _startoffset, _multiplier, _data_count, _track, _sector, 0)

/** @brief @internal initializer for a cbmimage_i_bam_selector, used as a BAM entry, for reverse order BAMs
 *
 * This macro can be used like \n
 * cbmimage_i_bam_selector selector = CBMIMAGE_BAM_SELECTOR_INIT_REVERSE(starttrack, startoffset, multiplier, data_count, track, sector) \n
 * in order to initialize the selector
 *
 * @param[in] _starttrack
 *   the number of the first track which is described in this BAM selector
 *
 * @param[in] _startoffset
 *   the offset inside of the block where this BAM entry is located
 *
 * @param[in] _multiplier
 *    multiplier for the BAM entries; that is, how many byte are the different tracks aways from each other?
 *
 * @param[in] _data_count
 *    the number of byte that form one BAM entry. This is restricted to a maximum of BAM_MASK_COUNT.
 *    In case of a cbmimage_i_bam_counter_selector, this must be 0.
 *
 * @param[in] _track
 *   the track where this BAM is located
 *
 * @param[in] _sector
 *   the sector where this BAM is located
 *
 * @remark
 *   This macro is only used if the BAM does not have the regular outline. \n
 *   Regularly, the BAM looks as follows: [number of free blocks on this track] [bitmap for this track] \n
 *   \n
 *   If the BAM looks that way, use CBMIMAGE_BAM_AND_BAM_COUNTER_CREATE() instead!
 *
 */
#define CBMIMAGE_BAM_SELECTOR_INIT_REVERSE(_starttrack, _startoffset, _multiplier, _data_count, _track, _sector) \
	CBMIMAGE_I_BAM_SELECTOR_INIT(_starttrack, _startoffset, _multiplier, _data_count, _track, _sector, 1)

/** @brief @internal initializer for a cbmimage_i_bam_selector, used as a BAM entry
 *
 * This macro can be used like \n
 * cbmimage_i_bam_selector selector = CBMIMAGE_BAM_SELECTOR_INIT(starttrack, startoffset, multiplier, data_count, track, sector) \n
 * in order to initialize the selector
 *
 * @param[in] _starttrack
 *   the number of the first track which is described in this BAM selector
 *
 * @param[in] _startoffset
 *   the offset inside of the block where this BAM entry is located
 *
 * @param[in] _multiplier
 *    multiplier for the BAM entries; that is, how many byte are the different tracks aways from each other?
 *
 * @param[in] _data_count
 *    the number of byte that form one BAM entry. This is restricted to a maximum of BAM_MASK_COUNT.
 *    In case of a cbmimage_i_bam_counter_selector, this must be 0.
 *
 * @param[in] _track
 *   the track where this BAM is located
 *
 * @param[in] _sector
 *   the sector where this BAM is located
 *
 * @remark
 *   This macro is only used if the BAM does not have the regular outline. \n
 *   Regularly, the BAM looks as follows: [number of free blocks on this track] [bitmap for this track] \n
 *   \n
 *   If the BAM looks that way, use CBMIMAGE_BAM_AND_BAM_COUNTER_CREATE() instead!
 *
 */
#define CBMIMAGE_BAM_SELECTOR_INIT(_starttrack, _startoffset, _multiplier, _data_count, _track, _sector) \
	CBMIMAGE_I_BAM_SELECTOR_INIT(_starttrack, _startoffset, _multiplier, _data_count, _track, _sector, 0)

/** @brief @internal initializer for a cbmimage_i_bam_selector, used as a BAM counter entry
 *
 * This macro can be used like \n
 * cbmimage_i_bam_selector selector = CBMIMAGE_BAM_COUNTER_SELECTOR_INIT(starttrack, startoffset, multiplier, track, sector) \n
 * in order to initialize the selector
 *
 * @param[in] _starttrack
 *   the number of the first track which is described in this BAM selector
 *
 * @param[in] _startoffset
 *   the offset inside of the block where this BAM entry is located
 *
 * @param[in] _multiplier
 *    multiplier for the BAM entries; that is, how many byte are the different tracks aways from each other?
 *
 * @param[in] _track
 *   the track where this BAM is located
 *
 * @param[in] _sector
 *   the sector where this BAM is located
 *
 * @remark
 *   This macro is only used if the BAM does not have the regular outline. \n
 *   Regularly, the BAM looks as follows: [number of free blocks on this track] [bitmap for this track] \n
 *   \n
 *   If the BAM looks that way, use CBMIMAGE_BAM_AND_BAM_COUNTER_CREATE() instead!
 *
 */
#define CBMIMAGE_BAM_COUNTER_SELECTOR_INIT(_starttrack, _startoffset, _multiplier, _track, _sector) \
	CBMIMAGE_BAM_SELECTOR_INIT(_starttrack, _startoffset, _multiplier, 0, _track, _sector)

/** @brief @internal initialize two cbmimage_i_bam_selector, one for the BAM and one for the BAM counter entry
 *
 * This macro assumes a special outline of the data. \n
 * Essentially, in _basename, there must be both a .bam[] and a .bam_counter[] array, and they must be indexed
 * in the same way. That is, .bam[x] and .bam_counter[x] must correspond to each other.
 *
 * In this case, this macro creates both the BAM selector and the BAM_COUNTER selector
 *
 * @param[in] _basename
 *   The name of the variable which holds a .bam[] and a .bam_counter[] array
 *
 * @param[in] _index
 *   The index into the .bam[] and .bam_counter[] arrays which will be set
 *
 * @param[in] _starttrack
 *   the number of the first track which is described in this BAM selector
 *
 * @param[in] _startoffset
 *   the offset inside of the block where this BAM entry is located.
 *   Note the remark below on the assumed outline.
 *
 * @param[in] _multiplier
 *    multiplier for the BAM entries; that is, how many byte are the different tracks aways from each other?
 *
 * @param[in] _data_count
 *    the number of byte that form one BAM entry. This is restricted to a maximum of BAM_MASK_COUNT.
 *    In case of a cbmimage_i_bam_counter_selector, this must be 0.
 *
 * @param[in] _track
 *   the track where this BAM is located
 *
 * @param[in] _sector
 *   the sector where this BAM is located
 *
 * @remark
 *   This macro can only used if the BAM has the regular outline. \n
 *   That is, at _startoffset, there is the number of free blocks on this
 *   track, and at _startoffset + 1, the bitmap starts. \n
 *
 *   If the BAM does not look that way, use CBMIMAGE_BAM__SELECTOR_INIT() and
 *   CBMIMAGE_BAM_COUNTER_SELECTOR_INIT() instead!
 *
 */
#define CBMIMAGE_BAM_AND_BAM_COUNTER_CREATE(_basename, _index, _starttrack, _startoffset, _multiplier, _data_count, _track, _sector) \
	_basename.bam[_index]         = CBMIMAGE_BAM_SELECTOR_INIT(        _starttrack, _startoffset + 1, _multiplier, _data_count, _track, _sector), \
	_basename.bam_counter[_index] = CBMIMAGE_BAM_COUNTER_SELECTOR_INIT(_starttrack, _startoffset,     _multiplier,              _track, _sector)

/** @brief BAM counter selector
 * @ingroup cbmimage_bam @internal
 *
 * A bam_counter selector is almost the same as a BAM selector.
 * Instead of pointing to the bit map of the BAM, though, it
 * points to the location where the number of free blocks on the
 * track is located.
 *
 * In most cases, these are located directly before the bitmap, so the BAM selector could be used.
 * However, in the case of the 2nd side of a D71 image, this is not true; that's why this own type
 * of selector is needed here.
 *
 * @remarks
 *   - see also cbmimage_i_bam_selector_s
 *   - data_count is unused and must be 0.
 *
 */
typedef cbmimage_i_bam_selector cbmimage_i_bam_counter_selector;

/** @brief BAM counter selector
 * @ingroup cbmimage_bam @internal
 *
 * this type contains helper functions that perform image format specific
 * operations on an image type
 */
typedef
struct cbmimage_fileimage_functions_s {

	/// pointer to image specific function to get number of sectors in the specific track
	/// If the pointer is NULL, then the number of sectors in all tracks are the same.
	cbmimage_get_sectors_in_track_fct * get_sectors_in_track;

	/// pointer to image specific function to convert T/S to LBA block address
	/// If the pointer is NULL, then the number of sectors in all tracks are the same.
	/// In this case, a simplified conversion is used.
	cbmimage_ts_to_blockaddress_fct * ts_to_blockaddress;

	/// pointer to image specific function to convert LBA to T/S block address
	/// If the pointer is NULL, then the number of sectors in all tracks are the same.
	/// In this case, a simplified conversion is used.
	cbmimage_lba_to_blockaddress_fct * lba_to_blockaddress;

	/// pointer to image specific function to change to a sub-dir
	/// If the pointer is NULL, then chdir is not supported with this image type
	cbmimage_chdir_fct * chdir;

	/// pointer to image specific function to fill the BAM for unused
	/// blocks, that are marked anyway. \n
	/// This is used for D71 images which occupy track 53 (2nd directory track)
	/// and for a directory/partition.
	/// The pointer can be NULL if no special handling of BAM entries is needed.
	cbmimage_set_bam_fct * set_bam;

} cbmimage_fileimage_functions;

/** @brief Image specific settings for D40, D64 and D71 images
 * @ingroup cbmimage_image @internal
 *
 * this type contains data that is specific to the D64, D71 and D40 images
 * (which are almost identical)
 *
 * The main contents are sectors_in_track and track_lba_start. Because
 * these images contain different number of sectors on a track, it is easier
 * to handle it via these arrays instead of calculating it every time it is
 * needed
 */
typedef
struct cbmimage_i_d40_d64_d71_image_settings_s {

	/// number of sectors in a track
	const uint8_t * sectors_in_track;

	/// the LBA of the start of each track
	uint16_t track_lba_start[1 + 70];

	/** the location of the BAM. At most two entries are needed (D71, D40 with 40
	 * or 42 tracks)
	 */
	cbmimage_i_bam_selector bam[2];

	/** the location of the BAM counter. At most two entries are needed (D71, D40
	 * with 40 or 42 tracks)
	 */
	cbmimage_i_bam_counter_selector bam_counter[2];

} cbmimage_i_d40_d64_d71_image_settings;

/** @brief Image specific settings for D81 images
 * @ingroup cbmimage_image @internal
 *
 * this type contains data that is specific to the D81 images
 */
typedef
struct cbmimage_i_d81_image_settings_s {
	/// the location of the BAM. At most two entries are needed (D71, D40 with 40 or 42 tracks)
	cbmimage_i_bam_selector bam[2];

	/// the location of the BAM counter. At most two entries are needed (D71, D40 with 40 or 42 tracks)
	cbmimage_i_bam_counter_selector bam_counter[2];

} cbmimage_i_d81_image_settings;

/** @brief Image specific settings for D80 and D82 images
 * @ingroup cbmimage_image @internal
 *
 * this type contains data that is specific to the D80 and D82 images
 * (which are almost identical)
 *
 * The main contents are sectors_in_track and track_lba_start. Because
 * these images contain different number of sectors on a track, it is easier
 * to handle it via these arrays instead of calculating it every time it is
 * needed
 */
typedef
struct cbmimage_i_d80_d82_image_settings_s {

	/// number of sectors in a track
	const uint8_t * sectors_in_track;

	/// the LBA of the start of each track
	uint16_t track_lba_start[1 + 154];

	/// the location of the BAM. At most two entries are needed (D71, D40 with 40 or 42 tracks)
	cbmimage_i_bam_selector bam[4];

	/// the location of the BAM counter. At most two entries are needed (D71, D40 with 40 or 42 tracks)
	cbmimage_i_bam_counter_selector bam_counter[4];

} cbmimage_i_d80_d82_image_settings;

/** @brief Image specific settings for CMD D1M, D2M and D4M images
 * @ingroup cbmimage_image @internal
 *
 * this type contains data that is specific to the D1M, D2M or D4M images
 */
typedef
struct cbmimage_i_d1m_d2m_d4m_image_settings_s {

	/** the location of the BAM. At most two entries are needed (D71, D40 with 40
	 * or 42 tracks)
	 */
	cbmimage_i_bam_selector bam[2];

	/** the location of the BAM counter. At most two entries are needed (D71, D40
	 * with 40 or 42 tracks)
	 */
	cbmimage_i_bam_counter_selector bam_counter[2];

} cbmimage_i_d1m_d2m_d4m_image_settings;

/** @brief Image specific settings for CMD DNP images
 * @ingroup cbmimage_image @internal
 *
 * this type contains data that is specific to the DNP images
 */
typedef
struct cbmimage_i_dnp_image_settings_s {

	/// the location of the BAM. With DNP, we always have 32 entries
	cbmimage_i_bam_selector bam[32];

} cbmimage_i_dnp_image_settings;

typedef struct cbmimage_image_settings_s cbmimage_image_settings;

/** @brief Image specific settings for all types of images
 * @ingroup cbmimage_image @internal
 *
 * this type contains data that is specific to each image
 * type, but that is needed from all types.
 *
 * This type is subject to change and not part of the API.
 */
typedef
struct cbmimage_image_settings_s {

	/// functions for image type specific operations
	cbmimage_fileimage_functions fct;

	/// back-pointer for nested images (directorys, partitions)
	cbmimage_image_settings * next_settings;

	/// pointer back to the cbmimage_fileimage that contains a pointer to here
	cbmimage_fileimage * image;

	/// the file type of the image
	cbmimage_imagetype imagetype;

	/// the name (user readable text) for the file type of the image
	const unsigned char * imagetype_name;

	/** accessor for the info block. \n
	 * On some images (e.g., D64, D71 and D40), this is identical to the BAM block. \n
	 * On others, it is not.
	 */
	cbmimage_blockaccessor * info;

	/// if there is a calculated FAT, this points to it. If there is none, it is NUL.
	cbmimage_fat * fat;

	/// the offset of the diskname that is stored in the info block
	uint8_t info_offset_diskname;

	/// address of the (first) directory block
	cbmimage_blockaddress dir;

	/// maximum number of tracks (e.g. 35 if track go from 1 to 35)
	uint8_t maxtracks;

	/** maximum number of sectors (e.g. 21 if sectors go from 0 to 20). Note that
	 * the number of sectors in a specific track can be lower than this.
	 */
	uint16_t maxsectors;

	/// the number of bytes in a block
	uint16_t bytes_in_block;


	/** the track(s) where the directory is located. If there is only 1 track,
	 * the other must be 0. \n
	 * The tracks must be numerically sorted, filling the
	 * remaining entries with a 0.
	 */
	uint8_t dir_tracks[2];

	/// address of the last block on this image
	cbmimage_blockaddress lastblock;

	/**
	 * 0 if side-sector blocks are normal side-sector blocks \n
	 * 1 if there is a super side-sector block instead (i.e. 1581)
	 */
	int has_super_sidesector;

	/// the number of bam and bam_counter entries
	size_t bam_count;

	/// the location of the BAM.
	cbmimage_i_bam_selector * bam;

	/// the location of the BAM counter.
	cbmimage_i_bam_counter_selector * bam_counter;

	/** @brief data offset for subdir
	 *
	 * For subdirs/partitions that are handled as part of an absolute section on the image
	 * (e.g., FD2000/FD4000 D64, D71 or D81 partitions), this offset is added in order to
	 * get the address of a block in the partition.
	 *
	 * If there is no subdir or no subdir is active, this must be 0.
	 */
	size_t subdir_data_offset;

	/// in case of an active sub-dir / sub-partition, this marks the start block of that partition
	cbmimage_blockaddress block_subdir_first;

	/// in case of an active sub-dir / sub-partition, this marks the last block of that partition
	cbmimage_blockaddress block_subdir_last;

	/** @brief flag if there is an active subdir with absolute addresses
	 *
	 * If this is 1, then subdirs/partitions are processed with absolute
	 * addresses (1581 style).
	 * That is: If the subdir starts at 3/0, then in the subdir, this block is
	 * named 3/0 and the link to the next block is 3/1.
	 *
	 * If no subdir/partition is active, then this must be 0.
	 */
	unsigned int subdir_global_addressing : 1;

	/** @brief flag if there is an active subdir with relative addresses
	 *
	 * If this is != 0, there is an active subdir/partition that is processed with relative
	 * addresses (CMD Fdx000 style).
	 * That is: If the subdir starts at 3/0, then in the subdir, this block is
	 * named 1/0 and the link to the next block is 1/1.
	 *
	 * If no subdir/partition is active, this must be 0.
	 */
	unsigned int subdir_relative_addressing : 1;

	/** @brief Define if the current "partition" is the partition table
	 *
	 * For partitions (CMD FD2000, FD4000), the partition table is handled
	 * as a directory. Thus, the directory processing functions must
	 * know if they handle the partition table (== this is set to 1) or not (== 0)
	 */
	unsigned int is_partition_table : 1;

	/** @brief set if this is a GEOS disk */
	unsigned int is_geos : 1;

	/// block of the GEOS border block.
	cbmimage_blockaddress geos_border;

	union {

		/// image specific settnigs for D64, D71 and D40 files
		cbmimage_i_d40_d64_d71_image_settings d40_d64_d71;

		/// image specific settnigs for D81 files
		cbmimage_i_d81_image_settings d81;

		/// image specific settnigs for D80 and D82 files
		cbmimage_i_d80_d82_image_settings d80_d82;

		/// image specific settnigs for D1M, D2M and D4M files
		cbmimage_i_d1m_d2m_d4m_image_settings d1m_d2m_d4m;

		/// image specific settnigs for DNP (CMD native partition) files
		cbmimage_i_dnp_image_settings dnp;

	}; ///< image specific settings
} cbmimage_image_settings;

/** @brief Parameter of the image
 * @ingroup cbmimage_image @internal
 *
 * this type contains data that is specific to the image
 * at hand.
 *
 * It is an own type, so the user cannot accidentially access it.
 *
 * This type is subject to change and not part of the API.
 */
typedef
struct cbmimage_image_parameter_s {

	/// size of the image
	size_t size;

	/// the filename as given by the user. May contain paths
	char * filename;

	/// the raw buffer of the disk image
	uint8_t * buffer;

	/// if non-null, contains a pointer to the error buffer
	uint8_t * errormap;

} cbmimage_image_parameter;

void cbmimage_i_d40_image_open(cbmimage_fileimage * image);
void cbmimage_i_d64_image_open(cbmimage_fileimage * image);
void cbmimage_i_d64_40track_image_open(cbmimage_fileimage * image);
void cbmimage_i_d64_40track_speeddos_image_open(cbmimage_fileimage * image);
void cbmimage_i_d64_40track_dolphin_image_open(cbmimage_fileimage * image);
void cbmimage_i_d64_40track_prologic_image_open(cbmimage_fileimage * image);
void cbmimage_i_d64_42track_image_open(cbmimage_fileimage * image);
void cbmimage_i_d71_image_open(cbmimage_fileimage * image);
void cbmimage_i_d81_image_open(cbmimage_fileimage * image);
void cbmimage_i_d80_image_open(cbmimage_fileimage * image);
void cbmimage_i_d82_image_open(cbmimage_fileimage * image);
void cbmimage_i_d1m_image_open(cbmimage_fileimage * image);
void cbmimage_i_d2m_image_open(cbmimage_fileimage * image);
void cbmimage_i_d4m_image_open(cbmimage_fileimage * image);
#if 0
void cbmimage_i_dnp_image_open(cbmimage_fileimage * image);
#endif

uint8_t * cbmimage_i_get_address_of_block(cbmimage_fileimage * image, cbmimage_blockaddress block);

void cbmimage_i_init_bam_selector(cbmimage_image_settings * settings, cbmimage_i_bam_selector * selector, size_t selector_count);


/** @brief init a BAM Counter selector
 *
 * As a BAM counter selector (cbmimage_i_bam_counter_selector,
 * cbmimage_i_bam_counter_selector_s) is the same type as a BAM selector
 * (cbmimage_i_bam_selector, cbmimage_i_bam_selector_s) but used differently,
 * just use the same function to initialize it. Thus, a macro does the job
 *
 * @param[in] _settings
 *    pointer to the fileimage settings
 *
 * @param[inout] _selector
 *    pointer to an array of selectors to initialize
 *
 * @param[in] _selector_count
 *    the number of selectors in the array _selector
 *
 * @remark
 *    if there is only one selector, use a  _selector_count of 1.
 */
#define cbmimage_i_init_bam_counter_selector(_settings, _selector, _selector_count) \
	cbmimage_i_init_bam_selector(_settings, _selector, _selector_count)

void cbmimage_i_create_last_block(cbmimage_fileimage * image);

void cbmimage_i_print(const char * text);
void cbmimage_i_fmt_print(const char * fmt, ...);

cbmimage_dir_entry * cbmimage_i_dir_get_clone(cbmimage_dir_entry * dir_entry);
int cbmimage_i_bam_check_really_unused(cbmimage_image_settings * settings, cbmimage_blockaddress block);

int cbmimage_i_dir_get_partition_data(cbmimage_dir_entry * dir_entry, cbmimage_blockaddress * block_first, cbmimage_blockaddress * block_last, size_t * block_count);


int cbmimage_i_blockaccessor_release(cbmimage_blockaccessor * accessor);

int cbmimage_i_dir_set_subpartition_global(cbmimage_image_settings * settings, cbmimage_blockaddress block_subdir_first, size_t block_count);
int cbmimage_i_dir_set_subpartition_relative(cbmimage_image_settings * settings, cbmimage_blockaddress block_subdir_first, cbmimage_blockaddress block_subdir_last);

int cbmimage_i_dnp_chdir_partition_init(cbmimage_image_settings * settings);
int cbmimage_i_d64_chdir_partition_init(cbmimage_image_settings * settings);
int cbmimage_i_d71_chdir_partition_init(cbmimage_image_settings * settings);
int cbmimage_i_d81_chdir_partition_init(cbmimage_image_settings * settings);

int cbmimage_i_validate_1581_partition(cbmimage_fileimage * image, cbmimage_blockaddress block_start, int count);

#endif // #ifndef CBMIMAGE_INTERNAL_H
