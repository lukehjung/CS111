/*
NAME: Luke Jung
EMAIL: lukejung@ucla.edu
UID: 904982644
*/

#include "SortedList.h"
#include <stdio.h>
#include <string.h>
#include <sched.h>

int yield = 0;

void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
	if(list == NULL || element == NULL)
		return;
	if(list->next == NULL)
	{
		if(INSERT_YIELD & yield)
			sched_yield();
		list->next = element;
		element-> prev = list;
		element -> next = NULL;
		return;
	}

	SortedListElement_t * head = list;
	SortedListElement_t * temp = list->next;

	while(temp != NULL && strcmp(element->key, temp->key) >= 0)
	{
		head = temp;
		temp = temp->next;
	}

	if(INSERT_YIELD & yield)
		sched_yield();
	if(temp == NULL)
	{
		head->next = element;
		element->prev = head;
		element->next = NULL;
	}

	else
	{
		temp->prev = element;
		head->next = element;
		element->prev = head;
		element->next = temp;
	}
}

int SortedList_delete( SortedListElement_t *element)
{
	if(element == NULL)
		return 1;
	else if(yield & DELETE_YIELD)
		sched_yield();
	if (element->next != NULL) {
		if (element->next->prev != element) {
			return 1;
		}
		else {
			element->next->prev = element->prev;
		}
	}

	if (element->prev != NULL) {
		if (element->prev->next != element) {
			return 1;
		} 
		else {
			element->prev->next = element->next;
		}
	}
    return 0;
}


SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) 
{
	if(list == NULL)
		return NULL;

	else
	{
		SortedListElement_t * temp = list->next;
		while(temp != NULL)
		{
			if(yield & LOOKUP_YIELD)
				sched_yield();
			if(strcmp(key, temp->key) == 0)
				return temp;
			temp = temp->next;
		}
	}
	return NULL;
}

int SortedList_length(SortedList_t *list) 
{
	int count = 0;
	if(list == NULL)
		return -1;
	SortedListElement_t * temp = list->next;

	while(temp != NULL)
	{
		if(yield & LOOKUP_YIELD)
			sched_yield();

		temp = temp->next;
		count += 1;
	}

	return count;
}