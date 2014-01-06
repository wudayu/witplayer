#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "list.h"

Node *InitList(Node *node)
{
	node = NULL;

	return node;
}

static Node *CreateNode(LrcData data)
{
	Node *node;

	node = malloc(sizeof(*node));
	if (node == NULL)
		return NULL;


	node->Data.time = data.time;
	strcpy(node->Data.str, data.str);
	node->next = NULL;

	return node;
}

int DestroyNode(Node *node)
{
	if (node->next == NULL)
		return 0;
	else
		free(node);

	return 0;
}

Node *InsertList(Node *first, LrcData data)
{
	Node *node, *iter = first;

	node = CreateNode(data);

	if (node == NULL)
		return NULL;

	if (first == NULL) {
		first = node;
	} else {
		if (first->next == NULL) {
			if (node->Data.time - first->Data.time > 0) {
				first->next = node;
			} else {
				node->next = first;
				first = node;
			}
		} else if (node->Data.time < first->Data.time) {
			node->next = first;
			first = node;
		} else {
			while (iter->next != NULL && node->Data.time > iter->next->Data.time)
				iter = iter->next;

			if (iter->next == NULL)
				iter->next = node;
			else {
				node->next = iter->next;
				iter->next = node;
			}
		}
	}

	return first;
}

Node *DeleteNode(Node *first, timeData time, strData str)
{
	Node *node, *iter;

	if (first == NULL)
		return first;

	if (fabs(first->Data.time - time) < 0.001) {
		node = first;
		strcpy(str, first->Data.str);
		first = first->next;
		DestroyNode(node);

		return first;
	}

	iter = first;
	while (iter->next) {
		if (fabs(iter->next->Data.time - time) < 0.001) {
			node = iter->next;
			strcpy(str, iter->next->Data.str);
			iter->next = iter->next->next;
			DestroyNode(node);

			return first;
		}
		iter = iter->next;
	}

	return first;
}
