/** @file lib/cbprint.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: return text print(f)-style
 *
 * @defgroup cbmimage_cbprint printf() functions
 */
#include "cbmimage/internal.h"

#include <stdarg.h>
#include <stdio.h>

/** @brief pointer to the callback function for output
 * @ingroup cbmimage_cbprint
 *
 * @remark
 *    If this is set to NULL, then the output is sent to stderr.
 */
static cbmimage_print_function_type * cbmimage_i_print_function = NULL;

/** @brief set the callback for output from the library
 * @ingroup cbmimage_cbprint
 *
 * @param[in] print_function
 *    pointer to the function that is called whenever
 *    the library wants to output something.
 *
 *    If this is set to NULL, then the output is sent to stderr.
 */
int
cbmimage_print_set_function(
		cbmimage_print_function_type print_function
		)
{
	cbmimage_i_print_function = print_function;
}

/** @brief @internal send output from the library
 * @ingroup cbmimage_cbprint
 *
 * @param[in] text
 *    pointer to a string that is output.
 *
 * @remark
 *    No printf() style formatting is possible. If this is needed, use
 *    cbmimage_i_fmt_print() instead.
 */
void
cbmimage_i_print(
		const char * text
		)
{
	if (cbmimage_i_print_function) {
		cbmimage_i_print_function(text);
	}
	else {
		fflush(stdout);
		fputs(text, stderr);
		fflush(stderr);
	}
}

/** @brief @internal send formatted output from the library
 * @ingroup cbmimage_cbprint
 *
 * @param[in] fmt
 *    pointer to a printf() format string that is output.
 *
 * @param[in] ...
 *    additional parameter needed for the fmt format string
 *
 * @remark
 *    - As (vsn)printf() is used internally, all formatting allowed
 *      by printf() can be used, too.
 *
 *    - If a string without printf() style formatting shall be sent,
 *      use the more efficient cbmimage_i_print() instead.
 */
void
cbmimage_i_fmt_print(
		const char * fmt,
		...
		)
{
	char message_buffer[2048];

	va_list ap;

	va_start(ap, fmt);

	vsnprintf(message_buffer, sizeof message_buffer, fmt, ap);

	va_end(ap);

	cbmimage_i_print(message_buffer);
}
