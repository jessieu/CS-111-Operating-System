#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <sys/types.h>
#include "SortedList.h"

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
    SortedListElement_t *cur = list->next;
    while (cur != list && strcmp(cur->key, element->key) > 0)
        cur = cur->next;

    if (opt_yield && INSERT_YIELD) sched_yield();

    element->next = cur;
    element->prev = cur->prev;
    cur->prev->next = element;
    cur->prev = element;
}

int SortedList_delete(SortedListElement_t *element) {
    SortedListElement_t *prev = element->prev;
    SortedListElement_t *next = element->next;
    // corrupted element
    if (element == NULL || prev->next != element || next->prev != element)
        return 1;

    if (opt_yield && DELETE_YIELD) sched_yield();

    prev->next = next;
    next->prev = prev;
    element = NULL;

    return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    // head cannot be deleted
    if (!key) return NULL;
    SortedListElement_t *cur = list->next;
    while (cur != list && strcmp(cur->key, key) != 0) {
        cur = cur->next;
        if (opt_yield && LOOKUP_YIELD) sched_yield();
    }

    if (cur == list) return NULL;

    return cur;
}

int SortedList_length(SortedList_t *list) {
    int length = 0;
    SortedListElement_t *cur = list->next;
    // corrupted list
    if (cur->prev != list) return -1;

    while (cur != list) {
        if (cur == NULL || cur->next == NULL || cur->prev == NULL) return -1;
        if (cur->prev->next != cur || cur->next->prev != cur) return -1;
        if (opt_yield && LOOKUP_YIELD) sched_yield();
        length++;
        cur = cur->next;
    }
    return length;
}
