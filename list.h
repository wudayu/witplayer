#pragma once

#define STRLEN 512

typedef float timeData;
typedef char *strData;

typedef struct _LrcData{
	timeData time;
	char str[STRLEN];
} LrcData;

typedef struct _ListNode{
	LrcData Data;
	struct _ListNode *next;
} Node;


Node *InitList(Node *);
Node *InsertList(Node *, LrcData);
Node *DeleteNode(Node *, timeData, strData);
void traverselist(Node *);
int DestroyNode(Node *);
