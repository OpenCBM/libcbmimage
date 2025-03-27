/** @file include/cbmimage/testhelper.h \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage Test helper functions
 *
 * These functions are not part of libcbmimage.
 * They are only used for the "testsuite" (which, currently, is not a suite).
 *
 * So, they are only compiled if CBMIMAGE_TESTLIB is defined (via make testlib)
 *
 */
#ifndef CBMIMAGE_TESTHELPER_H
#define CBMIMAGE_TESTHELPER_H 1

# ifdef CBMIMAGE_TESTLIB

/** @brief @internal check assertion
 * @ingroup testhelper
 *
 * @param[in] _x
 *   Condition that is to be tested.
 *
 * @remark
 *   - If the condition is not true, a specific message is generated and the
 *     execution aborts (in TEST_i_ASSERT_FAIL().
 *   - This macro is a replacement for the C assert() macro.
 *     The advantage w.r.t. assert() is that the output is defined by us,
 *     and so, we can test for the output in the test framework.
 */
#  define TEST_ASSERT(_x) if (_x) {} else TEST_i_ASSERT_FAIL( #_x, __FILE__, __LINE__ )

void TEST_i_ASSERT_FAIL(const char * expression, const char * file, int line);

void dump(const void * buffer, unsigned int size);

# else // #ifdef CBMIMAGE_TESTLIB

#  define TEST_ASSERT(_x) ((void)0)

# endif // #ifdef CBMIMAGE_TESTLIB

#endif // #ifndef CBMIMAGE_TESTHELPER_H
