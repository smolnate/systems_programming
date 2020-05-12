#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<openssl/md5.h>
#include<dirent.h>
#include <errno.h>
#include<signal.h>

int myClientID = -1;
int fileNameSize = 256;
int fileContentSize = 10000;

void signalhandler(int sig){
    printf("\nForced exit of client\n");
    exit(0);
}

char* addNewMemory(char* tok, int count){
    return (char*)realloc(tok, count);
}
int connectToServer(){
    int portNumber = -1;
    int n = -1;
    int sockfd = -1;
    char buffer[256];
    struct sockaddr_in serverAddressInfo;
    struct hostent* serverIPAddress;

    char* IPAddressBuffer = malloc(1);
    char* portBuffer = malloc(1);
    int configureFd = open("./.configure", O_RDONLY);
    if(configureFd < 0 ){
        printf("Have not configured IP Address and port yet. \n");
        exit(0);
    }
    
    int counter =0;
    int IPCurrently = 1;
    char ch;
    do{
        int status = read(configureFd, &ch, 1);
        if (status < 1) break;
        else{
            if (ch == '\n'){
                IPAddressBuffer[counter] = '\0';
                counter = 0;
                IPCurrently=0;
            }
            else{
                if(IPCurrently){
                    IPAddressBuffer[counter] = ch;
                    IPAddressBuffer = addNewMemory(IPAddressBuffer, counter+2);
                    counter++;
                }
                else{
                    portBuffer[counter] = ch;
                    portBuffer = addNewMemory(portBuffer, counter+2);
                    counter++;
                }
            }
        }

    }
    while(1);
    portBuffer[counter] = '\0';

    portNumber = atoi(portBuffer);
    serverIPAddress = gethostbyname(IPAddressBuffer);
    
    if(serverIPAddress == NULL){
        printf("Error: No such host exists. \n");
        exit(0);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero((char *)&serverAddressInfo, sizeof(serverAddressInfo));
    //set the address family to internet
    serverAddressInfo.sin_family = AF_INET;
    //set port number to connect to
    //htons converts numeric type to a general network short type
    serverAddressInfo.sin_port = htons(portNumber);
    //copy raw bytes from hostent struct into the sockaddr_in one
    bcopy((char*)serverIPAddress->h_addr, (char*)&serverAddressInfo.sin_addr.s_addr, serverIPAddress->h_length);
    while(connect(sockfd, (struct sockaddr*)&serverAddressInfo, sizeof(serverAddressInfo)) == -1){
        signal(SIGINT, signalhandler);
        printf("Failed to connect to server. Retrying...\n");
        system("sleep 3");
    }
    printf("Client successfully connected to server.\n");

    return sockfd;
}

int main(int argc, char** argv){
    if(strcmp(argv[1], "configure") == 0){
        //create configure file
        int fdConfigure = open("./.configure", O_RDWR | O_CREAT, 0600);
        write(fdConfigure, argv[2], strlen(argv[2]));
        write(fdConfigure, "\n", strlen("\n"));
        write(fdConfigure, argv[3], strlen(argv[3]));
        //close fd
        close(fdConfigure);
    }
    else{
        if(strcmp(argv[1], "checkout") == 0){
            int sockfd = connectToServer();
            char sizeOfName[10];
            sprintf(sizeOfName, "%d", strlen(argv[2])); //size of project name in string format
            char* versionCommand = malloc(strlen(argv[2]) + strlen("checkout:")+ strlen(sizeOfName)+ 1);
            strcat(versionCommand, "checkout:");
            strcat(versionCommand, sizeOfName);
            strcat(versionCommand, ":");
            strcat(versionCommand, argv[2]); //versioncommand = "currentversion:11:testProject"
            //printf("writing to sockfd the commands....\n");
            //printf("%s\n", versionCommand);
            write(sockfd, versionCommand, strlen(versionCommand));
        }
        else if(strcmp(argv[1], "rollback")==0){
            int sockfd = connectToServer();
            char sizeOfName[10];
            sprintf(sizeOfName, "%d", strlen(argv[2])); //size of project name in string format
            char* versionCommand = malloc(strlen(argv[2]) + strlen("rollback:")+ strlen(sizeOfName)+ 1);
            strcat(versionCommand, "rollback:");
            strcat(versionCommand, sizeOfName);
            strcat(versionCommand, ":");
            strcat(versionCommand, argv[2]); //versioncommand = "currentversion:11:testProject"
            strcat(versionCommand, ":");
            strcat(versionCommand, argv[3]);
            //printf("writing to sockfd the commands....\n");
            //printf("%s\n", versionCommand);
            write(sockfd, versionCommand, strlen(versionCommand));
        }
        else if(strcmp(argv[1], "update") == 0){
            int sockfd = connectToServer();
            char sizeOfName[10];
            sprintf(sizeOfName, "%d", strlen(argv[2])); //size of project name in string format
            char* updateCommand = malloc(strlen(argv[2]) + strlen("update:")+ strlen(sizeOfName)+ 1);
            strcat(updateCommand, "update:");
            strcat(updateCommand, sizeOfName);
            strcat(updateCommand, ":");
            strcat(updateCommand, argv[2]); //versioncommand = "update:11:testProject"
            write(sockfd, updateCommand, strlen(updateCommand));
            //printf("Wrote update command to server.\n");

            //read in status byte
            char statusByte;
            read(sockfd, &statusByte, 1);
            if(statusByte == '0'){
                printf("error: project does not exist on server\n");
                exit(0);
            }
            char* updatePath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/.Update"));
            strcpy(updatePath, "./clientDir/");
            strcat(updatePath, argv[2]);
            strcat(updatePath, "/.Update");

            //create .Update file
            int updateFd = open(updatePath, O_RDWR | O_CREAT, 0644 | O_APPEND);

            
            //read size of manifestContent
            char* manifestSizeString = malloc(1);
            int counter = 0;
            char ch;
            do{
                read(sockfd, &ch, 1);
                if(ch == ':'){
                    manifestSizeString[counter] = '\0';
                    break;
                }
                else{
                    manifestSizeString[counter] = ch;
                    manifestSizeString = addNewMemory(manifestSizeString, counter+2);
                    counter++;
                }

            }while(1);
            int manifestSizeConverted = atoi(manifestSizeString);
            

            //read manifest contents from socket and store it in manifestContent
            //read in version from server manifest
            char* serverVersion = malloc(1);
            counter = 0;
            do{
                read(sockfd, &ch,1);
                if(ch == '\n'){
                    //make server version null terminated
                    serverVersion[counter] = '\0'; 
                    break;
                }
                else{
                    serverVersion[counter] = ch;
                    serverVersion = addNewMemory(serverVersion, counter+2);
                    counter++;
                }
            }while(1);
        
            counter = 0;
            char* serverManifestContent = malloc(1);
            do{
                if(counter == manifestSizeConverted - 1 - strlen(serverVersion))
                    break;
                read(sockfd, &ch, 1);
                //if(status < 1) break;
                serverManifestContent[counter] = ch;
                serverManifestContent = addNewMemory(serverManifestContent, counter+2);
                counter++;
            }while(1); 
            //make manifestContent null terminating so strstr won't go awry later
            serverManifestContent[counter] = '\0';
            //printf("done reading server manifest content\n");

            

            //read in version byte from client manifest, as well as newline
            char* manifestPath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/.Manifest"));
            strcpy(manifestPath, "./clientDir/");
            strcat(manifestPath, argv[2]);
            strcat(manifestPath, "/.Manifest");
            int manifestFd = open(manifestPath, O_RDONLY);
            char* clientVersion = malloc(1);
            counter = 0;
            do{
                read(manifestFd, &ch, 1);
                if (ch == '\n'){
                    clientVersion[counter] = '\0';
                    break;
                }
                else{
                    clientVersion[counter] = ch;
                    clientVersion = addNewMemory(clientVersion, counter+2);
                    counter++;
                }

            }while(1);
         

            //compare the two versions
            if(strcmp(serverVersion,clientVersion) == 0){
                printf("the versions match\n");
                //Delete .Conflict if .Conflict exists
                char* conflictPath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/.Conflict"));
                strcpy(conflictPath, "./clientDir/");
                strcat(conflictPath, argv[2]);
                strcat(conflictPath, "/.Conflict");
                int a = open(conflictPath, O_RDONLY);
                if(a > 0){
                    //remove command
                    char* removeCommand = malloc(strlen("rm -r ") + strlen(conflictPath));
                    strcpy(removeCommand, "rm -r");
                    strcat(removeCommand, conflictPath);
                }
                close(a);

                //Output Up To Date to STDOUT
                char* stdoutMessage = "Up To Date\n";
                write(1, stdoutMessage, strlen(stdoutMessage));
                exit(0);
            }
            else{
                //otherwise, check for changes and traverse through client manifest

                //1: Check for Deletes
                //2: Check for Modifications
                //3: Check for Additions
                //char newLine;
                //read(manifestFd, &newLine, 1);
                //printf("differences in manifests found\n");

                //create conflictfd first
                char* conflictPath = malloc(strlen("./clientDir/") +strlen(argv[2]) + strlen("/.Conflict"));
                strcpy(conflictPath, "./clientDir/");
                strcat(conflictPath, argv[2]);
                strcat(conflictPath, "/.Conflict");
                int conflictFd = open(conflictPath, O_WRONLY | O_CREAT, 0644 | O_APPEND);

                //
                char* lineToken = malloc(1);
                counter = 0;
                char readByte;
                int conflictFlag = 0;

                do{
                    int status = read(manifestFd, &readByte, 1);
                    if (status<1){
                        break;
                    }
                    if(readByte == '\n'){
                        lineToken[counter] = '\0';
                        
                        //tokenize code
                        char* code = malloc(counter);
                        code = strtok(lineToken, " ");

                        //tokenize filename
                        char* fileName = malloc(counter);
                        fileName = strtok(NULL, " ");

                        //tokenize version
                        char* version = malloc(counter);
                        version = strtok(NULL, " ");

                        //tokenize client hash
                        char* hash = malloc(counter);
                        hash = strtok(NULL, "\n");

                        //check if filename doesn't exists on server manifest
                        char* occurrenceOfFileNameInBuffer = malloc(strlen(serverManifestContent));
                        if(!strstr(serverManifestContent,fileName)){
                            occurrenceOfFileNameInBuffer = NULL;
                        }
                        else{
                            strcpy(occurrenceOfFileNameInBuffer,strstr(serverManifestContent, fileName));
                        }
                        
                        //if filename does not exist on server and the file has been tracked before
                        if(!occurrenceOfFileNameInBuffer && (strcmp(code, "T") == 0 || (strcmp(version, "0") != 0 && strcmp(code, "M")==0))){
                            //filename does not exist on server manifest--> D
                            

                            //construct stdout message for delete
                            char* stdoutMessage = malloc(strlen("D ") + strlen(fileName) + strlen("\n") +1);
                            strcpy(stdoutMessage, "D ");
                            strcat(stdoutMessage, fileName);
                            strcat(stdoutMessage, "\n");
                            strcat(stdoutMessage, "\0"); //make message null termianting

                            //write stdout message
                            write(1, stdoutMessage, strlen(stdoutMessage));

                            //write into .Update file

                            char* DEntry = malloc(strlen("D ") + strlen(fileName) + strlen(" ") + strlen(hash) + 1);
                            strcpy(DEntry, "D ");
                            strcat(DEntry, fileName);
                            strcat(DEntry, " ");
                            strcat(DEntry, hash);
                            strcat(DEntry, "\n");
                            write(updateFd, DEntry, strlen(DEntry));
                        }
                        else if(!occurrenceOfFileNameInBuffer && strcmp(code, "A") == 0 && strcmp(version, "0") == 0){
                            //if file does not exist on server but it's new

                            //do nothing lol
                        }

                        else{
                            //filename does exist on server manifest --> M
                            //if there is an update for the client, but the client changed the file, write to a .Conflict file
                            char* fileNameInBuffer = malloc(strlen(serverManifestContent));
                            fileNameInBuffer = strtok(occurrenceOfFileNameInBuffer, " ");
                            char* versionInBuffer = malloc(strlen(serverManifestContent));
                            versionInBuffer = strtok(NULL, " ");
                            char* hashInBuffer = malloc(strlen(serverManifestContent));
                            hashInBuffer = strtok(NULL, "\n");  //get the hash of server manifest file entry

                            if(strcmp(hashInBuffer, hash) != 0){
                                //the file's hash differs in client manifest and server manifest
                                //check if client manifest is in the midst of a change and has not pulled:
                                if(strcmp(code,"M") == 0){
                                    //client needs to push first, there is conflict
                                    //write to conflict file
                                    //delete .Update file if there is one
                                    conflictFlag = 1;
                                   
                                    char* space = " ";
                                    char* conflictCode = "C ";
                                    write(conflictFd, conflictCode, strlen(conflictCode));
                                    write(conflictFd, fileNameInBuffer, strlen(fileNameInBuffer));
                                    write(conflictFd, space, strlen(space));

                                    //next thing we need to write is the new hash of the current file
                                    char* filePath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/") +strlen(fileName));
                                    strcpy(filePath, "./clientDir/");
                                    strcat(filePath, argv[2]);
                                    strcat(filePath, "/");
                                    strcat(filePath, fileName);
                                    char* systemCommand = malloc(strlen("md5sum ") + strlen(filePath) + strlen(" > hash.txt"));
                                    strcpy(systemCommand, "md5sum ");
                                    strcat(systemCommand, filePath);
                                    strcat(systemCommand, " > hash.txt");
                                    int s = system(systemCommand);
                                    //printf("system call returned: %d\n", s);
                                    char* hashPath = "./hash.txt";
                                    int hashFd = open(hashPath, O_RDONLY);
                                    char* hashContent = malloc(1);
                                    int counter = 0;
                                    char ch;
                                    do{
                                        int status = read(hashFd, &ch, 1);
                                        if(status < 1 || ch == ' ')break;
                                        else{
                                            hashContent[counter] = ch;
                                            hashContent = addNewMemory(hashContent, counter+2);
                                            counter++;
                                        }

                                    }while(1);
                                    hashContent[counter] = '\0';

                                    //finally write the hash to conflict
                                    write(conflictFd, hashContent, strlen(hashContent));

                                    //also write newline
                                    char* newLine = "\n";
                                    write(conflictFd, newLine, strlen(newLine));

                                    //write conflict message to stdout
                                    char* conflictMessage = malloc(strlen(conflictCode) + strlen(fileNameInBuffer) + strlen("\n")+ 1);
                                    strcpy(conflictMessage, "C ");
                                    strcat(conflictMessage, fileNameInBuffer);
                                    strcat(conflictMessage, "\n");
                                    strcat(conflictMessage, "\0");
                                    write(1, conflictMessage, strlen(conflictMessage));

                                    close(hashFd);
                                }
                                else{
                                    //hash differs but client has not modified anything
                                    char* stdoutMessage = malloc(strlen("M ") + strlen(fileName) + strlen("\n") + 1);
                                    strcpy(stdoutMessage, "M ");
                                    strcat(stdoutMessage, fileName);
                                    strcat(stdoutMessage, "\n");
                                    strcat(stdoutMessage, "\0"); //make message null termianting


                                    //write stdout message
                                    write(1, stdoutMessage, strlen(stdoutMessage));

                                    //write into .Update file

                                    char* MEntry = malloc(strlen("M ") + strlen(fileName) + strlen(" ") + strlen(hashInBuffer)+ strlen("\n"));
                                    strcpy(MEntry, "M ");
                                    strcat(MEntry, fileName);
                                    strcat(MEntry, " ");
                                    strcat(MEntry, hashInBuffer);
                                    strcat(MEntry, "\n");
                                    write(updateFd, MEntry, strlen(MEntry));

                                }
                            }
                            
                        }
                        //reset lineToken
                        counter = 0;
                        free(lineToken);
                        lineToken = malloc(1);
                    }
                    else{
                        lineToken[counter] = readByte;
                        lineToken = addNewMemory(lineToken, counter+2);
                        counter++;
                    }
                }while(1);

                close(manifestFd);

                //finished traversing through client manifest
                //Turn client manifest file into buffer
                int newManifestFd = open(manifestPath, O_RDONLY);
                char* clientBuffer = malloc(1);
                counter = 0;
                do{
                    int status = read(newManifestFd, &ch, 1);
                    if(status < 1 ) break;
                    clientBuffer[counter] = ch;
                    clientBuffer = addNewMemory(clientBuffer, counter+2);
                    counter++;
                }while(1);
                clientBuffer[counter] = '\0';

                //close fd
                close(newManifestFd);

                //Traverse through server manifest buffer and look at all file names ?
                char* copyOfServerManifestContent = malloc(strlen(serverManifestContent));
                strcpy(copyOfServerManifestContent,serverManifestContent);
                //char* versionNum = malloc(strlen(serverManifestContent));
                //versionNum = strtok(copyOfServerManifestContent, "\n");

                //initial, first entry
                int done =0 ;
                char* code = malloc(strlen(serverManifestContent));
                code = strtok(copyOfServerManifestContent, " ");
                if(!code){
                    done = 1;
                }
                char* fileName = malloc(strlen(serverManifestContent));
                fileName = strtok(NULL, " ");

                char* version = malloc(strlen(serverManifestContent));
                version = strtok(NULL, " ");

                char* hash = malloc(strlen(serverManifestContent));
                hash = strtok(NULL, "\n");

                char* occurrenceOfFileNameInClientBuffer = malloc(strlen(clientBuffer));
                if(!strstr(clientBuffer,fileName)){
                    occurrenceOfFileNameInClientBuffer = NULL;
                }
                else{
                    strcpy(occurrenceOfFileNameInClientBuffer,strstr(serverManifestContent, fileName));
                }
                if(!occurrenceOfFileNameInClientBuffer){
                    //the file cannot be found in client manifest --> add to .Update

                    //create add entry (A filename.txt hash)
                    char* addEntry = malloc(strlen("A ") + strlen(fileName) + strlen(" ") + strlen(hash) + strlen("\n"));
                    strcpy(addEntry, "A ");
                    strcat(addEntry, fileName);
                    strcat(addEntry, " ");
                    strcat(addEntry, hash);
                    strcat(addEntry, "\n");

                    //write add entry into update file
                    write(updateFd, addEntry, strlen(addEntry));

                    char* stdoutMessage = malloc(strlen("A ") + strlen(fileName) + strlen("/n"));
                    strcpy(stdoutMessage, "A ");
                    strcat(stdoutMessage, fileName);
                    strcat(stdoutMessage, "\n");
                    write(1, stdoutMessage, strlen(stdoutMessage));
                }
                
                do{
                    if(done){
                        break;
                    }
                    char* code = malloc(strlen(serverManifestContent));
                    code = strtok(NULL, " ");
                    if(!code){
                        break;
                    }
                    char* fileName = malloc(strlen(serverManifestContent));
                    fileName = strtok(NULL, " ");

                    char* version = malloc(strlen(serverManifestContent));
                    version = strtok(NULL, " ");

                    char* hash = malloc(strlen(serverManifestContent));
                    hash = strtok(NULL, "\n");

                    char* occurrenceOfFileNameInClientBuffer = malloc(strlen(clientBuffer));
                    if(!strstr(clientBuffer,fileName)){
                        occurrenceOfFileNameInClientBuffer = NULL;
                    }
                    else{
                        strcpy(occurrenceOfFileNameInClientBuffer,strstr(serverManifestContent, fileName));
                    }
                    if(!occurrenceOfFileNameInClientBuffer){
                        //the file cannot be found in client manifest --> add to .Update

                        //create add entry (A filename.txt hash)
                        char* addEntry = malloc(strlen("A ") + strlen(fileName) + strlen(" ") + strlen(hash) + strlen("\n"));
                        strcpy(addEntry, "A ");
                        strcat(addEntry, fileName);
                        strcat(addEntry, " ");
                        strcat(addEntry, hash);
                        strcat(addEntry, "\n");

                        //write add entry into update file
                        write(updateFd, addEntry, strlen(addEntry));

                        char* stdoutMessage = malloc(strlen("A ") + strlen(fileName) + strlen("\n"));
                        strcpy(stdoutMessage, "A ");
                        strcat(stdoutMessage, fileName);
                        strcat(stdoutMessage, "\n");
                        write(1, stdoutMessage, strlen(stdoutMessage));

                    }

                }while(1);
                
                if(conflictFlag){
                    char* conflictMessage = "Conflicts were found and must be resolved before the project can be updated\n";
                    write(1, conflictMessage, strlen(conflictMessage));
                    char* removeUpdateCommand = malloc(strlen("rm -f ") + strlen((updatePath)));
                    strcpy(removeUpdateCommand, "rm -f ");
                    strcat(removeUpdateCommand, updatePath);
                    system(removeUpdateCommand);
                }
                else{
                    //no conflicts, remove conflict file
                    char* removeConflictCommand = malloc(strlen("rm -f ") + strlen(conflictPath));
                    strcpy(removeConflictCommand, "rm -f ");
                    strcat(removeConflictCommand, conflictPath);
                    system(removeConflictCommand);
                }

                //close fd's
                close(updateFd);
                close(conflictFd);
                
            }
        }
        else if(strcmp(argv[1], "upgrade") == 0){
            char* updatePath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/.Update"));
            strcpy(updatePath,"./clientDir/");
            strcat(updatePath, argv[2]);
            strcat(updatePath, "/.Update");
            int updateFd = open(updatePath, O_RDONLY);
            if(updateFd < 0){
                //.Update does not exist
                printf("Error: no .Update file exists, please update first.");
                exit(0);
            }

            char ch;
            if(read(updateFd, &ch, 1) < 1){
                //.Update file exists, but is empty
                printf("Project is up to date.\n");

                //remove empty .Update
                char* removeCommand = malloc(strlen("rm -r ") + strlen(updatePath));
                strcpy(removeCommand, "rm -r ");
                strcat(removeCommand, updatePath);
                system(removeCommand);

                exit(0);
            }

            char* conflictPath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/.Conflict"));
            strcpy(conflictPath,"./clientDir/");
            strcat(conflictPath, argv[2]);
            strcat(conflictPath, "/.Conflict");
            int conflictFd = open(conflictPath, O_RDONLY);
            if(conflictFd >= 0){
                //.Conflict file exists
                printf("Error: .Conflict file exists, please resolve all conflicts and update.\n");
                exit(0);
            }
            
            
            //Otherwise, .Conflict does not exist and there is .Update to work with
            int sockfd = connectToServer();
            char sizeOfName[10];
            sprintf(sizeOfName, "%d", strlen(argv[2])); //size of project name in string format
            char* upgradeCommand = malloc(strlen(argv[2]) + strlen("upgrade:")+ strlen(sizeOfName)+ 1);
            strcat(upgradeCommand, "upgrade:");
            strcat(upgradeCommand, sizeOfName);
            strcat(upgradeCommand, ":");
            strcat(upgradeCommand, argv[2]); //versioncommand = "update:11:testProject"
            write(sockfd, upgradeCommand, strlen(upgradeCommand));

            char statusByte;
            read(sockfd, &statusByte, 1);
            if(statusByte == '0'){
                printf("Error: project doesn't exist on server\n");
            }
            
            //parse through .Update and see which files needed to make changes
            int newUpdateFd = open(updatePath, O_RDONLY);
            int counter = 0;
            char* lineEntry = malloc(1);
            int filesNeeded = 0;
            int done = 0;
            do{
                int status = read(newUpdateFd, &ch, 1);
                if(status<1){
                    done = 1;
                    break;
                } 

                if(ch == '\n'){
                    lineEntry[counter] ='\0';

                    char* code = malloc(counter);
                    code = strtok(lineEntry, " ");

                    char* filename = malloc(counter);
                    filename = strtok(NULL, " ");

                    if(strcmp(code, "M") == 0 || strcmp(code, "A") == 0){
                        //if file is to be added/modified, write filename to socket
                        filesNeeded++;
                        write(sockfd, filename, strlen(filename));
                        //char* comma = ",";
                        //write(sockfd, comma, strlen(comma));
                        free(lineEntry);
                        lineEntry = malloc(1);
                        counter = 0;
                        break;
                    }
                    else{
                        //remove file
                        char* removeCommand = malloc(strlen("rm -f ") + strlen(filename));
                        strcpy(removeCommand, "rm -f ");
                        strcat(removeCommand, filename);
                        system(removeCommand);
                    }

                    free(lineEntry);
                    lineEntry = malloc(1);
                    counter = 0;
                    
                }
                else{
                    lineEntry[counter] = ch;
                    lineEntry = addNewMemory(lineEntry, counter+2);
                    counter++;
                }

            }while(1);
            do{
                if(done)
                    break;
                int status = read(newUpdateFd, &ch, 1);
                if(status<1) break;

                if(ch == '\n'){
                    lineEntry[counter] ='\0';

                    char* code = malloc(counter);
                    code = strtok(lineEntry, " ");

                    char* filename = malloc(counter);
                    filename = strtok(NULL, " ");

                    if(strcmp(code, "M") == 0 || strcmp(code, "A") == 0){
                        //if file is to be added/modified, write filename to socket
                        filesNeeded++;
                        char* comma = ",";
                        write(sockfd, comma, strlen(comma));
                        write(sockfd, filename, strlen(filename));
                        //char* comma = ",";
                        //write(sockfd, comma, strlen(comma));
                        //break;
                    }
                    else{
                        //remove file
                        char* removeCommand = malloc(strlen("rm -f ") + strlen(filename));
                        strcpy(removeCommand, "rm -f ");
                        strcat(removeCommand, filename);
                        system(removeCommand);
                    }
                    free(lineEntry);
                    lineEntry = malloc(1);
                    counter = 0;
                }
                else{
                    lineEntry[counter] = ch;
                    lineEntry = addNewMemory(lineEntry, counter+2);
                    counter++;
                }

            }while(1);
            //append colon at end
            char* colon = ":";
            write(sockfd, colon, strlen(colon));

            //Read in filename, create file and write in it:
            do{
                if(filesNeeded == 0) break;
                char* fileName = malloc(fileNameSize);
                char* fileContent = malloc(fileContentSize);
                int status = read(sockfd, fileName, fileNameSize);
                if(status<1) break;
                read(sockfd, fileContent, fileContentSize);

                char*filePath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/") + strlen(fileName));
                strcpy(filePath, "./clientDir/");
                strcat(filePath, argv[2]);
                strcat(filePath, "/");
                strcat(filePath, fileName);

                int fileFd = open(filePath, O_RDONLY);
                if(fileFd >= 0){
                    //file does exist --> remove file first
                    char* removeCommand = malloc(strlen("rm -f ")+ strlen(filePath));
                    strcpy(removeCommand, "rm -f ");
                    strcat(removeCommand, filePath);
                    system(removeCommand);
                }
                //create new file
                int newFile = open(filePath, O_CREAT|O_RDWR, 00644);
                //change permissions
                char* chmodCommand = malloc(strlen("chmod 777 ") + strlen(filePath));
                strcpy(chmodCommand, "chmod 777 ");
                strcat(chmodCommand, filePath);
                int c = system(chmodCommand);
                //printf("system call num returned: %d \n", c);
                int a = write(newFile, fileContent, strlen(fileContent));
                //printf("current file content for %s: %s\n",filePath, fileContent);
                //printf("num bytes written: %d \n", a);
                //printf("the error: %s\n", strerror(errno));


                close(newFile);
                close(fileFd);

                //decrement filesNeeded
                filesNeeded--;
            }while(1);

            char* manifestPath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/.Manifest"));
            manifestPath = strcpy(manifestPath, "./clientDir/");
            manifestPath = strcat(manifestPath, argv[2]);
            manifestPath = strcat(manifestPath, "/.Manifest");

            //read in manifest size
            counter = 0;
            char* manifestSize = malloc(1);
            do{
                read(sockfd, &ch, 1);
                if(ch == ':'){
                    manifestSize[counter] = '\0';
                    break;
                }
                else{
                    manifestSize[counter] = ch;
                    manifestSize = addNewMemory(manifestSize, counter+2);
                    counter++;
                }
            }while(1);

            int manifestSizeConverted = atoi(manifestSize);
            //printf("manifest size: %d\n", manifestSizeConverted);

            //read in new manifest
            char* newManifestBuffer = malloc(manifestSizeConverted+1);
            memset(newManifestBuffer, '\0', manifestSizeConverted+1);
            counter = 0;
            int manifestFd = open(manifestPath, O_RDONLY);
            read(sockfd, newManifestBuffer, manifestSizeConverted);
            //printf("manifest content: %s\n", newManifestBuffer);

            //Done overwriting files, now to overwrite manifest
            char* removeManifestCommand = malloc(strlen("rm -f ") + strlen(manifestPath));
            strcpy(removeManifestCommand, "rm -f ");
            strcat(removeManifestCommand, manifestPath);
            system(removeManifestCommand);

            //create new manifest and write to it
            int manifestWriteFd = open(manifestPath, O_CREAT|O_RDWR, 00644);

            //printf("manifest write fd: %d\n", manifestWriteFd);
            //change permissions
            char* chmodCommand = malloc(strlen("chmod 777 ") + strlen(manifestPath));
            strcpy(chmodCommand, "chmod 777 ");
            strcat(chmodCommand, manifestPath);
            system(chmodCommand);
            write(manifestWriteFd, newManifestBuffer, strlen(newManifestBuffer));

            //remove update file since upgrade is successful
            char* removeUpdate = malloc(strlen("rm -f ") + strlen(updatePath));
            strcpy(removeUpdate, "rm -f ");
            strcat(removeUpdate, updatePath);
            system(removeUpdate);

            
            printf("Success: Upgrade complete! \n");

            
            close(updateFd);
            close(conflictFd);
            close(newUpdateFd);
            close(manifestFd);
            close(manifestWriteFd);

        }
        else if(strcmp(argv[1], "commit") == 0){
            
            char* conflictPath = malloc(strlen("./clientDir/") +strlen(argv[2]) + strlen("/.Conflict"));
            strcpy(conflictPath, "./clientDir/");
            strcat(conflictPath, argv[2]);
            strcat(conflictPath, "/.Conflict");
            int conflictFd = open(conflictPath, O_RDONLY);
            if(conflictFd > 0){
                printf("Error: .Conflict file exists\n");
                exit(0);
            }

            close(conflictFd);

            char* updatePath = malloc(strlen("./clientDir/") +strlen(argv[2]) + strlen("/.Update"));
            strcpy(updatePath, "./clientDir/");
            strcat(updatePath, argv[2]);
            strcat(updatePath, "/.Update");
            int updateFd = open(updatePath, O_RDONLY);
            if(updateFd > 0){
                char ch;
                int bytesRead = read(updateFd, &ch, 1);
                if(bytesRead == 1){
                    printf("Error: .Update file not empty\n");
                    exit(0);
                }
            }

            close(updateFd);

            char* manifestPath = malloc(strlen("./clientDir/") +strlen(argv[2]) + strlen("/.Manifest"));
            strcpy(manifestPath, "./clientDir/");
            strcat(manifestPath,argv[2]);
            strcat(manifestPath, "/.Manifest");

            int manifestFd = open(manifestPath, O_RDONLY);
            if(manifestFd < 0){
                printf("Error: Project does not exist on client\n");
                exit(0);
            }

            int sockfd = connectToServer();
        
            char sizeOfName[10];
            sprintf(sizeOfName, "%d", strlen(argv[2]));
            
            char* commitCommand = malloc(strlen(argv[2]) + strlen("commit:") + strlen(sizeOfName) + 1);
            strcpy(commitCommand, "commit:");
            strcat(commitCommand, sizeOfName);
            strcat(commitCommand, ":");
            strcat(commitCommand, argv[2]); //commitcommand = commit:11:testproject
            send(sockfd, commitCommand, strlen(commitCommand), 0);
            
            
            char statusByte;
            read(sockfd, &statusByte, 1);
            if(statusByte == 0){
                printf("Error: Failed to return current version- project not exist on server\n");
                exit(0);
            }
            

            /**
            char* manifestPath = malloc(strlen("./clientDir/") +strlen(argv[2]) + strlen("/.Manifest"));
            strcpy(manifestPath, "./clientDir/");
            strcat(manifestPath,argv[2]);
            strcat(manifestPath, "/.Manifest");

            int manifestFd = open(manifestPath, O_RDONLY);
            if(manifestFd < 0){
                printf("Project does not exist on client\n");
            }
            **/

            char* length = malloc(1);
            char readch;
            int counterch = 0;
            do{
                int status = read(sockfd, &readch, 1);
                if(readch == ':'){
                    length[counterch] = '\0';
                    break;
                }
                length[counterch] = readch;
                length = addNewMemory(length, counterch + 2);
                counterch++;
            } while(1);

            int lengthConverted = atoi(length); // get length of manfiest
            //read server version of manifest and read client version of manifest
           
            char* serverVersion = malloc(1);
            int serverCount = 0;
            char cha;
            do{
                read(sockfd, &cha, 1);
                if(cha == '\n'){
                    serverVersion[serverCount] = '\0';
                    break;
                }
                else{
                    serverVersion[serverCount] = cha;
                    serverVersion = addNewMemory(serverVersion, serverCount+2);
                    serverCount++;
                }
            }while(1);
            printf("Server's .Manifest version: %s\n", serverVersion);

            char* clientVersion = malloc(1);
            int clientCount = 0;
            do{
                read(manifestFd, &cha, 1);
                if(cha == '\n'){
                    clientVersion[clientCount] = '\0';
                    break;
                }
                else{
                    clientVersion[clientCount] = cha;
                    clientVersion = addNewMemory(clientVersion, clientCount+2);
                    clientCount++;
                }
            }while(1);
            printf("Client's .Manifest version: %s\n", clientVersion);
            
            
            //after reading version, READ THE REST OF THE MANIFEST
            char* serverManifestContent = malloc(1);
            int counter1 = 0;
            char readByte1;
            do{
                if(counter1 == lengthConverted-1 - strlen(serverVersion)){
                    break;
                }
                read(sockfd, &readByte1, 1);
                serverManifestContent[counter1] = readByte1;
                serverManifestContent = addNewMemory(serverManifestContent, counter1 + 2);
                counter1++;
            }while(1);
            serverManifestContent[counter1] = '\0'; //make content null terminating
            //printf("Manifest contents: %s\n", serverManifestContent);
            //check if the versions match
            if(strcmp(serverVersion, clientVersion) != 0){
                //if they don't, client needs to pull
                printf("Error: .Manifest versions do not match. Please update local project\n");
                char failByte = '0';
                send(sockfd, &failByte, 1, 0);
                exit(0);
            }
            else{
                //if they do, send success byte
                //printf("Sending success byte for .Commit...\n");
                char successByte = '1';
                send(sockfd, &successByte,1, 0);
            }
           
           //make new .Commit file
            char* commitPath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/.Commit"));
            strcpy(commitPath, "./clientDir/");
            strcat(commitPath, argv[2]);
            strcat(commitPath, "/.Commit");
            int commitFd = open(commitPath, O_RDWR | O_CREAT, 0644 | O_APPEND);

            //read through each entry of client manifest and write to .commit if needed
            int manifestReadFd = open(manifestPath, O_RDONLY);
            char readByte;
            char* lineToken = malloc(1);
            read(manifestReadFd, &readByte, 1); //read version 
            read(manifestReadFd, &readByte, 1); //read new line
            int counter = 0;
            do{
                int status = read(manifestReadFd, &readByte, 1);
                if (status<1){
                     break;
                }
                if(readByte == '\n'){
                    lineToken[counter] = '\0';
                    char* copyOfLineToken = malloc(counter);
                    strcpy(copyOfLineToken, lineToken);
                    char* code = malloc(counter);
                    code = strtok(lineToken, " "); //check code of client manifest file entry

                    if(strcmp(code, "A") == 0 || strcmp(code, "D") == 0){
                        char* addedCode = "A ";
                        char* deletedCode = "D ";
                        
                        if(strcmp(code, "A") == 0){
                            write(commitFd, addedCode, strlen(addedCode));
                        }
                        else{
                            write(commitFd, deletedCode, strlen(deletedCode));
                        }

                        char* filename = malloc(counter);
                        filename = strtok(NULL, " ");
                        write(commitFd, filename, strlen(filename));

                        char* space = " ";
                        write(commitFd, space, strlen(space));

                        char* version = malloc(counter);
                        version = strtok(NULL, " ");

                        char* hash = malloc(counter);
                        hash = strtok(NULL, " ");

                        int versionNum = atoi(version);
                        int updatedVersionNum = versionNum + 1;
                        char updatedVersionString[10];
                        sprintf(updatedVersionString,"%d",updatedVersionNum);

                        write(commitFd, version, strlen(version));
                        write(commitFd, space, strlen(space)); 

                        write(commitFd, hash, strlen(hash));
                        char* newline = "\n";
                        write(commitFd, newline, strlen(newline));

                        if(strcmp(code, "A") == 0){
                            char* stdoutMessage = malloc(strlen("A ") + strlen(filename)+ strlen("\n"));
                            strcpy(stdoutMessage, "A ");
                            strcat(stdoutMessage, filename);
                            strcat(stdoutMessage, "\n");
                            write(1,stdoutMessage,strlen(stdoutMessage));
                        }
                        else{
                            char* stdoutMessage = malloc(strlen("D ") + strlen(filename)+ strlen("\n"));
                            strcpy(stdoutMessage, "D ");
                            strcat(stdoutMessage, filename);
                            strcat(stdoutMessage, "\n");
                            write(1,stdoutMessage,strlen(stdoutMessage));
                        }
                    }
                    else if(strcmp(code, "M") == 0){
                        char* modifiedCode = "M ";
                        write(commitFd, modifiedCode, strlen(modifiedCode));

                        char* filename = malloc(counter);
                        filename = strtok(NULL, " ");
                        write(commitFd, filename, strlen(filename));

                        char* space = " ";
                        write(commitFd, space, strlen(space));
                        
                        char* version = malloc(counter);
                        version = strtok(NULL, " ");
                        int versionNum = atoi(version);
                        int updatedVersionNum = versionNum + 1;

                        char updatedVersionString[10];
                        sprintf(updatedVersionString,"%d",updatedVersionNum);

                        write(commitFd, updatedVersionString, strlen(updatedVersionString));
                        write(commitFd, space, strlen(space)); //write version and space

                        //next thing we need to write is the new hash of the current file
                        char* filePath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/") +strlen(filename));
                        strcpy(filePath, "./clientDir/");
                        strcat(filePath, argv[2]);
                        strcat(filePath, "/");
                        strcat(filePath, filename);

                        char* systemCommand = malloc(strlen("md5sum ") + strlen(filePath) + strlen(" > hash.txt"));
                        strcpy(systemCommand, "md5sum ");
                        strcat(systemCommand, filePath);
                        strcat(systemCommand, " > hash.txt");
                        int s = system(systemCommand);
                        //printf("system call returned: %d\n", s);
                        char* hashPath = "./hash.txt";
                        int hashFd = open(hashPath, O_RDONLY);
                        char* hashContent = malloc(1);
                        int counter = 0;
                        char ch;
                        do{
                            int status = read(hashFd, &ch, 1);
                            if(status < 1 || ch == ' ')break;
                            else{
                                hashContent[counter] = ch;
                                hashContent = addNewMemory(hashContent, counter+2);
                                counter++;
                            }

                        }while(1);
                        hashContent[counter] = '\0';

                        write(commitFd, hashContent, strlen(hashContent));
                        char* newline = "\n";
                        write(commitFd, newline, strlen(newline));

                        char* stdoutMessage = malloc(strlen("M ")+strlen(filename)+ strlen("\n"));
                        strcpy(stdoutMessage, "M ");
                        strcat(stdoutMessage, filename);
                        strcat(stdoutMessage, "\n");
                        write(1,stdoutMessage, strlen(stdoutMessage));

                        close(hashFd);

                    }
                    
                    counter = 0;
                    free(lineToken);
                    lineToken = malloc(1);
                    
                }
                else{
                    lineToken[counter] = readByte;
                    lineToken = addNewMemory(lineToken, counter+2);
                    counter++;
                }
            }while(1);

            //after this, the commit has been written and needs to be sent to server
            int commitFdRead = open(commitPath, O_RDONLY);

            char* commitContentBuffer = malloc(1);
            int counterC = 0;
            char c;
            do{
                int status = read(commitFdRead, &c, 1);
                if (status < 1) break;
                commitContentBuffer[counterC] = c;
                commitContentBuffer = addNewMemory(commitContentBuffer, counterC+2);
                counterC++;
            }while(1);
            commitContentBuffer[counterC] = '\0';

            int commitLength = strlen(commitContentBuffer);

            if(commitLength == 0){
                printf("Nothing to commit!\n");

                char* noCommit = "0:";
                write(sockfd, noCommit, strlen(noCommit));

                char* removeCommitCommand = malloc(strlen("rm -f ")+strlen(commitPath));
                strcpy(removeCommitCommand, "rm -f ");
                strcat(removeCommitCommand, commitPath);
                system(removeCommitCommand);
                exit(0);
            }
            char* commitLengthString = malloc(10);
            sprintf(commitLengthString, "%d", commitLength);
            
            write(sockfd, commitLengthString, strlen(commitLengthString)); //send commit length
            char* colonDelim = ":";
            write(sockfd, colonDelim, strlen(colonDelim));
            write(sockfd, commitContentBuffer, strlen(commitContentBuffer));
            printf("Success: .Commit has been created and sent to server\n");

            //read clientID given
            char* IDLength = malloc(1);
            counter = 0;
            do{
                read(sockfd, &c, 1);
                if(c == ':'){
                    IDLength[counter] = '\0';
                    break;
                }
                else{
                    IDLength[counter] = c;
                    IDLength = addNewMemory(IDLength, counter+2);
                    counter++;
                }
            }while(1);
            int lengthOfID = atoi(IDLength);

            char* clientIDString = malloc(1);
            counter = 0;
            do{
                if(counter == lengthOfID){
                    clientIDString[counter] = '\0';
                    break;
                }
                read(sockfd, &c, 1);
                clientIDString[counter] = c;
                clientIDString = addNewMemory(clientIDString, counter+2);
                counter++;
            }while(1);
            
            char* clientIDPath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/clientID.txt"));
            strcpy(clientIDPath, "./clientDir/");
            strcat(clientIDPath, argv[2]);
            strcat(clientIDPath, "/clientID.txt");

            //delete clientID.txt first
            char* deleteClientCommand = malloc(strlen("rm -f ") + strlen(clientIDPath));
            strcpy(deleteClientCommand, "rm -f ");
            strcat(deleteClientCommand, clientIDPath);
            system(deleteClientCommand);

            //write clientID into clientID.txt file
            //int clientIDFd = open(clientIDPath, O_RDWR, 0644 | O_CREAT, 0644);

            //recreate clientID
            int clientIDFd = open(clientIDPath, O_CREAT|O_WRONLY, 0777);
            //printf("%s\n",clientIDString);
            int a = write(clientIDFd, clientIDString, strlen(clientIDString));
            //printf("num bytes written: %d\n", a);
            //printf("errno: %s\n", strerror(errno));


            close(clientIDFd);
            close(manifestFd);
            close(commitFd);
            close(manifestReadFd);
            close(commitFdRead);
        }
        else if(strcmp(argv[1], "push") == 0){
            int sockfd = connectToServer();
            char sizeOfName[10];
            sprintf(sizeOfName, "%d", strlen(argv[2])); //size of project name in string format
            char* pushCommand = malloc(strlen(argv[2]) + strlen("push:")+ strlen(sizeOfName)+ 1);
            strcat(pushCommand, "push:");
            strcat(pushCommand, sizeOfName);
            strcat(pushCommand, ":");
            strcat(pushCommand, argv[2]); //versioncommand = "update:11:testProject"
            write(sockfd, pushCommand, strlen(pushCommand));

            char* clientID = malloc(1);
            char* clientIDPath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/clientID.txt"));
            strcpy(clientIDPath, "./clientDir/");
            strcat(clientIDPath, argv[2]);
            strcat(clientIDPath, "/clientID.txt");
            int clientIDFdRead = open(clientIDPath, O_RDONLY);
            //printf("fd value: %d\n", clientIDFdRead);
            int counter = 0;
            char ch;
            
            //get clientID
            do{
                int status = read(clientIDFdRead, &ch, 1);
                if(status<1)break;
                clientID[counter] = ch;
                clientID = addNewMemory(clientID, counter+2);
                counter++;
            }while(1);
            clientID[counter] = '\0';

            //printf("client id: %s\n", clientID);
            close(clientIDFdRead);

            //get :
            char* colon = ":";

            //get commit size and commit
            char* commitPath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/.Commit"));
            strcpy(commitPath, "./clientDir/");
            strcat(commitPath, argv[2]);
            strcat(commitPath, "/.Commit");

            int commitFdRead = open(commitPath, O_RDONLY);
            if(commitFdRead < 1){
                printf("Error: no commit file.\n");
                //Erase .Commit
                char* removeCommitCommand = malloc(strlen("rm -f ") + strlen(commitPath));
                strcpy(removeCommitCommand, "rm -f ");
                strcat(removeCommitCommand, commitPath);
                system(removeCommitCommand);
                exit(0);
            }

            char* commitContent = malloc(1);
            counter=0;
            do{
                int status = read(commitFdRead, &ch, 1);
                if(status<1)break;
                commitContent[counter] = ch;
                commitContent = addNewMemory(commitContent, counter+2);
                counter++;
            }while(1);
            commitContent[counter] = '\0';

            close(commitFdRead);
            char* commitSize = malloc(10);
            sprintf(commitSize, "%d", strlen(commitContent));


            int a = write(sockfd, clientID, strlen(clientID));
            //printf("num bytes written for client: %d\n", a);
            int b = write(sockfd, colon, strlen(colon));
            //printf("num bytes written for colon: %d\n", b);
            int c = write(sockfd, commitSize, strlen(commitSize));
            //printf("num bytes written for commit size: %d\n", c);

            int d = write(sockfd, colon, strlen(colon));
            //printf("num bytes written for colon: %d\n", d);

            int e= write(sockfd, commitContent, strlen(commitContent));
            //printf("num bytes written for commit content: %d\n", e);

            //read status byte
            read(sockfd, &ch, 1);
            if(ch == '0'){
                printf("Your push did not go through because someone else pushed.\n");
                exit(0);
            }
            else{
                printf(".Commit has been sent and accepted\n");
            }
            
            //if good, send all files needed
            
            int commitFd = open(commitPath, O_RDONLY);
            counter = 0;
            char* lineToken = malloc(1);
            do{
                int status = read(commitFd, &ch, 1);
                if (status<1){
                    break;
                }
                if(ch == '\n'){
                    lineToken[counter] = '\0';
                    char* code = malloc(counter);
                    code = strtok(lineToken, " ");

                    if(strcmp(code, "M")== 0 || strcmp(code,"A") == 0){
                        char* fileName = malloc(counter);
                        fileName = strtok(NULL, " ");

                        //fill fileNameBuffer
                        char* fileNameBuffer = malloc(fileNameSize);
                        memset(fileNameBuffer, '\0',fileNameSize);
                        strcat(fileNameBuffer, fileName);

                        //get file's path
                        char* fileNamePath = malloc(strlen("./clientDir/")+strlen(argv[2]) + strlen("/") + strlen(fileName));
                        strcpy(fileNamePath, "./clientDir/");
                        strcat(fileNamePath, argv[2]);
                        strcat(fileNamePath, "/");
                        strcat(fileNamePath, fileName);

                        //get file's fd
                    
                        int filefd = open(fileNamePath, O_RDONLY);
                        //printf("current filefd number: %d\n", filefd);

                        //read file content into buffer
                        char* buffer = malloc(1);
                        counter = 0;
                        do{
                            int status = read(filefd, &ch, 1);
                            if(status < 1) break;
                            buffer[counter] = ch;
                            buffer = addNewMemory(buffer, counter+2);
                            counter++;
                        }while(1);
                        buffer[counter] = '\0';

                        //file fileContentBuffer
                        char* fileContentBuffer = malloc(fileContentSize);
                        memset(fileContentBuffer,'\0',fileContentSize);
                        strcat(fileContentBuffer, buffer);

                        write(sockfd, fileNameBuffer, fileNameSize);
                        write(sockfd, fileContentBuffer, fileContentSize);

                        close(filefd);

                    }
                    counter = 0;
                    free(lineToken);
                    lineToken = malloc(1);
                }
                else{
                    lineToken[counter] = ch;
                    lineToken = addNewMemory(lineToken, counter+2);
                    counter++;
                }
            }while(1);
            //after sending all files, update manifest

            char* manifestPath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/.Manifest"));
            strcpy(manifestPath, "./clientDir/");
            strcat(manifestPath, argv[2]);
            strcat(manifestPath, "/.Manifest");
            int manifestFd = open(manifestPath, O_RDONLY);
            char* manifestCurrentVersion = malloc(1);
            counter = 0;
            do{
                read(manifestFd, &ch, 1);
                if(ch=='\n'){
                    manifestCurrentVersion[counter] = '\0';
                    break;
                }
                else{
                    manifestCurrentVersion[counter] = ch;
                    manifestCurrentVersion = addNewMemory(manifestCurrentVersion, counter+2);
                    counter++;
                }
            }while(1);
            //get updated manifest version
            int updatedManifestVersion = atoi(manifestCurrentVersion) + 1;
            char* updatedManifestVersionConverted = malloc(10);
            sprintf(updatedManifestVersionConverted, "%d", updatedManifestVersion);
            //printf("updated manifest version is: %s\n",updatedManifestVersionConverted);



            //make new manifest called manifest2
            char* newManifestPath = malloc(strlen("./clientDir/")+strlen(argv[2])+strlen("/.Manifest2"));
            strcpy(newManifestPath, "./clientDir/");
            strcat(newManifestPath, argv[2]);
            strcat(newManifestPath, "/.Manifest2");
            int newManifestPathFd = open(newManifestPath, O_RDWR | O_CREAT, 0644 | O_APPEND);

            //make sure that file got the permissions
            char* chmodCommand = malloc(strlen("chmod 777 ") + strlen(newManifestPath));
            strcpy(chmodCommand, "chmod 777 ");
            strcat(chmodCommand, newManifestPath);
           // printf("new manifest fd: %d\n", newManifestPathFd);
            //printf("error: %s\n", strerror(errno));

            //write updated version and new line on new manifest
            write(newManifestPathFd, updatedManifestVersionConverted, strlen(updatedManifestVersionConverted));
            char* newline = "\n";
            write(newManifestPathFd, newline, strlen(newline));

            
            //retrieve all tracked files 
            counter = 0;
            char* trackedEntry= malloc(1);
            do{
                int status = read(manifestFd, &ch, 1);
                if(status <1 )break;
                if(ch == '\n'){
                    trackedEntry[counter] = '\0';
                    char* copyOfTrackedEntry = malloc(strlen(trackedEntry));
                    strcpy(copyOfTrackedEntry,trackedEntry);

                    char* code = malloc(counter);
                    code = strtok(copyOfTrackedEntry, " ");
                    if(strcmp(code, "T") == 0){
                        write(newManifestPathFd, trackedEntry, strlen(trackedEntry));
                        char* newline = "\n";
                        write(newManifestPathFd, newline, strlen(newline));
                    }

                    free(trackedEntry);
                    trackedEntry = malloc(1);
                    counter = 0;
                }
                else{
                    trackedEntry[counter] = ch;
                    trackedEntry = addNewMemory(trackedEntry, counter+2);
                    counter++;
                }

            }while(1);

            

            //remove the old manifest
            char* removeCommand = malloc(strlen("rm -f ") + strlen(manifestPath));
            strcpy(removeCommand, "rm -f ");
            strcat(removeCommand, manifestPath);
            system(removeCommand);

            //go through commit to see which lines to write to new manifest
            counter = 0;
            free(lineToken);
            lineToken = malloc(1);
            int newCommitFd = open(commitPath, O_RDONLY);
            do{
                int status = read(newCommitFd, &ch, 1);
                if (status<1){
                     break;
                }
                if(ch == '\n'){
                    lineToken[counter] = '\0';
                    //char* copyOfLineToken = malloc(counter);
                    //strcpy(copyOfLineToken, lineToken);
                    char* code = malloc(counter);
                    code = strtok(lineToken, " "); 

                    if(strcmp(code, "A") == 0 || strcmp(code, "M") == 0){
                        //manifest entry has T code
                        char* trackedCode = "T ";
                        //tokenize filename
                        char* fileName = malloc(counter);
                        fileName = strtok(NULL, " ");
                        //tokenize version
                        char* version = malloc(counter);
                        version = strtok(NULL, " ");
                        //tokenize hash
                        char* hash = malloc(counter);
                        hash = strtok(NULL, "\n");
                        //newline
                        char* newline = "\n";
                        //space
                        char* space = " ";

                        write(newManifestPathFd, trackedCode, strlen(trackedCode));
                        write(newManifestPathFd, fileName, strlen(fileName));
                        write(newManifestPathFd, space, strlen(space));
                        write(newManifestPathFd, version, strlen(version));
                        write(newManifestPathFd, space, strlen(space));
                        write(newManifestPathFd, hash, strlen(hash));
                        write(newManifestPathFd, newline, strlen(newline));

                    }
                    counter = 0;
                    free(lineToken);
                    lineToken = malloc(1);
                }
                else{
                    lineToken[counter] = ch;
                    lineToken = addNewMemory(lineToken, counter+2);
                    counter++;
                }
            }while(1);
            
            rename(newManifestPath, manifestPath);
            //read new manifest content into a buffer
            int manifestReadFd = open(manifestPath, O_RDONLY);
            counter = 0;
            char* newManifestBuffer = malloc(1);
            do{
                int status = read(manifestReadFd, &ch, 1);
                if(status < 1)break;
                newManifestBuffer[counter] = ch;
                newManifestBuffer = addNewMemory(newManifestBuffer, counter+2);
                counter++;
            }while(1);
            newManifestBuffer[counter] = '\0';

            //get size of manifest content
            char* sizeOfNewManifest = malloc(10);
            sprintf(sizeOfNewManifest, "%d", strlen(newManifestBuffer));

            //write new size and manifest to server
            write(sockfd, sizeOfNewManifest, strlen(sizeOfNewManifest));
            write(sockfd, colon, strlen(colon));
            write(sockfd, newManifestBuffer, strlen(newManifestBuffer));

            //Erase .Commit
            char* removeCommitCommand = malloc(strlen("rm -f ") + strlen(commitPath));
            strcpy(removeCommitCommand, "rm -f ");
            strcat(removeCommitCommand, commitPath);
            system(removeCommitCommand);


            //Erase clientID.txt
            char* removeClientCommand = malloc(strlen("rm -f ") + strlen(clientIDPath));
            strcpy(removeClientCommand, "rm -f ");
            strcat(removeClientCommand, clientIDPath);
            system(removeClientCommand);

            //Recreate clientID.txt
            int newClientIDFile = open(clientIDPath, O_RDONLY | O_CREAT, 0644);

            char* chmodClient = malloc(strlen("chmod 777 ") + strlen(clientIDPath));
            strcpy(chmodClient, "chmod 777 ");
            strcat(chmodClient, clientIDPath);
            system(chmodClient);
            
            //close fd's
            close(newClientIDFile);
            close(manifestReadFd);
            close(newCommitFd);
            close(newManifestPathFd);
            close(commitFd);
            close(manifestFd);

            printf("Push successful!\n");



        }
     
        else if(strcmp(argv[1], "create") == 0){
            int sockfd = connectToServer();
            char sizeOfName[10];
            mkdir("./clientDir", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            sprintf(sizeOfName, "%d", strlen(argv[2])); //size of project name in string format
            char* createCommand = malloc(strlen(argv[2]) + strlen("create:")+ strlen(sizeOfName)+ 1);
            strcat(createCommand, "create:");
            strcat(createCommand, sizeOfName);
            strcat(createCommand, ":");
            strcat(createCommand, argv[2]); //createcommand = "create:11:testProject"
            write(sockfd, createCommand, strlen(createCommand));

            char ch;
            read(sockfd, &ch, 1);

            if(ch == '0'){
                //error has occurred
                printf("Error: Project name already exists on server\n");
                exit(0);
            }
            else{
                //assume createResponse has the .Manifest folder
                char* folderPath = malloc(strlen("./clientDir/") + strlen(argv[2]));
                strcat(folderPath, "./clientDir/");
                strcat(folderPath,argv[2]);
                mkdir(folderPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

                char* filePath = malloc(strlen(folderPath) + strlen("/.Manifest"));
                strcat(filePath,folderPath);
                strcat(filePath, "/.Manifest");
                
                int fd = open(filePath, O_RDWR | O_CREAT, 0600);
                char* buffer = "0\n";
                write(fd, buffer,2); //write version 0
                printf("Successfully created new project\n");
                //close fd's
                close(fd);
            }
            char* clientIDPath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/clientID.txt"));
            strcpy(clientIDPath, "./clientDir/");
            strcat(clientIDPath, argv[2]);
            strcat(clientIDPath, "/clientID.txt");
            int fdclientid = open(clientIDPath, O_CREAT);
            char* chmodCommand = malloc(strlen("chmod 777 ") + strlen(clientIDPath));
            strcpy(chmodCommand, "chmod 777 ");
            strcat(chmodCommand, clientIDPath);
            system(chmodCommand);

            close(fdclientid);
        }
        else if (strcmp(argv[1], "destroy") == 0){
            int sockfd = connectToServer();
            char sizeOfName[10];
            sprintf(sizeOfName, "%d", strlen(argv[2])); //size of project name in string format
            char* destroyCommand = malloc(strlen(argv[2]) + strlen("destroy:")+ strlen(sizeOfName)+ 1);
            strcat(destroyCommand, "destroy:");
            strcat(destroyCommand, sizeOfName);
            strcat(destroyCommand, ":");
            strcat(destroyCommand, argv[2]); //createcommand = "create:11:testProject"
            write(sockfd, destroyCommand, strlen(destroyCommand));
  
            char destroyResponse;
            read(sockfd, &destroyResponse, 1);

            if(destroyResponse == '0'){
                printf("Error: project does not exist\n");
            }
            else{
                printf("Successfully destroyed project on server\n");
            }

        }
        else if (strcmp(argv[1], "add") == 0 || strcmp(argv[1], "remove") == 0){
            char* projectPath = malloc(strlen("./clientDir/") + strlen(argv[2]) + strlen("/")+ 1);
            strcpy(projectPath, "./clientDir/");
            strcat(projectPath, argv[2]);
            strcat(projectPath, "/");
            projectPath[strlen("./clientDir/") + strlen(argv[2]) + strlen("/")] = '\0';
            //printf("Project path is: %s\n",projectPath);

            char* filePath = malloc(strlen(projectPath) + strlen(argv[3]));
            strcpy(filePath, projectPath);
            strcat(filePath, argv[3]);
            //printf("file path is: %s\n", filePath);

            char* manifestPath = malloc(strlen(projectPath) + strlen(".Manifest") + 1);
            strcpy(manifestPath,projectPath);
            strcat(manifestPath, ".Manifest");
            strcat(manifestPath, "\0");
            //printf("Manifest file path is: %s\n", manifestPath);

            

            DIR* dir = opendir(projectPath);
            int fileFd = open(filePath, O_RDONLY);
            int manifestFd = open(manifestPath, O_RDWR | O_APPEND);
            if (dir == NULL){
                //project does not exist
                printf("Error: Project does not exist\n");
                exit(0);
            }
            else if(fileFd < 0){
                printf("Error: File does not exist\n");
            }
            else if(manifestFd < 0){
                printf("Error: manifest does not exist\n");
            }
            else{
                if(strcmp(argv[1],"add") == 0){
                    int fileIsNew =1;


                    //check if fileIsNew
                    int manifestReadFd = open(manifestPath, O_RDONLY);
                    char readByte;
                    char* lineToken = malloc(1);
                    do{
                        read(manifestReadFd, &readByte, 1);
                        if(readByte == '\n'){
                            break;
                        }
                    }while(1);
                    
                    int counter = 0;
                    do{
                        int status = read(manifestReadFd, &readByte, 1);
                        if (status<1){
                            break;
                        }
                        if(readByte == '\n'){
                            lineToken[counter] = '\0';
                            char* code = malloc(counter);
                            code = strtok(lineToken, " ");

                            char* fileName = malloc(counter);
                            fileName = strtok(NULL, " ");
                            if(strcmp(fileName, argv[3])==0){
                                fileIsNew = 0;
                                //printf("file is not new\n");
                                char* version = malloc(counter);
                                char* manifestHashOfFile = malloc(counter);
                                version = strtok(NULL, " ");
                                manifestHashOfFile = strtok(NULL, " ");

                                char* systemCommand = malloc(strlen("md5sum ") + strlen(filePath) + strlen(" > hash.txt"));
                                strcpy(systemCommand, "md5sum ");
                                strcat(systemCommand, filePath);
                                strcat(systemCommand, " > hash.txt");
                                int s = system(systemCommand);
                                //printf("system call returned: %d\n", s);
                                char* hashPath = "./hash.txt";
                                int hashFd = open(hashPath, O_RDONLY);
                                char* hashContent = malloc(1);
                                int counter = 0;
                                char ch;
                                do{
                                    int status = read(hashFd, &ch, 1);
                                    if(status < 1 || ch == ' ')break;
                                    else{
                                        hashContent[counter] = ch;
                                        hashContent = addNewMemory(hashContent, counter+2);
                                        counter++;
                                    }

                                }while(1);
                                hashContent[counter] = '\0';

                                if(strcmp(hashContent, manifestHashOfFile) == 0){
                                    printf("File has already been added and no new changes have been made.\n");
                                    exit(0);
                                }
                                break;

                                close(hashFd);
                            }
                            counter = 0;
                            free(lineToken);
                            lineToken = malloc(1);
                        }
                        else{
                            lineToken[counter] = readByte;
                            lineToken = addNewMemory(lineToken, counter+2);
                            counter++;
                        }
                    }while(1);
                    
                    if(fileIsNew){
                       
                        char* systemCommand = malloc(strlen("md5sum ") + strlen(filePath) + strlen(" > hash.txt"));
                        strcpy(systemCommand, "md5sum ");
                        strcat(systemCommand, filePath);
                        strcat(systemCommand, " > hash.txt");
                        int s = system(systemCommand);
                        //printf("system call returned: %d\n", s);

                        char* hashPath = "./hash.txt";
                        int hashFd = open(hashPath, O_RDONLY);
                        char* hashContent = malloc(1);
                        int counter = 0;
                        char ch;
                        do{
                            int status = read(hashFd, &ch, 1);
                            if(status < 1 || ch == ' ')break;
                            else{
                                hashContent[counter] = ch;
                                hashContent = addNewMemory(hashContent, counter+2);
                                counter++;
                            }

                        }while(1);
                        hashContent[counter] = '\0';

                        char* code = "A ";
                        char* version = " 0 ";
                        char* newline = "\n";
                        int a = write(manifestFd, code, strlen(code)); //code for Add
                        //printf("num bytes written: %d\n", a);
                       
                        int b = write(manifestFd, argv[3], strlen(argv[3])); //file name
                        //printf("num bytes written: %d\n", b);

                        int c = write(manifestFd, version, strlen(version)); //version 0
                        //printf("num bytes written: %d\n", c);

                        int d = write(manifestFd, hashContent, strlen(hashContent)); //hash of file
                        //printf("num bytes written: %d\n", d);

                        int e = write(manifestFd, newline, strlen(newline)); //new line
                        //printf("num bytes written: %d\n", e);


                        printf("Success: New file has been added.\n");

                        close(hashFd);
                    }
                    else{
                        //add M to file entry
                        int newManifestFdRead = open(manifestPath, O_RDONLY);
                        int manifestFdWrite = open(manifestPath, O_RDWR);
                        int offset = 0;
                        int bytesRead =0;
                        char ch;
                        int counter = 0;
                        
                        do{
                            read(newManifestFdRead, &ch, 1);
                            offset = offset+1;
                            if(ch == '\n')
                                break;
                        }while(1);
                     
                        char* fileEntry = malloc(1);

                        do{
                            int status = read(newManifestFdRead,&ch, 1);
                            if(status <1) break;
                            else if(ch == '\n'){
                                fileEntry[counter] = '\0';

                                char* code = malloc(counter);
                                code = strtok(fileEntry, " ");

                                char* fileName = malloc(counter);
                                fileName = strtok(NULL, " ");

                                if(strcmp(fileName, argv[3]) == 0){
                                    //printf("fileName matches\n");
                                    lseek(manifestFdWrite, offset, SEEK_SET);
                                    char* codeWrite = "M";
                                    int a = write(manifestFdWrite, codeWrite, strlen(codeWrite));
                                    printf("Modified file successfully added\n");
                                    //printf("num bytes written: %d\n", a);
                                    //use offset
                                    break;
                                }
                                else{
                                    offset = offset + counter+1;
                                }


                                counter = 0;
                                free(fileEntry);
                                fileEntry = malloc(1);
                            }
                            else{
                                bytesRead = bytesRead +1;
                                fileEntry[counter] = ch;
                                fileEntry = addNewMemory(fileEntry, counter+2);
                                counter++;
                            }
                        }while(1);

                        //close fd's
                        close(newManifestFdRead);
                        close(manifestFdWrite);
                    }
                    //close fd's
                    close(manifestReadFd);
                }
                else{
                    //remove the file
                    int manifestFdRead = open(manifestPath, O_RDONLY);
                    int manifestFdWrite = open(manifestPath, O_RDWR);
                    int offset = 0;
                    int bytesRead =0;
                    char ch;
                    int counter = 0;

                    do{
                        read(manifestFdRead, &ch, 1);
                        offset = offset+1;
                        if(ch == '\n'){
                            break;
                        }
                    }while(1);
                  
                    char* fileEntry = malloc(1);
                    int fileFound = 0;

                    do{
                        int status = read(manifestFdRead,&ch, 1);
                        if(status <1) break;
                        else if(ch == '\n'){
                            fileEntry[counter] = '\0';

                            char* code = malloc(counter);
                            code = strtok(fileEntry, " ");

                            char* fileName = malloc(counter);
                            fileName = strtok(NULL, " ");

                            if(strcmp(fileName, argv[3]) == 0){
                                //printf("fileName matches\n");
                                fileFound = 1;
                                lseek(manifestFdWrite, offset, SEEK_SET);
                                char* codeWrite = "D";
                                int a = write(manifestFdWrite, codeWrite, strlen(codeWrite));
                                printf("File has been successfully deleted.\n");
                                //printf("num bytes written: %d\n", a);
                                //use offset
                                break;
                            }
                            else{
                                offset = offset + counter+1;
                            }


                            counter = 0;
                            free(fileEntry);
                            fileEntry = malloc(1);
                        }
                        else{
                            bytesRead = bytesRead +1;
                            fileEntry[counter] = ch;
                            fileEntry = addNewMemory(fileEntry, counter+2);
                            counter++;
                        }
                    }while(1);
                    if(fileFound == 0){
                        printf("error: File was never committed.\n");
                        exit(0);
                    }

                    //close fd's

                    close(manifestFdRead);
                    close(manifestFdWrite);


                }
                
            }
            //close fd's
            close(fileFd);
            close(manifestFd);
            close(dir);
            
        
        }
        else if(strcmp(argv[1], "currentversion") == 0){
            int sockfd = connectToServer();
            char sizeOfName[10];
            sprintf(sizeOfName, "%d", strlen(argv[2])); //size of project name in string format
            char* versionCommand = malloc(strlen(argv[2]) + strlen("currentversion:")+ strlen(sizeOfName)+ 1);
            strcat(versionCommand, "currentversion:");
            strcat(versionCommand, sizeOfName);
            strcat(versionCommand, ":");
            strcat(versionCommand, argv[2]); //versioncommand = "currentversion:11:testProject"
            //printf("writing to sockfd the commands....\n");
            write(sockfd, versionCommand, strlen(versionCommand));

            char ch;
            read(sockfd, &ch, 1);
            
            if(ch == '0'){
                printf("Error: failed to return current version- project may not exist on the server\n");
                exit(0);
            }

            char* manifestSize = malloc(1);
            int counter = 0;
            do{
                read(sockfd, &ch, 1);
                if(ch == ':'){
                    manifestSize[counter] = '\0';
                    break;
                }
                else{
                    manifestSize[counter] = ch;
                    manifestSize = addNewMemory(manifestSize, counter+2);
                    counter++;
                }
            }while(1);

            int manifestSizeConverted = atoi(manifestSize);

            int bytesReadIntoManifest = 0;
            do{
                int status = read(sockfd, &ch, 1);
                if (status <1 )break;
                bytesReadIntoManifest++;
                if(ch == '\n')
                    break;
            }while(1);
            
          
            counter = 0;
            char* fileEntry = malloc(1);
            int bytesReadAfter = 0;
            do{
                if(bytesReadAfter == (manifestSizeConverted - bytesReadIntoManifest))
                    break;
                int status = read(sockfd,&ch, 1);
                if(status <1) break;
                else if(ch == '\n'){
                    fileEntry[counter] = '\0';
                    char* code = malloc(counter);
                    code = strtok(fileEntry, " ");
                    char* fileName = malloc(counter);
                    fileName = strtok(NULL, " ");

                    char* fileVersion = malloc(counter);
                    fileVersion = strtok(NULL, " ");
                    printf("Filename is: %s\n", fileName);
                    printf("Version is: %s\n", fileVersion);

                    counter = 0;
                    free(fileEntry);
                    fileEntry = malloc(1);
                    bytesReadAfter++;
                }
                else{
                    fileEntry[counter] = ch;
                    fileEntry = addNewMemory(fileEntry, counter+2);
                    counter++;
                    bytesReadAfter++;
                }
            }while(1);


        }

        else if(strcmp(argv[1], "history") == 0){
            int sockfd = connectToServer();
            char sizeOfName[10];
            sprintf(sizeOfName, "%d", strlen(argv[2])); //size of project name in string format
            char* versionCommand = malloc(strlen(argv[2]) + strlen("history:")+ strlen(sizeOfName)+ 1);
            strcat(versionCommand, "history:");
            strcat(versionCommand, sizeOfName);
            strcat(versionCommand, ":");
            strcat(versionCommand, argv[2]); //versioncommand = "currentversion:11:testProject"
            //printf("writing to sockfd the commands....\n");
            write(sockfd, versionCommand, strlen(versionCommand));

            char ch;
            read(sockfd, &ch, 1);
            if(ch == '0'){
                printf("Error: project does not exist on server\n");
                exit(0);
            }
            
            char* sizeOfHistoryString = malloc(1);
            int counter =0;
            do{
                read(sockfd, &ch, 1);
                if(ch == ':'){
                    sizeOfHistoryString[counter] = '\0';
                    break;
                }
                else{
                    sizeOfHistoryString[counter] = ch;
                    sizeOfHistoryString = addNewMemory(sizeOfHistoryString, counter+2);
                    counter++;
                }
            }while(1);
            int sizeOfHistoryConverted = atoi(sizeOfHistoryString);

            char* historyBuffer = malloc(sizeOfHistoryConverted+1);
            counter = 0;
            do{
                if(counter == sizeOfHistoryConverted){
                    historyBuffer[counter] = '\0';
                    break;
                }
                else{
                    read(sockfd, &ch, 1);
                    historyBuffer[counter] = ch;
                    historyBuffer = addNewMemory(historyBuffer, counter+2);
                    counter++;
                }
            }while(1);
            printf("%s", historyBuffer);

        }
        else if(strcmp(argv[1], "rollback") == 0){
            int sockfd = connectToServer();
            char sizeOfName[10];
            sprintf(sizeOfName, "%d", strlen(argv[2])); //size of project name in string format
            char* versionCommand = malloc(strlen(argv[2]) + strlen("rollback:")+ strlen(sizeOfName)+ 1);
            strcat(versionCommand, "rollback:");
            strcat(versionCommand, sizeOfName);
            strcat(versionCommand, ":");
            strcat(versionCommand, argv[2]); //versioncommand = "currentversion:11:testProject"
            printf("writing to sockfd the commands....\n");
            write(sockfd, versionCommand, strlen(versionCommand));

        }
        else{
            //invalid instruction
            printf("Invalid command \n");
            exit(0);
        }
    
    }
    return 0;
}