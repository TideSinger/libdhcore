/***********************************************************************************
 * Copyright (c) 2012, Sepehr Taghdisian
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 ***********************************************************************************/


#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

#include "types.h"
#include "core-api.h"

/**
 * @defgroup ll Linked-list
 * Common linked-list structure
 * Usage :\n
 * For each data type (structure) that you need to make it linked-list\n
 * Define linked_list data in it, I call it 'node'.\n
 * Keep track of root link in the parent structure (structure owning the linked-list)\n
 * By defining a pointer to linked_list in it, I call it 'first'\n
 * Use **first** as plist in linked-list functions.\n
 * Use **node** as item in linked-list functions.\n
 * provide owner structure for each node in list_add/list_remove functions to reference the owner\n
 * Example: \n @code
 * struct my_listitem   {
 *      struct linked_list node;
 *      uint my_id;
 * };
 *
 * // add to list
 * struct my_listitem items[10];
 * struct linked_list* my_list = NULL;
 * for (uint i = 0; i < 10; i++)  {
 *      list_add(&my_list, &items[i].node, &items[i]);
 * }
 * // iterate
 * struct linked_list* node = my_list;
 * while (node != NULL) {
 *      struct my_listitem* item = node->data;
 *      printf("ID: %d\n", item->my_id);
 *      node = node->next;
 * }
 * @endcode
 * @ingroup ll
 * @see list_add
 * @see list_addlast
 * @see list_remove
 */
struct linked_list
{
    struct linked_list* next;
    struct linked_list* prev;
    void* data;

#ifdef __cplusplus
    linked_list() : next(NULL), prev(NULL)  {}
#endif
};

/**
 * add item to the linked-list, this function adds the list_item to the head of the list \n
 * so linked_list pointer will be swaped with new item
 * @param plist pointer to the root item of the list (can be NULL)
 * @param item new item to be added
 * @param data custom data pointer, mostly owner of the list 'item'
 * @ingroup ll
 */
INLINE void list_add(struct linked_list** plist, struct linked_list* item, void* data)
{
    item->next = (*plist);
    item->prev = NULL;
    if (*plist != NULL)
        (*plist)->prev = item;
    *plist = item;
    item->data = data;
}

/**
 * add item to the end of the linked-list
 * @see list_add    @ingroup ll
 */
INLINE void list_addlast(struct linked_list** plist, struct linked_list* item, void* data)
{
    if (*plist != NULL)     {
        struct linked_list* last = *plist;
        while (last->next != NULL)    last = last->next;
        last->next = item;
        item->prev = last;
        item->next = NULL;
    }    else    {
        *plist = item;
        item->prev = item->next = NULL;
    }

    item->data = data;
}

/**
 * remove item from linked-list
 * @param plist pointer to the root item the list, if the last item is removed *plist will be NULL
 * @param item item that will be removed
 * @ingroup ll
 */
INLINE void list_remove(struct linked_list** plist, struct linked_list* item)
{
    if (item->next != NULL)     item->next->prev = item->prev;
    if (item->prev != NULL)     item->prev->next = item->next;
    if (*plist == item)         *plist = item->next;
    item->next = item->prev = NULL;
}

#ifdef __cplusplus
namespace dh {
template <typename _T>
class LinkedList
{
private:
    LinkedList<_T> *m_next = nullptr;
    LinkedList<_T> *m_prev = nullptr;
    _T *m_obj = nullptr;

public:
    LinkedList() = default;
    _T* obj() const     {   return m_obj;   }
    LinkedList<_T>* next() const    {   return m_next;  }
    LinkedList<_T>* prev() const    {   return m_prev;  }

public:

    static void add(LinkedList<_T> **plist, LinkedList<_T> *new_item, _T *obj)
    {
        new_item->m_next = (*plist);
        new_item->m_prev = nullptr;
        if (*plist)
            (*plist)->m_prev = new_item;
        *plist = new_item;
        new_item->m_obj = obj;
    }

    static void add_last(LinkedList<_T> **plist, LinkedList<_T> *new_item, _T *obj)
    {
        if (*plist)     {
            LinkedList<_T> *last = *plist;
            while (last->m_next)
                last = last->m_next;
            last->m_next = new_item;
            new_item->m_prev = last;
            new_item->m_next = nullptr;
        }    else    {
            *plist = new_item;
            new_item->m_prev = new_item->m_next = nullptr;
        }

        new_item->m_obj = obj;
    }

    static void remove(LinkedList<_T> **plist, LinkedList<_T> *item)
    {
       if (item->m_next)
           item->m_next->m_prev = item->m_prev;
       if (item->m_prev)
           item->m_prev->m_next = item->m_next;
       if (*plist == item)
           *plist = item->m_next;
       item->m_next = item->m_prev = nullptr;
    }
};
}
#endif

#endif /* __LINKEDLIST_H__ */
