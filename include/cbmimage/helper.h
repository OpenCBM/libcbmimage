/** @file include/cbmimage/helper.h \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage internals
 *
 * @defgroup cbmimage_helper internal helper
 */
#ifndef CBMIMAGE_HELPER_H
#define CBMIMAGE_HELPER_H 1

/** @brief Helper to determine the size of a C style array
 * @ingroup cbmimage_helper
 *
 * @param[in] _x
 *    The array to examine
 *
 * @return
 *   The number of  array elements in _x
 */
#define CBMIMAGE_ARRAYSIZE(_x) \
	(sizeof (_x) / sizeof (_x)[0])

/** @brief @internal compute the minimum of two values
 * @ingroup cbmimage_helper
 *
 * @param[in] a
 *   one value
 *
 * @param[in] b
 *   the other value
 *
 * @return
 *   Returns the minimum of both values a and b.
 */
static inline
int
min(int a, int b)
{
	return (a < b) ? a : b;
}

/** @brief @internal compute the maximum of two values
 * @ingroup cbmimage_helper
 *
 * @param[in] a
 *   one value
 *
 * @param[in] b
 *   the other value
 *
 * @return
 *   Returns the maximum of both values a and b.
 */
static inline
int
max(int a, int b)
{
	return (a > b) ? a : b;
}

#endif // #ifndef CBMIMAGE_HELPER_H
