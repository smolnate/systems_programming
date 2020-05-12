#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "hashtable.h"
#include "huffmantree.h"

// Global Variables //
extern int elements;

// Function Prototypes //
void compress(char*, char*);
void decompress(char*, char*);
void build(HashTable**, char*);
void methodpicker(char*, char*, char*);
void recursive(char*, char*, char*, HashTable**);
void addTokensToHashTable(HashTable**, int);
char* addNewMemory(char*, int);

// Function Definitions //
char* addNewMemory(char* tok, int count){
    return (char*)realloc(tok, count);
}
void addTokensToHashTable(HashTable **table, int fd){
    //delimiters \t, \n, \v, \f, \r
    char *token = malloc(1);
    char ch;
    int counter = 0;
    do{
        int status = read(fd, &ch, 1);
        if(status < 1) break;
        else{
            //control code or space found
            if(ch == '\t' || ch == '\n' || ch == '\v' || ch == '\f' || ch == '\r' || ch == ' '){ 
                if(counter != 0){ //there exists a string token
                    insert(table, token);
                    free(token);
                    token = malloc(1);
                    counter = 0;
                }
                
                if(ch == '\t')    
                    insert(table, "^#&$#*t");
                else if(ch == '\n')    
                    insert(table, "^#&$#*n");
                else if(ch == '\v')    
                    insert(table, "^#&$#*v");
                else if(ch == '\f')    
                    insert(table, "^#&$#*f");
                else if(ch == '\r')    
                    insert(table, "^#&$#*r");
                else if(ch == ' ')
                    insert(table, " ");
                
                    //insert(table, &ch);
            }
            //any other character found
            else{
                token[counter] = ch;
                token = addNewMemory(token, counter+2);
                counter++;
            }
        }
    } while(1);
    if(counter != 0){
        insert(table, token);
        free(token);
        token = malloc(1);
        counter = 0;
    }
}
void compress(char* fileToCompressPath, char* codebookPath){
    //punctuation part of 
    //tokenize1 in the file to compress
    //find token in codebook and write corresponding code on new file
    HashTableCode* table = malloc(sizeof(HashTableCode));
    int fd = open(codebookPath, O_RDWR);
    char ch;
    char* escape = malloc(6);
    int escapeExists= 0;
    int counter = 0;
    char* wordToken = malloc(1);
    char* codeToken = malloc(1);
    int wordCurrently = 0;

    read(fd, escape, 6); //read the 6 byte long escape string into escape
    read(fd, &ch,1); //read the newline proceeding 

    /**
    if(!isdigit(ch)){ //save escape
        escape = ch;
        escapeExists = 1;
        //printf("the escape character is:%c\n",ch);
        //read once more to get rid of newline
        escape[counter] = ch;
        escape = addNewMemory(escape, counter+2);
        counter++;
        while(read(fd,&ch,1) >=1 && ch != "\n"){
            escape[counter] = ch;
            escape = addNewMemory(escape, counter+2); 
            counter++;
        }
        counter = 0; //reset counter for code token to use
    }
    **/
    /**
    else{ //if not escape, save char in codeToken
        //printf("character read: %c \n", ch);
        codeToken[counter] = ch;
        codeToken = addNewMemory(codeToken, counter+2);
        counter++;
    }
    **/
    do{
        int status = read(fd, &ch, 1);
        //printf("character read: %c \n", ch);
        if(status < 1) break;
        else{
            //tab or newline
            if(ch == '\t'){
		        codeToken[counter] = '\0'; 
                counter = 0;
                wordCurrently = 1; //indicate that we are done with code, move on to getting word
                //printf("i see tab \n");
            }
            else if (ch == '\n'){
                //printf("code token sent to table is:z%sz\n", codeToken);
                //printf("length of code token is:%d\n",strlen(codeToken));
                wordToken[counter] = '\0';
                put(&table, codeToken, wordToken);
                counter = 0;
                wordCurrently = 0;
                free(codeToken);
                free(wordToken);
                codeToken = malloc(1);
                wordToken = malloc(1);
            }
                //any other character found
            else{
                if(wordCurrently){
                    wordToken[counter] = ch;
                    wordToken = addNewMemory(wordToken, counter+2);
                    counter++;
                }
                else{
                    codeToken[counter] = ch;
                    codeToken = addNewMemory(codeToken, counter+2);
                    counter++;
                }
            }
        }
    } while(1);

    //printHashTableCode(table);

    //At this point, table has all mappings word->code from codebook
    //read the file
    
    int fdReadFile = open(fileToCompressPath, O_RDONLY); //open file to read
    
    char* writePath = malloc(strlen(fileToCompressPath)+strlen(".hcz")); //adding .hcz
    writePath = strcat(writePath, fileToCompressPath);
    char* extension = malloc(sizeof(".hcz"));
    strcat(extension,".hcz");
    writePath = strcat(writePath, extension);
    //the writepath will be the current file path and then add.hcz
    int fdWriteFile = open(writePath, O_RDWR  |  O_CREAT,  00600);//create file to be written in
    char *tokenFile = malloc(1);
    char chFile;
    int counterFile = 0;
    do{
        int status = read(fdReadFile, &chFile, 1);
        //printf("character read:z%cz \n", chFile);
        if(status < 1) break;
        else{
            //control code or space found
            //when control code is found, write the accumulated token's corresponding code to the write file
            //furthermore, write the control code's corresponding code to write file as well
            //before looking it up to hashtable, control code has to be converted in a char array because that's how it is represented in hashtable
            //mental note: the control code char array adds a null terminator but this probably does not make a difference since the hash function will return the same key with and without null temrinator
            if(chFile == '\t' || chFile == '\n' || chFile == '\v' || chFile == '\f' || chFile == '\r' || chFile == ' '){ 
                if(counterFile != 0){
                     //there exists a word
                    write(fdWriteFile, getCode(&table, tokenFile), strlen(getCode(&table, tokenFile)));
                }
                    //convert delimiter to string
                    char convertedDelim[8];//converteddelim first has the 6 escape, then the delim character, and then the null terminator
                    int j;
                    for(j=0; j<6; j++)
                        convertedDelim[j] = escape[j];
                    //printf("The converted delim before appending is:%s\n", convertedDelim);
                    int i;
                    convertedDelim[7] = '\0';
                    for(i=0; i<8; i++)
                        //printf("the character at %d is:%c\n",i,convertedDelim[i]);
                    
                    if(chFile == '\t'){
                        convertedDelim[6] = 't';
                        //printf("The converted delim is:%s\n", convertedDelim);
                        //printf("the strlen is:%d\n",strlen(convertedDelim));
                    }
                    else if(chFile == '\n'){
                        convertedDelim[6] = 'n';
                        //printf("The converted delim is:%s\n", convertedDelim);
                    }
                    else if(chFile == '\v'){
                        convertedDelim[6] = 'v';
                    }
                    else if(chFile == '\f'){
                        convertedDelim[6] = 'f';
                    }
                    else if(chFile == '\r'){
                        convertedDelim[6] = 'r';
                    }
                    else{
                        convertedDelim[0] = ' ';
                        convertedDelim[1] = '\0';
                    }
                    /**
                    if(chFile != ' '){
                        if(escapeExists)
                            convertedDelim[0] = escape;
            
                    }
                    **/
                    //printf("The delim is:%s\n",convertedDelim);
                    //printf("The code for space%sspace is %s \n", convertedDelim, getCode(&table, convertedDelim));
                    write(fdWriteFile, getCode(&table, convertedDelim), strlen(getCode(&table, convertedDelim)));
                    free(tokenFile); //reset token after writing to file
                    tokenFile = malloc(1);
                    counterFile = 0;
                }
            //any other character found
            else{
                tokenFile[counterFile] = chFile;
                tokenFile = addNewMemory(tokenFile, counterFile+2);
                counterFile++;
            }
        }
    } while(1);
    if(counter != 0){
        write(fdWriteFile, getCode(&table, tokenFile), strlen(getCode(&table, tokenFile)));
        free(tokenFile);
        tokenFile = malloc(1);
        counterFile = 0;
    }
    
}
void decompress(char* fileToDecompressPath, char* codebookPath){
    //let's run it back for decompress

    //construct hashtable to map code-->word 
    HashTableCode* table = malloc(sizeof(HashTableCode));
    int fd = open(codebookPath, O_RDWR);
    char ch;
    char* escape = malloc(6);
    int escapeExists = 0;
    int counter = 0;
    char* wordToken = malloc(1);
    char* codeToken = malloc(1);
    int wordCurrently = 0;

    read(fd, escape, 6);
    read(fd,&ch,1);
    /**
    if(!isdigit(ch)){
        escape = ch;
        escapeExists = 1;
        //printf("the escape character is%c\n", ch);
        read(fd,&ch,1);
    }
    else{
        //printf("character read %c\n", ch);
        codeToken[counter] = ch;
        codeToken = addNewMemory(codeToken, counter+2);
        counter++;
    }
    **/
    do{
        int status = read(fd, &ch, 1);
        //printf("character read:%c\n", ch);
        if(status<1) break;
        else{
            if (ch=='\t'){
                codeToken[counter] = '\0'; //make it a string so its strlen is correct
                counter = 0; //reset
                wordCurrently =1; //we build word token now
                //printf("i see tab\n");
            }
            else if(ch=='\n'){
                wordToken[counter] ='\0';
                //printf("word token sent to table is:z%sz\n", wordToken);
                //printf("length of word token is:%d\n", strlen(wordToken));
                put(&table, wordToken, codeToken); //opposite of compress, map code to word
                counter = 0;
                wordCurrently = 0;
                free(codeToken);
                free(wordToken);
                codeToken = malloc(1);
                wordToken = malloc(1);
                
            }
            else{
                if(wordCurrently){
                    wordToken[counter] = ch;
                    wordToken = addNewMemory(wordToken, counter+2);
                    counter++;
                }
                else{
                    codeToken[counter] = ch;
                    codeToken = addNewMemory(codeToken, counter+2);
                    counter++;
                }

            }
        }
    }while(1);

    //printHashTableCode(table);

    //read file now
    int fdReadFile = open(fileToDecompressPath, O_RDONLY);
    char* writePath = malloc(strlen(fileToDecompressPath));
    strcpy(writePath, fileToDecompressPath);
    writePath[strlen(writePath)-4] = '\0';
    //printf("new writePath is:%s\n", writePath);

    int fdWriteFile = open(writePath,O_RDWR  |  O_CREAT,  00600);
    char* tokenFile = malloc(1);
    char chFile;
    int counterFile = 0;

    /**
    char e = '\\'; //default escape character is \ but e will be changed to custom escape character is there is one
    if(escapeExists){
        e = escape;
    }
    **/

    //read character-->add character to token-->check if token is valid
    //if token is valid, check if it is a delimiter
    //if it is a delimiter then write the actual delimiter into file
    //if not then write corresponding code into file
    //after writing, free token and restart counter
    //if token is not valid, add more memory to token to add more characters next iteration
    while(read(fdReadFile, &chFile,1) ==1){
        if(chFile == '\n')
            break;
        //printf("character read:%c\n",chFile); 
        tokenFile[counterFile] = chFile;
        //printf("current token is now: %s \n", tokenFile);    
        if(getCode(&table,tokenFile) != NULL){
            //write
            //if the code (word) is a delimiter, then write the actual delimiter, not the string delimiter

            char delimiterReturned[8];
            char actualDelimiter[8];
            //strcat(delimiterReturned,getCode(&table,tokenFile));
            int i;
            for(i = 0; i<7; i++)
                delimiterReturned[i] = getCode(&table,tokenFile)[i];
            delimiterReturned[7] = '\0';

            for(i = 0; i<=5; i++)
                actualDelimiter[i] = escape[i];
            actualDelimiter[6] ='t';
            actualDelimiter[7] = '\0';


            if(strcmp(actualDelimiter, delimiterReturned) ==0){
                //printf("delimiter written \n");
                write(fdWriteFile,"\t",1);
                free(tokenFile);
                tokenFile = malloc(1);
                counterFile = 0;
                continue;
            }

            actualDelimiter[6] ='n';
            if(strcmp(actualDelimiter, delimiterReturned) == 0){
                //printf("delimiter written \n");
                write(fdWriteFile,"\n",1);
                free(tokenFile);
                tokenFile = malloc(1);
                counterFile = 0;
                continue;
            }

            actualDelimiter[6] ='v';
            if(strcmp(actualDelimiter, delimiterReturned) == 0){
                //printf("delimiter written \n");
                write(fdWriteFile,"\v",1);
                free(tokenFile);
                tokenFile = malloc(1);
                counterFile = 0;
                continue;
            }

            actualDelimiter[6] ='f';
            if(strcmp(actualDelimiter, delimiterReturned) == 0){
                //printf("delimiter written \n");
                write(fdWriteFile,"\f",1);
                free(tokenFile);
                tokenFile = malloc(1);
                counterFile = 0;
                continue;
            }
            actualDelimiter[6] ='r';
            if(strcmp(actualDelimiter, delimiterReturned) == 0){
                //printf("delimiter written \n");
                write(fdWriteFile,"\r",1);
                free(tokenFile);
                tokenFile = malloc(1);
                counterFile = 0;
                continue;
            }
            //printf("no delimiters found \n");
            write(fdWriteFile,getCode(&table,tokenFile),strlen(getCode(&table,tokenFile)));
            free(tokenFile);
            tokenFile = malloc(1);
            counterFile = 0;
        }
        
        else{
            tokenFile= addNewMemory(tokenFile, counterFile+2);
            counterFile++;
        }
        
    }
}
void build(HashTable **table, char *filename){
    int fd = open(filename, O_RDONLY);
    if(fd < 0){
        printf("Fatal error: file \"%s\" does not exist\n", filename);
        exit(0);
    }
    addTokensToHashTable(table, fd);
}
void recursive(char* flag, char* file, char* codeBook, HashTable** table){
    //printf("filepath passed is:%s\n", file);
    DIR* directory = opendir(file);
    if(directory==NULL){
        printf("Fatal error: filepath does not exist\n");
        exit(0);
    }
    struct dirent* currentThing = NULL;
    readdir(directory);
    readdir(directory);
    currentThing = readdir(directory); //reads first thing in directory
    while(currentThing != NULL){
        if(currentThing->d_type == DT_REG){
            char* lastDot = strrchr(currentThing->d_name, '.');
            if(strcmp(flag, "-c") == 0 && strcmp(lastDot, ".hcz") != 0){ //make sure that the file passed to compress isnt a .hcz file
                //printf("current filename is:%s\n", currentThing->d_name);
                char* str = malloc(strlen(file)+strlen(currentThing->d_name)+1);
                str = strcat(str,file); //append file path, forward slash if necessary, and current filename
                if(str[strlen(file)-1] != '/')
                    str=strcat(str,"/");
                str=strcat(str, currentThing->d_name);
                compress(str, codeBook);
            } 
            else if (strcmp(flag, "-d") == 0 && strcmp(lastDot, ".txt") != 0){ //make sure that the file passed to decompress isnt a .txt
                char* str = malloc(strlen(file)+strlen(currentThing->d_name)+1);
                str = strcat(str,file);
                if(str[strlen(file)-1] != '/')
                    str=strcat(str,"/");
                str=strcat(str, currentThing->d_name);
                decompress(str, codeBook);
            }
            else if (strcmp(flag, "-b") == 0){
                char *str = malloc(strlen(file)+strlen(currentThing->d_name)+1);
                str = strcat(str, file);
                if(str[strlen(file)-1] != '/') //if passed filepath does not contain / at end. ex: ./folder
                    str = strcat(str, "/");
                str = strcat(str, currentThing->d_name);
                build(table, str);
            }
        }
        else if (currentThing->d_type == DT_DIR){
            char* newFile = malloc(strlen(file)+strlen(currentThing->d_name)+1); //construct new filepath to go into direcotry
            newFile = strcat(newFile, file);
            if(newFile[strlen(file)-1] != '/') //if passed filepath does not contain / at end. ex: ./folder
                newFile = strcat(newFile, "/");          
            newFile = strcat(newFile, currentThing->d_name);
            recursive(flag, newFile,codeBook, table);
        }
        currentThing = readdir(directory);
    }

}
int main(int argc, char** argv){
    if(argc < 3){
        printf("Fatal error: insufficient number of arguments\n");
        exit(0);
    }
    if(strcmp(argv[1], "-R") == 0){ //if recursive, isolate to separate recursive method
        if(strcmp(argv[2], "-b") == 0){ //if mode is build, only pass in 1 files
            int writeptr = open("./HuffmanCodebook", O_RDWR  |  O_CREAT,  00600);
            char *escapechar = "^#&$#*\n";
            HashTable *table = malloc(sizeof(HashTable));
            if(argc < 4){
                printf("Fatal error: insufficient amount of arguments\n");
                exit(0);
            }
            recursive(argv[2], argv[3], NULL, &table); //after this, build was called on every file possible in all subdirs.
            item *minHeap[elements];
            if(elements>0){
                int i;
                for(i=0; i<elements; i++){
                    minHeap[i] = malloc(sizeof(item));
                }
                createArray(table, minHeap);
                buildMinHeap(minHeap, elements);
                buildHuffmanTree(minHeap);
                write(writeptr, escapechar, strlen(escapechar));
                buildCodebook(minHeap[0], "", 'X', writeptr);
            }
            free(table);
            int i;
            for(i=0; i<elements; i++){
                if(minHeap[i] != NULL)
                    free(minHeap[i]);
            }
        }
        else if(strcmp(argv[2], "-c")==0 || strcmp(argv[2], "-d")==0){
            if(argc != 5){
                printf("Fatal error: insufficient amount of arguments\n");
                exit(0);
            }
            if(strcmp(argv[4], "HuffmanCodebook") != 0){
                printf("Fatal error: codebook not supplied in argument\n");
                exit(0);
            }
            recursive(argv[2], argv[3], argv[4], NULL); //otherwise pass in 1 file, 1 codebook
        }
        else{
            printf("Fatal error: invalid flags\n");
            exit(0);
        }
    }
    else{ //otherwise call directly (for one file)
        if(strcmp(argv[1], "-b") == 0){
            int writeptr = open("./HuffmanCodebook", O_RDWR  |  O_CREAT,  00644);
            char *escapechar = "^#&$#*\n";
            HashTable *table = malloc(sizeof(HashTable));
            char *filename = argv[2];
            build(&table, filename); 
            item *minHeap[elements];
            if(elements>0){
                int i;
                for(i=0; i<elements; i++){
                    minHeap[i] = malloc(sizeof(item));
                }
                createArray(table, minHeap);
                buildMinHeap(minHeap, elements);
                buildHuffmanTree(minHeap);
                write(writeptr, escapechar, strlen(escapechar));
                buildCodebook(minHeap[0], "", 'X', writeptr);
            }
            else{
                write(writeptr, escapechar, strlen(escapechar));
            }
            free(table);
            int i;
            for(i=0; i<elements; i++){
                if(minHeap[i] != NULL)
                    free(minHeap[i]);
            }
        }
        else if(strcmp(argv[1], "-c") == 0){
            char *strtoken = strrchr(argv[2], '.');
            if(strtoken == NULL){
                printf("fatal error: invalid file\n");
                exit(0);
            }
            if(argc != 4){
                printf("Fatal error: insufficient amount of arguments\n");
                exit(0);
            }
            if(strcmp(argv[3], "HuffmanCodebook") != 0){
                printf("Fatal error: codebook not supplied in argument\n");
                exit(0);
            }
            compress(argv[2], argv[3]);
        }
        else if(strcmp(argv[1], "-d")==0){
            char* strtoken = strrchr(argv[2], '.');
            if(strtoken == NULL){
                printf("fatal error: invalid file\n");
                exit(0);
            }
            if(strcmp(strtoken,".hcz")==0){
                if(argc != 4){
                    printf("Fatal error: insufficient amount of arguments\n");
                    exit(0);
                }
                if(strcmp(argv[3], "HuffmanCodebook") != 0){
                    printf("Fatal error: codebook not supplied in argument\n");
                    exit(0);
                }
                decompress(argv[2], argv[3]);
            }
        }
        else {
            printf("Fatal error: invalid arguments\n");
        }
    }
    return 0;
}