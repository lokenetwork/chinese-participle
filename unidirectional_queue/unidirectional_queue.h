//Use of single row chain
//author：nuaazdh
//date：December 9,2011
#include <stdio.h>
#include <malloc.h>
#include <ctype.h>
#include <stdlib.h>
#include "../main.h";

#define OK 1
#define ERROR 0
#define TRUE 1
#define FALSE 0

typedef int Status;
typedef struct TREE_CUT_WORDS_RESULT * QElemType;

typedef struct QNode{//the struc of element in queue.
	QElemType data;
	struct QNode* next;
}QNode,*QueuePtr;

typedef struct{//queue struct
	QueuePtr front;//top point
	QueuePtr rear;//bottom point
}LinkQueue;

//Create a empty queue.
Status InitQueue(LinkQueue *Q);
//Delete the queue,all will destroy in queue.
Status DestroyQueue(LinkQueue *Q);
//make the queue empty.
Status ClearQueue(LinkQueue *Q);
//Check the queue is empty or not.
Status QueueEmpty(LinkQueue Q);
int QueueLength(LinkQueue Q);
//copy element which in top queue to e,and return ok.If queue is empty,return error.
Status GetHead(LinkQueue Q,QElemType *e);
//put "e" into the queue bottom
Status EnQueue(LinkQueue *Q,QElemType e);
/*
 * If queue is not empty,copy the top element to "e",and delete the top element,return ok.
 * If queue is empty,just return error.
 * */
Status DeQueue(LinkQueue *Q,QElemType *e);
//Traverse the queue "Q" from top to bottom.
Status QueueTraverse(LinkQueue Q);

//show the menu tips.
void PrintMenu();
//get the user input
char getOption();
//put the new element in to queue depend on the input of user.
void NewNodeEnQueue(LinkQueue *Q);
//Delete the element,and show the ramin in queue.
void DeleteNode(LinkQueue *Q);
//Show the top element.
void ShowHeadNode(LinkQueue *Q);
//show the end tips.
void ShowBye();
//show how many element in queue.
void ShowLength(LinkQueue *Q);
//=============main()================//
