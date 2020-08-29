/**
 *  Singlely linked list implementation. Public interface.
 *
 *  All rights reserved.
 *  Elliot Buller
 *  2014, 2018
 */
#ifndef _LL_H_
#define _LL_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ll_t {
  struct _ll_t *next;
} list_t;

  // Get data structure from list  
#define ll_entry(ptr, type, member)                                 \
  ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

  // Iterate over a list
#define ll_for_each(cur, head)                              \
  for (cur = (head)->next; cur != head; cur = cur->next)
  
  // Must be used if deleting items
#define ll_for_each_safe(cur, n, head)                                  \
  for (cur = (head)->next, n = cur->next; cur != head; cur = n, n = cur->next)
  
#define LL_INIT(list)     ((list)->next = list)
#define LL_IS_EMPTY(list) ((list)->next == list)
#define LL_IS_NULL(list)  ((list)->next == NULL)

void ll_add_head (list_t *head, list_t *e);
void ll_add_tail (list_t *head, list_t *e);
void ll_rm (list_t *head, list_t *e);
void ll_insert_after (list_t *elem, list_t *n_elem);

#ifdef __cplusplus
}
#endif

#endif /* _LL_H_ */
