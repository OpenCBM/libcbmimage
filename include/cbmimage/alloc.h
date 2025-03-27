/** @file include/cbmimage/alloc.h \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage alloc function definitions
 *
 */

#ifndef CBMIMAGE_ALLOC_H
#define CBMIMAGE_ALLOC_H 1

#include <stddef.h>

void * cbmimage_i_xalloc(size_t size);
void   cbmimage_i_xfree(void * ptr);
void * cbmimage_i_xalloc_and_copy(size_t newsize, const void *oldbuffer, size_t oldsize);

/** @brief Type for a cbmimage_i_xalloc() style callback
 *
 * Used with cbmimage_set_alloc_functions()
 */
typedef void * cbmimage_alloc_xalloc_function_type(size_t size);

/** @brief Type for a cbmimage_i_xalloc_and_copy() style callback
 *
 * Used with cbmimage_set_alloc_functions()
 */
typedef void * cbmimage_alloc_xalloc_and_copy_function_type(size_t newsize, const void *oldbuffer, size_t oldsize);

/** @brief Type for a cbmimage_i_xfree() style callback
 *
 * Used with cbmimage_set_alloc_functions()
 */
typedef void cbmimage_alloc_xfree_function_type(void * ptr);

int cbmimage_alloc_set_functions(cbmimage_alloc_xalloc_function_type, cbmimage_alloc_xfree_function_type, cbmimage_alloc_xalloc_and_copy_function_type);

#endif // #ifndef CBMIMAGE_ALLOC_H
