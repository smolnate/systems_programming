#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "hashtable.h"

int elements;


int hashfunction(int key){
    return key % SIZE;
}

void printHashTable(HashTable* hashtable){
    printf("Printing hashtable: \n");
    int i;
    for(i = 0; i < SIZE; i++){
        Node* current = hashtable->table[i];
        while(current != NULL){
            printf("Word: %s \t", current->word);
            printf("Frequency: %d \n", current->frequency);
            current = current->next;
        }
    }
}

void insert(HashTable** hashtable, char* word){
    //check if word is already in hash table

    int key = 0;
    int i = 0;
    while(word[i] != '\0'){
        key = key + (int)word[i];
        i++;
    }
    if((*hashtable)->table[hashfunction(key)] == NULL){
        //add word
        Node* newNode = malloc(sizeof(Node));
        newNode->word = malloc(strlen(word));
        strcpy(newNode->word, word);
        newNode->frequency = 1;
        newNode->next = NULL;
        (*hashtable)->table[hashfunction(key)] = newNode;
        //printf("Word added: %s \n", word);
        elements++;
    }
    else{
        //word may already exist
        Node* prev = (*hashtable)->table[hashfunction(key)];
        Node* current = (*hashtable)->table[hashfunction(key)];
        while(current != NULL){
            if (strcmp(word, current->word)== 0){ //need to use comparator
                current->frequency = current->frequency + 1;
                //printf("Frequency updated \n");
                return;
            }
            prev = current;
            current = current->next; 
        }
        //at this point, we know word does not exist
        Node* newNode = malloc(sizeof(Node));
        newNode->word = malloc(strlen(word));
        strcpy(newNode->word, word);
        newNode->frequency = 1;
        newNode->next = NULL;
        prev->next = newNode;
        elements++;
        //printf("Word added to end of chain: %s \n", word);
    }


}
void put(HashTableCode** hashtable, char* code, char* word){
    int key = 0;
    int i = 0;
    while(word[i] != '\0'){
        key = key + (int)word[i];
        i++;
    }
    NodeCode* newNode = malloc(sizeof(NodeCode));
    newNode->word = malloc(strlen(word));
    strcpy(newNode->word, word); 
    newNode->code = malloc(strlen(code));
    strcpy(newNode->code, code);
    newNode->next = NULL;
    if((*hashtable)->table[hashfunction(key)] == NULL){
        (*hashtable)->table[hashfunction(key)] = newNode;
        //printf("passed in code is:%s\n",code);
        //printf("Mapped word %s to code %s \n", newNode->word,newNode->code );
        //printf("strlen of passed in code: %d\n",strlen(code));
        //printf("stlen of code in hashtable: %d\n",strlen(newNode->code));
    }
    else{
        NodeCode* current = (*hashtable)->table[hashfunction(key)];
        while(current->next != NULL){
            current = current->next;
        }
        current->next = newNode;
        //printf("Mapped word %s to code %s with chaining \n", newNode->word, newNode->code);

    }
    
}

char* getCode(HashTableCode** hashtable, char* word){
    int key = 0;
    int i = 0;
    while(word[i] != '\0'){
        key = key + (int)word[i];
        i++;
    }
    NodeCode* current = (*hashtable)->table[hashfunction(key)];
    while(current != NULL){
        if (strcmp(current->word, word)==0){
            //printf("code returned for %s:%s\n", word,current->code);
            //printf("code strlen is:%d\n", strlen(current->code));
            return current->code;
        }
            
        else
            current = current->next;
    }
    return NULL; //not found

}
void printHashTableCode(HashTableCode* hashtable){
    printf("Printing hashtable: \n");
    int i;
    for(i = 0; i < SIZE; i++){
        NodeCode* current = hashtable->table[i];
        while(current != NULL){
            printf("At index %d \n", i);
            printf("Word: %s \n", current->word);
            printf("Code: %s \n", current->code);
            current = current->next;
        }
    }
}
