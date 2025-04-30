#ifndef TRUNK_STACK_H
#define TRUNK_STACK_H

typedef int StackData;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	StackData *data;
	int capacity;
	int top;
} Stack;

extern Stack *newSlack(int capacity);

extern int freeSlack(Stack *stack);

extern int pushData(Stack *stack, StackData data);

extern StackData popData(Stack *stack);

extern int isFull(Stack *stack);

extern int isEmpty(Stack *stack);

extern StackData getStackTop(Stack *stack);

#ifdef __cplusplus
}
#endif

#endif
