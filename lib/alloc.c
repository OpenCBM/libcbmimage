/** @file lib/alloc.c \n
 * @author Spiro Trikaliotis \n
 * \n
 * @brief cbmimage: allocation functions
 *
 * @defgroup xalloc Allocation functions (internal)
 */
#include "cbmimage/alloc.h"
#include "cbmimage/internal.h"

#include <stdlib.h>
#include <string.h>

// #define CBMIMAGE_ALLOC_DEBUG_SIZE_AND_CLEAR 1
// #define CBMIMAGE_ALLOC_DEBUG_SIZE_AND_CLEAR_PRINT 1

/** @brief @internal allocate dynamic memory, internal implementation
 * @ingroup xalloc
 *
 * The dynamic memory is initialized with 0.
 *
 * @param[in] size
 *    size of the block to be allocated
 *
 * @return
 *    a pointer to the memory block, or 0 if the allocation failed
 *
 * @remark
 *    - The memory must be freed by cbmimage_i_xfree()
 */
static
void *
cbmimage_ii_xalloc(
		size_t size
		)
{
	return calloc(1, size);
}

/** @brief @internal free dynamic memory, internal implementation
 * @ingroup xalloc
 *
 * @param[in] ptr
 *    ptr to a block that was allocated by cbmimage_i_xalloc()
 */
static
void
cbmimage_ii_xfree(
		void * ptr
		)
{
	free(ptr);
}

/** @brief @internal allocate dynamic memory and copy into it, internal implementation
 * @ingroup xalloc
 *
 * Allocates memory like cbmimage_i_xalloc(), but initializes
 * the memory by copying from another buffer into it
 *
 * @param[in] newsize
 *    size of the block to be allocated
 *
 * @param[in] oldbuffer
 *    ptr to a buffer from which to copy
 *
 * @param[in] oldsize
 *    the size of the data in the oldbuffer
 *
 * @return
 *    a pointer to the memory block, or 0 if the allocation failed
 *
 * @remark
 *    - The memory must be freed by cbmimage_i_xfree()
 */
static
void *
cbmimage_ii_xalloc_and_copy(
		size_t newsize,
		const void *oldbuffer,
		size_t oldsize
		)
{
	void * buffer = cbmimage_i_xalloc(newsize);

	if (buffer) {
		memcpy(buffer, oldbuffer, oldsize);
	}

	return buffer;
}



/** @brief pointer to the callback function for cbmimage_i_xalloc()
 * @ingroup xalloc
 *
 * @remark
 *   - If this is set to NULL, then the internal variant is used
 *   - For a discussion if this function can be NULL or not, cf. the remarks at
 *   cbmimage_alloc_set_functions()
 *
 */
static cbmimage_alloc_xalloc_function_type * cbmimage_i_alloc_xalloc_function = cbmimage_ii_xalloc;

/** @brief pointer to the callback function for cbmimage_i_xalloc_and_copy()
 * @ingroup xalloc
 *
 * @remark
 *   - If this is set to NULL, then the internal variant is used
 *   - For a discussion if this function can be NULL or not, cf. the remarks at
 *   cbmimage_alloc_set_functions()
 *
 */
static cbmimage_alloc_xalloc_and_copy_function_type * cbmimage_i_alloc_xalloc_and_copy_function = cbmimage_ii_xalloc_and_copy;

/** @brief pointer to the callback function for cbmimage_i_xfree()
 * @ingroup xalloc
 *
 * @remark
 *   - If this is set to NULL, then the internal variant is used
 *   - For a discussion if this function can be NULL or not, cf. the remarks at
 *   cbmimage_alloc_set_functions()
 *
 */
static cbmimage_alloc_xfree_function_type * cbmimage_i_alloc_xfree_function = cbmimage_ii_xfree;

/** @brief @internal allocate dynamic memory
 * @ingroup xalloc
 *
 * The dynamic memory is initialized with 0.
 *
 * @param[in] size
 *    size of the block to be allocated
 *
 * @return
 *    a pointer to the memory block, or 0 if the allocation failed
 *
 * @remark
 *    - The memory must be freed by cbmimage_i_xfree()
 */
void *
cbmimage_i_xalloc(
		size_t size
		)
{
#if CBMIMAGE_ALLOC_DEBUG_SIZE_AND_CLEAR
	size += sizeof(size_t);

	void * p = cbmimage_i_alloc_xalloc_function(size);

	size_t * psize = p;

	*psize = size;

	p = psize + 1;

#if CBMIMAGE_ALLOC_DEBUG_SIZE_AND_CLEAR_PRINT
	cbmimage_i_fmt_print("Alloc %6u at %p (%p)\n", size, p, psize);
#endif

	return p;
#else
	return cbmimage_i_alloc_xalloc_function(size);
#endif
}

/** @brief @internal free dynamic memory
 * @ingroup xalloc
 *
 * The dynamic memory is initialized with 0.
 *
 * @param[in] ptr
 *    ptr to a block that was allocated by cbmimage_i_xalloc()
 */
void
cbmimage_i_xfree(
		void * ptr
		)
{
#if CBMIMAGE_ALLOC_DEBUG_SIZE_AND_CLEAR
	if (ptr == 0) {
#if CBMIMAGE_ALLOC_DEBUG_SIZE_AND_CLEAR_PRINT2
		cbmimage_i_fmt_print("Free      at %p\n", ptr);
#endif
	}
	else {
		size_t * psize = ptr;
		size_t size = *--psize;

#if CBMIMAGE_ALLOC_DEBUG_SIZE_AND_CLEAR_PRINT
		cbmimage_i_fmt_print("Free  %6u at %p (%p)\n", size, ptr, psize);
#endif

		memset(psize, 0xde, size);

		cbmimage_i_alloc_xfree_function(psize);
	}
#else
	cbmimage_i_alloc_xfree_function(ptr);
#endif
}

/** @brief @internal allocate dynamic memory and copy into it
 * @ingroup xalloc
 *
 * Allocates memory like cbmimage_i_xalloc(), but initializes
 * the memory by copying from another buffer into it
 *
 * @param[in] newsize
 *    size of the block to be allocated
 *
 * @param[in] oldbuffer
 *    ptr to a buffer from which to copy
 *
 * @param[in] oldsize
 *    the size of the data in the oldbuffer
 *
 * @return
 *    a pointer to the memory block, or 0 if the allocation failed
 *
 * @remark
 *    - The memory must be freed by cbmimage_i_xfree()
 */
void *
cbmimage_i_xalloc_and_copy(
		size_t newsize,
		const void *oldbuffer,
		size_t oldsize
		)
{
	return cbmimage_i_alloc_xalloc_and_copy_function(newsize, oldbuffer, oldsize);
}


/** @brief set the callback for output from the library
 * @ingroup alloc
 *
 * @param[in] xalloc_function
 *    pointer to the function that is called whenever
 *    the library wants to xalloc() something
 *
 *    If this is set to NULL, then a library internal xalloc() is used
 *
 * @param[in] xfree_function
 *    pointer to the function that is called whenever
 *    the library wants to xfree() something
 *
 *    If this is set to NULL, then a library internal xfree() is used
 *
 * @param[in] xalloc_and_copy_function
 *    pointer to the function that is called whenever
 *    the library wants to xalloc_and_copy() something
 *
 *    If this is set to NULL, then a library internal xalloc_and_copy() is used
 *
 * @remark
 *    - xalloc_function and xfree_function must correspond to each other.
 *      Because of this, do not set one of these functions, but not the other!
 *
 *    - If xalloc_and_copy_function is set, it must correspond to xalloc_function,
 *      too. However, if it is not defined (= NULL), then an internal implementation
 *      is used that used xalloc_function; thus, in this case, the usage is safe
 */
int
cbmimage_alloc_set_functions(
		cbmimage_alloc_xalloc_function_type          xalloc_function,
		cbmimage_alloc_xfree_function_type           xfree_function,
		cbmimage_alloc_xalloc_and_copy_function_type xalloc_and_copy_function
		)
{
	cbmimage_i_alloc_xalloc_function          = xalloc_function          ? xalloc_function          : cbmimage_ii_xalloc;
	cbmimage_i_alloc_xalloc_and_copy_function = xalloc_and_copy_function ? xalloc_and_copy_function : cbmimage_ii_xalloc_and_copy;
	cbmimage_i_alloc_xfree_function           = xfree_function           ? xfree_function           : cbmimage_ii_xfree;
}
