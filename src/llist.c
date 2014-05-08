#include "common.h"

/**
 * ipinfo_compare
 * 
 * compares the items in the ipinfo list
 * 
 * @param _a
 * @param _b
 * @return 
 */
bool ipinfo_compare(const void * _a, const void * _b) {
    const ipinfo *a = (const ipinfo *) _a;
    const ipinfo *b = (const ipinfo *) _b;

    if (a->src_sd == b->src_sd)
        return true;
    else
        return false;
}

/**
 * sd_compare
 * 
 * checks the node if the contains the specified descriptor
 * 
 * @param _a
 * @param _b
 * @return 
 */
bool sd_compare(const void * _a, const int _b) {
    const ipinfo *a = (const ipinfo *) _a;
    const int b = _b;

    if (a->src_sd == b)
        return true;
    else
        return false;
}

/**
 * portfwd_compare
 * 
 * compares items in the portfwd list
 * 
 * @param _a
 * @param _b
 * @return 
 */
bool portfwd_compare(const void * _a, const void * _b) {
    const port_fwd *a = (const port_fwd *) _a;
    const port_fwd *b = (const port_fwd *) _b;

    if (a->src_sd == b->src_sd)
        return true;
    else
        return false;
}

/* Linked list */

/**
 * llist_new
 * 
 * mallocs a space for a list.
 * 
 * @return l list
 */
llist *llist_new(void) {
    llist *l = malloc(sizeof (llist));
    if (l == NULL)
        fprintf(stderr, "llist_new: malloc() failed\n");
    l->link = NULL;
    return l;
}

/**
 * llist_free
 * 
 * @param l
 * @param free_func
 */
void llist_free(llist *l, void (*free_func)(void *)) {
    node *n = l->link;
    while (n) {
        if (n->data != NULL)
            free_func(n->data);
        l->link = l->link->next;
        free(n);
    }
    free(l);
}

/**
 * llist_append
 * 
 * adds to the list
 * 
 * @param l
 * @param data
 * @return 
 */
llist *llist_append(llist *l, void *data) {
    node *a = NULL;
    node *b = NULL;

    a = l->link;

    if (l == NULL)
        return l;

    if (a == NULL) {
        a = malloc(sizeof (node));
        a->data = data;
        a->next = NULL;
        l->link = a;
    } else {
        while (a->next != NULL)
            a = a->next;

        b = malloc(sizeof (node));
        b->data = data;
        b->next = NULL;
        a->next = b;
    }

    return l;
}

/**
 * llist_remove
 * 
 * removes from the list
 * 
 * @param l
 * @param data
 * @param compare_func
 * @return 
 */
llist *llist_remove(llist *l, void *data,
        bool(*compare_func)(const void*, const void*)) {
    node *old = NULL;
    node *temp = NULL;

    if (l == NULL)
        fprintf(stderr, "Empty list, can't delete data\n");
    else {
        temp = l->link;
        while (temp != NULL) {
            if (compare_func(temp->data, data)) {
                if (temp == l->link) /* first Node case */
                    l->link = temp->next; /* shift the header node */
                else
                    old->next = temp->next;
                free(temp);
                return l;
            } else {
                old = temp;
                temp = temp->next;
            }
        }
    }

    return l;
}

/**
 * llist_find
 * 
 * finds an item in the list
 * 
 * @param l
 * @param fd
 * @param compare_func
 * @return 
 */
void *llist_find(llist *l, int fd,
        bool(*compare_func)(const void*, int fd)) {
    node *temp = NULL;
    void *d = NULL;
    if (l == NULL)
        fprintf(stderr, "Empty list, unable to find");
    else {
        temp = l->link;
        while (temp != NULL) {
            if (compare_func(temp->data, fd)) {
                d = temp->data;
                break;
            } else
                temp = temp->next;
        }
    }
    return d;
}

/**
 * llist_lenght
 * 
 * returns the length of the list
 * 
 * @param l
 * @return 
 */
int llist_length(llist *l) {
    int n;
    node *p = l->link;
    n = 0;
    while (p != NULL) {
        n++;
        p = p->next;
    }
    return n;
}

/**
 * llist_is_empty
 * 
 * checks of the list is empty or not
 * 
 * @param l
 * @return 
 */
bool llist_is_empty(llist *l) {
    if (l == NULL || l->link == NULL)
        return true;
    return false;
}