#include "Stack.h"
#include <stdlib.h>
#include <stdio.h>
#include "debug.h"
#include <string.h>

Stack *newSlack(int capacity) {
	if (capacity <= 0) {
		db_error("init stack failed, capcipty:%d", capacity);
		return NULL;
	}
	Stack *slack = (Stack *) malloc(sizeof(Stack));
	slack->data = (StackData *) malloc(sizeof(StackData) * capacity);
	memset(slack->data, 0, sizeof(StackData) * capacity);
	slack->top = 0;
	slack->capacity = capacity;

	return slack;
}

int freeSlack(Stack *slack) {
	if (slack == NULL) {
		return 0;
	}
	if (NULL != slack->data) {
		free(slack->data);
	}
	slack->data = NULL;
	free(slack);
	return 0;
}

int pushData(Stack *slack, StackData data) {
	if (NULL == slack) {
		db_error("push stack failed stack:%p, data:%d", slack, data);
		return -1;
	}
	int capacity = slack->capacity;
	StackData *newData = NULL;
	if (isFull(slack)) {
		newData = (StackData *) malloc(sizeof(StackData) * 2 * slack->capacity);
		memset(newData, 0, sizeof(StackData) * 2 * slack->capacity);
		memcpy(newData, slack->data, sizeof(StackData) * slack->capacity);
		free(slack->data);
		slack->data = newData;
	}
	slack->data[slack->top] = data;
	slack->top++;

	return 0;
}

StackData popData(Stack *slack) {
	if (NULL == slack) {
		db_error("pop stack failed slack:%p", slack);
		return -1;
	}
	if (isEmpty(slack)) {
		return -1;
	}
	StackData data = slack->data[slack->top - 1];
	slack->top--;
	return data;
}

int isFull(Stack *slack) {
	if (NULL == slack) {
		db_error("invalid stack:%p", slack);
		return -1;
	}
	if (slack->capacity <= slack->top) {
		return 1;
	}

	return 0;
}

int isEmpty(Stack *slack) {
	if (NULL == slack) {
		db_error("invalid stack:%p", slack);
		return -1;
	}
	if (slack->top <= 0) {
		return 1;
	}
	return 0;
}

StackData getStackTop(Stack *stack) {
	if (NULL == stack) {
		db_error("invalid stack:%p", stack);
		return -1;
	}
	if (!isEmpty(stack)) {
		return stack->data[stack->top - 1];
	}
	return -1;
}