# microtar

A lightweight tar library written in ANSI C.

This version is a fork of [rxi's microtar](https://github.com/rxi/microtar)
with bugfixes and API changes aimed at improving usability, but still keeping
with the minimal design of the original library.

## License

This library is free software; you can redistribute it and/or modify it under
the terms of the MIT license. See [LICENSE](LICENSE) for details.


## Supported format variants

No effort has been put into handling every tar format variant. Basically
what is accepted is the "old-style" format, which appears to work well
enough to access basic archives created by GNU `tar`.


## Basic usage

The library consists of two files, `microtar.c` and `microtar.h`, which only
depend on a tiny part of the standard C library & can be easily incorporated
into a host project's build system.

The core library does not include any I/O hooks as these are supposed to be
provided by the host application. If the C library's `fopen` and friends is
good enough, you can use `microtar-stdio.c`.


### Initialization

Initialization is very simple. Everything the library needs is contained in
the `mtar_t` struct; there is no memory allocation and no global state. It is
enough to zero-initialize an `mtar_t` object to put it into a "closed" state.
You can use `mtar_is_open()` to query whether the archive is open or not.

An archive can be opened for reading _or_ writing, but not both. You have to
specify which access mode you're using when you create the archive.

```c
mtar_t tar;
mtar_init(&tar, MTAR_READ, my_io_ops, my_stream);
```

Or if using `microtar-stdio.c`:

```c
int error = mtar_open(&tar, "file.tar", "rb");
if(error) {
    /* do something about it */
}
```

Note that `mtar_init()` is called for you in this case and the access mode is
deduced from the mode flags.


### Iterating and locating files

If you opened an archive for reading, you'll likely want to iterate over
all the files. Here's the long way of doing it:

```c
mtar_t tar;
int err;

/* Go to the start of the archive... Not necessary if you've
 * just opened the archive and are already at the beginning.
 * (And of course you normally want to check the return value.) */
mtar_rewind(&tar);

/* Iterate over the archive members */
while((err = mtar_next(&tar)) == MTAR_ESUCCESS) {
    /* Get a pointer to the current file header. It will
     * remain valid until you move to another record with
     * mtar_next() or call mtar_rewind() */
    const mtar_header_t* header = mtar_get_header(&tar);

    printf("%s (%d bytes)\n", header->name, header->size);
}

if(err != MTAR_ENULLRECORD) {
    /* ENULLRECORD means we hit end of file; any
     * other return value is an actual error. */
}
```

There's a useful shortcut for this type of iteration which removes the
loop boilerplate, replacing it with another kind of boilerplate that may
be more palatable in some cases.

```c
/* Will be called for each archive member visited by mtar_foreach().
 * The member's header is passed in as an argument so you don't need
 * to fetch it manually with mtar_get_header(). You can freely read
 * data (if present) and seek around. There is no special cleanup
 * required and it is not necessary to read to the end of the stream.
 *
 * The callback should return zero (= MTAR_SUCCESS) to continue the
 * iteration or return nonzero to abort. On abort, the value returned
 * by the callback will be returned from mtar_foreach(). Since it may
 * also return normal microtar error codes, it is suggested to use a
 * positive value or pass the result via 'arg'.
 */
int foreach_cb(mtar_t* tar, const mtar_header_t* header, void* arg)
{
    // ...
    return 0;
}

void main()
{
    mtar_t tar;

    // ...

    int ret = mtar_foreach(&tar, foreach_cb, NULL);
    if(ret < 0) {
        /* Microtar error codes are negative and may be returned if
         * there is a problem with the iteration. */
    } else if(ret == MTAR_ESUCCESS) {
        /* If the iteration reaches the end of the archive without
         * errors, the return code is MTAR_ESUCCESS. */
    } else if(ret > 0) {
        /* Positive values might be returned by the callback to
         * signal some condition was met; they'll never be returned
         * by microtar */
    }
}
```

The other thing you're likely to do is look for a specific file:

```c
/* Seek to a specific member in the archive */
int err = mtar_find(&tar, "foo.txt");
if(err == MTAR_ESUCCESS) {
    /* File was found -- read the header with mtar_get_header() */
} else if(err == MTAR_ENOTFOUND) {
    /* File wasn't in the archive */
} else {
    /* Some error occurred */
}
```

Note this isn't terribly efficient since it scans the entire archive
looking for the file.


### Reading file data

Once pointed at a file via `mtar_next()` or `mtar_find()` you can read the
data with a simple POSIX-like API.

- `mtar_read_data(tar, buf, count)` reads up to `count` bytes into `buf`,
  returning the actual number of bytes read, or a negative error value.
  If at EOF, this returns zero.

- `mtar_seek_data(tar, offset, whence)` works exactly like `fseek()` with
  `whence` being one of `SEEK_SET`, `SEEK_CUR`, or `SEEK_END` and `offset`
  indicating a point relative to the beginning, current position, or end
  of the file. Returns zero on success, or a negative error code.

- `mtar_eof_data(tar)` returns nonzero if the end of the file has been
  reached. It is possible to seek backward to clear this condition.


### Writing archives

Microtar has limited support for creating archives. When an archive is opened
for writing, you can add new members using `mtar_write_header()`.

- `mtar_write_header(tar, header)` writes out the header for a new member.
  The amount of data that follows is dictated by `header->size`, though if
  the underlying stream supports seeking and re-writing data, this size can
  be updated later with `mtar_update_header()` or `mtar_update_file_size()`.

- `mtar_update_header(tar, header)` will re-write the previously written
  header. This may be used to change any header field. The underlying stream
  must support seeking. On a successful return the stream will be returned
  to the position it was at before the call.

File data can be written with `mtar_write_data()`, and if the underlying stream
supports seeking, you can seek with `mtar_seek_data()` and read back previously
written data with `mtar_read_data()`. Note that it is not possible to truncate
the file stream by any means.

- `mtar_write_data(tar, buf, count)` will write up to `count` bytes from
  `buf` to the current member's data. Returns the number of bytes actually
  written or a negative error code.

- `mtar_update_file_size(tar)` will update the header size to reflect the
  actual amount of written data. This is intended to be called right before
  `mtar_end_data()` if you are not declaring file sizes in advance.

- `mtar_end_data(tar)` will end the current member. It will complain if you
  did not write the correct amount data provided in the header. This must be
  called before writing the next header.

- `mtar_finalize(tar)` is called after you have written all members to the
  archive. It writes out some null records which mark the end of the archive,
  so you cannot write any more archive members after this.

Note that `mtar_close()` can fail if there was a problem flushing buffered
data to disk, so its return value should always be checked.


## Error handling

Most functions that return `int` return an error code from `enum mtar_error`.
Zero is success and all other error codes are negative. `mtar_strerror()` can
return a string describing the error code.

A couple of functions use a different return value convention:

- `mtar_foreach()` may error codes or an arbitrary nonzero value provided
  by the callback.
- `mtar_read_data()` and `mtar_write_data()` returns the number of bytes read
  or written, or a negative error code. In particular zero means that no bytes
  were read or written.
- `mtar_get_header()` may return `NULL` if there is no valid header.
  It is only possible to see a null pointer if misusing the API or after
  a previous error so checking for this is usually not necessary.

There is essentially no support for error recovery. After an error you can
only do two things reliably: close the archive with `mtar_close()` or try
rewinding to the beginning with `mtar_rewind()`.


## I/O hooks

You can provide your own I/O hooks in a `mtar_ops_t` struct. The same ops
struct can be shared among multiple `mtar_t` objects but each object gets
its own `void* stream` pointer.

Name    | Arguments                                 | Required
--------|-------------------------------------------|------------
`read`  | `void* stream, void* data, unsigned size` | If reading
`write` | `void* stream, void* data, unsigned size` | If writing
`seek`  | `void* stream, unsigned pos`              | If reading
`close` | `void* stream`                            | Always

`read` and `write` should transfer the number of bytes indicated
and return the number of bytes actually read or written, or a negative
`enum mtar_error` code on error.

`seek` must have semantics like `lseek(..., pos, SEEK_SET)`; that is,
the position is an absolute byte offset in the stream. Seeking is not
optional for read support, but the library only performs backward
seeks under two circumstances:

- `mtar_rewind()` seeks to position 0.
- `mtar_seek_data()` may seek backward if the user requests it.

Therefore, you will be able to get away with a limited forward-only
seek function if you're able to read everything in a single pass use
the API carefully. Note `mtar_find()` and `mtar_foreach()` will call
`mtar_rewind()`.

`close` is called by `mtar_close()` to clean up the stream. Note the
library assumes that the stream handle is cleaned up by `close` even
if an error occurs.

`seek` and `close` should return an `enum mtar_error` code, either
`MTAR_SUCCESS`, or a negative value on error.
