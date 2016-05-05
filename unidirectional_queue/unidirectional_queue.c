//Use of single row chain
//author：nuaazdh
//date：December 9,2011
#include <stdio.h>
#include <malloc.h>
#include <ctype.h>
#include <stdlib.h>
#include "unidirectional_queue.h"

//-------------user interface deal-----------------//
void PrintMenu(){
	//show the menu tips.
	printf("------Welcome------\n");
	printf(" Please input your choice:\n");
	printf(" E: New element enqueue.\n");
	printf(" D: Delete an element from the Queue.\n");
	printf(" S: Show all elements in the Queue.\n");
	printf(" H: Get the first element in the Queue.\n");
	printf(" L: Get the length of the Queue.\n");
	printf(" Q: Quit.\n");
	printf(" ohters: do nothing.\n");
}

void ShowBye(){
	//show the end tips.
	printf("------Bye bye------\n");
}

char getOption(){
	//get the user input
	char *input,*op;
	input=(char *)malloc(sizeof(char)*256);
	printf(">>");
	gets(input);
	op=input;
	if(*(op+1)!='\0'){
		return 'A';
	}
	else
		return toupper(*op);
}

void NewNodeEnQueue(LinkQueue *Q){
	//put the new element in to queue depend on the input of user.
	char *p;
	p=(char *)malloc(sizeof(char)*256);
	printf("Please input a char(only one):");
	gets(p);
	if((*p>='a')&&(*p<='z')||(*p>='A')&&(*p<='Z')){
		EnQueue(Q,*p);
		printf("New element \'%c\' enqueu.\n",*p);
	}else
		printf("Sorry, your input is not accepted.\n");
}

void DeleteNode(LinkQueue *Q){
	//Delete the element,and show the ramin in queue.
	QElemType e;
	if(TRUE==QueueEmpty(*Q)){
		printf("The Queue is empty.\n");
		return;
	}
	DeQueue(Q,&e);
	printf("%c delete from the Queue,",e);
	if(QueueEmpty(*Q)==TRUE)
		printf("The Queue is empty now.\n");
	else
		//printf("Current Queue is:");
		QueueTraverse(*Q);

}

void ShowHeadNode(LinkQueue *Q)
{
	//Show the top element.
	QElemType e;
	if(QueueEmpty(*Q)){
		printf("The Queue is empty.\n");
		return;
	}else{
		GetHead(*Q,&e);
		printf("The first element of the Queue is:%c\n",e);
	}
}

void ShowLength(LinkQueue *Q)
{
	//show how many element in queue.
	if(QueueEmpty(*Q)==TRUE){
		printf("The count of element is 0.\n");
	}else{
		printf("The count of element is：%d\n",QueueLength(*Q));
	}
}

//-------------Queue achieve----------------//
Status InitQueue(LinkQueue *Q)
{
	//Create a empty queue.
	Q->front = Q->rear = (QueuePtr) malloc(sizeof(QNode));
	if(Q->front==NULL){
		printf("Apply for memory failed.\n");
		exit(0);
	}
	Q->rear->next=NULL;
	return OK;
}

Status DestroyQueue(LinkQueue *Q){
	//Delete the queue,all will destroy in queue.
	while(Q->front){
		Q->rear=Q->front->next;
		free(Q->front);
		Q->front=Q->rear;
	}
	Q->front=Q->rear=NULL;
	return OK;
}

Status ClearQueue(LinkQueue *Q){
	//make the queue empty.
	QueuePtr p;  //Temporary queue
	Q->rear=Q->front->next;
	while(Q->rear){
		p=Q->rear->next;//aim to next element which will free
		free(Q->rear);
		Q->rear=p;
	}
	Q->rear=Q->front;//edit the end of the queue
	return OK;
}

Status QueueEmpty(LinkQueue Q){
    //If queue is empty, return true,not return false.
	if(Q.front==Q.rear&&Q.front!=NULL){
        return TRUE;
    }
	else
    {
        return FALSE;
    }
}

int QueueLength(LinkQueue Q){
	QueuePtr p;//temporary point
	int count=0;
	p=Q.front->next;
	while(p!=NULL){
		p=p->next;
		count++;
	}
	return count;
}

Status GetHead(LinkQueue Q,QElemType *e){
    //If queue isnot empty,put the top one in "e",and retutn ok.
    //If queue is empty,just return error.
	if(QueueEmpty(Q)==TRUE){
        return ERROR;
    }
	*e=Q.front->next->data;
	return OK;
}

Status EnQueue(LinkQueue *Q,QElemType e){
    //put "e" into the queue bottom
	QueuePtr p;
	if(Q->front==NULL||Q->rear==NULL)
		return ERROR;
	p=(QueuePtr)malloc(sizeof(QNode));
	if(!p){
		printf("Apply for memory error, element enqueue failed.\n");
		exit(0);
	}
	p->data=e;
	p->next=NULL;
	Q->rear->next=p;
	Q->rear=p;
	return OK;
}

Status DeQueue(LinkQueue *Q,QElemType *e){
    /*
     * If queue is not empty,copy the top element to "e",and delete the top element,return ok.
     * If queue is empty,just return error.
     * */
	QueuePtr p;
	if(Q->front==Q->rear){
        return ERROR;
    }
	p=Q->front->next;
	*e=p->data;
	Q->front->next=p->next;
	if(Q->rear==p){//Just one element in queues
        Q->rear=Q->front;
    }
	free(p);
	return OK;

}

Status QueueTraverse(LinkQueue Q){
    //Traverse the queue "Q" from top to bottom.
	QueuePtr p;
	p=Q.front->next;
	if(p==NULL){
		return ERROR;
	}
	printf("Elements of the Queue:");
	while(p!=NULL){
		printf("%c ",p->data);
		p=p->next;
	}
	printf("\n");
	return OK;
}