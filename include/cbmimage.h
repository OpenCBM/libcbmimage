/** @file include/cbmimage.h \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage API and their definitions
 *
 */
#ifndef CBMIMAGE_H
#define CBMIMAGE_H 1

#include <stddef.h>
#include <stdint.h>

/* BAM handling */

/** @brief state of the BAM
 * @ingroup cbmimage_bam
 */
typedef
enum cbmimage_BAM_state_e {
	BAM_UNKNOWN,          ///< BAM state of block is unknown
	BAM_REALLY_FREE,      ///< BAM state of block is "free", and the block does not contain any data
	BAM_FREE,             ///< BAM state of block is "free", but the block contains some data
	BAM_USED,             ///< BAM state of block is "used"
	BAM_DOES_NOT_EXIST    ///< the block does not exist
} cbmimage_BAM_state;


/** @brief Address on floppy: Track/Sector variant
 * @ingroup blockaddress
 *
 * see also cbmimage_lba
 */
typedef
struct cbmimage_track_sector_s {
	uint8_t track;        ///< the track of the block; if track is 0, this T/S is invalid
	uint8_t sector;       ///< the sector of the block
} cbmimage_track_sector;

/** @brief initializer for a cbmimage_track_sector
 *
 * This macro can be used like \n
 * cbmimage_track_sector var = CBMIMAGE_TS_INIT(track, sector) \n
 * in order to initialize the var.
 *
 * @param[in] _track
 *   the track to use as initializer
 *
 * @param[in] _sector
 *   the sector to use as initializer
 *
 */
#define CBMIMAGE_TS_INIT(_track, _sector) \
	{ .track = (_track), .sector = (_sector) }

/** @brief set a cbmimage_track_sector
 *
 * This macro can be used like \n
 * CBMIMAGE_TS_INIT(var, track, sector) \n
 * in order to initialize the var with track and sector
 *
 * @param[inout] _ts
 *   The variable which is modified
 *
 * @param[in] _track
 *   the track to use as initializer
 *
 * @param[in] _sector
 *   the sector to use as initializer
 *
 */
#define CBMIMAGE_TS_SET(_ts, _track, _sector) \
	do { (_ts).track = (_track); (_ts).sector = (_sector); } while (0)

/** @brief clear a cbmimage_track_sector
 *
 * Clearing a track/sectors means to put it into an "invalid" state
 *
 * This macro can be used like \n
 * CBMIMAGE_TS_CLEAR(var) \n
 * in order to initialize the var with track and sector
 *
 * @param[inout] _ts
 *   The variable which is modified
 *
 */
#define CBMIMAGE_TS_CLEAR(_ts) \
	CBMIMAGE_TS_SET((_ts), 0, 0)

/** @brief Address on floppy: LBA variant
 * @ingroup blockaddress
 *
 * If the value is 0, this LBA is invalid
 *
 * see also cbmimage_track_sector, struct cbmimage_track_sector_s
 */
typedef uint16_t cbmimage_lba;

/** @brief Address on drive
 * @ingroup blockaddress
 *
 * @remark Both variants (LBA and TS) must be valid at all times
 */
typedef
struct cbmimage_blockaddress_s {
	cbmimage_track_sector ts;    ///< Track/Sector style address
	cbmimage_lba          lba;   ///< LBA style address
} cbmimage_blockaddress;

extern const cbmimage_blockaddress cbmimage_block_unused;

/** @brief initializer for a cbmimage_blockaddress
 *
 * This macro can be used like \n
 * cbmimage_blockaddress var = CBMIMAGE_BLOCK_INIT(track, sector, lba) \n
 * in order to initialize the var.
 *
 * @param[in] _track
 *   the track to use as initializer
 *
 * @param[in] _sector
 *   the sector to use as initializer
 *
 * @param[in] _lba
 *   the LBA to use as initializer
 *
 */
#define CBMIMAGE_BLOCK_INIT(_track, _sector, _lba) \
 { .lba = (_lba), .ts = CBMIMAGE_TS_INIT((_track), (_sector)) }

/** @brief initializer for a cbmimage_blockaddress
 *
 * This macro can be used like \n
 * cbmimage_blockaddress var = CBMIMAGE_BLOCK_INIT_FROM_TS(track, sector) \n
 * in order to initialize the var.
 *
 * @param[in] _track
 *   the track to use as initializer
 *
 * @param[in] _sector
 *   the sector to use as initializer
 *
 * @remark
 *   The LBA part is set to 0, marking that it is still not initialized.
 *
 */
#define CBMIMAGE_BLOCK_INIT_FROM_TS(_track, _sector) \
	CBMIMAGE_BLOCK_INIT((_track), (_sector), 0)

/** @brief initializer for a cbmimage_blockaddress
 *
 * This macro can be used like \n
 * cbmimage_blockaddress var = CBMIMAGE_BLOCK_INIT_FROM_LBA(lba) \n
 * in order to initialize the var.
 *
 * @param[in] _lba
 *   the LBA to use as initializer
 *
 * @remark
 *   The track and sector part is set to 0, marking that it is still not
 *   initialized.
 *
 */
#define CBMIMAGE_BLOCK_INIT_FROM_LBA(_lba) \
	CBMIMAGE_INIT_BLOCK(0, 0, (_lba))

/** @brief initializer for a cbmimage_blockaddress
 *
 * This macro can be used like \n
 * CBMIMAGE_BLOCK_SET_FROM_TS(image, block, track, sector) \n
 * in order to initialize the var.
 *
 * @param[in] _image
 *    pointer to the image data
 *
 * @param[inout] _block
 *    the cbmimage_blockaddress to be modified
 *
 * @param[in] _track
 *   the track to use as initializer
 *
 * @param[in] _sector
 *   the sector to use as initializer
 *
 * @remark
 *   The LBA part is calculated from the _track and _sector values
 *
 */
#define CBMIMAGE_BLOCK_SET_FROM_TS(_image, _block, _track, _sector) \
	do { \
		CBMIMAGE_TS_SET((_block).ts, (_track), (_sector)); \
		cbmimage_blockaddress_init_from_ts((_image), &(_block)); \
	} while (0)

/** @brief initializer for a cbmimage_blockaddress
 *
 * This macro can be used like \n
 * CBMIMAGE_BLOCK_SET_FROM_LBA(image, block, lba) \n
 * in order to initialize the var.
 *
 * @param[in] _image
 *    pointer to the image data
 *
 * @param[inout] _block
 *    the cbmimage_blockaddress to be modified
 *
 * @param[in] _lba
 *   the LBA to use as initializer
 *
 * @remark
 *   The track and sector values are calculated from the _lba value
 *
 */
#define CBMIMAGE_BLOCK_SET_FROM_LBA(_image, _block, _lba) \
	do { \
		(_block).lba = (_lba); \
		cbmimage_blockaddress_init_from_lba((_image), &(_block)); \
	} while (0)

/* directory handling */

/** @brief The type of a directory entry cbmimage_dir_entry_s
 * @ingroup cbmimage_dir
 *
 */
typedef
enum cbmimage_dir_type_e {
	DIR_TYPE_DEL        = 0,     ///< this directory entry is for a file that is deleted
	DIR_TYPE_SEQ        = 1,     ///< this directory entry is for a SEQuential file
	DIR_TYPE_PRG        = 2,     ///< this directory entry is for a PRogRamm  file
	DIR_TYPE_USR        = 3,     ///< this directory entry is for a USeR file
	DIR_TYPE_REL        = 4,     ///< this directory entry is for a RELative file
	DIR_TYPE_PART1581   = 5,     ///< this directory entry is for a 1581 style partition file (D81, D1M, D2M, D4M only)
	DIR_TYPE_CMD_NATIVE = 6,     ///< this directory entry is for a CMD NATive partition (D1M, D2M, D4M only)

	DIR_TYPE_PART_OFFSET     = 0x100,
	DIR_TYPE_PART_NO         = DIR_TYPE_PART_OFFSET,
	DIR_TYPE_PART_CMD_NATIVE = DIR_TYPE_PART_OFFSET + 0x01,
	DIR_TYPE_PART_D64        = DIR_TYPE_PART_OFFSET + 0x02,
	DIR_TYPE_PART_D71        = DIR_TYPE_PART_OFFSET + 0x03,
	DIR_TYPE_PART_D81        = DIR_TYPE_PART_OFFSET + 0x04,
	DIR_TYPE_PART_SYSTEM     = DIR_TYPE_PART_OFFSET + 0xFF,
} cbmimage_dir_type;

/** @brief The GEOS type of a directory entry cbmimage_dir_entry_s
 * @ingroup cbmimage_dir
 *
 * @remark The GEOS type is GEOS_FILETYPE_NON_GEOS if no GEOS disk was found!
 */
typedef
enum cbmimage_geos_filetype_e {
	GEOS_FILETYPE_NON_GEOS          = 0x00, ///< no GEOS file
	GEOS_FILETYPE_BASIC             = 0x01, ///< a BASIC file
	GEOS_FILETYPE_ASSEMBLER         = 0x02, ///< an assembler file
	GEOS_FILETYPE_DATA_FILE         = 0x03, ///< a data file
	GEOS_FILETYPE_SYSTEM_FILE       = 0x04, ///< a system file
	GEOS_FILETYPE_DESK_ACCESSORY    = 0x05, ///< a file for a GEOS desk accessory
	GEOS_FILETYPE_APPLICATION       = 0x06, ///< a file for a GEOS application
	GEOS_FILETYPE_APPLICATION_DATA  = 0x07, ///< some user-created document from a GEOS application or desk accessory
	GEOS_FILETYPE_FONT_FILE         = 0x08, ///< a GEOS font
	GEOS_FILETYPE_PRINTER_DRIVER    = 0x09, ///< a GEOS printer driver
	GEOS_FILETYPE_INPUT_DRIVER      = 0x0A, ///< a GEOS input driver
	GEOS_FILETYPE_DISK_DRIVER       = 0x0B, ///< a GEOS disk driver or disk device
	GEOS_FILETYPE_SYSTEM_BOOT_FILE  = 0x0C, ///< a GEOS system boot file
	GEOS_FILETYPE_TEMPORARY         = 0x0D, ///< a GEOS temporary file. Will be deleted on termination of the program
	GEOS_FILETYPE_AUTO_EXECUTE_FILE = 0x0E, ///< a GEOS auto-execute file
} cbmimage_geos_filetype;

/** @brief The name of a directory header or directory entry
 * @ingroup cbmimage_dir
 *
 * This entry is used in cbmimage_dir_header as well as cbmimage_dir_entry.
 * It describes the name of the header or directory that is to be processed.
 */
typedef
struct cbmimage_dir_header_name_s {

	/** the index of the end of the file/header name */
	int end_index;

	/** the total length of the name of the file/header */
	int length;

	/** the name of the file/header as taken from the directory entry. No
	 * processing (for example, PETSCII conversion) is performed!
	 */
	const char text[24];

} cbmimage_dir_header_name;

/** @brief A directory header
 * @ingroup cbmimage_dir
 *
 * This entry has all the data that is needed to handle the image header and the count of free blocks
 *
 * see also:
 * cbmimage_dir_get_first, cbmimage_dir_get_next, cbmimage_dir_get_close, cbmimage_dir_is_deleted
 */
typedef
struct cbmimage_dir_header_s {

	/// the name of the image with some extra info
	cbmimage_dir_header_name name;

	/// the count of free blocks on this image
	uint16_t free_block_count;

	/// if set to 1, this is a GEOS file
	unsigned int is_geos : 1;

} cbmimage_dir_header;

/** @brief A directory entry
 * @ingroup cbmimage_dir
 *
 * The functions that enumerate directory entrys all operate on this entry type.
 *
 * see also:
 * cbmimage_dir_get_first, cbmimage_dir_get_next, cbmimage_dir_get_close, cbmimage_dir_is_deleted
 */
typedef
struct cbmimage_dir_entry_s {

	/// the name of the image with some extra info
	cbmimage_dir_header_name name;

	/// type of this directory entry (DEL, SRQ, PRG, USR, REL, ...)
	cbmimage_dir_type type;

	/** if set to 1, this file is locked. That is, it cannot be changed or
	 * deleted. The CBM DOS shows such files with a "<" (i.e., "PRG<")
	 */
	unsigned int is_locked : 1;

	/** if set to 1 (normal), this file is closed correctly. If set to 0, the
	 * file is not (yet?) closed. The CBM DOS shows such files with a "*" (i.e.,
	 * "*PRG")
	 */
	unsigned int is_closed : 1;

	/// if set to 1, this file has valid year, month, day, hour and minute entries
	unsigned int has_datetime : 1;

	/// if set to 1, this is a GEOS file
	unsigned int is_geos : 1;

	/// if is_geos and this is set to 1, this is a GEOS VLIR file
	unsigned int geos_is_vlir : 1;

	/// if the directory entry is valid, this is 1
	unsigned int is_valid : 1;

	/** the start of the file on the disk. Note that for GEOS VLIR files
	 * (is_geos_vlir is set to 1), this contains the record map, which itself
	 * points to the real starts of the streams
	 */
	cbmimage_blockaddress start_block;

	/** if type is DIR_TYPE_REL, this points to the first sidesektor block on the
	 * disk. Note that in case of 1581 and other drives, this is a SUPER SIDE
	 * SECTOR BLOCK
	 */
	cbmimage_blockaddress rel_sidesector_block;

	/// if type is DIR_TYPE_REL, this is the record length of the REL file
	uint16_t rel_recordlength;

	/// the count of occupied blocks on disk
	uint16_t block_count;


	/// if has_datetime is 1, the year of the file creation or last modification.
	uint16_t year;

	/// if has_datetime is 1, the month of the file creation or last modification.
	uint8_t month;

	/// if has_datetime is 1, the day of the file creation or last modification.
	uint8_t day;

	/// if has_datetime is 1, the hour of the file creation or last modification.
	uint8_t hour;

	/// if has_datetime is 1, the minute of the file creation or last modification.
	uint8_t minute;


	/// if is_geos is 1, this contains a pointer to the GEOS infoblock
	cbmimage_blockaddress geos_infoblock;

	/// if is_geos is 1, this is the GEOS filtype
	cbmimage_geos_filetype geos_filetype;

} cbmimage_dir_entry;

/* disk image handling */

/** @brief The type of the libcbmimage image
 * @ingroup cbmimage_image
 *
 * libcbmimage can handle different types of disk images.
 * This enum contains all the type the libcbmimage can process.
 */
typedef
enum cbmimage_imagetype_e {
	TYPE_UNKNOWN,               ///< the image type is unknown. In this case, libcbmimage tries to determine the type itself.
	TYPE_D40,                   ///< 2040/3040 5,25" SS format (similar to D64, but 20 sectors on track 18-24)
	TYPE_D64,                   ///< 2031/1540/1541/1570 5,25" SS file format
	TYPE_D64_40TRACK,           ///< 1541 5,25" SS file format with 40 tracks
	TYPE_D64_40TRACK_SPEEDDOS,  ///< 1541 5,25" SS file format with 40 tracks, SPEED DOS variant
	TYPE_D64_40TRACK_DOLPHIN,   ///< 1541 5,25" SS file format with 40 tracks, Dolphin DOS variant
	TYPE_D64_40TRACK_PROLOGIC,  ///< 1541 5,25" SS file format with 40 tracks, Prologic DOS variant
	TYPE_D64_42TRACK,           ///< 1541 5,25" SS file format with 42 tracks
	TYPE_D71,                   ///< 1571 5,25" DS file format
	TYPE_D81,                   ///< 1581 3,5" file format
	TYPE_D80,                   ///< 8050 5,25" SS file format
	TYPE_D82,                   ///< 8250 5,25" DS file format
//	TYPE_D60,                   ///< 9060 file format
//	TYPE_D90,                   ///< 9090 file format
//	TYPE_D16,                   ///< Ruud Baltissen's own D16 format (tracks 1-255, sectors 0-255 per track)
	TYPE_CMD_D1M,               ///< CMD FD2000/FD4000 800 KB file format
	TYPE_CMD_D2M,               ///< CMD FD2000/FD4000 1,6 MB file format
	TYPE_CMD_D4M,               ///< CMD FD4000 3,2 MB file format
	TYPE_CMD_NATIVE,            ///< CMD Native Partition file format
	TYPE_LAST                   ///< not a type, just an end marker
} cbmimage_imagetype;

/* disk image handling */

struct cbmimage_image_parameter_s;
struct cbmimage_image_settings_s;

/** @brief Type that describes a CBM disk image on which to operate
 * @ingroup cbmimage_image
 *
 */
typedef
struct cbmimage_fileimage_s {

	/// internal information; do not change or access
	struct cbmimage_image_settings_s * settings;

	/// internal information: the settings of the image itself (w/o subdirs)
	struct cbmimage_image_settings_s * global_settings;

	/// internal parameter of this image
	struct cbmimage_image_parameter_s * parameter;

	/** some additional space for the various data. Don't make any assumptions
	 * about the contents or the layout here!
	 */
	uint8_t bufferarray[];

} cbmimage_fileimage;

/** @brief cbmimage loop detector struct
 * @ingroup cbmimage_loop
 *
 * In order to recognize loops in the T/S links, the cbmimage_loop_*()
 * functions use this structure to mark blocks already visited.
 *
 */
typedef
struct cbmimage_loop_s {

	/// the fileimage to which this loop detector belongs
	cbmimage_fileimage * image;

	/// @internal map that marks already visited blocks
	void * map;

	/// @internal size of the map.
	size_t map_size;

	/** some additional space for the various data. Don't make any assumptions
	 * about the contents or the layout here!
	 */
	uint8_t bufferarray[];

} cbmimage_loop;


/** @brief cbmimage block accessor data structure
 * @ingroup cbmimage_blockaccessor
 *
 * In order to access blocks, this data structure can be used.
 * For small images, it gives back pointers to the in-memory
 * copy of a block.
 *
 * For bigger images, it accesses the on-disk image and gives
 * a copy of it somewhere in memory.
 *
 */
typedef
struct cbmimage_blockaccessor_s {

	/// the image which this accessor handles
	cbmimage_fileimage * image;

	/// the (current) block address of the block of this accessor
	cbmimage_blockaddress  block;

	/// pointer to the data buffer where the block resides
	uint8_t * data;

} cbmimage_blockaccessor;

/* FAT handling */

/** @brief cbmimage One entry of the FAT structure
 * @ingroup cbmimage_fat
 *
 * This structure holds the entry for one block inside of
 * the file allocaton table (FAT) into the image, build from
 * the T/S links at the beginning of each block.
 *
 * It helps in fast navigation inside of an image.
 *
 */
typedef
struct cbmimage_fat_entry_s {
	/// the LBA of where the next block resides
	uint16_t lba;

} cbmimage_fat_entry;

/** @brief cbmimage FAT structure
 * @ingroup cbmimage_fat
 *
 * This structure holds a file allocaton table (FAT) into the image, build from
 * the T/S links at the beginning of each block.
 *
 * It helps in fast navigation inside of an image.
 *
 */
typedef
struct cbmimage_fat_s {

	/// the FAT entries for each LBA block inside of this image.
	cbmimage_fat_entry *entry;

	/// the image data
	cbmimage_fileimage *image;

	/// the number of elements in this FAT
	size_t elements;

	/** some additional space for the various data. Don't make any assumptions
	 * about the contents or the layout here!
	 */
	uint8_t bufferarray[];

} cbmimage_fat;

/* file handling */

/** @brief type to describe a chain that is followed
 * @ingroup cbmimage_chain
 */
typedef
struct cbmimage_chain_s {
	/** The image on which this chain is followed */
	cbmimage_fileimage *   image;

	/** start starting block of this chain. \n
	 * This is never changed after the chain has started.
	 * It can be used for diagnoses. Otherwise, it is unused.
	 */
	cbmimage_blockaddress  block_start;

	/** mark if we already read after the end of the chain.
	 */
	unsigned int           is_done : 1;

	/** mark if we fell in a loop
	 */
	unsigned int           is_loop : 1;

	/** loop detector
	 */
	cbmimage_loop *        loop_detector;

	/** the block accessor for the current block of the chain
	 */
	cbmimage_blockaccessor * block_accessor;

} cbmimage_chain;


/** @brief file handling data
 *
 * The structure is used for processing a file
 * from a file image.
 *
 * @todo Add VLIR and REL file processing and needed structures
 */
typedef
struct cbmimage_file_s {

	/// the fileimage to which this file belongs
	cbmimage_fileimage * image;

	/// a directory entry which described this file
	cbmimage_dir_entry * dir_entry;

	/// loop detector
	cbmimage_loop * loop_detector;

	/// the file chain that follows this file
	cbmimage_chain * chain;

	/// offset as read pointer into the current block
	uint16_t block_current_offset;

	/// remaining byte in the current block that are still unread
	uint16_t block_current_remain;

	/// the number of bytes in a block
	uint16_t bytes_in_block;

	/// != 0 if an error occurred
	int error;

	/** some additional space for the various data. Don't make any assumptions
	 * about the contents or the layout here!
	 */
	uint8_t bufferarray[];

} cbmimage_file;


cbmimage_imagetype   cbmimage_image_guesstype                  (const uint8_t * buffer, size_t size, int * extra_errormap);
cbmimage_imagetype   cbmimage_image_file_guesstype             (const char * filename);
cbmimage_fileimage * cbmimage_image_open                       (const uint8_t * buffer, size_t size, cbmimage_imagetype);
cbmimage_fileimage * cbmimage_image_openfile                   (const char * filename, cbmimage_imagetype);
void                 cbmimage_image_readfile                   (cbmimage_fileimage *, const char * filename);
void                 cbmimage_image_writefile                  (cbmimage_fileimage *, const char * filename);
const void *         cbmimage_image_get_raw                    (cbmimage_fileimage *);
size_t               cbmimage_image_get_raw_size               (cbmimage_fileimage *);
void                 cbmimage_image_close                      (cbmimage_fileimage *);
void                 cbmimage_image_fat_dump                   (cbmimage_fileimage *, int linear);

const char *         cbmimage_get_imagetype_name               (cbmimage_fileimage *);
const char *         cbmimage_get_filename                     (cbmimage_fileimage *);

uint16_t             cbmimage_get_max_track                    (cbmimage_fileimage *);
uint16_t             cbmimage_get_max_sectors                  (cbmimage_fileimage *);
uint16_t             cbmimage_get_max_lba                      (cbmimage_fileimage *);
uint16_t             cbmimage_get_bytes_in_block               (cbmimage_fileimage *);
uint16_t             cbmimage_get_sectors_in_track             (cbmimage_fileimage *, uint16_t track);

int                  cbmimage_blockaddress_ts_exists           (cbmimage_fileimage *, uint8_t track, uint8_t sector);
int                  cbmimage_blockaddress_lba_exists          (cbmimage_fileimage *, uint16_t lba);
int                  cbmimage_blockaddress_init_from_ts        (cbmimage_fileimage *, cbmimage_blockaddress * block);
int                  cbmimage_blockaddress_init_from_lba       (cbmimage_fileimage *, cbmimage_blockaddress * block);
int                  cbmimage_blockaddress_init_from_ts_value  (cbmimage_fileimage *, cbmimage_blockaddress * block, uint8_t track, uint8_t sector);
int                  cbmimage_blockaddress_init_from_lba_value (cbmimage_fileimage *, cbmimage_blockaddress * block, uint16_t lba);
int                  cbmimage_blockaddress_advance             (cbmimage_fileimage *, cbmimage_blockaddress * block);
int                  cbmimage_blockaddress_advance_in_track    (cbmimage_fileimage *, cbmimage_blockaddress * block);
int                  cbmimage_blockaddress_add                 (cbmimage_fileimage *, cbmimage_blockaddress * blockresult, cbmimage_blockaddress block_adder);

int                  cbmimage_read_block                       (cbmimage_fileimage *, cbmimage_blockaddress block, void * buffer, size_t buffersize);
int                  cbmimage_write_block                      (cbmimage_fileimage *, cbmimage_blockaddress block, void * buffer, size_t buffersize);

int                  cbmimage_read_next_block                  (cbmimage_fileimage *, cbmimage_blockaddress * block, void * buffer, size_t buffersize);

cbmimage_blockaccessor * cbmimage_blockaccessor_create         (cbmimage_fileimage * image, cbmimage_blockaddress block);
cbmimage_blockaccessor * cbmimage_blockaccessor_create_from_ts (cbmimage_fileimage * image, uint8_t track, uint8_t sector);
cbmimage_blockaccessor * cbmimage_blockaccessor_create_from_lba(cbmimage_fileimage * image, uint16_t lba);
void                     cbmimage_blockaccessor_close          (cbmimage_blockaccessor * accessor);
int                      cbmimage_blockaccessor_set_to         (cbmimage_blockaccessor * accessor, cbmimage_blockaddress block);
int                      cbmimage_blockaccessor_set_to_ts      (cbmimage_blockaccessor * accessor, uint8_t track, uint8_t sector);
int                      cbmimage_blockaccessor_set_to_lba     (cbmimage_blockaccessor * accessor, uint16_t lba);
int                      cbmimage_blockaccessor_advance        (cbmimage_blockaccessor * accessor);
int                      cbmimage_blockaccessor_follow         (cbmimage_blockaccessor * accessor);
int                      cbmimage_blockaccessor_get_next_block (cbmimage_blockaccessor * accessor, cbmimage_blockaddress * block_next);

int                  cbmimage_bam_check_consistency            (cbmimage_fileimage *);
int                  cbmimage_get_blocks_free                  (cbmimage_fileimage *);
cbmimage_BAM_state   cbmimage_bam_get                          (cbmimage_fileimage *, cbmimage_blockaddress block);
int                  cbmimage_bam_get_free_on_track            (cbmimage_fileimage * image, uint8_t track);

cbmimage_dir_header *cbmimage_dir_get_header                   (cbmimage_fileimage *);
void                 cbmimage_dir_get_header_close             (cbmimage_dir_header *);
cbmimage_dir_entry * cbmimage_dir_get_first                    (cbmimage_fileimage *);
int                  cbmimage_dir_get_next                     (cbmimage_dir_entry *);
int                  cbmimage_dir_get_is_valid                 (cbmimage_dir_entry *);
char *               cbmimage_dir_extract_name                 (cbmimage_dir_header_name *, char * name, size_t len);
int                  cbmimage_dir_is_deleted                   (cbmimage_dir_entry *);
int                  cbmimage_dir_chdir                        (cbmimage_dir_entry *);
int                  cbmimage_dir_chdir_close                  (cbmimage_fileimage *);
void                 cbmimage_dir_get_close                    (cbmimage_dir_entry *);

cbmimage_loop *      cbmimage_loop_create                      (cbmimage_fileimage *);
void                 cbmimage_loop_close                       (cbmimage_loop *);
int                  cbmimage_loop_mark                        (cbmimage_loop *, cbmimage_blockaddress block);
int                  cbmimage_loop_check                       (cbmimage_loop *, cbmimage_blockaddress block);

int                  cbmimage_validate                         (cbmimage_fileimage *);

cbmimage_file * cbmimage_file_open_by_name(const char * filename);
cbmimage_file * cbmimage_file_open_by_dir_entry(cbmimage_dir_entry * dir_entry);
void            cbmimage_file_close(cbmimage_file * file);
int             cbmimage_file_read_next_block(cbmimage_file * file, uint8_t * buffer, size_t buffer_size);

/** @brief Type for a print() style callback
 *
 * Used with cbmimage_print_set_function()
 */
typedef void cbmimage_print_function_type(const char * text);
int cbmimage_print_set_function(cbmimage_print_function_type print_function);

cbmimage_fat *         cbmimage_fat_create(cbmimage_fileimage * image);
int                    cbmimage_fat_set   (cbmimage_fat *, cbmimage_blockaddress block, cbmimage_blockaddress target);
int                    cbmimage_fat_clear (cbmimage_fat *, cbmimage_blockaddress block);
cbmimage_blockaddress  cbmimage_fat_get   (cbmimage_fat *, cbmimage_blockaddress block);
int                    cbmimage_fat_is_used(cbmimage_fat *, cbmimage_blockaddress block);
void                   cbmimage_fat_dump  (cbmimage_fat *, int linear);
void                   cbmimage_fat_close (cbmimage_fat *);

cbmimage_chain *       cbmimage_chain_start      (cbmimage_fileimage *, cbmimage_blockaddress);
void                   cbmimage_chain_close      (cbmimage_chain * chain);
int                    cbmimage_chain_advance    (cbmimage_chain * chain);
int                    cbmimage_chain_last_result(cbmimage_chain * chain);
int                    cbmimage_chain_is_done    (cbmimage_chain * chain);
int                    cbmimage_chain_is_loop    (cbmimage_chain * chain);
cbmimage_blockaddress  cbmimage_chain_get_current(cbmimage_chain * chain);
cbmimage_blockaddress  cbmimage_chain_get_next   (cbmimage_chain * chain);
uint8_t *              cbmimage_chain_get_data   (cbmimage_chain * chain);

#endif // #ifndef CBMIMAGE_H
