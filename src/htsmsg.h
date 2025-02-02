/*
 *  Functions for manipulating HTS messages
 *  Copyright (C) 2007 Andreas Öman
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <stdlib.h>
#include <inttypes.h>
#include "queue.h"
#include "uuid.h"
#include "build.h"

#define HTSMSG_ERR_FIELD_NOT_FOUND       -1
#define HTSMSG_ERR_CONVERSION_IMPOSSIBLE -2

TAILQ_HEAD(htsmsg_field_queue, htsmsg_field);

typedef struct htsmsg {
  /**
   * fields 
   */
  struct htsmsg_field_queue hm_fields;

  /**
   * Set if this message is a list, otherwise it is a map.
   */
  int hm_islist;

  /**
   * Data to be free'd when the message is destroyed
   */
  const void *hm_data;
  size_t hm_data_size;
} htsmsg_t;


#define HMF_MAP  1
#define HMF_S64  2
#define HMF_STR  3
#define HMF_BIN  4
#define HMF_LIST 5
#define HMF_DBL  6
#define HMF_BOOL 7
#define HMF_UUID 8

typedef struct htsmsg_field {
  TAILQ_ENTRY(htsmsg_field) hmf_link;
  uint8_t hmf_type;
  uint8_t hmf_flags;

#define HMF_ALLOCED        0x1
#define HMF_INALLOCED      0x2
#define HMF_NONAME         0x4

  union {
    int64_t  s64;
    const char *str;
    const uint8_t *uuid;
    struct {
      const char *data;
      size_t len;
    } bin;
    htsmsg_t *msg;
    double dbl;
    int boolean;
  } u;

#if ENABLE_SLOW_MEMORYINFO
  size_t hmf_edata_size;
#endif
  const char _hmf_name[0];
} htsmsg_field_t;

#define hmf_s64     u.s64
#define hmf_msg     u.msg
#define hmf_str     u.str
#define hmf_uuid    u.uuid
#define hmf_bin     u.bin.data
#define hmf_binsize u.bin.len
#define hmf_dbl     u.dbl
#define hmf_bool    u.boolean

// backwards compat
#define htsmsg_get_map_by_field(f) htsmsg_field_get_map(f)
#define htsmsg_get_list_by_field(f) htsmsg_field_get_list(f)

#define HTSMSG_FOREACH(f, msg) TAILQ_FOREACH(f, &(msg)->hm_fields, hmf_link)
#define HTSMSG_FIRST(msg)      TAILQ_FIRST(&(msg)->hm_fields)
#define HTSMSG_NEXT(f)         TAILQ_NEXT(f, hmf_link)

/**
 * Aligned memory allocation
 */
static inline size_t htsmsg_malloc_align(int type, size_t len)
{
  if (type == HMF_LIST || type == HMF_MAP)
    return (len + (size_t)7) & ~(size_t)7;
  return len;
}

/**
 * Get a field name
 */
static inline const char *htsmsg_field_name(htsmsg_field_t *f)
{
  if (f->hmf_flags & HMF_NONAME) return "";
  return f->_hmf_name;
}

/**
 * Create a new map
 */
htsmsg_t *htsmsg_create_map(void);

/**
 * Create a new list
 */
htsmsg_t *htsmsg_create_list(void);

/**
 * Concat msg2 to msg1 (list or map)
 */
void htsmsg_concat(htsmsg_t *msg1, htsmsg_t *msg2);

/**
 * Remove a given field from a msg
 */
void htsmsg_field_destroy(htsmsg_t *msg, htsmsg_field_t *f);

/**
 * Destroys a message (map or list)
 */
void htsmsg_destroy(htsmsg_t *msg);

/**
 * Add an boolean field.
 */
void htsmsg_add_bool(htsmsg_t *msg, const char *name, int b);

/**
 * Add/update an boolean field.
 */
void htsmsg_set_bool(htsmsg_t *msg, const char *name, int b);

/**
 * Add an integer field where source is signed 64 bit.
 */
void htsmsg_add_s64(htsmsg_t *msg, const char *name, int64_t s64);

/**
 * Add/update an integer field where source is signed 64 bit.
 */
int  htsmsg_set_s64(htsmsg_t *msg, const char *name, int64_t s64);

/**
 * Add an integer field where source is unsigned 32 bit.
 */
static inline void
htsmsg_add_u32(htsmsg_t *msg, const char *name, uint32_t u32)
  { htsmsg_add_s64(msg, name, u32); }

/**
 * Add/update an integer field
 */
static inline int
htsmsg_set_u32(htsmsg_t *msg, const char *name, uint32_t u32)
  { return htsmsg_set_s64(msg, name, u32); }

/**
 * Add an integer field where source is signed 32 bit.
 */
static inline void
htsmsg_add_s32(htsmsg_t *msg, const char *name,  int32_t s32)
  { htsmsg_add_s64(msg, name, s32); }

/**
 * Add/update an integer field
 */
static inline int
htsmsg_set_s32(htsmsg_t *msg, const char *name,  int32_t s32)
  { return htsmsg_set_s64(msg, name, s32); }

/**
 * Add a string field.
 */
void htsmsg_add_str(htsmsg_t *msg, const char *name, const char *str);

/**
 * Add a string field (NULL check).
 */
void htsmsg_add_str2(htsmsg_t *msg, const char *name, const char *str);

/**
 * Add a string field (allocated using malloc).
 */
void htsmsg_add_str_alloc(htsmsg_t *msg, const char *name, char *str);

/**
 * Add a string field to a list only once.
 */
void htsmsg_add_str_exclusive(htsmsg_t *msg, const char *str);

/**
 * Add a string using printf-style for the value.
 */
void
htsmsg_add_str_printf(htsmsg_t *msg, const char *name, const char *fmt, ...)
  __attribute__((format(printf,3,4)));;

/**
 * Add/update a string field
 */
int  htsmsg_set_str(htsmsg_t *msg, const char *name, const char *str);
int  htsmsg_set_str2(htsmsg_t *msg, const char *name, const char *str);

/**
 * Update a string field
 */
int  htsmsg_field_set_str(htsmsg_field_t *f, const char *str);
int  htsmsg_field_set_str_force(htsmsg_field_t *f, const char *str);

/**
 * Add an field where source is a list or map message.
 */
htsmsg_t *htsmsg_add_msg(htsmsg_t *msg, const char *name, htsmsg_t *sub);

/**
 * Add/update an field where source is a list or map message.
 */
htsmsg_t *htsmsg_set_msg(htsmsg_t *msg, const char *name, htsmsg_t *sub);

/**
 * Add an field where source is a double
 */
void htsmsg_add_dbl(htsmsg_t *msg, const char *name, double dbl);

/**
 * Add an field where source is a list or map message.
 *
 * This function will not strdup() \p name but relies on the caller
 * to keep the string allocated for as long as the message is valid.
 */
void htsmsg_add_msg_extname(htsmsg_t *msg, const char *name, htsmsg_t *sub);

/**
 * Update an binary field
 */
int  htsmsg_field_set_bin(htsmsg_field_t *f, const void *bin, size_t len);
int  htsmsg_field_set_bin_force(htsmsg_field_t *f, const void *bin, size_t len);

/**
 * Add an binary field. The data is copied to a inallocated storage.
 */
void htsmsg_add_bin(htsmsg_t *msg, const char *name, const void *bin, size_t len);

/**
 * Add an binary field. The passed data must be mallocated.
 */
void htsmsg_add_bin_alloc(htsmsg_t *msg, const char *name, const void *bin, size_t len);

/**
 * Add an binary field. The data is not copied, instead the caller
 * is responsible for keeping the data valid for as long as the message
 * is around.
 */
void htsmsg_add_bin_ptr(htsmsg_t *msg, const char *name, const void *bin, size_t len);

/**
 * Add/update a uuid field
 */
int htsmsg_set_uuid(htsmsg_t *msg, const char *name, tvh_uuid_t *u);

/**
 * Add an uuid field.
 */
void htsmsg_add_uuid(htsmsg_t *msg, const char *name, tvh_uuid_t *u);

/**
 * Get an integer as an unsigned 32 bit integer.
 *
 * @return HTSMSG_ERR_FIELD_NOT_FOUND - Field does not exist
 *         HTSMSG_ERR_CONVERSION_IMPOSSIBLE - Field is not an integer or
 *              out of range for the requested storage.
 */
int htsmsg_get_u32(htsmsg_t *msg, const char *name, uint32_t *u32p);

int htsmsg_field_get_u32(htsmsg_field_t *f, uint32_t *u32p);

/**
 * Get an integer as an signed 32 bit integer.
 *
 * @return HTSMSG_ERR_FIELD_NOT_FOUND - Field does not exist
 *         HTSMSG_ERR_CONVERSION_IMPOSSIBLE - Field is not an integer or
 *              out of range for the requested storage.
 */
int htsmsg_get_s32(htsmsg_t *msg, const char *name,  int32_t *s32p);

int htsmsg_field_get_s32(htsmsg_field_t *f, int32_t *s32p);

/**
 * Get an integer as an signed 64 bit integer.
 *
 * @return HTSMSG_ERR_FIELD_NOT_FOUND - Field does not exist
 *         HTSMSG_ERR_CONVERSION_IMPOSSIBLE - Field is not an integer or
 *              out of range for the requested storage.
 */
int htsmsg_get_s64(htsmsg_t *msg, const char *name,  int64_t *s64p);

int htsmsg_field_get_s64(htsmsg_field_t *f, int64_t *s64p);

/*
 * Return the field \p name as an s64.
 */
int64_t htsmsg_get_s64_or_default(htsmsg_t *msg, const char *name, int64_t def);

int bool_check(const char *str);

int htsmsg_field_get_bool(htsmsg_field_t *f, int *boolp);

int htsmsg_get_bool(htsmsg_t *msg, const char *name, int *boolp);

int htsmsg_get_bool_or_default(htsmsg_t *msg, const char *name, int def);


/**
 * Get pointer to a binary field. No copying of data is performed.
 *
 * @param binp Pointer to a void * that will be filled in with a pointer
 *             to the data
 * @param lenp Pointer to a size_t that will be filled with the size of
 *             the data
 *
 * @return HTSMSG_ERR_FIELD_NOT_FOUND - Field does not exist
 *         HTSMSG_ERR_CONVERSION_IMPOSSIBLE - Field is not a binary blob.
 */
int htsmsg_get_bin(htsmsg_t *msg, const char *name, const void **binp,
		   size_t *lenp);

/**
 * Get uuid struct from a uuid field.
 *
 * @param u Pointer to the tvh_uuid_t structure.
 *
 * @return HTSMSG_ERR_FIELD_NOT_FOUND - Field does not exist
 *         HTSMSG_ERR_CONVERSION_IMPOSSIBLE - Field is not a binary blob.
 */
int htsmsg_get_uuid(htsmsg_t *msg, const char *name, tvh_uuid_t *u);

/**
 * Get a field of type 'list'. No copying is done.
 *
 * @return NULL if the field can not be found or not of list type.
 *         Otherwise a htsmsg is returned.
 */
htsmsg_t *htsmsg_get_list(const htsmsg_t *msg, const char *name);

htsmsg_t *htsmsg_field_get_list(htsmsg_field_t *f);

/**
 * Get a field of type 'string'. No copying is done.
 *
 * @return NULL if the field can not be found or not of string type.
 *         Otherwise a pointer to the data is returned.
 */
const char *htsmsg_get_str(htsmsg_t *msg, const char *name);

/**
 * Get a field of type 'map'. No copying is done.
 *
 * @return NULL if the field can not be found or not of map type.
 *         Otherwise a htsmsg is returned.
 */
htsmsg_t *htsmsg_get_map(htsmsg_t *msg, const char *name);

htsmsg_t *htsmsg_field_get_map(htsmsg_field_t *f);

/**
 * Traverse a hierarchy of htsmsg's to find a specific child.
 */
htsmsg_t *htsmsg_get_map_multi(htsmsg_t *msg, ...)
  __attribute__((__sentinel__(0)));

/**
 * Traverse a hierarchy of htsmsg's to find a specific child.
 */
const char *htsmsg_get_str_multi(htsmsg_t *msg, ...)
  __attribute__((__sentinel__(0)));

/**
 * Get a field of type 'double'.
 *
 * @return HTSMSG_ERR_FIELD_NOT_FOUND - Field does not exist
 *         HTSMSG_ERR_CONVERSION_IMPOSSIBLE - Field is not an integer or
 *              out of range for the requested storage.
 */
int htsmsg_get_dbl(htsmsg_t *msg, const char *name, double *dblp);

int htsmsg_field_get_dbl(htsmsg_field_t *f, double *dblp);

/**
 * Given the field \p f, return a string if it is of type string, otherwise
 * return NULL
 */
const char *htsmsg_field_get_string(htsmsg_field_t *f);
#define htsmsg_field_get_str(f) htsmsg_field_get_string(f)

/**
 * Given the field \p f, return a uuid if it is of type string, otherwise
 * return NULL
 */
int htsmsg_field_get_uuid(htsmsg_field_t *f, tvh_uuid_t *u);

/**
 * Get a field of type 'bin'.
 *
 * @return HTSMSG_ERR_FIELD_NOT_FOUND - Field does not exist
 *         HTSMSG_ERR_CONVERSION_IMPOSSIBLE - Field is not an integer or
 *              out of range for the requested storage.
 */
int htsmsg_field_get_bin(htsmsg_field_t *f, const void **binp, size_t *lenp);

/**
 * Return the field \p name as an u32.
 *
 * @return An unsigned 32 bit integer or NULL if the field can not be found
 *         or if conversion is not possible.
 */
int htsmsg_get_u32_or_default(htsmsg_t *msg, const char *name, uint32_t def);

/**
 * Return the field \p name as an s32.
 *
 * @return A signed 32 bit integer or NULL if the field can not be found
 *         or if conversion is not possible.
 */
int32_t htsmsg_get_s32_or_default(htsmsg_t *msg, const char *name, 
				  int32_t def);

/**
 * Remove the given field called \p name from the message \p msg.
 */
int htsmsg_delete_field(htsmsg_t *msg, const char *name);

/**
 * Is list/map empty
 */
int htsmsg_is_empty(htsmsg_t *msg);

/**
 * Detach will remove the given field (and only if it is a list or map)
 * from the message and make it a 'standalone message'. This means
 * the the contents of the sub message will stay valid if the parent is
 * destroyed. The caller is responsible for freeing this new message.
 */
htsmsg_t *htsmsg_detach_submsg(htsmsg_field_t *f);

/**
 * Print a message to stdout. 
 */
void htsmsg_print(htsmsg_t *msg);

/**
 * Create a new field. Primarily intended for htsmsg internal functions.
 */
htsmsg_field_t *htsmsg_field_add(htsmsg_t *msg, const char *name,
				 int type, int flags, size_t esize);

/**
 * Get a field, return NULL if it does not exist
 */
htsmsg_field_t *htsmsg_field_find(const htsmsg_t *msg, const char *name);

/**
 * Get a last field, return NULL if it does not exist
 */
htsmsg_field_t *htsmsg_field_last(htsmsg_t *msg);

/**
 * Clone a message.
 */
htsmsg_t *htsmsg_copy(const htsmsg_t *src);

/**
 * Copy only one field from one htsmsg to another (with renaming).
 */
void htsmsg_copy_field(htsmsg_t *dst, const char *dstname,
                       const htsmsg_t *src, const char *srcname);

/**
 * Compare a message.
 */
int htsmsg_cmp(const htsmsg_t *m1, const htsmsg_t *m2);

#define HTSMSG_FOREACH(f, msg) TAILQ_FOREACH(f, &(msg)->hm_fields, hmf_link)


/**
 * Misc
 */
htsmsg_t *htsmsg_get_map_in_list(htsmsg_t *m, int num);

htsmsg_t *htsmsg_get_map_by_field_if_name(htsmsg_field_t *f, const char *name);

const char *htsmsg_get_cdata(htsmsg_t *m, const char *field);

char *htsmsg_list_2_csv(htsmsg_t *m, char delim, int human);

htsmsg_t *htsmsg_csv_2_list(const char *str, char delim);

htsmsg_t *htsmsg_create_key_val(const char *key, const char *val);

int htsmsg_is_string_in_list(htsmsg_t *list, const char *str);

int htsmsg_remove_string_from_list(htsmsg_t *list, const char *str);

/**
 *
 */
struct memoryinfo;
extern struct memoryinfo htsmsg_memoryinfo;
extern struct memoryinfo htsmsg_field_memoryinfo;
