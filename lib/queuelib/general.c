
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h> /* for NULL */
#include <stdlib.h>

/***
 ** Compacted pointer lists
 **
 ** N-length list requires N+1 elements to ensure NULL-termination.
 **/

/* Find a pointer in a pointer array. Returns the addess of the element if
 * found or the address of the terminating NULL otherwise. This can be used
 * to bounds check and add items. */
void ** find_array_ptr(void **arr, void *ptr)
{
    void *curr;
    for (curr = *arr; curr != NULL && curr != ptr; curr = *(++arr));
    return arr;
}

/* Remove a pointer from a pointer array if it exists. Compacts it so that
 * no gaps exist. Returns 0 on success and -1 if the element wasn't found. */
int remove_array_ptr(void **arr, void *ptr)
{
    void *curr;
    arr = find_array_ptr(arr, ptr);

    if (*arr == NULL)
        return -1;

    /* Found. Slide up following items. */
    do
    {
        void **arr1 = arr + 1;
        *arr++ = curr = *arr1;
    }
    while (curr != NULL);

    return 0;
}

void panicf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    exit(-1);
}
