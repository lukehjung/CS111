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

	if(INSERT_YIELD & yield)
			sched_yield();

	SortedListElement_t * head = list;
	SortedListElement_t * temp = list->next;

	while(temp != head && strcmp(element->key, temp->key) >= 0)
	{
		temp = temp->next;
	}

	element->prev = temp->prev;
	element->next = temp;
	temp->prev->next = element;
	temp->prev = element;
}

int SortedList_delete( SortedListElement_t *element)
{
	if(element == NULL)
		return 1;
	else if(yield & DELETE_YIELD)
		sched_yield();
	element->prev->next = element->next;
	element->next->prev = element->prev;
	return 0;
}


SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) 
{
	if(list == NULL)
		return NULL;

	else
	{
		SortedListElement_t * temp = list->next;
		if(yield & LOOKUP_YIELD)
				sched_yield();
		while(temp != NULL)
		{	
			if(strcmp(key, temp->key) == 0)
				return temp;
			temp = temp->next;
		}
	}
	return NULL;
}

int SortedList_length(SortedList_t *list) 
{
	int size = 0;
	SortedListElement_t *temp = list->next;

	if (opt_yield & LOOKUP_YIELD) 
	{
		sched_yield();
	}

	while (temp != list) 
	{
		if ((temp->prev->next != temp) || (temp->next->prev != temp)) 
		{
			return -1;
		}

		size++;
		temp = temp->next;
	}
	return size;
}