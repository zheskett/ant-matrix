#pragma once
#ifndef DYNARR_H
#define DYNARR_H

#include <stdlib.h>
#include <string.h>

#define DYN_ARR_INIT_CAPACITY 8

#define dyn_arr_def(type)                                                                                              \
  struct {                                                                                                             \
    type *data;                                                                                                        \
    int length;                                                                                                        \
    int capacity;                                                                                                      \
  }

// The do, while(0) allows the macro to be used in a single statement context (if(<statement>) <macro>;)
#define dyn_arr_init(dyn_arr)                                                                                          \
  do {                                                                                                                 \
    (dyn_arr).data = malloc(DYN_ARR_INIT_CAPACITY * sizeof(*(dyn_arr).data));                                          \
    if (!(dyn_arr).data) {                                                                                             \
      (dyn_arr).length = 0;                                                                                            \
      (dyn_arr).capacity = 0;                                                                                          \
    } else {                                                                                                           \
      (dyn_arr).length = 0;                                                                                            \
      (dyn_arr).capacity = DYN_ARR_INIT_CAPACITY;                                                                      \
    }                                                                                                                  \
  } while (0)

#define dyn_arr_get(dyn_arr, i) (((dyn_arr).data)[(i)])

#define dyn_arr_push(dyn_arr, elem)                                                                                    \
  do {                                                                                                                 \
    if ((dyn_arr).length >= (dyn_arr).capacity) {                                                                      \
      (dyn_arr).capacity *= 2;                                                                                         \
      (dyn_arr).data = realloc((dyn_arr).data, (dyn_arr).capacity * sizeof(*(dyn_arr).data));                          \
    }                                                                                                                  \
    (dyn_arr).data[(dyn_arr).length++] = (elem);                                                                       \
  } while (0)

#define dyn_arr_pusharr(dyn_arr, elems, n)                                                                             \
  do {                                                                                                                 \
    if ((dyn_arr).length + (n) > (dyn_arr).capacity) {                                                                 \
      while ((dyn_arr).length + (n) > (dyn_arr).capacity) {                                                            \
        (dyn_arr).capacity *= 2;                                                                                       \
      }                                                                                                                \
      (dyn_arr).data = realloc((dyn_arr).data, (dyn_arr).capacity * sizeof(*(dyn_arr).data));                          \
    }                                                                                                                  \
    memcpy((dyn_arr).data + (dyn_arr).length, (elems), (n) * sizeof(*(dyn_arr).data));                                 \
    (dyn_arr).length += (n);                                                                                           \
  } while (0)

#define dyn_arr_remove(dyn_arr, i)                                                                                     \
  do {                                                                                                                 \
    memmove((dyn_arr).data + (i), (dyn_arr).data + (i) + 1, ((dyn_arr).length - (i) - 1) * sizeof(*(dyn_arr).data));   \
    (dyn_arr).length--;                                                                                                \
  } while (0)

#define dyn_arr_clear(dyn_arr) (dyn_arr).length = 0

#define dyn_arr_free(dyn_arr)                                                                                          \
  do {                                                                                                                 \
    free((dyn_arr).data);                                                                                              \
    (dyn_arr).data = NULL;                                                                                             \
    (dyn_arr).length = 0;                                                                                              \
    (dyn_arr).capacity = 0;                                                                                            \
  } while (0)

typedef dyn_arr_def(double) dyn_arr_dbl_t;

#endif /* DYNARR_H */