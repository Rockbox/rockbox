#ifndef GENERAL_H
#define GENERAL_H

/***
 ** Compacted pointer lists
 **
 ** N-length list requires N+1 elements to ensure NULL-termination.
 **/

/* Find a pointer in a pointer array. Returns the addess of the element if
   found or the address of the terminating NULL otherwise. This can be used
   to bounds check and add items. */
void ** find_array_ptr(void **arr, void *ptr);

/* Remove a pointer from a pointer array if it exists. Compacts it so that
   no gaps exist. Returns 0 on success and -1 if the element wasn't found. */
int remove_array_ptr(void **arr, void *ptr);

void panicf(const char *fmt, ...);

#endif /* GENERAL_H */
