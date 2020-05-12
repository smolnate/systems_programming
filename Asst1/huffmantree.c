#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "huffmantree.h"
#include "hashtable.h"
// Global Variable for Heap Elements //
extern int elements;
/* 
Functions required for minheap:
    -push
    -pop
    -siftUp (insert)
    -siftDown (delete)
    -buildMinHeap
    -combine top two
*/

void printTree(item *root){
    if(root == NULL)
        return;
    if(root->left != NULL)
        printTree(root->left);
    printf("Node: %s\t", root->word);
    if(root->right != NULL)
        printTree(root->right);
}

void buildCodebook(item *root, char *bitstring, char mode, int fd){
    //create copy of bitstring
    char *temp = malloc(strlen(bitstring));
    strcpy(temp, bitstring);
    //check mode, if mode is -1, root. if 0, left branch. if 1, right branch.
    //append mode to end of bitstring
    if(mode == '0' || mode == '1'){
        temp = realloc(temp, strlen(temp)+2);
        temp[strlen(temp)] = mode;
    }
    if(root == NULL) return;
    if(root->left != NULL)
        buildCodebook(root->left, temp, '0', fd);
    if(strcmp(root->word, "subtree") != 0){
        write(fd, temp, strlen(temp));
        write(fd, "\t", 1);
        /*
        if(iscntrl(root->word[0]) != 0){
            char *ctrl = malloc(2);
            ctrl[0] = '\\';
            ctrl[1] = root->word[0];
            write(fd, ctrl, 2);
        } 
        else*/
        write(fd, root->word, strlen(root->word));
        write(fd, "\n", 1);
        //printf("%s\t%s\n", temp, root->word);
    }
    if(root->right != NULL)
        buildCodebook(root->right, temp, '1', fd);

}

void createArray(HashTable* hashtable, item **arr){
    int i;
    int ct=0;
    for(i=0; i<SIZE; i++){
        Node* current = hashtable->table[i];
        while(current != NULL){
            arr[ct]->word = malloc(strlen(current->word));
            arr[ct]->frequency = current->frequency;
            strcpy(arr[ct]->word, current->word);
            arr[ct]->left = NULL;
            arr[ct]->right = NULL;
            ct++;
            current = current->next;
        }
    }
}

void buildHuffmanTree(item **arr){
    while(elements>1){
        combine_two(arr);
        //printHeap(arr, elements);
        //printf("\n");
    }
}

void combine_two(item **arr){
    item *combined = malloc(sizeof(item));
    item *a = pop(arr, elements--);
    item *b = pop(arr, elements--);
    combined->word = "subtree";
    combined->frequency = a->frequency + b->frequency;
    combined->left = a;
    combined->right = b;
    push(arr, combined, ++elements);
}

item* pop(item **arr, int elements){
    item *toPop = malloc(sizeof(item));
    toPop->frequency = arr[0]->frequency;
    toPop->left = arr[0]->left;
    toPop->right = arr[0]->right;
    toPop->word = arr[0]->word;
    swap(arr[0], arr[elements-1]);
    arr[elements-1]->frequency = -1;
    arr[elements-1]->word = NULL;
    arr[elements-1]->left = NULL;
    arr[elements-1]->right = NULL;
    siftDown(arr, 0, elements-1);
    return toPop;
}

void push(item **arr, item *add, int elements){
    arr[elements-1] = add;
    siftUp(arr, elements-1, elements);
}

void siftUp(item **arr, int index, int elements){
    int temp = index;
    while(temp > 0){
        int parent = (temp-1)/2;
        if(arr[temp]->frequency<arr[parent]->frequency){
            swap(arr[temp], arr[parent]);
            temp = parent;
        }
        else
            break;
    }
}

void buildMinHeap(item **arr, int elements){
    int i;
    for(i=(elements/2); i>=0; i--){
        siftDown(arr, i, elements);
    }
}

void siftDown(item **arr, int index, int elements){
    int leftChild = (2*index)+1;
    int rightChild = leftChild+1;
    int temp = index;
    if(leftChild < elements && arr[leftChild]->frequency < arr[temp]->frequency)
        temp = leftChild;
    if(rightChild < elements && arr[rightChild]->frequency < arr[temp]->frequency)
        temp = rightChild;
    if(index != temp){
        swap(arr[index], arr[temp]);
        siftDown(arr, temp, elements);
    }
}

void swap(item *a, item *b){
    item *temp = malloc(sizeof(item));
    temp->frequency = a->frequency;
    temp->word = a->word;
    temp->left = a->left;
    temp->right = a->right;
    a->frequency = b->frequency;
    a->word = b->word;
    a->left = b->left;
    a->right = b->right;
    b->frequency = temp->frequency;
    b->word = temp->word;
    b->left = temp->left;
    b->right = temp->right;
}

void printHeap(item **arr, int elements){
    int i;
    for(i=0; i<elements; i++)
        printf("%s: %d\n", arr[i]->word, arr[i]->frequency);
}


/**

/*
int main(int argc, char **argv){
    int heapsize = 5;
    elements = 5;
    char *bitstring = malloc(1);
    item *minHeap[heapsize];
    int i;
    for(i=0; i<heapsize; i++){
        minHeap[i] = malloc(sizeof(item));
    }
    minHeap[0]->frequency = 7;
    minHeap[0]->word = "a";
    minHeap[0]->left = NULL;
    minHeap[0]->right = NULL;
    minHeap[1]->frequency = 1;
    minHeap[1]->word = "b";
    minHeap[1]->left = NULL;
    minHeap[1]->right = NULL;
    minHeap[2]->frequency = 9;
    minHeap[2]->word = "c";
    minHeap[2]->left = NULL;
    minHeap[2]->right = NULL;
    minHeap[3]->frequency = 3;
    minHeap[3]->word = "d";
    minHeap[3]->left = NULL;
    minHeap[3]->right = NULL;
    minHeap[4]->frequency = 8;
    minHeap[4]->word = "e";
    minHeap[4]->left = NULL;
    minHeap[4]->right = NULL;
    //printf("array before\n");
    //printHeap(minHeap, elements);
    //printf("building min heap\n");
    buildMinHeap(minHeap, elements);
    //printHeap(minHeap, elements);
    //printf("first elem popped\n");
    pop(minHeap, elements--);
    //printHeap(minHeap, elements);
    //printf("push new item f: 2\n");
    item *newItem = malloc(sizeof(item*));
    newItem->frequency = 2;
    newItem->word = "f";
    newItem->left = NULL;
    newItem->right = NULL;
    push(minHeap, newItem, ++elements);
    //printHeap(minHeap, elements);
    //printf("Building Huffman Tree\n");
    buildHuffmanTree(minHeap);
    printf("Codebook\n");
    buildCodebook(minHeap[0], "", 'X');


    return 0;
}
**/

