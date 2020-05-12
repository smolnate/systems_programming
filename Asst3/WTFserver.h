#ifndef WTFSERVER_H_
#define WTFSERVER_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include "WTFserver.h"

//Globals//
int clientIDCounter = 0;

typedef struct filenamelist {
    char* filename;
    struct filenamelist* next;
}filenamelist;

typedef struct commit_list {
    char* clientid;
    char* filecontents;
    char* projectname;
    struct commit_list* next;
}commit_list;

commit_list *head = NULL;

///////////

//Function Prototypes//
void insertfilename(filenamelist**, char*);
char* listToStringOfFileNames(filenamelist*);
void clearfilelist(filenamelist**);
void insert_first(commit_list**, char*, char*, char*);
void printfilenamelist(filenamelist*);
void print_list(commit_list*);
void addBackup(char*, char*);
void clear_list(commit_list**);
char* addMemory(char*, int);
int upgrade(char*, int);
int push(char*, int);
int update(char*, int);
int commit(char*, int);
int checkout(char*);
int currentVersion(char*, int);
int destroy(char*);
int create(char*);
int history(char*, int);
int compareCommit(commit_list*, char*, char*);
int rollback(char*, char*);

///////////////////////

int rollback(char* projectName, char* versionnumber){
    //check if project exists, check if version number exists
    char *direc = malloc(1000);
    memset(direc, '\0', 1000);
    strcpy(direc, "./serverDir/");
    direc = strcat(direc, projectName);
    DIR *temp = opendir(direc);
    if(temp == NULL){
        write(1, "error: project does not exist\n", strlen("error: project does not exist\n"));
        return 0;
    }
    char *backup = malloc(1000);
    memset(backup, '\0', 1000);
    strcpy(backup, "backup");
    strcat(backup, versionnumber);
    strcat(backup, ".tar.gz");
    //check project dir
    char *backuppath = malloc(1000);
    memset(backuppath, '\0', 1000);
    strcpy(backuppath, direc);
    strcat(backuppath, "/.Backups/");
    strcat(backuppath, backup);
    //printf("backup file path: %s\n", backuppath);
    int tempfd = open(backuppath, O_RDWR, 00644);
    if(tempfd==-1){
        write(1, "error: backup for this version does not exist\n", strlen("error: backup for this version does not exist\n"));
        close(tempfd);
        return 0;
    }
    close(tempfd);

    //move this backup to serverdir, delete projectdir, then untar backup and delete backup tar
    int version = atoi(versionnumber);
    //mv cmd
    char *mvcmd = malloc(10001);
    memset(mvcmd, '\0', 10001);
    strcpy(mvcmd, "mv ./serverDir/");
    strcat(mvcmd, projectName);
    strcat(mvcmd, "/.Backups/backup");
    strcat(mvcmd, versionnumber);
    strcat(mvcmd, ".tar.gz ./");
    //printf("mvcmd: %s\n", mvcmd);
    //rm cmd
    char *rmcmd = malloc(10001);
    memset(rmcmd, '\0', 10001);
    strcpy(rmcmd, "rm -r ./serverDir/");
    strcat(rmcmd, projectName);
    //printf("rmcmd: %s\n", rmcmd);
    //mkdir
    //char *mkdircmd = malloc(10001);
    //memset(mkdircmd, '\0', 10001);
    //strcpy(mkdircmd, "mkdir -p ./serverDir/");
    //strcat(mkdircmd, projectName);
    //printf("mkdircmd: %s\n", rmcmd);

    //tar cmd
    char *tarcmd = malloc(10001);
    memset(tarcmd, '\0', 10001);
    strcpy(tarcmd, "tar -xzf backup");
    strcat(tarcmd, versionnumber);
    strcat(tarcmd, ".tar.gz -C ./serverDir/");
    //printf("tarcmd: %s\n", tarcmd);
    system(mvcmd);
    system(rmcmd);
    //system(mkdircmd);
    system(tarcmd);
    memset(rmcmd, '\0', 10001);
    strcpy(rmcmd, "rm -f ./");
    strcat(rmcmd, backup);
    //printf("removing backup %s\n", rmcmd);
    system(rmcmd);

    //free(mkdircmd);
    free(backuppath);
    free(direc);
    free(backup);
    free(mvcmd);
    free(rmcmd);
    free(tarcmd);
    return 1;
}

int compareCommit(commit_list* head, char* clientID, char* tempcommit){
    //search LL using clientID
    commit_list *ptr = head;
    while(ptr != NULL){
        if(strcmp(ptr->clientid, clientID) == 0){ //client ID's match. check temp commits
            //printf("%s\n\n%s\n", ptr->filecontents, tempcommit);
            if(strcmp(ptr->filecontents, tempcommit)==0){//commits match. return 1
                return 1;
            }
            else{ //commits don't match. return 0
                return 0;
            }
        }
        else{ //client id's do not match. check next node
            ptr = ptr->next;
        }
    }
    //could not find any commits in LL, return 0.
    return 0;
}

void insertfilename(filenamelist** head, char* filename){
    filenamelist *temp = malloc(sizeof(filenamelist));
    int filenamelen = strlen(filename);
    temp->filename = malloc(filenamelen);
    memcpy(temp->filename, filename, filenamelen);
    temp->next = *head;
    *head = temp;
    //printf("Filename %s inserted\n", temp->filename);
}

char* listToStringOfFileNames(filenamelist* head){
    int streamsize=0;
    char* filenamestream;
    
    filenamelist *ptr = head;
    if(ptr != NULL){
        streamsize += (strlen(ptr->filename)+1);
        filenamestream = malloc(streamsize);
        memcpy(filenamestream,ptr->filename, strlen(ptr->filename));
        filenamestream = strcat(filenamestream, ",");
        ptr = ptr->next;
    }
    else{
        //printf("Nothing to be done. File name list is empty...\n");
        return NULL;
    }
    while(ptr != NULL){
        //add filename
        int filenamelen = strlen(ptr->filename);
        streamsize += (filenamelen+1); //accounting for ','
        filenamestream = addMemory(filenamestream, streamsize);
        filenamestream = strcat(filenamestream, ptr->filename);
        if(ptr->next != NULL)
            filenamestream = strcat(filenamestream, ",");
        ptr = ptr->next;
    }
    return filenamestream;

}

void clearfilelist(filenamelist** head){
    filenamelist *ptr = *head;
    filenamelist *prev = NULL;
    while(ptr != NULL){
        *head = ptr->next;
        free(*head);
        ptr = ptr->next;
    }
}

void insert_first(commit_list** head, char* clientid, char* filecontents, char* projectname){
    commit_list *temp = malloc(sizeof(commit_list));
    int clientidlen = strlen(clientid);
    int filecontentslen = strlen(filecontents);
    int projectnamelen = strlen(projectname);
    temp->clientid = malloc(clientidlen);
    temp->filecontents = malloc(filecontentslen);
    temp->projectname = malloc(projectnamelen);
    memcpy(temp->clientid, clientid, clientidlen);
    memcpy(temp->filecontents, filecontents, filecontentslen);
    memcpy(temp->projectname, projectname, projectnamelen);
    temp->next = *head;
    *head = temp;
    write(1,"Commit stored from client\n", strlen("Commit stored from client\n"));
    //printf("Project name: %s, Client ID: %s\n", temp->projectname, temp->clientid);
}

void printfilenamelist(filenamelist* head){
    filenamelist* ptr = head;
    if(ptr == NULL){
        printf("Filenamelist is empty\n");
        return;
    }
    while(ptr != NULL){
        printf("Filename: %s\n", ptr->filename);
        ptr = ptr->next;
    }
}

void print_list(commit_list* head){
    commit_list* ptr = head;
    if(ptr == NULL){
        printf("List is empty\n");
        return;
    }

    while(ptr != NULL){
        printf("Commit for project: %s, clientid: %s\n", ptr->projectname, ptr->clientid);
        ptr = ptr->next;
    }
}

void addBackup(char* projectName, char *versionnumber){
//IF EXISTS, CLEAR COMMIT LL. 
    //CREATE BACKUP TAR OF PROJECT DIRECTORY, STORE IN .BACKUPS OF THIS PROJECT DIRECTORY.
    //getting version number from manifest

    //int version = atoi(versionnumber);
    //printf("version number: %s\n", versionnumber);
    //tar -C ./serverDir/ -czf backup.tar.gz testProject  
    //mv backup(versionnumber).tar.gz ./serverdir/projectname/.Backups/
    char *tarfile = malloc(1000);
    char *tarcmd = malloc(1000);
    char *mvcmd = malloc(1000);
    char *pathproject = malloc(1000);
    memset(pathproject, '\0', 1000);
    memset(tarfile, '\0', 1000);
    memset(tarcmd, '\0', 1000);
    memset(mvcmd, '\0', 1000);
    tarfile = strcpy(tarfile, "backup");
    tarfile = strcat(tarfile, versionnumber);
    tarfile = strcat(tarfile, ".tar.gz ");
    tarcmd = strcpy(tarcmd, "tar -C ");
    tarcmd = strcat(tarcmd, "./serverDir/ ");
    tarcmd = strcat(tarcmd, "-czf ");
    tarcmd = strcat(tarcmd, tarfile);
    tarcmd = strcat(tarcmd, projectName);
    //printf("tar cmd: %s\n", tarcmd);
    system(tarcmd);
    pathproject = strcpy(pathproject, "./serverDir/");
    pathproject = strcat(pathproject, projectName);
    pathproject = strcat(pathproject, "/");
    pathproject = strcat(pathproject, ".Backups/");
    mvcmd = strcpy(mvcmd, "mv ");
    mvcmd = strcat(mvcmd, tarfile);
    mvcmd = strcat(mvcmd, pathproject);
    //printf("mvcmd: %s\n", mvcmd);
    system(mvcmd);
}

void clear_list(commit_list** head) {
    commit_list *ptr = *head;
    commit_list *prev = NULL;
    while(ptr != NULL){
        *head = ptr->next;
        free(*head);
        ptr = ptr->next;
    }
    *head = NULL;
}

char* addMemory(char* buffer, int buffersize){
    int newbuffersize = buffersize+5;
    char *newbuffer = malloc(newbuffersize);
    memset(newbuffer, '\0', newbuffersize);
    memcpy(newbuffer, buffer, buffersize);
    return newbuffer;
}

int upgrade(char *projectName, int client_socket){
    char *projectpath = malloc(1000);
    memset(projectpath, '\0', 1000);
    strcpy(projectpath, "./serverDir/");
    strcat(projectpath, projectName);
    DIR *temp = opendir(projectpath);
    if(temp == NULL){
        write(1, "error: project doesn't exist on server\n", strlen("error: project doesn't exist on server\n"));
        write(client_socket, "0", 1);
    }
    else{
        write(client_socket, "1", 1);
    }
    int filenamelen = 256;
    int filecontentlen = 10000;
    char *namestream = malloc(100000);
    memset(namestream, '\0', 100000);
    //read namestream, assuming the stream ends with a : and are comma separated
    char ch;
    int x=0;
    int filesneeded = 0;
    do{
        int status = read(client_socket, &ch, 1);
        if(status < 1) break;
        if(ch == ':'){
            filesneeded++;
            break;
        }
        if(ch == ',')
            filesneeded++;
        namestream[x] = ch;
        x++;
    }while(1);
    namestream[x] = ',';
    printf("Stream: %s\n", namestream);
    printf("Files needed: %d\n", filesneeded);
    //separate filenames in a loop, send files accordingly.
    char *tempstream = malloc(strlen(namestream)+1);
    memset(tempstream, '\0', strlen(namestream)+1);
    tempstream = strcpy(tempstream, namestream);
    char *serverpath = malloc(1000);
    memset(serverpath, '\0', 1000);
    serverpath = strcpy(serverpath, "./serverDir/");
    serverpath = strcat(serverpath, projectName);
    serverpath = strcat(serverpath, "/");
    for(x=0; x<filesneeded; x++){
        char *tempfilepath = malloc(1000);
        memset(tempfilepath, '\0', 1000);
        char *tempfilename = malloc(filenamelen);
        memset(tempfilename, '\0', filenamelen);
        if(x==0)
            tempfilename = strtok(tempstream, ",");
        else
        {
            tempfilename = strtok(NULL, ",");
        }
        
        printf("file name: %s\n", tempfilename);
        tempfilepath = strcpy(tempfilepath, serverpath);
        tempfilepath = strcat(tempfilepath, tempfilename);
        //now tempfilepath holds a filepath, find this in server and get its contents.
        char *tempfilecontents = malloc(filecontentlen);
        memset(tempfilecontents, '\0', filecontentlen);
        int filefd = open(tempfilepath, O_RDWR, 00644);
        if(filefd<0){
            write(1, "error: file not found on server\n", strlen("error: file not found on server\n"));
            free(tempfilepath);
            free(tempfilename);
            free(tempfilecontents);
            free(serverpath);
            return 0;
        }
        //read from filefd into tempfilecontents
        x=0;
        ch = ' ';
        do{
            int stat = read(filefd, &ch, 1);
            if(stat<1) break;
            tempfilecontents[x] = ch;
            x++;
        }while(1);
        //tempfilecontents holds current file's contents. write this to client now.
        write(client_socket, tempfilename, filenamelen);
        write(client_socket, tempfilecontents, filecontentlen);
        //free(tempfilename);
        free(tempfilepath);
        free(tempfilecontents);
        close(filefd);
    }
    //all files written to client. write manifest to client.

    char* manifestpath = strcat(serverpath, ".Manifest");
    int manfd = open(manifestpath, O_RDWR, 00644);
    if(manfd<0){
        write(1, "error: could not open manifest for the given project\n", strlen("error: could not open manifest for the given project\n"));
        return 0;
    }
    //read manifest into a manifest buffer. store manifest length
    int manlength=0;
    int manbuffsize = 5;
    char *mbuffer = malloc(manbuffsize);
    memset(mbuffer, '\0', manbuffsize);
    ch = ' ';
    do{
        int stat = read(manfd, &ch, 1);
        if(stat < 1) break;
        mbuffer[manlength] = ch;
        manlength++;
        if(manlength == (manbuffsize-1)){
            manbuffsize += 5;
            mbuffer = addMemory(mbuffer, manbuffsize);
        }
    }while(1);
    //convert manlength to a char
    int templength = snprintf(NULL, 0, "%d", manlength);
    char *manlen = malloc(templength+1);
    memset(manlen, '\0', templength+1);
    snprintf(manlen, templength+1, "%d", manlength);
    //write manlen : mbuffer
    write(client_socket, manlen, strlen(manlen));
    write(client_socket, ":", 1);
    write(client_socket, mbuffer, strlen(mbuffer));
    printf("%s:%s\n", manlen, mbuffer);

    close(manfd);
    free(manlen);
    free(mbuffer);
    free(serverpath);
    return 1;
}

int history(char *projectName, int client_socket){
    int historyfd;
    int histbuffsize = 5;
    int bytes_read=0;
    char *historypath = malloc(1000);
    historypath = strcpy(historypath, "./serverDir/");
    historypath = strcat(historypath, projectName);
    historypath = strcat(historypath, "/.History");
    historyfd = open(historypath, O_RDWR, 00644);
    if(historyfd < 0){
        write(1, "Error: project does not exist on server...\n", strlen("Error: project does not exist on server...\n"));
        write(client_socket, "0", 1);
        return 0;
    }
    char *historybuffer = malloc(histbuffsize);
    char ch;
    do{
        int status = read(historyfd, &ch, 1);
        if(status < 1) break;
        historybuffer[bytes_read] = ch;
        bytes_read++;
        if(bytes_read == (histbuffsize-1)){
            histbuffsize += 5;
            historybuffer = addMemory(historybuffer, histbuffsize);
        }
    }while(1);
    //create strlen of length
    int templength = snprintf(NULL, 0, "%d", bytes_read);
    char *histlength = malloc(templength+1);
    memset(histlength, '\0', templength+1);
    snprintf(histlength, templength+1, "%d", bytes_read);
    //write(1, "History length: ", strlen("History length: "));
    //write(1, histlength, strlen(histlength));
    //write(1, "\n", 1);
    //output history contents
    //write(1, "History contents...\n", strlen("History contents...\n"));
    //write(1, historybuffer, strlen(historybuffer));

    write(client_socket, "1", 1);
    write(client_socket, histlength, strlen(histlength));
    write(client_socket, ":", 1);
    write(client_socket, historybuffer, strlen(historybuffer));
    close(historyfd);
    return 1;
}

int push(char *projectName, int client_socket){
    //FIRST READ COMMIT FILE FROM CLIENT
    //THEN SEARCH COMMIT LL TO SEE IF THIS COMMIT FROM THIS CLIENT (USING CLIENT ID) EXISTS
    //do while read loop from socket, increment a counter called bytes read
    char *clientID = malloc(1000);
    memset(clientID, '\0', 1000);
    char cha;
    int idct=0;
    do{
        int status = read(client_socket, &cha, 1);
        if(status < 1 || cha == ':') break;
        clientID[idct] = cha;
        idct++;
    }while(1);
    char *commitlen = malloc(1000);
    memset(commitlen, '\0', 1000);
    int clenct=0;
    do{
        int status = read(client_socket, &cha, 1);
        if(status < 1 || cha == ':') break;
        commitlen[clenct] = cha;
        clenct++;
    }while(1);
    int commitbufferlen = atoi(commitlen);
    char *tempcommit = malloc(commitbufferlen+1); 
    memset(tempcommit, '\0', commitbufferlen+1);
    recv(client_socket, tempcommit, commitbufferlen, 0);
    //write(1, "commit contents:\n", strlen("commit contents:\n"));
    //write(1, tempcommit, strlen(tempcommit));

    //now that the commit is read into tempcommit, check commit LL using ID and compare commit
    print_list(head);
    int comparestatus = compareCommit(head, clientID, tempcommit);
    //write(1, "comparing commit to LL...\n", strlen("comparing commit to LL...\n"));
    if(comparestatus == 0){
        //write(1, "0\n", 2);
        write(client_socket, "0", 1);
        return 0;
    }
    else{
        //write(1, "1\n", 2);
        write(client_socket, "1",1);
    }
    clear_list(&head);
    
    //Adding backup to backups folder
    char *filepath = malloc(500);
    memset(filepath, '\0', 500);
    filepath = strcpy(filepath, "./serverDir/");
    if((strlen(projectName)+strlen(filepath)+2) >= 500){
        printf("Error: filepath exceeds max_length of 500\n");
        return;
    }
    filepath = strcat(filepath, projectName);
    if((strlen(projectName)+strlen(filepath)+2) >= 500){
        printf("Error: filepath exceeds max_length of 500\n");
        return;
    }
    filepath = strcat(filepath, "/.Manifest");
    //printf("Manifest path: %s\n", filepath);
    //check if file can be opened
    int manfd = open(filepath, O_RDWR);
    if(manfd<0){
        printf("Error manifest cannot be opened\n");
        return 0;
    }
    char *versionnumber = malloc(10);
    memset(versionnumber, '\0', 10);
    int z = 0;
    char ch;
    do{
        int status = read(manfd,&ch, 1);
        if(status < 1 || ch == '\n') break;
        else{
            versionnumber[z] = ch;
            z++;
        }
    }while(1);
    addBackup(projectName, versionnumber);

    //append this successful commit to history file
    //output should be similar to update output, but with a version number and newline separating each push's log of changes
    int historyfd;
    char *historypath = malloc(1000);
    historypath = strcpy(historypath, "./serverDir/");
    historypath = strcat(historypath, projectName);
    historypath = strcat(historypath, "/.History");
    //printf("History path: %s\n", historypath);
    historyfd = open(historypath, O_RDWR | O_APPEND);
    if(historyfd < 0){
        write(1, "Error: could not open history file on the project directory.\n", strlen("Error: could not open history file on the project directory.\n"));
    }
    //write version number and newline first
    write(historyfd, versionnumber, strlen(versionnumber));
    write(historyfd, "\n", 1);
    //write rest of commit
    write(historyfd, tempcommit, strlen(tempcommit));
    close(historyfd);
    //THEN PARSE THROUGH COMMIT FILE, REMOVE FILES THAT HAVE D FLAG. A and M are accounted for by client.
    int linebuffersize = 5;
    char *line = malloc(linebuffersize);
    filenamelist *filenamehead = NULL;
    memset(line, '\0', linebuffersize);
    int len=0;
    int i;
    int filesrequired=0;
    for(i=0; i<strlen(tempcommit); i++){
        char ch = tempcommit[i];
        //check \n
        if(ch == '\n'){ //new line
            //all of the line is read into line currently, check the code and see if filename needs to be added
            //printf("Line of Commit: %s\n", line);
            char *copyline = malloc(len);
            memcpy(copyline, line, len);
            char *code = malloc(1);
            code = strtok(copyline, " ");
            if(strcmp(code, "A")==0 || strcmp(code, "M")==0){ //add this filename to filenamelist
                char *entryfilename = strtok(NULL, " ");
                insertfilename(&filenamehead, entryfilename);
                filesrequired++;
            }
            else{ //DELETE the file! Code is: D
                char *entryfilename = strtok(NULL, " ");
                char *deletefilepath = malloc(1000);
                deletefilepath = strcpy(deletefilepath, "rm -f ./serverDir/");
                deletefilepath = strcat(deletefilepath, projectName);
                deletefilepath = strcat(deletefilepath, entryfilename);
                //printf("Calling: %s\n", deletefilepath);
                system(deletefilepath);

                //need to find path for filename
                //rm -f ./serverDir/projectName/anysubdirectory/entryfilename
            }
            free(line);
            line = malloc(linebuffersize);
            memset(line, '\0', linebuffersize);
            len = 0;
        }
        else{ //add this character to current line
            if(len == (linebuffersize - 1)){
                linebuffersize += 5;
                line = addMemory(line, linebuffersize);
            }
            line[len] = ch;
            len++;
        }
    }

    //print out all files that are needed from client
    //printfilenamelist(filenamehead);
    char* namestream = listToStringOfFileNames(filenamehead);
    write(1, "Files required from client...\n", strlen("Files required from client...\n"));
    write(1, namestream, strlen(namestream)-1);
    write(1, "\n", 1);

    //get all files and contents back from client, delete old versions of files
    //Filename[256] Filesize[11] Filecontents[10000]
    //create path
    char* cfilepath = malloc(1000);
    char* tempfilepath = malloc(1000);
    memset(cfilepath, '\0', 1000);
    cfilepath = strcpy(cfilepath, "./serverDir/");
    cfilepath = strcat(cfilepath, projectName);
    cfilepath = strcat(cfilepath, "/");
    char* cfilename = malloc(256);
    //char* cfilesize = malloc(11);
    char *cfilecontents = malloc(10000);
    memset(cfilename, '\0', 256);
    //memset(cfilesize, '\0', 11);
    memset(cfilecontents, '\0', 10000);
    for(i=0; i<filesrequired; i++){
        //read the components
        recv(client_socket, cfilename, 256, 0);
        recv(client_socket, cfilecontents, 10000, 0);

        //create path to the file, delete and create new file with contents
        memset(tempfilepath, '\0', 1000);
        tempfilepath = strcpy(tempfilepath, cfilepath);
        tempfilepath = strcat(tempfilepath, cfilename);
        //delete file
        char *deletecall = malloc(10000);
        memset(deletecall, '\0', 10000);
        deletecall = strcpy(deletecall, "rm -f ");
        deletecall = strcat(deletecall, tempfilepath);
        system(deletecall);
        //create file, write with contents
        int cfilefd;
        cfilefd = open(tempfilepath, O_CREAT | O_RDWR, 00644);
        while(cfilefd < 0){ //path does not exist, create subdirectories
            close(cfilefd);
            //printf("hold up! directories dont exist yet.\n");
            char *makedir = malloc(10000);
            char *directorypath = malloc(10000);
            memset(makedir, '\0', 10000);
            memset(directorypath, '\0', 10000);
            directorypath = strcpy(directorypath, tempfilepath);
            //get directory path without the .txt
            char charac = directorypath[strlen(directorypath)-1];
            while(charac != '/'){
                //subtract one char
                directorypath[strlen(directorypath)-1] = '\0';
                charac = directorypath[strlen(directorypath)-1];
            }
            makedir = strcpy(makedir, "mkdir -p ");
            makedir = strcat(makedir, directorypath);
            //printf("makedir call: %s\n", makedir);
            system(makedir);
            cfilefd = open(tempfilepath, O_CREAT | O_RDWR, 00644);
        }
        write(cfilefd, cfilecontents, strlen(cfilecontents));
        close(cfilefd);
        free(deletecall);
    }
    
    
    //delete server manifest, read client manifest, recreate.
    char *cmanpath = strcat(cfilepath, ".Manifest");
    char *deletemanifest = malloc(1000);
    memset(deletemanifest, '\0', 1000);
    deletemanifest = strcpy(deletemanifest,("rm -f "));
    deletemanifest = strcat(deletemanifest, cmanpath);
    system(deletemanifest);
    char *cmanifestlen = malloc(1000);
    memset(cmanifestlen, '\0', 1000);
    int x=0;
    do{
        int status = read(client_socket, &cha, 1);
        if(status < 1 || cha == ':') break;
        cmanifestlen[x] = cha;
        x++;
    }while(1);
    int cmanifestbufflen = atoi(cmanifestlen);
    char *tempmanifest = malloc(cmanifestbufflen+1); 
    memset(tempmanifest, '\0', cmanifestbufflen+1);
    recv(client_socket, tempmanifest, cmanifestbufflen, 0);
    int cmanfd = open(cmanpath, O_RDWR | O_CREAT, 00644);
    if(cmanfd<0){
        write(1, "Error: Could not change manifest on server side.", strlen("Error: Could not change manifest on server side."));
        return 0;
    }
    write(cmanfd, tempmanifest, strlen(tempmanifest));
    //manifest overwritten!
    close(cmanfd);
    clearfilelist(&filenamehead);
    close(manfd);
    
    return 1;
}

int update(char *projectName, int client_socket){
    //get path of project
    int statusbyte;
    char *filepath = malloc(500);
    memset(filepath, '\0', 500);
    filepath = strcpy(filepath, "./serverDir/");
    if((strlen(projectName)+strlen(filepath)+2) >= 500){
        printf("Error: filepath exceeds max_length of 500\n");
        return;
    }
    filepath = strcat(filepath, projectName);
    if((strlen(projectName)+strlen(filepath)+2) >= 500){
        printf("Error: filepath exceeds max_length of 500\n");
        return;
    }
    filepath = strcat(filepath, "/.Manifest");
    //printf("Manifest path: %s\n", filepath);
    //check if file can be opened
    int manfd = open(filepath, O_RDWR);
    if(manfd < 0){
        statusbyte = 0;
        printf("Error: failed to return current version- project may not exist on the server\n");
        write(client_socket, "0", 1);
        return 0;
    }
    else{ //file opened; read in contents of file
        write(client_socket, "1", 1);
        int buffersize = 5;
        char ch = ' ';
        char *buffer = malloc(buffersize);
        memset(buffer, '\0', buffersize);
        int bytes_read = 0;
        do{
            int status = read(manfd, &ch, 1);
            if(status < 1){ //EOF, buffer has all of .manifest contents
                break;
            }
            buffer[bytes_read] = ch;
            bytes_read++;
            if(bytes_read == (buffersize - 1)){ //resize the buffer
                buffersize += 5;
                buffer = addMemory(buffer, buffersize);
            }

        } while(1);
        //All reading done, write status byte, then bytes read (size of manifest), then contents of manifest (buffer)
        
        int templength = snprintf(NULL, 0, "%d", bytes_read);
        char *filesize = malloc(templength+1);
        memset(filesize, '\0', templength+1);
        snprintf(filesize, templength+1, "%d", bytes_read);
        //printf("Size of .Manifest: %s\n", filesize);
        //printf(".Manifest contents: %s\n", buffer);
        write(client_socket, filesize, strlen(filesize));
        char* colon = ":";
        write(client_socket, colon, strlen(colon));
        send(client_socket, buffer, bytes_read,0);
    }
    close(manfd);
    return 1;
}

int commit(char *projectName, int client_socket){
    //get path of project
    int statusbyte;
    char *filepath = malloc(500);
    memset(filepath, '\0', 500);
    filepath = strcpy(filepath, "./serverDir/");
    if((strlen(projectName)+strlen(filepath)+2) >= 500){
        printf("Error: filepath exceeds max_length of 500\n");
        return;
    }
    filepath = strcat(filepath, projectName);
    if((strlen(projectName)+strlen(filepath)+2) >= 500){
        printf("Error: filepath exceeds max_length of 500\n");
        return;
    }
    filepath = strcat(filepath, "/.Manifest");
    //printf("Manifest path: %s\n", filepath);
    //check if file can be opened
    int manfd = open(filepath, O_RDWR);
    if(manfd < 0){
        statusbyte = 0;
        printf("Error: failed to return current version- project may not exist on the server\n");
        write(client_socket, "0", 1);
    }
    else{ //file opened; read in contents of file
        write(client_socket, "1", 1);
        int buffersize = 5;
        char ch = ' ';
        char *buffer = malloc(buffersize);
        memset(buffer, '\0', buffersize);
        int bytes_read = 0;
        do{
            int status = read(manfd, &ch, 1);
            if(status < 1){ //EOF, buffer has all of .manifest contents
                break;
            }
            buffer[bytes_read] = ch;
            bytes_read++;
            if(bytes_read == (buffersize - 1)){ //resize the buffer
                buffersize += 5;
                buffer = addMemory(buffer, buffersize);
            }

        } while(1);
        //All reading done, write status byte, then bytes read (size of manifest), then contents of manifest (buffer)
        
        int tlength = snprintf(NULL, 0, "%d", bytes_read);
        char *filesize = malloc(tlength+2);
        memset(filesize, '\0', tlength+2);
        snprintf(filesize, tlength+1, "%d", bytes_read);
        filesize = strcat(filesize, ":");
        //printf("Size of .Manifest: %s\n", filesize);
        //printf(".Manifest contents: %s\n", buffer);
        write(client_socket, filesize, strlen(filesize));
        send(client_socket, buffer, bytes_read,0);

        //manifest and exit bit sent. now time to read commit status bit, .commit
        //if status 0, print error and return 0. if 1, read .commit 
        char commitstatus;
        //printf("Reading commit status from client...\n");
        recv(client_socket, &commitstatus, 1,0);
        if(commitstatus == '0'){
            write(1, "error: project versions of server and client manifests do not match\n", strlen("error: project versions of server and client manifests do not match\n"));
            return 0;
        }

        //read commit until socket is empty
        int ct=0;
        char *commitlength = malloc(10);
        //read strlen of commit
        do{
            char c;
            int status = read(client_socket, &c, 1);
            if(status < 1) break;
            if(c == ':') break;
            else{
                commitlength[ct] = c;
                ct++;
            }
        }while(1);
        int commitlen = atoi(commitlength);
        if(commitlen == 0){
            write(1, "nothing new to commit\n", strlen("nothing new to commit\n"));
            return 0;
        }
        //printf("Length of commit: %d\n", commitlen);
        char *commitbuffer = malloc(commitlen+1);
        memset(commitbuffer, '\0', commitlen+1);
        //read commit
        //printf("Reading commit file...\n");
        recv(client_socket, commitbuffer, commitlen, 0);
        //printf("Commit contents...\n");
        //printf("%s", commitbuffer);
        //commit is read into a buffer. server generates new client ID using counter.
        //write clientID to client.
        //create a new node for linked list with client id, commit contents, project name. insert this to linked list
        

        //generating ID
        clientIDCounter++;
        int templength = snprintf(NULL, 0, "%d", clientIDCounter);
        char *client_id = malloc(templength+1);
        memset(client_id, '\0', templength+1);
        snprintf(client_id, templength+1, "%d", clientIDCounter);
        //write(1, "Client ID generated: ", strlen("Client ID generated: "));
        //write(1, client_id, strlen(client_id));
        //write(1, "\n", 1);
        int idlen = snprintf(NULL, 0, "%d", templength);
        char *id_length = malloc(idlen+1);
        memset(id_length, '\0', idlen+1);
        snprintf(id_length, idlen+1, "%d", templength);
        //printf("IDLength: %s\n", id_length);
        send(client_socket, id_length, strlen(id_length), 0);
        send(client_socket, ":", 1, 0);
        send(client_socket, client_id, strlen(client_id), 0);

        //commit Linked List stuff
        insert_first(&head, client_id, commitbuffer, projectName);
        //print_list(head);

    }
    close(manfd);
    return 1;
}

int checkout(char* projectName){
    char *serverdirpath = "./serverDir/";
    char *projectpath = malloc(strlen(serverdirpath)+strlen(projectpath)+5);
    projectpath = strcpy(projectpath, serverdirpath);
    projectpath = strcat(projectpath, projectName);
    //printf("checkout project path: %s\n", projectpath);
    DIR *temp = opendir(projectpath);
    if(temp == NULL){ //PROJECT DOES NOT EXIST
        printf("Error on checkout: project does not exist on server\n");
        return 0;
    }
    //cp -avr
    char *checkout2 = " ./clientDir";
    char *option = "cp -avr ";
    char *callstring = malloc(strlen(projectpath)+strlen(option)+strlen(checkout2)+10);
    callstring = strcpy(callstring, option);
    callstring = strcat(callstring, projectpath);
    callstring = strcat(callstring, checkout2);
    system(callstring);
    //printf("%s\n", callstring);
    //printf("Checkout success\n");
    closedir(temp);
    return 1;
}

int currentVersion(char* projectName, int client_socket){
    //get path of project
    int statusbyte;
    char *filepath = malloc(500);
    memset(filepath, '\0', 500);
    filepath = strcpy(filepath, "./serverDir/");
    if((strlen(projectName)+strlen(filepath)+2) >= 500){
        printf("Error: filepath exceeds max_length of 500\n");
        return 0;
    }
    filepath = strcat(filepath, projectName);
    if((strlen(projectName)+strlen(filepath)+2) >= 500){
        printf("Error: filepath exceeds max_length of 500\n");
        return 0;
    }
    filepath = strcat(filepath, "/.Manifest");
    //"Manifest path: %s\n", filepath);
    //check if file can be opened
    int manfd = open(filepath, O_RDWR);
    if(manfd < 0){
        statusbyte = 0;
        printf("Error: failed to return current version- project may not exist on the server\n");
        write(client_socket, "0", 1);
        return 0;
    }
    else{ //file opened; read in contents of file
        int buffersize = 5;
        char ch = ' ';
        char *buffer = malloc(buffersize);
        memset(buffer, '\0', buffersize);
        int bytes_read = 0;
        do{
            int status = read(manfd, &ch, 1);
            if(status < 1){ //EOF, buffer has all of .manifest contents
                break;
            }
            buffer[bytes_read] = ch;
            bytes_read++;
            if(bytes_read == (buffersize - 1)){ //resize the buffer
                buffersize += 5;
                buffer = addMemory(buffer, buffersize);
            }

        } while(1);
        //All reading done, write status byte, then bytes read (size of manifest), then contents of manifest (buffer)
        
        int templength = snprintf(NULL, 0, "%d", bytes_read);
        char *filesize = malloc(templength+1);
        memset(filesize, '\0', templength+1);
        snprintf(filesize, templength+1, "%d", bytes_read);
        //printf("Successfully returned current version of project\n");
        //printf("Size of .Manifest: %s\n", filesize);
        //printf(".Manifest contents: %s\n", buffer);
        write(client_socket, "1", 1);
        write(client_socket, filesize, strlen(filesize));
        write(client_socket, ":", 1);
        write(client_socket, buffer, bytes_read);
    }
    close(manfd);
    return 1;

}

int destroy(char* path){
    //remove_dir(path);
    DIR *x = opendir(path);
    if(x == NULL)
        return 0; //proj doesn't exist
    char *stringcall = malloc(strlen(path)+10);
    stringcall = strcpy(stringcall, "rm -r ");
    stringcall = strcat(stringcall, path);
    system(stringcall);
    closedir(x);
    return 1;
}

int create(char* projectName){
    //create serverDir folder if not already created
    mkdir("serverDir", 00700);
    int check; //var to see if mkdir succeeded or not.
    char *projpath = malloc(255);
    memset(projpath, '\0', 255);
    projpath = strcpy(projpath, "./serverDir/");
    projpath = strcat(projpath, projectName);
    char *projpathbackup = malloc(255);
    projpathbackup = strcpy(projpathbackup, projpath);
    check = mkdir(projpath, 00700);
    char *historypath = strcat(projpath, "/.History");
    //printf("History path: %s\n", historypath);
    int historyfd = open(historypath, O_RDWR | O_CREAT, 00644);
    close(historyfd);
    char *backuppath = strcat(projpathbackup, "/.Backups/");
    //printf("Backup path: %s\n", backuppath);
    mkdir(backuppath, 00700);
    if(check == -1){ //could not create directory
        return 0;
    } 
    //create .Manifest inside directory
    //get file path including this directory
    else{
        char *serverdirpath = "./serverDir/";
        char *filepath = malloc(strlen(projectName) + strlen(".Manifest")+strlen(serverdirpath)+4);
        filepath = strcpy(filepath, serverdirpath);
        filepath = strcat(filepath, projectName);
        filepath = strcat(filepath, "/.Manifest");
        //printf("Manifest filepath: %s\n", filepath);
        int fd = open(filepath, O_RDWR | O_CREAT, 00644);
        write(fd, "0\n", 2);
        close(fd);
        return 1;
    }
}

#endif