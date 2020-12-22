#include <iostream>
#include <arpa/inet.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <string.h> 
#define PORT 8181                                                   // define local port

int main() 
{   
    char command[BUFSIZ];                                           // command from the user, BUFSIZ is 1024 bytes
    int cSock = 0;                                                  // socket fd
    char rbuff[BUFSIZ] = {0};                                       // read buffer (initially zero'd out)
    struct sockaddr_in caddr;                                       // socket address struct
    int caddrlen = sizeof(caddr);                                   // get address size

    // while the word "exit" is not typed, continue the client
    while(strcmp(command, "exit") != 0) {
        
        bzero((char*)rbuff, BUFSIZ);                                // zero out read buffer
        std::cout << "\nEnter a command (Or type 'exit' to quit): ";// print user instruction
        std::cin.getline(command, BUFSIZ);                          // get the command from the user
        
        if (strcmp(command, "exit") == 0) {
            break;                                                  // if the word "exit" is entered, exit the loop
        }

        cSock = socket(AF_INET, SOCK_STREAM, 0);                    // create the socket

        if (cSock < 0) { 
            std::cout << "Socket Failed \n";                        // if socket fails issue error
            exit(1);                                                // exit in the case of an error 
        } 
   
        memset(&caddr, '\0', caddrlen);                             // clear out the struct
        caddr.sin_family = AF_INET;                                 // for IPv4 (from socket)
        caddr.sin_port = htons(PORT);                               // set port to listen to local PORT
   
        if (connect(cSock, (struct sockaddr *)&caddr, caddrlen) < 0) { 
            std::cout << "Connection failed \n";                    // if can't connect to server, issue error
            exit(1);                                                // exit in the case of an error
        } 
        
        std::cout << "\nCommand Entered: " << command << "\n";      // print the command
        send(cSock, command, strlen(command), 0);                   // send command through socket
        
        if(command[0] == 'I') {                                     // if the command sent starts with "I"
            long cylinders;                                         
            long sectors; 
            read(cSock, &cylinders, sizeof(long));                  // read the number of cylinders sent by the server
            read(cSock, &sectors, sizeof(long));                    // read the number of sectors sent by the server
            std::cout << "Cylinders: " << std::to_string(cylinders) << "\nSectors: " << std::to_string(sectors) << "\n";       // print the number of cylinders and sectors to the user
        }
        else if (command[0] == 'W') {                               // if the command sent starts with "W"
            int code;
            read(cSock, &code, sizeof(int));                        // get the return code from the server
            std::cout << "Returned Code: " << std::to_string(code) << "\n";    // print the return code to the user
        }
        else if (command[0] == 'F') {                               // if the command sent starts with "F"
            int code;
            read(cSock, &code, sizeof(int));                        // get the code sent back by the server
            if (code == 1) {
                std::cout << "Successfully formatted file system." << "\n";     // if the code is a 1 tell user that the disk was formatted successfully
            }
        }
        else if (command[0] == 'C') {                               // if the command sent starts with "C"
            int code;
            read(cSock, &code, sizeof(int));                        // get the return code from the server
            std::cout << "Returned Code: " << std::to_string(code) << "\n";     // print the return code to the user
        }
        else if (command[0] == 'D') {                               // if the command sent starts with "D"
            int code;                                               
            read(cSock, &code, sizeof(int));                        // get the return code from the server
            std::cout << "Returned Code: " << std::to_string(code) << "\n";    // print the return code to the user
        }
        else {
            read(cSock, rbuff, 1024);                                   // read the results
            std::cout << rbuff << "\n";                                 // print the results
        }

        
    }
    close(cSock);                                                   // close the socket
    return 0;                                                       // exit the program 
}