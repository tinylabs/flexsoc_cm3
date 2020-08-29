/**
 *  Singlely linked list implementation.
 *
 *  All rights reserved.
 *  Elliot Buller
 *  2014, 2018
 */
#include "ll.h"

void ll_add_head (list_t *head, list_t *e)
{
  if (head->next == head) {
    head->next = e;
    e->next = head;
  }
  else {
    e->next = head->next;
    head->next = e;
  }
}

void ll_add_tail (list_t *head, list_t *e)
{
  
  if (head->next == head) {
    head->next = e;
    e->next = head;
  }
  else {
    list_t *cur, *tmp;

    // Iterate over list the slow way
    ll_for_each_safe (cur, tmp, head) {
      if (cur->next == head) {
        cur->next = e;
        e->next = head;
        break;
      }
    }
  }
}

void ll_rm (list_t *head, list_t *e)
{
  list_t *cur, *tmp, *prev = head;

  // Iterate over list the slow way
  ll_for_each_safe (cur, tmp, head) {

    if (cur == e) {
      prev->next = cur->next;
      break;
    }
    prev = cur;
  }  
}

void ll_insert_after (list_t *elem, list_t *n_elem)
{
  list_t *tmp;

  if ((elem == NULL) || (n_elem == NULL))
    return;

  // Save element next
  tmp = elem->next;
  elem->next = n_elem;
  n_elem->next = tmp;
}
