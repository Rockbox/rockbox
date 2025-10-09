#ifndef CSLICE_H_
#define CSLICE_H_

#define CVECTOR_LINEAR_GROWTH
#include "cvector.h"

/* note: for ease of porting go code to c, many functions (macros) names
   remain similar to the ones used by go */

#define nil         NULL

/**
 * @brief cslice - The slice type used in this library
 * @param type The type of slice to act on.
 */
#define cslice(type) cvector(type)

/**
 * @brief cslice_make - creates a new slice.  Automatically initializes the slice.
 * @param slice - the slice
 * @param count - new size of the slice
 * @param value - the value to initialize new elements with
 * @return void
 */
#define cslice_make(slice, capacity, value)       \
    do {                                          \
        slice = nil;                              \
        cvector_init(slice, capacity, nil);       \
        cvector_resize(slice, capacity, value);   \
    } while(0)

/**
 * @brief cslice_size - gets the current size of the slice
 * @param slice - the slice
 * @return the size as a size_t
 */
#define cslice_len(slice) cvector_size(slice)

/**
 * @brief cslice_capacity - gets the current capacity of the slice
 * @param slice - the slice
 * @return the capacity as a size_t
 */
#define cslice_cap(slice) cvector_capacity(slice)

/**
 * @brief cslice_set_elem_destructor - set the element destructor function
 * used to clean up removed elements. The vector must NOT be NULL for this to do anything.
 * @param slice - the slice
 * @param elem_destructor_fn - function pointer of type cslice_elem_destructor_t used to destroy elements
 * @return void
 */
#define cslice_set_elem_destructor(slice, elem_destructor_fn) \
    cvector_set_elem_destructor(slice, elem_destructor_fn)

/**
 * @brief cslice_free - frees all memory associated with the slice
 * @param slice - the slice
 * @return void
 */
#define cslice_clear(slice) cvector_free(slice)

#endif  /* CSLICE_H_ */
