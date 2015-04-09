#include "ucos_ii.h"
#include <stdio.h>
#include <stdlib.h>

#define null '\0'
#define CURRENT_TASK_SUM 64

MinHeap* heapInitialize(){
	MinHeap *heap;
	heap=(MinHeap*)malloc(sizeof(MinHeap));
	if(heap==null){
		exit(1);
	}
	heap->size=0;
	heap->capacity=CURRENT_TASK_SUM;
	//the first element of array is sentinel_element -1, but when compute size and capacity, it is not included

	heap->elements=(Heap_Data*)malloc(sizeof(Heap_Data)*(CURRENT_TASK_SUM+1));
	if(heap->elements==null){
		exit(1);
	}else{
		sentinel.deadline=0;
		sentinel.prio=-1;
		heap->elements[0]=sentinel;
	}
	return heap;
}

void clearHeap(MinHeap* heap){
	if(heap!=null){
		heap->size=0;
	}else{
		exit(1);
	}
}
void heapInsert(Heap_Data* data,MinHeap* heap){
	if(heap==null){
		exit(1);
	}
	if(isFull(heap)){
		exit(1);
	}else{
		int i;
		for(i=++heap->size;data->deadline<(heap->elements[i/2]).deadline;i/=2){
			heap->elements[i]=heap->elements[i/2];
		}
		heap->elements[i].deadline=data->deadline;
		heap->elements[i].prio=data->prio;
	}
}
Heap_Data* heapDeleteMin(MinHeap* heap){
	int i;
	int child;
	Heap_Data min;
	Heap_Data last;

	if(heap==null){
		exit(1);
	}
	if(isEmpty(heap)){
		return &sentinel;
	}
	min=heap->elements[1];
	last=heap->elements[heap->size--];
	for(i=1;i*2<=heap->size;i=child){
		child=i*2;
		if(child<heap->size&&heap->elements[child].deadline > heap->elements[child+1].deadline){
			child++;
		}
		if(last.deadline < heap->elements[child].deadline){
			break;
		}else{
			heap->elements[i]=heap->elements[child]; //иобк
		}
	}
	heap->elements[i]=last;
	return &min;
}
void printHeap(MinHeap* heap){
	int i=0;
	if(heap==null){
		exit(1);
	}
	if(heap->elements==null){
		exit(1);
	}
	if(isEmpty(heap)){
		printf("empty\n");
	}
	for(i=1;i<=heap->size;i++){
		printf("element[%d]=%d\n",i,heap->elements[i].deadline);
	}
	printf("\n");
}
INT8U isFull(MinHeap* heap){
	if(heap==null){
		exit(0);
	}else{
		return heap->size==heap->capacity;
	}
}
INT8U isEmpty(MinHeap* heap){
	if(heap==null){
		exit(1);
	}else{
		return heap->size==0;
	}
}