#ifndef LIST_H
#define LIST_H

#include "bistack.h"
#include "utils.h"

#define MULTISTRING_BUFSIZE ((int)(sizeof(void*) * 3))

typedef struct listnode {
    struct listnode *next;
    void *val;
} LIST_NODE;

typedef struct list {
    /*
    * A reusable list.
    * If head is NULL, there are no nodes currently allocated.
    * The tail node should be examined to see if it has a non-null next;
    */
    struct listnode *head;
    struct listnode *tail;
    uint8_t count;
} LIST;

typedef struct multistring {
    LIST stringbufs;
    int totallen;
    char bufpos;
} MULTISTRING;

MULTISTRING *ms_new(BISTACK *bs);

/* Writes a character to the variable length string */
MULTISTRING *ms_writechar(MULTISTRING *ms, BISTACK *bs, char ch);

int ms_strncmp(MULTISTRING *ms, char *str, int n);

/* Places the multistring into a single char buffer.
** The allocated buffer must be stringlen+1 in length or overflow results. */
char *ms_assemble(MULTISTRING *ms, char *buff);

/* Iterates over the multistring character by character */
char ms_chariter(MULTISTRING *ms, int *counter, struct listnode** ptrbuf);

uint32_t ms_hash(MULTISTRING *ms);

/* Returns the total length of the multistring, not including \0 terminator */
static inline int ms_length(MULTISTRING *ms) {
    return ms->totallen;
}

LIST *list_new(BISTACK *bs);
LIST *list_init(LIST *list);
void *list_append(LIST *list, BISTACK *bs, void *val);
int list_count(LIST *list);
void *list_iter(LIST *list, void **iter);
void list_reset(LIST *list);
void *list_pop(LIST *list);
void *list_unshift(LIST *list, BISTACK *bs, void *val);
void *list_shift(LIST *list);

static inline void *list_first(LIST *list) {
    if (list->head != NULL) {
        return list->head->val;
    } else {
        return NULL;
    }
}

static inline void *list_second(LIST *list) {
    LIST_NODE *head = list->head;
    if (head != NULL && head->next != NULL) {
        return head->next->val;
    } else {
        return NULL;
    }
}
#endif