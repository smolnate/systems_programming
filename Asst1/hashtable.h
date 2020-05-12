#ifndef HASHTABLE
#define HASHTABLE

#define SIZE 100

typedef struct Node{
    char* word;
    int frequency;
    struct Node* next;
} Node;

typedef struct HashTable{
    Node* table[SIZE];
} HashTable;

int hashfunction(int);

void printHashTable(HashTable*);

void insert(HashTable**, char*);

//For Compress and Decompress 
typedef struct NodeCode{
    char* word;
    char* code;
    struct NodeCode* next;
} NodeCode;

typedef struct HashTableCode{
    NodeCode* table[SIZE];
}HashTableCode;

void put(HashTableCode**, char*, char*);

char* getCode(HashTableCode**, char*);

void printHashTableCode(HashTableCode*);
#endif