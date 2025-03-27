#include "cbmimage.h"
#include "cbmimage/helper.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int argc;
static char **argv;

static int loglevel = 1;

static cbmimage_fileimage * image = NULL;

static
char *
get_arg_parameter(
		char * arg
		)
{
	char * p_equal = arg ? strchr(arg, '=') : NULL;

	if (p_equal) {
		return p_equal + 1;
	}

	return NULL;
}

static
int
get_arg_parameter_int(
		char * arg,
		int    default_int
		)
{
	char * p = get_arg_parameter(arg);

	if (p) {
		return atoi(p);
	}
	else {
		return default_int;
	}
}


static
int
get_arg_is_option(
		char * arg,
		char * option
		)
{
	int option_length = strlen(option);

	if (arg && strncmp(arg, option, option_length) == 0) {
		return 1;
	}

	return 0;
}


static
char *
get_current_arg(
		void
		)
{
	if (argc == 0) {
		return NULL;
	}

	return *argv;
}

static
char *
get_next_arg(
		void
		)
{
	if (argc == 0) {
		return NULL;
	}

	--argc;
	return *(argv++);
}

static
void
fmt_print_error(
		const char * fmt,
		...
		)
{
	va_list args;
	va_start(args, fmt);

	fflush(stdout);
	fflush(stderr);
	vfprintf(stderr, fmt, args);
	fflush(stderr);

	va_end(args);
}

static
void
fmt_print_verbose(
		int level,
		const char * fmt,
		...
		)
{
	if (level <= loglevel) {
		va_list args;
		va_start(args, fmt);

		vprintf(fmt, args);

		va_end(args);
	}
}


/** @brief @internal dump a buffer to stdout
 * @ingroup testhelper
 *
 * @param[in] buffer_void
 *   Pointer to the buffer that wants to be dumped
 *
 * @param[in] size
 *   The number of bytes that are being dumped from the buffer.
 */
void
dump(
		const void * buffer_void,
		unsigned int size
		)
{
	const uint8_t * buffer = buffer_void;

	for (int row = 0; row < size; row += 16) {
		printf("%04X:  ", row);

		int end_of_line = min(row + 16, size);

		for (int col = row; col < end_of_line; ++col) {
			printf("%02X ", buffer[col]);
		}

		for (int col = end_of_line; col < row + 16; ++col) {
			printf("   ");
		}

		for (int col = row; col < end_of_line; ++col) {
			char ch = buffer[col];
			printf("%c", (ch >= 0x20 && ch <= 127) ? ch : '.');
		}
		printf("\n");
	}
}

static void
output_bam(
	cbmimage_fileimage *image
	)
{
	uint16_t maxtrack = cbmimage_get_max_track(image);

	cbmimage_blockaddress block;

	cbmimage_blockaddress_init_from_ts_value(image, &block, 1, 0);

	do {
		if (block.ts.sector == 0) {
			printf("\n%3u: (%2u) ",
					block.ts.track,
					cbmimage_bam_get_free_on_track(image, block.ts.track)
					);
		}
		cbmimage_BAM_state bam_state = cbmimage_bam_get(image, block);

		switch (bam_state) {
			case BAM_UNKNOWN:
				printf("?");
				break;
			case BAM_REALLY_FREE:
				printf(".");
				break;
			case BAM_FREE:
				printf(":");
				break;
			case BAM_USED:
				printf("*");
				break;
			case BAM_DOES_NOT_EXIST:
				break;
		}

	} while (!cbmimage_blockaddress_advance(image, &block));
	printf("\n");
}

static
int
do_close(
		void
		)
{
	int ret = -1;

	fmt_print_verbose(2, "Closing image... ");
	if (image) {
		cbmimage_image_close(image);
		image = NULL;
		fmt_print_verbose(2, "SUCCESS\n");
		ret = 0;
	}
	else {
		fmt_print_verbose(2, "ERROR.\n");
	}

	return ret;
}

static
int
do_open(
		void
		)
{
	int ret = -1;

	char * name = get_next_arg();

	if (image) {
		do_close();
	}

	if (name) {
		fmt_print_verbose(2, "Opening file '%s': ", name);

		image = cbmimage_image_openfile(name, TYPE_UNKNOWN);
		if (!image) {
			fmt_print_verbose(2, "ERROR\n");
		}
		else {
			fmt_print_verbose(2, "SUCCESS\n");
			ret = 0;
		}
	}
	else {
		fmt_print_verbose(1, "No filename provided for opening.\n");
	}
	return ret;
}

static int
do_bam(
		void
		)
{
	int ret = -1;
	if (image) {
		output_bam(image);
		ret = 0;
	}

	return ret;
}

static int
do_checkbam(
		void
		)
{
	int ret = -1;
	if (image) {
		cbmimage_bam_check_consistency(image);
		ret = 0;
	}

	return ret;
}

static int
do_validate(
		void
		)
{
	int ret = -1;
	if (image) {
		cbmimage_validate(image);
		ret = 0;
	}

	return ret;
}

static int
do_fat(
		void
		)
{
	int ret = -1;
	if (image) {
		int trackformat = 0;

		char * param = get_current_arg();

		while (argc > 0 && param[0] == '-') {
			param = get_next_arg();

			if (get_arg_is_option(param, "--disklayout")) {
				trackformat = get_arg_parameter_int(param, 256);
			}
			else {
				fprintf(stdout, "unknown parameter '%s' found.\n", param);
				return -1;
			}

			param = get_current_arg();
		}

		cbmimage_image_fat_dump(image, trackformat);
		ret = 0;
	}

	return ret;
}

static
void
dir_output_name(
		cbmimage_fileimage *       image,
		cbmimage_dir_header_name * name_entry
		)
{
	char buffer[26];

	char * extra_text = cbmimage_dir_extract_name(name_entry, buffer, sizeof buffer);

	char output_buffer[sizeof buffer + 2];
	sprintf(output_buffer, "\"%s\"%s", buffer, extra_text);
	printf("%-18s", output_buffer);
}

static void
output_dir(
	cbmimage_fileimage * image
	)
{
	cbmimage_dir_entry * dir_entry = NULL;

	cbmimage_dir_header * header_entry = cbmimage_dir_get_header(image);

	if (header_entry) {
		printf("%5u ", 0);

		dir_output_name(image, &header_entry->name);

		printf("\n");
	}

	for (dir_entry = cbmimage_dir_get_first(image);
	     cbmimage_dir_get_is_valid(dir_entry);
	     cbmimage_dir_get_next(dir_entry)
	    )
	{
		if (cbmimage_dir_is_deleted(dir_entry)) {
			continue;
		}

		printf("%5u ", dir_entry->block_count);

		dir_output_name(image, &dir_entry->name);

		char char_is_closed = dir_entry->is_closed ? ' ' : '*';
		char char_is_locked = dir_entry->is_locked ? '<' : ' ';
		char * imagetype = "   ";

		switch (dir_entry->type) {
			case DIR_TYPE_DEL:
				imagetype = "DEL";
				break;
			case DIR_TYPE_SEQ:
				imagetype = "SEQ";
				break;
			case DIR_TYPE_PRG:
				imagetype = "PRG";
				break;
			case DIR_TYPE_USR:
				imagetype = "USR";
				break;
			case DIR_TYPE_REL:
				imagetype = "REL";
				break;
			case DIR_TYPE_PART1581:
				imagetype = "CBM";
				break;
			case DIR_TYPE_CMD_NATIVE:
				imagetype = "NAT";
				break;

			case DIR_TYPE_PART_NO:
				imagetype = "NOP";
				break;
			case DIR_TYPE_PART_CMD_NATIVE:
				imagetype = "CNP";
				break;
			case DIR_TYPE_PART_D64:
				imagetype = "D64";
				break;
			case DIR_TYPE_PART_D71:
				imagetype = "D71";
				break;
			case DIR_TYPE_PART_D81:
				imagetype = "D81";
				break;
			case DIR_TYPE_PART_SYSTEM:
				imagetype = "SYS";
				break;

			default:
				imagetype = "   ";
				break;
		}

		printf("%c%s%c - %3u/%3u",
				char_is_closed, imagetype, char_is_locked,
				dir_entry->start_block.ts.track,
				dir_entry->start_block.ts.sector
				);

		if (dir_entry->has_datetime) {
			printf("   %02u.%02u.%04u %02u:%02u",
					dir_entry->day,
					dir_entry->month,
					dir_entry->year,
					dir_entry->hour,
					dir_entry->minute
					);
		}
		else {
			if (dir_entry->is_geos || dir_entry->type == DIR_TYPE_REL) {
				printf("                   ");
			}
		}

		if (dir_entry->is_geos) {
			printf(" - GEOS %-5s[%3u] %3u/%3u",
					(dir_entry->geos_is_vlir ? "VLIR" : ""),
					dir_entry->geos_filetype,
					dir_entry->geos_infoblock.ts.track,
					dir_entry->geos_infoblock.ts.sector
					);
		}
		else if (dir_entry->type == DIR_TYPE_REL) {
			printf(" - [%3u] %3u/%3u",
					dir_entry->rel_recordlength,
					dir_entry->rel_sidesector_block.ts.track,
					dir_entry->rel_sidesector_block.ts.sector
					);
		}

		printf("\n");
	}

	if (header_entry) {
		printf("%5u BLOCKS FREE\n", header_entry->free_block_count);
	}

	cbmimage_dir_get_close(dir_entry);
	cbmimage_dir_get_header_close(header_entry);
}

static int
do_dir(
		void
		)
{
	int ret = -1;

	if (image) {
		output_dir(image);
		ret = 0;
	}

	return ret;
}

static int
get_blockaddress(
		cbmimage_fileimage *    image,
		cbmimage_blockaddress * block,
		const char * const      parameter
		)
{
	char * p = strchr(parameter, '/');

	if (p) {
		// we have a T/S

		char * pend;

		block->ts.track = strtol(parameter, &pend, 0);
		if (pend != p) {
			fmt_print_verbose(1, "Error converting the track!\n");
			exit(1);
		}
		block->ts.sector = strtol(p+1, &pend, 0);
		if (pend && *pend != 0) {
			fmt_print_verbose(1, "Error converting the sector!\n");
			exit(1);
		}
		fmt_print_verbose(1, "Reading block %u/%u\n", block->ts.track, block->ts.sector);
		cbmimage_blockaddress_init_from_ts(image, block);
	}
	else {
		char * pend;

		block->lba = strtol(parameter, &pend, 0);
		if (pend && *pend != 0) {
			fmt_print_verbose(1, "Error converting the LBA!\n");
			exit(1);
		}
		cbmimage_blockaddress_init_from_lba(image, block);
	}
}

static int
do_read(
		void
		)
{
	int ret = -1;

	if (image) {
		char * parameter_block = get_next_arg();

		cbmimage_blockaddress block;
		get_blockaddress(image, &block, parameter_block);

		fmt_print_verbose(1, "\nblock %u/%u = %u:\n\n", block.ts.track, block.ts.sector, block.lba);

		cbmimage_blockaccessor * accessor = cbmimage_blockaccessor_create(image, block);
		if (accessor) {
			dump(accessor->data, cbmimage_get_bytes_in_block(image));
			cbmimage_blockaccessor_close(accessor);
		}

		ret = 0;
	}

	return ret;
}

static void
showfile_output_no(
	cbmimage_fileimage * image,
	int                  no
	)
{
	int counter_entry = 1;

	cbmimage_dir_entry * dir_entry;

	for (dir_entry = cbmimage_dir_get_first(image);
	     cbmimage_dir_get_is_valid(dir_entry);
	     cbmimage_dir_get_next(dir_entry)
	    )
	{
		if (cbmimage_dir_is_deleted(dir_entry)) {
			continue;
		}

		if (counter_entry++ == no) {
			char name_buffer[26];
			char *extra_text = cbmimage_dir_extract_name(&dir_entry->name, name_buffer, sizeof name_buffer);

			fmt_print_verbose(1, "Opening file \"%s\":\n", name_buffer);

			cbmimage_file * file = cbmimage_file_open_by_dir_entry(dir_entry);

			if (file) {
				int read;

				do {
					static char buffer[256];

					read = cbmimage_file_read_next_block(file, buffer, sizeof buffer);

					if (read >= 0) {
						dump(buffer, read);
					}
				} while (read >= 0);

				cbmimage_file_close(file);
			}

			break;
		}
	}
	cbmimage_dir_get_close(dir_entry);
}

static int
do_showfile(
		void
		)
{
	int ret = -1;

	if (image) {
		int numerical_file = 0;
		int number_of_file = 0;

		char * param = get_current_arg();

		while (argc > 0 && param[0] == '-') {
			param = get_next_arg();

			if (get_arg_is_option(param, "--numerical")) {
				numerical_file = 1;
				number_of_file = get_arg_parameter_int(param, -1);
			}
			else {
				fprintf(stdout, "unknown parameter '%s' found.\n", param);
				return -1;
			}

			param = get_current_arg();
		}

		if (numerical_file) {
			fmt_print_verbose(1, "Checking file No. %u\n", number_of_file);

			showfile_output_no(image, number_of_file);

			ret = 0;
		}
	}

	return ret;
}

static void
chdir_dir_no(
	cbmimage_fileimage * image,
	int                  no
	)
{
	int counter_entry = 1;

	cbmimage_dir_entry * dir_entry;

	for (dir_entry = cbmimage_dir_get_first(image);
	     cbmimage_dir_get_is_valid(dir_entry);
	     cbmimage_dir_get_next(dir_entry)
	    )
	{
		if (cbmimage_dir_is_deleted(dir_entry)) {
			continue;
		}

		if (counter_entry++ == no) {
			char name_buffer[26];
			char *extra_text = cbmimage_dir_extract_name(&dir_entry->name, name_buffer, sizeof name_buffer);

			fmt_print_verbose(1, "chdir to file \"%s\":\n", name_buffer);

			if (cbmimage_dir_chdir(dir_entry)) {
				fmt_print_verbose(1, "Error chdir'ing to dir entry!\n");
			}

			break;
		}
	}
	cbmimage_dir_get_close(dir_entry);
}

static int
do_chdir(
		void
		)
{
	int ret = -1;

	if (image) {
		int numerical_dir = 0;
		int number_of_dir = 0;
		int dir_upwards = 0;

		char * param = get_current_arg();

		while (argc > 0 && param[0] == '-') {
			param = get_next_arg();

			if (get_arg_is_option(param, "--numerical")) {
				numerical_dir = 1;
				number_of_dir = get_arg_parameter_int(param, -1);
			}
			else if (get_arg_is_option(param, "..")) {
				dir_upwards = 1;
			}
			else {
				fprintf(stdout, "unknown parameter '%s' found.\n", param);
				return -1;
			}

			param = get_current_arg();
		}

		if (dir_upwards) {
			ret = cbmimage_dir_chdir_close(image);
		}
		else if (numerical_dir) {
			fmt_print_verbose(1, "chdir to file No. %u\n", number_of_dir);

			chdir_dir_no(image, number_of_dir);
			ret = 0;
		}
	}

	return ret;
}

typedef int execute_fct(void);

typedef
struct commands_s {
	char *        name;
	execute_fct * fct;
	char *        help_short;
	char *        help_text;
} commands;

static
int
do_help(
		void
		);

commands command_table[] = {
	{ "help", do_help, "output info about the various commands",
		"",
	},

	{ "open", do_open, "open an image file",
		"",
	},

	{ "close", do_close, "close an image file",
		"",
	},

	{ "dir", do_dir, "show the directory of an image",
		"",
	},

	{ "bam", do_bam, "show the BAM of an image",
		"",
	},

	{ "checkbam", do_checkbam, "check the BAM for consitency",
		"",
	},

	{ "fat", do_fat, "create and output the FAT of an image",
		"",
	},

	{ "read", do_read, "read a block of an image",
		"",
	},

	{ "showfile", do_showfile, "show/extract a file from an image",
		"",
	},

	{ "validate", do_validate, "validate an image",
		"",
	},

	{ "chdir", do_chdir, "change to a subdir",
		"",
	},
};

static
int
get_command_number(
		char * name
		)
{
	if (name != NULL) {
		for (int i = 0; i < CBMIMAGE_ARRAYSIZE(command_table); ++i) {
			if (strcmp(name, command_table[i].name) == 0) {
				return i;
			}
		}
	}

	return -1;
}

static
int
do_help(
		void
		)
{
	printf("cbmimage Commodore image processing tool\n\n");

	if (argc == 0) {
		printf("Possible commands:\n\n");
		for (int i = 0; i < CBMIMAGE_ARRAYSIZE(command_table); ++i) {
			printf(" %-10s - %s\n", command_table[i].name, command_table[i].help_short);
		}
	}

	char * cmd;

	while ((cmd = get_next_arg()) != NULL) {
		int no = get_command_number(cmd);

		if (no < 0) {
			printf(" help wanted for unknown command '%s':.\n", cmd);
			return -1;
		}
		else {
			printf(" %s:\n\n%s\n", command_table[no].name, command_table[no].help_text);
		}
	}

	return 0;
}

int
main(
		int     l_argc,
		char ** l_argv
		)
{
	int ret = 0;

	argc = l_argc;
	argv = l_argv;

	if (argc == 1) {
		return ret;
	}
	get_next_arg();

	char * name;

	while (((name = get_next_arg()) != NULL) && (ret == 0)) {
		int no = get_command_number(name);

		if (no >= 0) {
				ret = command_table[no].fct();
		}
	}

	if (image) {
		do_close();
	}

	return ret;
}
