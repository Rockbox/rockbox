#ifndef CMAP_H_
#define CMAP_H_

#define CVECTOR_LINEAR_GROWTH
#include "cvector.h"

/* note: for ease of porting go code to c, many functions (macros) names
   remain similar to the ones used by go */

/* note2: this is a very basic map implementation.  It does not do any sorting, and only works for basic types (and pointers) that can be compared with the equality operator */

#define nil         NULL

#define cmap_elem_destructor_t cvector_elem_destructor_t

/**
 * @brief cmap_declare - The map type used in this library
 * @param name - The name associated to a map type.
 * @param key_type - The map pair key type.
 * @param val_type - The map pair value type.
 * @param compare_func - The function used to compare for key_type. Should return value < 0 when a < b, 0 when a == b and value > 0 when a > b.
 */
#define cmap_declare(name, key_type, val_type)            \
typedef struct {                                          \
    key_type key;                                         \
    val_type value;                                       \
} name##_pair_t;                                          \
\
typedef struct {                                          \
    cvector(name##_pair_t) tree;                          \
    cmap_elem_destructor_t elem_destructor;               \
} name##_map_t;                                           \
\
static inline val_type name##_get_(                       \
  name##_map_t *this, const key_type key)                 \
{                                                         \
    if (this) {                                           \
        size_t i;                                         \
        for (i = 0; i < cvector_size(this->tree); i++) {  \
            if (key == this->tree[i].key) {               \
                return this->tree[i].value;               \
            }                                             \
        }                                                 \
    }                                                     \
    return 0;                                             \
}                                                         \
\
static inline val_type name##_get_ptr_(                   \
  name##_map_t *this, const key_type key)                 \
{                                                         \
    if (this) {                                           \
        size_t i;                                         \
        for (i = 0; i < cvector_size(this->tree); i++) {  \
            if (key == this->tree[i].key) {               \
                return this->tree[i].value;               \
            }                                             \
        }                                                 \
    }                                                     \
    return nil;                                           \
}                                                         \
\
static inline void name##_set_(                                         \
    name##_map_t *this, const key_type key, val_type value)             \
{                                                                       \
    if (this) {                                                         \
        size_t i;                                                       \
        for (i = 0; i < cvector_size(this->tree); i++) {                \
            if (key == this->tree[i].key) {                             \
                this->tree[i].value = value;                            \
                return;                                                 \
            }                                                           \
        }                                                               \
        name##_pair_t new_pair = (name##_pair_t) { key, value };        \
        cvector_push_back(this->tree, new_pair);                        \
    }                                                                   \
}                                                                       \
\
static inline void name##_delete_(                            \
    name##_map_t *this, const key_type key)                   \
{                                                             \
    if (this) {                                               \
        size_t i;                                             \
        for (i = 0; i < cvector_size(this->tree); i++) {      \
            if (key == this->tree[i].key) {                   \
                cvector_erase(this->tree, i);                 \
                return;                                       \
            }                                                 \
        }                                                     \
    }                                                         \
}                                                             \
\
static inline name##_map_t* name##_map_make_(void)                    \
{                                                                     \
    name##_map_t *map = (name##_map_t*) malloc(sizeof(name##_map_t)); \
    if (map) {                                                        \
        map->tree = nil;                                              \
        cvector_init(map->tree, 0, nil);                              \
        return map;                                                   \
    }                                                                 \
    return nil;                                                       \
}                                                                     \
\
static inline void name##_clear_(name##_map_t *this)          \
{                                                             \
    if (this) {                                               \
        cvector_free(this->tree);                             \
        free(this);                                           \
    }                                                         \
}                                                             \
\


/**
 * @brief cmap - The map type used in this library
 * @param name - The name associated to a map type.
 */
#define cmap(name) name##_map_t *

/**
 * @brief cmap_make - creates a new map.  Automatically initializes the map.
 * @param name - the name asociated to the map type
 * @return a pointer to a new map.
 */
#define cmap_make(name) name##_map_make_()

/**
 * @brief cmap_size - gets the current size of the map
 * @param map_ptr - the map pointer
 * @return the size as a size_t
 */
#define cmap_len(map_ptr) cvector_size(map_ptr->tree)

/**
 * @brief cmap_get - gets value associated to a key.
 * @param name - the name asociated to the map type
 * @param map_ptr - the map pointer
 * @param key - the key to search for
 * @return the value associated to a key
 */
#define cmap_get(name, map_ptr, key) name##_get_(map_ptr, key)

/**
 * @brief cmap_get-ptr - gets ptr_value associated to a key.  Use it to avoid assigning a ptr to 0.
 * @param name - the name asociated to the map type
 * @param map_ptr - the map pointer
 * @param key - the key to search for
 * @return the value associated to a key
 */
#define cmap_get_ptr(name, map_ptr, key) name##_get_ptr_(map_ptr, key)


/**
 * @brief cmap_set - sets value associated to a key.
 * @param name - the name asociated to the map type
 * @param map_ptr - the map pointer
 * @param key - the key to search for
 * @param value - the new value
 * @return void
 */
#define cmap_set(name, map_ptr, key, val) name##_set_(map_ptr, key, val)

/**
 * @brief cmap_delete - deletes map entry associated to a key.
 * @param name - the name asociated to the map type
 * @param map_ptr - the map pointer
 * @param key - the key to search for
 * @return void
 */
#define cmap_delete(name, map_ptr, key) name##_delete_(map_ptr, key)

/**
 * @brief cmap_set_elem_destructor - set the element destructor function
 * used to clean up removed elements. The map must NOT be NULL for this to do anything.
 * @param map_ptr - the map pointer
 * @param elem_destructor_fn - function pointer of type cvector_elem_destructor_t used to destroy elements
 * @return void
 */
#define cmap_set_elem_destructor(map_ptr, elem_destructor_fn) \
    cvector_set_elem_destructor(map_ptr->tree, elem_destructor_fn)

/**
 * @brief cmap_clear - deletes all map entries.  And frees memory is an element destructor was set previously.
 * @param name - the name asociated to the map type
 * @param map_ptr - the map pointer
 * @return void
 */
#define cmap_clear(name, map_ptr) name##_clear_(map_ptr)

/**
 * @brief cmap_iterator - The iterator type used for cmap
 * @param type The type of iterator to act on.
 */
#define cmap_iterator(name) cvector_iterator(name##_pair_t)

/**
 * @brief cmap_begin - returns an iterator to first element of the vector
 * @param map_ptr - the map pointer
 * @return a pointer to the first element (or NULL)
 */
#define cmap_begin(map_ptr) ((map_ptr) ? cvector_begin(map_ptr->tree) : nil)

/**
 * @brief cmap_end - returns an iterator to one past the last element of the vector
 * @param map_ptrs - the map pointer
 * @return a pointer to one past the last element (or NULL)
 */
#define cmap_end(map_ptr) ((map_ptr) ? cvector_end(map_ptr->tree) : nil)

#endif  /* CMAP_H_ */
