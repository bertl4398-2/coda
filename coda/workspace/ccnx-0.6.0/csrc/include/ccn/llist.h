/*
 * llist.h
 *
 *  Created on: May 9, 2012
 *      Author: Francisco de Meneses Neves Ramos dos Santos
 *      Email: francisco.santos@epfl.ch
 */

#ifndef LLIST_H_
#define LLIST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct data_entry {
	void * data;
	unsigned int nBytes;
};

typedef struct data_entry data_entry_t;

struct list_entry {
	data_entry_t entry;
	struct list_entry * next;
	struct list_entry * previous;
};

typedef struct list_entry list_entry_t;

struct linkedlist {
	list_entry_t* first;
	list_entry_t* last;
	unsigned int size;
	unsigned int modCount;
};

typedef struct linkedlist linkedlist_t;

list_entry_t* createListEntry(void *, unsigned int);
int destroyListEntry(list_entry_t*);

linkedlist_t* createList(void);
int destroyList(linkedlist_t*);
list_entry_t* _llgetEntry(linkedlist_t*, int);
void llremoveEntry(linkedlist_t*, list_entry_t*);
data_entry_t* llgetFirst(linkedlist_t*);
data_entry_t* llgetLast(linkedlist_t*);
data_entry_t* llremoveFirst(linkedlist_t*);
data_entry_t* llremoveLast(linkedlist_t*);
void lladdFirst(linkedlist_t*, void*, unsigned int);
void lladdLast(linkedlist_t*, void*, unsigned int);
void _lladdLastEntry(linkedlist_t*, list_entry_t*);
int llcontains(linkedlist_t*, void*, unsigned int);
unsigned int lllength(linkedlist_t*);
int lladd(linkedlist_t*, void*, unsigned int);
int llremoveData(linkedlist_t*, void*);
data_entry_t* llget(linkedlist_t*, int);
data_entry_t* llremoveIndex(linkedlist_t*, int);
void llclear(linkedlist_t*);
void _llcheckBoundsExclusive(linkedlist_t*, int);
data_entry_t* lltoArray(linkedlist_t*);
void llsort(linkedlist_t*, int(*comp)(const void*, const void*));

struct listiterator {
	linkedlist_t* list;
	unsigned int knownMod;
	list_entry_t* next;
	list_entry_t* previous;
	list_entry_t* lastReturned;
	int position;
};

typedef struct listiterator listiterator_t;

listiterator_t* createListIterator(linkedlist_t*, int);
int destroyListIterator(listiterator_t*);
void licheckMod(listiterator_t*);
int linextIndex(listiterator_t*);
int lipreviousIndex(listiterator_t*);
int lihasNext(listiterator_t*);
int lihasPrevious(listiterator_t*);
data_entry_t* linext(listiterator_t*);
data_entry_t* liprevious(listiterator_t*);
void liremove(listiterator_t*);
void liadd(listiterator_t*, void*, unsigned int);
void liset(listiterator_t*, void*, unsigned int);

#endif /* LLIST_H_ */
