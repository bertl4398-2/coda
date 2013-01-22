/**
 * llist.c
 *
 *  Created on: May 9, 2012
 *      Author: Francisco de Meneses Neves Ramos dos Santos
 *      Email: francisco.santos@epfl.ch
 *      Used: java.util.LinkedList
 */

#include <ccn/llist.h>

/*
 * linkedlist
 */

list_entry_t* createListEntry(void* pData, unsigned int nBytes) {
	list_entry_t* newListEntry = calloc(1, sizeof(list_entry_t));
	assert(newListEntry != (list_entry_t*) 0);
	assert(nBytes > 0);
	newListEntry->entry.data = calloc(nBytes, 1);
	newListEntry->entry.nBytes = nBytes;
	memcpy(newListEntry->entry.data, pData, nBytes);
	return newListEntry;
}
int destroyListEntry(list_entry_t* pEntry) {
	assert(pEntry != (list_entry_t*) 0);
	assert(pEntry->entry.data != (void*) 0);
	free(pEntry->entry.data);
	free(pEntry);
	return 1;
}

linkedlist_t* createList() {
	linkedlist_t* newLinkedList = calloc(1, sizeof(linkedlist_t));
	assert(newLinkedList != (linkedlist_t *) 0);
	return newLinkedList;
}
void llclear(linkedlist_t* pList) {
	list_entry_t* pEntry = (list_entry_t*) 0;
	list_entry_t* pAux = (list_entry_t*) 0;
	if (pList->size > 0) {
		pList->modCount++;
		pEntry = pList->first;
		while (pEntry != (list_entry_t *) 0) {
			pAux = pEntry;
			pEntry = pEntry->next;
			destroyListEntry(pAux);
		}
		pList->first = pList->last = (list_entry_t *) 0;
		pList->size = 0;
	}
}
int destroyList(linkedlist_t* pList) {
	llclear(pList);
	free(pList);
	return 1;
}

list_entry_t* _llgetEntry(linkedlist_t* pList, int entry_number) {
	list_entry_t* pEntry = (list_entry_t*) 0;
	if (entry_number < pList->size / 2) {
		pEntry = pList->first;
		/* entry_number less than size/2 iterate from start */
		while (entry_number-- > 0) {
			pEntry = pEntry->next;
		}
	} else {
		pEntry = pList->last;
		/* entry_number greater than size/2 iterate from end */
		while (++entry_number < pList->size) {
			pEntry = pEntry->previous;
		}
	}
	return pEntry;
}

void llremoveEntry(linkedlist_t* pList, list_entry_t* pEntry) {
	assert(pEntry != (list_entry_t*) 0);
	pList->modCount++;
	pList->size--;
	if (pList->size == 0) {
		assert(pList->first != (list_entry_t*) 0);
		destroyListEntry(pList->first);
		pList->first = pList->last = (list_entry_t *) 0;
	} else {
		if (pEntry == pList->first) {
			pList->first = pEntry->next;
			assert(pEntry->next->previous != (list_entry_t *) 0);
			destroyListEntry(pEntry->next->previous);
			pEntry->next->previous = (list_entry_t*) 0;
		} else if (pEntry == pList->last) {
			pList->last = pEntry->previous;
			assert(pEntry->previous->next != (list_entry_t *) 0);
			destroyListEntry(pEntry->previous->next);
		} else {
			pEntry->next->previous = pEntry->previous;
			pEntry->previous->next = pEntry->next;
			destroyListEntry(pEntry);
			pEntry = (list_entry_t*) 0;
		}
	}
}
data_entry_t* llgetFirst(linkedlist_t* pList) {
	assert(pList->size > 0);
	return &pList->first->entry;
}
data_entry_t* llgetLast(linkedlist_t* pList) {
	assert(pList->size > 0);
	return &pList->last->entry;
}
data_entry_t* llremoveFirst(linkedlist_t* pList) {
	data_entry_t* pData = (data_entry_t*) 0;
	assert(pList->size > 0);
	pList->modCount++;
	pList->size--;
	pData = &pList->first->entry;
	if (pList->first->next != (list_entry_t *) 0) {
		assert(pList->first->next->previous != (list_entry_t *) 0);
		destroyListEntry(pList->first->next->previous);
		pList->first->next->previous = (list_entry_t *) 0;
	} else {
		assert(pList->last);
		destroyListEntry(pList->last);
		pList->last = (list_entry_t *) 0;
	}
	pList->first = pList->first->next;
	return pData;
}
data_entry_t* llremoveLast(linkedlist_t* pList) {
	data_entry_t* pData = (data_entry_t*) 0;
	assert(pList->size > 0);
	pList->modCount++;
	pList->size--;
	pData = &pList->last->entry;
	if (pList->last->previous != (list_entry_t *) 0) {
		assert(pList->last->previous->next != (list_entry_t *) 0);
		destroyListEntry(pList->last->previous->next);
		pList->last->previous->next = (list_entry_t *) 0;
	} else {
		assert(pList->first);
		destroyListEntry(pList->first);
		pList->first = (list_entry_t *) 0;
	}

	pList->last = pList->last->previous;

	return pData;
}

void lladdFirst(linkedlist_t* pList, void* pData, unsigned int nBytes) {
	list_entry_t *pEntry = (list_entry_t *) 0;
	pEntry = createListEntry(pData, nBytes);
	pList->modCount++;
	if (pList->size == 0) {
		pList->first = pList->last = pEntry;
	} else {
		pEntry->next = pList->first;
		pList->first->previous = pEntry;
		pList->first = pEntry;
	}
	pList->size++;
}

void lladdLast(linkedlist_t* pList, void* pData, unsigned int nBytes) {
	_lladdLastEntry(pList, createListEntry(pData, nBytes));
}
void _lladdLastEntry(linkedlist_t* pList, list_entry_t* pEntry) {
	pList->modCount++;
	if (pList->size == 0) {
		pList->first = pList->last = pEntry;
	} else {
		pEntry->previous = pList->last;
		pList->last->next = pEntry;
		pList->last = pEntry;
	}
	pList->size++;
}
int llcontains(linkedlist_t* pList, void* pData, unsigned int nBytes) {
	list_entry_t* pEntry = pList->first;
	while (pEntry != (list_entry_t *) 0) {
		if (pEntry->entry.nBytes == nBytes
				&& memcmp(pEntry->entry.data, pData, nBytes) == 0) {
			return 1;
		}
		pEntry = pEntry->next;
	}
	return 0;
}
unsigned int lllength(linkedlist_t* pList) {
	return pList->size;
}
int lladd(linkedlist_t* pList, void* pData, unsigned int nBytes) {
	_lladdLastEntry(pList, createListEntry(pData, nBytes));
	return 1;
}
int llremoveData(linkedlist_t* pList, void* pData) {
	list_entry_t* pEntry = pList->first;
	while (pEntry != (list_entry_t *) 0) {
		if (pData == pEntry->entry.data) {
			llremoveEntry(pList, pEntry);
			return 1;
		}
		pEntry = pEntry->next;
	}
	return 0;
}
data_entry_t* llget(linkedlist_t* pList, int index) {
	_llcheckBoundsExclusive(pList, index);
	return &_llgetEntry(pList, index)->entry;
}
data_entry_t* llremoveIndex(linkedlist_t* pList, int index) {
	list_entry_t* pEntry = (list_entry_t *) 0;
	_llcheckBoundsExclusive(pList, index);
	pEntry = _llgetEntry(pList, index);
	llremoveEntry(pList, pEntry);
	return &pEntry->entry;

}
void _llcheckBoundsExclusive(linkedlist_t* pList, int index) {
	assert(index >= 0 && index < pList->size);
}

data_entry_t* lltoArray(linkedlist_t* pList) {
	list_entry_t* pEntry = (list_entry_t *) 0;
	data_entry_t* arr = (data_entry_t *) 0;
	int i = 0;
	if (pList->size == 0) {
		return (data_entry_t *) 0;
	}

	arr = (data_entry_t *) calloc(pList->size, sizeof(data_entry_t));

	for (pEntry = pList->first; pEntry != (list_entry_t *) 0;
			pEntry = pEntry->next) {
		arr[i].data = pEntry->entry.data;
		arr[i].nBytes = pEntry->entry.nBytes;
		i++;
	}
	return arr;
}

void llsort(linkedlist_t* list, int(*comp)(const void*, const void*)){
	int i = 0;
	listiterator_t* iter = (listiterator_t*)0;
	data_entry_t *arr = lltoArray(list);
	qsort(&arr[0], list->size, sizeof(data_entry_t), comp);
	iter = createListIterator(list, 0);
	for(;i<list->size; ++i){
		linext(iter);
		liset(iter, arr[i].data, arr[i].nBytes);
	}
	destroyListIterator(iter);
	free(arr);

}

/*
 * linkedlist iterator
 */

listiterator_t* createListIterator(linkedlist_t* list, int index) {
	listiterator_t* itr = calloc(1, sizeof(listiterator_t));
	itr->list = list;
	itr->knownMod = list->modCount;

	if (index == list->size) {
		itr->next = (list_entry_t*) 0;
		itr->previous = (list_entry_t *) list->last;
	} else {
		itr->next = _llgetEntry(list, index);
		itr->previous = itr->next->previous;
	}
	itr->position = index;
	return itr;
}
int destroyListIterator(listiterator_t* listIter) {
	free(listIter);
	return 1;
}
void licheckMod(listiterator_t* listIter) {
	assert(listIter->knownMod == listIter->list->modCount);
}
int linextIndex(listiterator_t* listIter) {
	return listIter->position;
}
int lipreviousIndex(listiterator_t* listIter) {
	return listIter->position - 1;
}
int lihasNext(listiterator_t* listIter) {
	return (listIter->next != (list_entry_t*) 0);
}
int lihasPrevious(listiterator_t* listIter) {
	return (listIter->previous != (list_entry_t*) 0);
}
data_entry_t* linext(listiterator_t* listIter) {
	licheckMod(listIter);
	assert(listIter->next != (list_entry_t *)0);
	listIter->position++;
	listIter->lastReturned = listIter->previous = listIter->next;
	listIter->next = listIter->lastReturned->next;
	return &listIter->lastReturned->entry;
}
data_entry_t* liprevious(listiterator_t* listIter) {
	licheckMod(listIter);
	assert(listIter->previous != (list_entry_t *) 0);
	listIter->position--;
	listIter->lastReturned = listIter->next = listIter->previous;
	listIter->previous = listIter->lastReturned->previous;
	return &listIter->lastReturned->entry;
}
void liremove(listiterator_t* listIter) {
	licheckMod(listIter);
	assert(listIter->lastReturned != (list_entry_t*)0);
	if (listIter->lastReturned == listIter->previous) {
		listIter->position--;
	}
	listIter->next = listIter->lastReturned->next;
	listIter->previous = listIter->lastReturned->previous;
	llremoveEntry(listIter->list, listIter->lastReturned);
	listIter->knownMod++;
	listIter->lastReturned = (list_entry_t *) 0;
}

void liadd(listiterator_t* listIter, void* data, unsigned int nBytes) {
	licheckMod(listIter);
	listIter->list->modCount++;
	listIter->knownMod++;
	listIter->list->size++;
	listIter->position++;
	list_entry_t* e = createListEntry(data, nBytes);
	e->previous = listIter->previous;
	e->next = listIter->next;
	if (listIter->previous != (list_entry_t*) 0) {
		listIter->previous->next = e;
	} else {
		listIter->list->first = e;
	}

	if (listIter->next != (list_entry_t*) 0) {
		listIter->next->previous = e;
	} else {
		listIter->list->last = e;
	}
	listIter->previous = e;
	listIter->lastReturned = (list_entry_t *) 0;
}

void liset(listiterator_t* listIter, void* data, unsigned int nBytes) {
	licheckMod(listIter);
	assert(listIter->lastReturned != (list_entry_t *)0);
	listIter->lastReturned->entry.data = data;
	listIter->lastReturned->entry.nBytes = nBytes;
}
