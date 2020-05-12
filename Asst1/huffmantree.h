#ifndef HUFFMANTREE_H
#define HUFFMANTREE_H
#include "hashtable.h"
// Struct Definition //
typedef struct item{
    int frequency;
    char* word;
    struct item *left;
    struct item *right;
} item;

// Function Prototypes //
void buildMinHeap(item**, int);
void buildHuffmanTree(item**);
void siftDown(item**, int, int);
void swap(item*, item*);
void printHeap(item**, int);
void siftUp(item**, int, int);
void push(item**, item*, int);
item* pop(item**, int);
void combine_two(item**);
void createArray(HashTable*, item**);
void buildCodebook(item*, char*, char, int);
#endif