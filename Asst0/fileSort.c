#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>

/**** Definitions ****/

//enum type for integer or string mode
typedef enum mode{Integer, String} mode;

// Linked List structure
typedef struct Node{
    char* value;
    struct Node *next;
} Node;

/***************************************************/

/**** Function Prototypes ****/

char* addNewMemory(char* tok, int count);
void printlist(Node *head, mode Mode);
int integerComparator(void* first, void* second);
int stringComparator(void* first, void* second);
int insertionSort(void *head, int (*comparator)(void*, void*));
void swap(Node* a, Node* b);
int quickSort(void *head, int (*comparator)(void*, void*));
int quickSort_helper(Node *left, Node *right, int (*comparator)(void*, void*));
Node* getTail(void *head);
int checkEmptyFile(int fd);

/***************************************************/

/**** Functions ****/

int checkEmptyFile(int fd){
    char c;
    int status = read(fd, &c, 1);
    //if c is 0, EOF and file is empty
    //if -1, error
    //move file offset back to 0 since we tried to reach first byte
    lseek(fd, 0, SEEK_SET);
    return status;
}

int quickSort_helper(Node *left, Node *right, int (*comparator)(void*, void*)){
	//check if singleton list is passed into qsort- base case.
	if(left == right)
		return 1;
	if (!(left != NULL && right != NULL))
		return 1;
	/* Create two pointers, one points to head, the other points to the next node.
	 * l will point to head, r will point to next node. Essentially we will use swap rather than manipulate ptrs
	 * General algorithm: USING FIRST NODE AS PIVOT
	 * l will be ptr to last node of a "sublist" that contains every node w/ value smaller than pivot.
	 * r will traverse the list- once a node w/ value smaller than list is found, move l to l->next, then swap l and r.
	 * Repeat recursively on the left and right sublists */
	Node *l = left, *r = left->next;
	//traverse r until whole list traversed, meaning all of the list is partitioned
	while(r != NULL){ 
		//case where pivot is greater than the node pointed to in "r"
		if(comparator(left->value, r->value) > 0) {
			l = l->next;
			swap(l, r);
		}
		// move r to next node
		r = r->next;
	}
	//at this point, the whole list has been traversed by r, with the list partitioned based on pivot. however, pivot needs to be swapped into the correct pos.
	swap(left, l);

	//now recursively call quicksort on left sublist and right sublist
	quickSort_helper(left, l, comparator);
	quickSort_helper(l->next, right, comparator);
	return 1;
}
Node* getTail(void *head){
	Node *ptr = (Node*)head;
	while(ptr->next != NULL){
		ptr = ptr->next;
	}
	return ptr;
}
int quickSort(void *toSort, int (*comparator)(void*, void*)){
	Node *headptr = (Node*) toSort;
	Node *tailptr = getTail(toSort);
	return quickSort_helper(headptr, tailptr, comparator);
}
int stringComparator(void* first, void* second){
	char *firstStringPtr = (char*) first;
	char *secondStringPtr = (char*) second;
	int val=0, val2=0;
	while(*firstStringPtr == *secondStringPtr && *secondStringPtr != '\0'){
		firstStringPtr++, secondStringPtr++;
	}
	val = *firstStringPtr > *secondStringPtr;
	val2 = *firstStringPtr < *secondStringPtr;
	return val - val2;
}
int integerComparator(void* first, void* second){
	char *firstStringPtr = (char*) first;
	char *secondStringPtr = (char*) second;
	int val=0, val2=0;
	if(atoi(firstStringPtr) != 0){
		val = atoi(firstStringPtr);
		val2 = atoi(secondStringPtr);
		if(val > val2)
			return 1;
		else if (val == val2)
			return 0;
		else
			return -1;
	}

}
void swap(Node* a, Node* b){
	Node* temp = malloc(sizeof(Node));
  	temp->value = a->value;
	a->value = b->value;
	b->value = temp->value;
	free(temp); 
}
int insertionSort(void *toSort, int (*comparator)(void*,void*)){
	Node *ptr, *curr, *temp;
	// head - head of list
	// ptr - iterator to traverse list from head
	// curr - points to last element of "sorted" section
	// temp - points to first "unsorted" element	
	ptr = (Node*) toSort;
	curr = (Node*) toSort;
	temp = curr->next;
	//loop until whole list is sorted, indicated by curr as last elem in list.
	while(curr->next != NULL) {
		ptr = (Node*) toSort;
		temp = curr->next;
		if(comparator(curr->value, temp->value) <= 0){
			curr = curr->next;
			temp = temp->next;
		}
		else{
			//time to swap some s***
			while(comparator(ptr->value, temp->value) <= 0)
				ptr = ptr->next;
			while(ptr != temp) {
				swap(ptr, temp);
				ptr = ptr->next;
			}
		}
	}
	return 1;
}
char* addNewMemory(char* tok, int count){
    return (char*)realloc(tok, count);
}
void printlist(Node *head, mode Mode){
	Node *ptr = head;
    if(Mode == Integer){
        while(ptr != NULL){
            printf("%s\n", ptr->value);
            ptr = ptr->next;
        }
    }
    else{
        while(ptr != NULL){
            if(stringComparator(ptr->value, "0") == 0)
                printf("\n");
            else
                printf("%s\n", ptr->value);
            ptr = ptr->next;
        }
    }
}

/***************************************************/

int main(int argc, char **argv){
    if(argc!=3){
        printf("Fatal error: expected two arguments, insufficient number of arguments\n");
        exit(0);
    }
    int (*comparator)(void*,void*);
    char* sortmode = argv[1];
    if(!(stringComparator(sortmode, "-q") == 0 || stringComparator(sortmode, "-i") == 0)){
        printf("Fatal Error: \"%s\" is not a valid sort flag\n", sortmode);
        exit(0);
    }
    char* filename = argv[2];
    int fd = open(filename, O_RDWR);
    if(fd < 0){
        printf("Fatal error: file \"%s\" does not exist\n", filename);
        close(fd);
        exit(0);
    }
    int checkfile = checkEmptyFile(fd);
    if(checkfile == 0){ //eof, means empty file
        printf("Warning: File \"%s\" is empty!\n", filename);
    }
    Node* head = NULL;
    Node* crnt = NULL;
    int counter =0;
    char ch;
    char* token = (char*)malloc(1);
    //token by default is size 1 byte
    do{
        //read in next character
        int status = read(fd, &ch, 1);
        //if EOF
        if(status < 1)
            break;
        //otherwise consider ch
        else{
            //if ch is valid
            if(isalpha(ch) || isdigit(ch) || ch == '-'){
                if(counter == 0){
                    *token = ch;
                }
                else{
                    token = addNewMemory(token, counter+1);
                    *(token + counter) = ch;
                }
                counter++;
            }
            else{
                if(ch == ' ' || ch== '\t' || ch == '\n')
                    continue;
                //add token to linkedlist
                Node* newNode = (Node*)malloc(sizeof(Node));
                newNode->next = NULL;

                if(counter == 0)
                    newNode->value = "0";
                else{
                    newNode->value = malloc(counter);
                    memcpy(newNode->value, token, counter);
                }
                if(head == NULL){
                    head = newNode;
                    crnt = head;
                }
                else {
                    while(crnt->next != NULL)
                        crnt = crnt->next;
                    crnt->next = newNode;
                }
                //printToken(token, counter);
                free(token);
                token = (char*)malloc(1);
                counter = 0;
            }
        }
    }while(1);	
    if(counter != 0){
        //add token to linkedlist
        Node* newNode = (Node*)malloc(sizeof(Node));
        newNode->next = NULL;
        newNode->value = malloc(counter);
        memcpy(newNode->value, token, counter);
        if(head == NULL){
            head = newNode;
            crnt = head;
        }
        else {
            while(crnt->next != NULL)
                crnt = crnt->next;
            crnt->next = newNode;
        }
        //printToken(token, counter);
        free(token);
    }
     //printlist(head);
    if(head==NULL){
        exit(0);
    }
    char* test = head->value;

    mode Mode;
    if(atoi(test) == 0 && test[0] != '0'){ //string found
        Mode = String;
    }
    else {
        Mode = Integer;
    }
    //set fnptr based on mode
    if(Mode == String)
        comparator = stringComparator;
    else
        comparator = integerComparator;
    if(stringComparator(sortmode, "-q")==0)//quicksort
        quickSort(head, comparator);
    else if(stringComparator(sortmode, "-i")==0) //insertion sort
        insertionSort(head, comparator);
    printlist(head, Mode);
    close(fd);
}


