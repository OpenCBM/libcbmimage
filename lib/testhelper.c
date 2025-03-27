/** @file lib/testhelper.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage Test helper functions
 *
 * These functions are not part of libcbmimage.
 * They are only used for the "testsuite" (which, currently, is not a suite).
 *
 * So, they are only compiled if CBMIMAGE_TESTLIB is defined (via make testlib)
 *
 * @defgroup testhelper Test helper definitions, not part of the library
 */
#ifdef CBMIMAGE_TESTLIB

#include "cbmimage/testhelper.h"
#include "cbmimage/internal.h"
#include "cbmimage/helper.h"

#include <stdint.h>
#include <stdlib.h>

/** @brief @internal output and exit after an assertion failed
 * @ingroup testhelper
 *
 * @param[in] expression
 *   (string) the condition that was tested and did not hold
 *
 * @param[in] file
 *   The name of the file the had the test in it
 *
 * @param[in] line
 *   The line number in the file where the test was placed
 *
 * @remark
 *   - This function does not return
 *   - Be careful when changing the output: The last line is tested by the test
 *     framework. Do not change it if you do not want to change the test framework itself.
 */
void
TEST_i_ASSERT_FAIL(
		const char * expression,
		const char * file, int line
		)
{
	cbmimage_i_fmt_print("in file %s(%u):\n", file, line);
	cbmimage_i_fmt_print("%s\n", expression);
	exit(1);
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
		cbmimage_i_fmt_print("%04X:  ", row);
		for (int col = row; col < min(row + 16, size); ++col) {
			cbmimage_i_fmt_print("%02X ", buffer[col]);
		}
		cbmimage_i_fmt_print("\n");
	}
}

#endif // #ifdef CBMIMAGE_TESTLIB
