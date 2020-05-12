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
#include <pthread.h>
#include "WTFserver.h"
#include <signal.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//global linked list for commits on server side
extern commit_list *head;

//global int counter for generating client ID's
extern int clientIDCounter;

void signalhandler(int sig){
    printf("\nForced exit of server\n");
    exit(0);
}

void *clienthandler(void *arg){
    int client_socket = *((int*)arg);
    // READING TO FIRST DELIMITER TO FIND COMMAND
    pthread_mutex_lock(&lock);
    char command[255] = "\0";
    char ch;
    int counter = 0;
    do{

        int status = read(client_socket, &ch, 1);
        if(status < 1) break;
        //delimiter : found
        if(ch == ':'){ //return whatever is in command
            break;
        }
        else{ //any other character found
            command[counter] = ch;
            counter++;
        }
    } while(1);
    write(1, "Client called command: ", strlen("Client called command: "));
    write(1, command, strlen(command));
    write(1, "\n", 1);
    //CHECK COMMAND
    if(strcmp(command, "create") == 0){ //"create:strlen(name):name"
        char *projName;
        char plen[255] = "\0";
        int projectnameLength=0;
        int count = 0;
        ch = ' ';
        //read strlen of projectname and :, set to integer
        do{
            int status = read(client_socket, &ch, 1);
            if(status<1) break;
            if(ch == ':'){
                break;
            }
            else{
                plen[count] = ch;
                count++;
            }
        } while(1);
        projectnameLength = atoi(plen);
        //read project name
        projName = malloc(projectnameLength+1);
        read(client_socket, projName, projectnameLength);
        int statusbyte = create(projName);
            if(statusbyte == 1){
                write(1, "Create success!\n", strlen("Create success!\n"));
                write(client_socket, "1", 1);
            }
            else{
                write(1, "Create failed...\n", strlen("Create failed...\n"));
                write(client_socket, "0", 1);
            }
    }
    else if(strcmp(command, "destroy") == 0){ //destroy:strlen(project):project
        char *serverdirpath = "./serverDir/";
        char *pathDestroy = malloc(255);
        memset(pathDestroy,'\0', 255);
        char pathlength[255] = "\0";
        memcpy(pathDestroy, serverdirpath, strlen(serverdirpath));
        int ct=0;
        int pathlen;
        //read strlen of project
        do{
            char c;
            int status = read(client_socket, &c, 1);
            if(status < 1) break;
            if(c == ':') break;
            else{
                pathlength[ct] = c;
                ct++;
            }
        }while(1);
        pathlen = atoi(pathlength);
        //read proejct name
        char *projectName = malloc(255);
        read(client_socket, projectName, pathlen);
        pathDestroy = strcat(pathDestroy, projectName);
        printf("Path to destroy: %s\n", pathDestroy);
        int statusbyte = destroy(pathDestroy);
        if(statusbyte == 1){
            write(1, "Destroy success!\n", strlen("Destroy success!\n"));
            write(client_socket, "1", 1);
        }
        else{
            write(1, "Destroy fail...\n", strlen("Destroy fail...\n"));
            write(client_socket, "0", 1);
        }
    }
    else{ // "currentversion OR checkout :strlen(project):project"
        char projectName[255] = "\0";
        char plen[255] = "\0";
        int projectnameLength=0;
        int count = 0;
        ch = ' ';
        //read strlen of projectname and :, set to integer
        do{
            int status = read(client_socket, &ch, 1);
            if(status<1) break;
            if(ch == ':'){
                break;
            }
            else{
                plen[count] = ch;
                count++;
            }
        } while(1);
        projectnameLength = atoi(plen);
        //read project name
        read(client_socket, projectName, projectnameLength);
        if(strcmp(command, "currentversion") == 0){
            int status = currentVersion(projectName, client_socket);
            if(status == 1){
                write(1, "currentversion success!\n", strlen("currentversion success!\n"));
            }
            else{
                write(1, "currentversion failed...\n", strlen("currentversion failed...\n"));
            }
        }
        else if(strcmp(command, "checkout") == 0){
            int status = checkout(projectName);
            if(status == 1){
                write(1, "checkout success!\n", strlen("checkout success!\n"));
            }
            else{
                write(1, "checkout failed...\n", strlen("checkout failed...\n"));
            }
        }
        else if(strcmp(command, "commit") == 0){
            int status = commit(projectName, client_socket);
            if(status == 1){
                write(1, "commit success!\n", strlen("commit success!\n"));
            }
            else{
                write(1, "commit failed...\n", strlen("commit failed...\n"));
            }
        }
        else if(strcmp(command, "update")==0){
            int status = update(projectName, client_socket);
            if(status == 1){
                write(1, "update success!\n", strlen("update success!\n"));
            }
            else{
                write(1, "update failed...\n", strlen("update failed...\n"));
            }
        }
        else if(strcmp(command, "push")==0){
            int status = push(projectName, client_socket);
            if(status == 1){
                write(1, "push success!\n", strlen("push success!\n"));
            }
            else{
                write(1, "push failed...\n", strlen("push failed...\n"));
            }
        }
        else if(strcmp(command, "history")==0){
            int status = history(projectName, client_socket);
            if(status == 1){
                write(1, "history success!\n", strlen("history success!\n"));
            }
            else{
                write(1, "history failed...\n", strlen("history failed...\n"));
            }
        }
        else if(strcmp(command, "upgrade")==0){
            int status = upgrade(projectName, client_socket);
            if(status == 1){
                write(1, "upgrade success!\n", strlen("upgrade success!\n"));
            }
            else{
                write(1, "upgrade failed...\n", strlen("upgrade failed...\n"));
            }
        }
        else if(strcmp(command, "rollback")==0){
            read(client_socket, &ch, 1);
            char *versionnumber = malloc(100);
            memset(versionnumber, '\0', 100);
            int x = 0;
            do{
                int status = read(client_socket, &ch, 1);
                if(status < 1) break;
                versionnumber[x] = ch;
                x++;
            }while(1);
            //printf("version number: %s\n", versionnumber);
            int status = rollback(projectName, versionnumber);
            if(status == 1){
                write(1, "rollback success!\n", strlen("rollback success!\n"));
            }
            else{
                write(1, "rollback failed...\n", strlen("rollback failed...\n"));
            }
            free(versionnumber);
        }
        else{
            printf("Error: command is not valid\n");
            exit(0);
        }
    }
    pthread_mutex_unlock(&lock);
    
}


int main(int argc, char** argv){
    
    char buffer[256];

    struct sockaddr_in serverAddressInfo;
    struct sockaddr_in clientAddressInfo;
    socklen_t clientAddressSize;

    if(argc <2){
        printf("Error: No port provided\n");
        exit(0);
        //error, no port provided
    }

    int portNumber = atoi(argv[1]); //set port number for server to run on
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); //build socket

    if(sockfd < 0){
        printf("Error: Could not open socket\n");
        exit(0);
        //error opening socket
    }


    //initialize everything
    bzero((char *)&serverAddressInfo, sizeof(serverAddressInfo));
    serverAddressInfo.sin_port = htons(portNumber); //setup the struct info for serverAddress
    serverAddressInfo.sin_family = AF_INET;
    serverAddressInfo.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr*) &serverAddressInfo, sizeof(serverAddressInfo));//bind the socket we built to serveraddress

    if(listen(sockfd, 50)==0) //marks socket as passive socket that will accept requests
        printf("Listening...\n");
    else
    {
        printf("error on listening\n");
        exit(0);
    }
    pthread_t threadid[50];
    int counter = 0;
    while(1){
        signal(SIGINT, signalhandler);
        write(1, "Awaiting connection request from client...\n", strlen("Awaiting connection request from client...\n"));
        int client_socket = accept(sockfd, (struct sockaddr*) &clientAddressInfo, &clientAddressSize);
        if(client_socket < 0) {
            printf("error: failed to accept connection request.\n");
            exit(0);
        }
        else {
            write(1,"Server successfully accepted connection request.\n", strlen("Server successfully accepted connection request.\n"));
            //accept extracts first connection from queue and returns a new socket 
            //client_socket is now the new socket we read 
            write(1, "Waiting for command from client...\n", strlen("Waiting for command from client...\n"));
        }
        if(pthread_create(&threadid[counter], NULL, clienthandler, &client_socket) != 0){
            printf("Failed to create thread\n");
        }
        if(counter >=50)
        {
            counter=0;
            while(counter < 50){
                pthread_join(threadid[counter++],NULL);
            }
            counter=0;
        } 
    }
    

    return 0;
}

