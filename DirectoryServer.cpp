#include <iostream>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h> 
#include <pthread.h>
#include <cstring>
#include <strings.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <cmath>
#define PORT 8181                                                       // define local port


std::vector<std::vector<std::vector<char>>> disk;                       // multidimensional array to represent disk. dimensions: m x n x 128
std::vector<std::vector<int>> freeBlockTable;                           // free block table to store which blocks are free at any point in time

struct block {                      // block struct to represent the position of a block in the disk
    int cylinder;                   // cylinder of block
    int sector;                     // sector of block
};


struct file {                       // file struct to represent a file
    std::string name;               // name of file
    int length;                     // length of data stored in file
    std::vector<block> blockList;   // list of blocks being used by the file
};


struct directory {                          // directory struct to represent directory (works like an m-tree node)
    std::string name;                       // name of directory             
    std::vector<directory *> children;      // list of pointers to children directories
    std::vector<file> files;                // list of files in directory
    struct directory *parent;               // pointer to parent directory

    directory(std::string name) {
        this->name = name;
        this->parent = NULL;
    }
};

void removeDirectory(directory *dir) {                                                                                  // utility function to delete directory (allows deletion of all files in subdirectories)
    for (int i = 0; i < dir->files.size(); i++) {                                                                       // for every file in the directory
        for (int j = 0; j < dir->files[i].blockList.size(); j++) {                                                      // for every block in the file
            for (int k = 0; k < 128; k++) {                                                                             // fill block with '\0' (remove all information from block)
                disk[dir->files[i].blockList[j].cylinder][dir->files[i].blockList[j].sector][k] = '\0';
            }
            freeBlockTable[dir->files[i].blockList[i].cylinder][dir->files[i].blockList[i].sector] = 0;                 // set block to free in the free block table
        }
    }

    for (int i = 0; i < dir->children.size(); i++) {                                                                    // for every child directory
        removeDirectory(dir->children[i]);                                                                              // call removeDirectory recursively
    }
}


std::string getPath(directory *dir) {                                  // utility function to get the full path of a directory
    
    directory *currentDir = dir;                                       // set current directory to the directory passed to getPath
    std::string path = dir->name;                                      // set path to the name of the current directoy
    
    while (currentDir->parent != NULL) {                               // iterate until the current directory is the root directory
        path = currentDir->parent->name + "/" + path;                  // add directory to the path
        currentDir = currentDir->parent;                               // change current directory to the current directory's parent
    }

    return path;                                                       // return the path string once the root directory is reached
}


struct directory *root = new directory("root");                        // root variable represents the current directory

int microSeconds;                                                      // variable to store the track time in microseconds


/* This is the thread function that receives a command from the client
upon successfull connection (through a socket), executes command and 
sends results back through the socket */
void *connection(void *newS) 
{
    int newSock = static_cast<int>(reinterpret_cast<intptr_t>(newS));   // cast the sock-fd back to an int     
    char rbuf[BUFSIZ];                                                  // allocate 1024 bytes to read buffer
    char *cmdAr[BUFSIZ];                                                // command array allocated 1024 bytes

    for (int i = 0; i < BUFSIZ; i++) {
        cmdAr[i] = NULL;
    }

    bzero((char*)rbuf, BUFSIZ);                                         // zero out read buffer
    bzero((char*)cmdAr, BUFSIZ);                                        // zero out command buffer
            
    if ((read(newSock, rbuf, BUFSIZ)) > 0) {
        std::cout << "Command received is: " << rbuf << std::endl;      // print command if received
    }
    else {
        std::cout << "Couldn't receive command /n";                     // issue error if could not receive command
    }

    int cmdLen = 0;                                                     // counter, will count how long the command is
    char* token = strtok(rbuf, " ");                                    // tokenize, split the command by words, space is the delimiter
        
    // while iterating through the command, increment the cmdLen counter, 
    // to determine size of the command, and add each word to the cmdAr array
    while (token) {
        cmdAr[cmdLen] = token;
        cmdLen++;
        token = strtok(NULL, " ");
    }

    char * cmd = cmdAr[0];           // set cmd to the first element of the command array (this will be used to determine what command the server receives)
    
    long cylinderSize = disk.size();    // get the number of cylinders
    long sectorSize = disk[0].size();   // get the number of sectors
    
    if (strcmp(cmd, "I") == 0) {                                     // if the command is "I"
        send(newSock, &cylinderSize ,sizeof(cylinderSize), 0);       // send the number of cylinders
        send(newSock, &sectorSize, sizeof(sectorSize), 0);           // send the number of sectors
        close(newSock); 
    }
    else if (strcmp(cmd, "R") == 0) {                                // if the command is "R"
        
        bool readSector = true;                                      // boolean to store whether the R command is for reading a sector or reading a file

        if (cmdAr[2] == NULL) {                                      // if a second argument is not present, the command is for reading a file
            readSector = false;
        }

        if (readSector) {                                            // if the command is for reading a sector
            try {
                usleep(microSeconds);                                                                                                                                       // simulate track time by waiting for a user inputted microseconds    
                long reqCylinder = std::stoi(std::string(cmdAr[1]));                                                                                                        // store the requested cylinder from the command            
                long reqSector = std::stoi(std::string(cmdAr[2]));                                                                                                          // store the requested sector from the command
                std::string outputData = "";                                                                                                                                // initialize output string to be a blank string
                if (reqCylinder > 0 && reqSector > 0 && reqCylinder <= cylinderSize && reqSector <= sectorSize) {                                                           // if the cylinder and sector are valid
                    outputData = outputData + std::to_string(1) + std::string(disk[reqCylinder - 1][reqSector - 1].begin(), disk[reqCylinder - 1][reqSector - 1].end());    // append 1 and the contents of the sector to the string
                }
                else {                                                                                                                                                      // otherwise
                    outputData = std::to_string(0);                                                                                                                         // append 0 to the string
                }
                send(newSock, outputData.c_str(), 1024, 0);                                                                                                                 // send the string to the client
                close(newSock);
            }
            catch (std::invalid_argument& e) {
                send(newSock, "2", 1024, 0);                                                                                                                                // if there is an exception send the code 2 to the client
                close(newSock);
            }
        }
        else {                                                                                                                                      // if the command is for reading a file
            try {
                std::string fileName = std::string(cmdAr[1]);                                                                                           // get the filename
                bool fileFound = false;                                                                                                                 // boolean that stores whether or not the file exists
                int fileNo;                                                                                                                             // integer that stores the file's position in the directory's file vector
                for (int i = 0; i < root->files.size(); i++) {                                                                                          // search through every file in the directory's file list to see if there is a file with a matching filename
                    if (strcmp(root->files[i].name.c_str(), fileName.c_str()) == 0) {
                        fileFound = true;                                                                                                               // if a match is found set fileFound to true
                        fileNo = i;                                                                                                                     // if a match is found set fileNo to the matched index
                    }
                }
                std::string outputString = "";                                                                                                          // create an empty output string
                if (fileFound) {
                    outputString = outputString + "0 " + std::to_string(root->files[fileNo].length) + " ";                                              // add 0 and the length of the file to the string
                    for (int i = 0; i < root->files[fileNo].blockList.size(); i++) {                                                                    // iterate through every block in the blocklist of the file
                        std::vector<char> sectorData = disk[root->files[fileNo].blockList[i].cylinder][root->files[fileNo].blockList[i].sector];        // get the data from each block
                        outputString = outputString + std::string(sectorData.begin(), sectorData.end());                                                // add the data to the output string
                    } 
                }
                else {                                                                                                                                  // if the file was not found
                    outputString = outputString + "1";                                                                                                  // add 1 to the output string
                }
                send(newSock, outputString.c_str(), 1024, 0);                                                                                           // send the output string to the client   
                close(newSock);
            } catch (std::invalid_argument& e) {                                                                                                          // if an exception is found
                send(newSock, "2", 1024, 0);                                                                                           // send an output code of 2 to the client   
                close(newSock);
            }
        }
    }
    else if (strcmp(cmd, "W") == 0) {                                            // if the command is "W"
        
        bool writeSector = true;                                                 // initialize a variable to store whether we want to read from a file or a sector

        if (cmdAr[4] == NULL) {
            writeSector = false;                                                 // if 4 or less arguments are given, we are writing to a file
        }
        
        if (writeSector) {                                                       // if we are writing to a sector
            
            try {
                usleep(microSeconds);                                            // simulate track time by pausing the program for a few microseconds
                long reqCylinder = std::stoi(std::string(cmdAr[1]));             // get the requested cylinder number
                long reqSector = std::stoi(std::string(cmdAr[2]));               // get the requested sector number
                long dataLength  = std::stoi(std::string(cmdAr[3]));             // get the input data length
                std::string data = std::string(cmdAr[4]);                        // get the input data
                
                int code;                                                        // create a varibale to store the output code

                if (dataLength <= 128 && dataLength > 0 && reqCylinder > 0 && reqSector > 0 && reqCylinder <= cylinderSize && reqSector <= sectorSize &&  (dataLength == data.size())) {           // if the input arguments are valid
                    for (int i = 0; i < 128; i++) {                                                                                                                                                // set all the characters in the sector to null
                        disk[reqCylinder - 1][reqSector - 1][i] = '\0';
                    }
                    for (int i = 0; i < dataLength; i++) {                                                                                                                                         // copy the characters from the data string to the sector
                        disk[reqCylinder - 1][reqSector - 1][i] = data[i];
                    }
                    code = 1;                                                                                                                                                                      // set the output code to 1
                }
                else {                                                                                                                                                                             // if the input arguments are invalid
                    code = 0;                                                                                                                                                                      // set the output code to 0                                                                                   
                }                                                                                               
                
                send(newSock, &code , sizeof(int), 0);                                                                                                                                             // send the output code back to the client
                close(newSock);
            } catch(std::invalid_argument& e) {                                            // if there is an exception
                int code = 2;   
                send(newSock, &code , sizeof(int), 0);                  // send an output code of 2 back to the client   
                close(newSock);
            }
        }
        else {                                                                                  // if we are writing to a file
            try {
                std::string fileName = std::string(cmdAr[1]);                                   // get the filename
                
                bool fileFound = false;                                                         
                int fileNo;
                for (int i = 0; i < root->files.size(); i++) {                                  // check if the file exists by iterating through every file and checking if it has the same name as the input filename
                    if (strcmp(root->files[i].name.c_str(), fileName.c_str()) == 0) {
                        fileFound = true;
                        fileNo = i;
                    }
                }

                if (fileFound) {                                                                // if the file is found

                    int dataLength = std::stoi(std::string(cmdAr[2]));                          // get input data length

                    std::string data = std::string(cmdAr[3]);                                   // get input data

                    int numBlocks = ceil(dataLength/128.0);                                     // calculate the number of blocks necessary in order to store the input data

                    int existingBlocks = root->files[fileNo].blockList.size();                  // get the number of existing blocks being used by the file

                    if (existingBlocks > numBlocks) {                                           // if the file is using more blocks than the required number of blocks

                        while (root->files[fileNo].blockList.size() > numBlocks) {              // keep removing blocks until the number of blocks being used by the file is equal to the number of blocks required by the file
                            block temp = root->files[fileNo].blockList.back();
                            freeBlockTable[temp.cylinder][temp.sector] = 0;                     // set the block to be free in the free block table 
                            for (int i = 0; i < 128; i++) {
                                disk[temp.cylinder][temp.sector][i] = '\0';                     // before removing the block from the file remove all its data
                            }
                            root->files[fileNo].blockList.pop_back();
                        }
                        
                        for (int i = 0; i < root->files[fileNo].blockList.size(); i++) {        // clear data from all remaining blocks being used by the file
                            int cylinder = root->files[fileNo].blockList[i].cylinder;
                            int sector = root->files[fileNo].blockList[i].sector;
                            for (int j = 0; j < 128; j++) {
                                disk[cylinder][sector][j] = '\0';
                            }
                        }

                        int index = 0;                                                           // store the current character index of the input data
                        
                        for (int i = 0; i < root->files[fileNo].blockList.size(); i++) {        // iterate through every block in the blocklist of the file
                            int cylinder = root->files[fileNo].blockList[i].cylinder;
                            int sector = root->files[fileNo].blockList[i].sector;
                            for (int j = 0; j < 128; j++) {                                     // assign 128 bytes from the input data to the block before moving on to the next block
                                if (index > dataLength - 1) {
                                    break;
                                }
                                disk[cylinder][sector][j] = data[index];
                                index++;
                            }
                        }

                        root->files[fileNo].length = dataLength;                                // set the length of the file to the datalength
                        
                        int code = 0;                                                           
                        send(newSock, &code , sizeof(int), 0);                                  // send a return code of 0 back to the client
                        close(newSock);
                    }
                    else if (existingBlocks == numBlocks) {                                           // if the number of existing blocks and the number of requested blocks are the same
                        for (int i = 0; i < root->files[fileNo].blockList.size(); i++) {              // clear data from all remaining blocks being used by the file
                            int cylinder = root->files[fileNo].blockList[i].cylinder;
                            int sector = root->files[fileNo].blockList[i].sector;
                            for (int j = 0; j < 128; j++) {
                                disk[cylinder][sector][j] = '\0';
                            }
                        }

                        int index = 0;                                                              // store the current character index of the input data
                        
                        for (int i = 0; i < root->files[fileNo].blockList.size(); i++) {            // iterate through every block in the blocklist of the file
                            int cylinder = root->files[fileNo].blockList[i].cylinder;
                            int sector = root->files[fileNo].blockList[i].sector;
                            for (int j = 0; j < 128; j++) {                                         // assign 128 bytes from the input data to the block before moving on to the next block
                                if (index > dataLength - 1) {
                                    break;
                                }
                                disk[cylinder][sector][j] = data[index];
                                index++;
                            }
                        }

                        root->files[fileNo].length = dataLength;                                    // set the length of the file to the datalength

                        int code = 0;
                        send(newSock, &code , sizeof(int), 0);                                      // send a return code of 0 back to the server
                        close(newSock);
                    }
                    else {                                                                          // if the number of existing blocks is less than the requested blocks

                        int newBlocks = 0;                                                          // create a variable to store the number of new blocks added (used when there is no more space left)
                        bool freeSpaceAvailable = true;                                             // create a variable to store whether or not there is enough space available

                        while (root->files[fileNo].blockList.size() < numBlocks) {                  // while the number of existing blocks is less than the number of requested blocks        
                            int cylinder;
                            int sector;
                            bool freeBlockFound = false;                                            // create a variable to store whether or not a free block is found
                            for (int i = 0; i < freeBlockTable.size(); i++) {                       // iterate through the cylinders in the free block table
                                if (freeBlockFound) {
                                    break;
                                }
                                for (int j = 0; j < freeBlockTable[0].size(); j++) {                // iterate through the sectors in the free block table
                                    if (freeBlockTable[i][j] == 0) {                                // if a block with a 0 is found then we have found a free block
                                        freeBlockFound = true;
                                        cylinder = i;                                               // store the cylinder and sector numbers of the free block
                                        sector = j;
                                        freeBlockTable[i][j] = 1;                                   // set the block to not be free
                                        break;
                                    }
                                }
                            }
                            if (freeBlockFound) {                                                   // if a free block is found
                                block b = {cylinder, sector};
                                root->files[fileNo].blockList.push_back(b);                         // add a new block with the cylinder and sector numbers of the free block to the file
                                newBlocks++;
                            }
                            else {                                                                  // if a free block was not found
                                freeSpaceAvailable = false;                                         
                                for (int i = 0; i < newBlocks; i++) {                               // for every time a new block was added
                                    block temp = root->files[fileNo].blockList.back();              // get the last block in the block list
                                    freeBlockTable[temp.cylinder][temp.sector] = 0;                 // set the block to be free
                                    root->files[fileNo].blockList.pop_back();                       // remove the block from the file
                                }
                                break;                                                              // exit the loop since there is no more space available
                            }
                        }


                        if (freeSpaceAvailable) {                                                   // if there is free space available

                            for (int i = 0; i < root->files[fileNo].blockList.size(); i++) {        // clear all data in the blocks allocated to the file
                                int cylinder = root->files[fileNo].blockList[i].cylinder;
                                int sector = root->files[fileNo].blockList[i].sector;
                                for (int j = 0; j < 128; j++) {
                                    disk[cylinder][sector][j] = '\0';
                                }
                            }

                            int index = 0;                                                          // create a variable to store the current index of the input data
                            
                            for (int i = 0; i < root->files[fileNo].blockList.size(); i++) {        // iterate through every block in the list of allocated blocks
                                int cylinder = root->files[fileNo].blockList[i].cylinder;
                                int sector = root->files[fileNo].blockList[i].sector;
                                for (int j = 0; j < 128; j++) {                                     // add chunks of 128 bytes from the input data to the blocks
                                    if (index > dataLength - 1) {
                                        break;
                                    }
                                    disk[cylinder][sector][j] = data[index];
                                    index++;
                                }
                            }

                            root->files[fileNo].length = dataLength;                                // set the file length to the input data length

                            int code = 0;
                            send(newSock, &code , sizeof(int), 0);                                  // send a code of 0 back to the client
                            close(newSock);

                        }
                        else {

                            int code = 2;
                            send(newSock, &code , sizeof(int), 0);                                  // if there is no space available send a code of 2 back to the client
                            close(newSock);

                        }

                    }
                }
                else {
                    int code = 1;
                    send(newSock, &code , sizeof(int), 0);                                          // if a file was not found send a code of 0 back to the client
                    close(newSock);
                }
            } catch (std::invalid_argument& e) {
                int code = 2;
                send(newSock, &code , sizeof(int), 0);                                              // if an exception was encountered send a code of 2 back to the client
                close(newSock);
            }
        }
    }
    else if (strcmp(cmd, "F") == 0) {                                       // if the command is "F"
        //clear all data from disk                                          
        for (int i = 0; i < disk.size(); i++) {
            for (int j = 0; j < disk[0].size(); j++) {
                for (int k = 0; k < 128; k++) {
                    disk[i][j][k] = '\0';
                }
            }
        }

        // set root to the actual root directory instead of the current directory
        while(root->parent != NULL) {
            root = root->parent;
        }


        int size = root->files.size();                                      // store the number of files in the root directory

        for (int i = 0; i < size; i++) {                                    // remove all files from the root directory
            root->files.pop_back();
        }

        int dirListSize = root->children.size();

        for (int i = 0; i < dirListSize; i++) {                             // remove all child directories from the root directory
            root->children.pop_back();
        }

        for (int i = 0; i < freeBlockTable.size(); i++) {                   // set all blocks to be free in the free block table
            for (int j = 0; j < freeBlockTable[0].size(); j++) {
                freeBlockTable[i][j] = 0;
            }
        }

        int code = 1;

        send(newSock, &code , sizeof(int), 0);                              // send a code of 0 back to the client
        close(newSock);
    }
    else if (strcmp(cmd, "C") == 0) {                                           // if the command is "C"
        std::string fileName = std::string(cmdAr[1]);                           // get the filename

        bool fileAlreadyExists = false;                                         

        for (int i = 0; i < root->files.size(); i++) {                              // check if the file already exists
            if (strcmp(root->files[i].name.c_str(), fileName.c_str()) == 0) {
                fileAlreadyExists = true;
            }
            break;
        }
        if (!fileAlreadyExists) {                                                   // if the file doesn't already exist
            int cylinder;
            int sector;
            bool freeBlockFound = false;
            for (int i = 0; i < freeBlockTable.size(); i++) {                       // find a free block in the free block table
                if (freeBlockFound) {
                    break;
                }
                for (int j = 0; j < freeBlockTable[0].size(); j++) {
                    if (freeBlockTable[i][j] == 0) {
                        freeBlockFound = true;
                        cylinder = i;
                        sector = j;
                        freeBlockTable[i][j] = 1;
                        break;
                    }
                }
            }
            if (freeBlockFound) {                                   // if a free block is found
                std::vector<block> blocks;
                block b = {cylinder, sector};                       // create a new block with the free block's cylinder and sector
                blocks.push_back(b);                                // add it to a new block list

                file newFile = {                                    // create a new file with the created block list
                    fileName, 
                    0,
                    blocks
                };

                root->files.push_back(newFile);                     // add the file to the list of files in the directory

                int code = 0;
                send(newSock, &code , sizeof(int), 0);              // return a code of 0 to the client
                close(newSock);
            }
            else {                                                  // if there is no space remaining
                int code = 2;
                send(newSock, &code , sizeof(int), 0);              // send a code of 2 back to the client
                close(newSock);
            }
        }
        else {
            int code = 1;
            send(newSock, &code , sizeof(int), 0);                  // if the file wasn't found send a code of 1 back to the client
            close(newSock);
        }
    }
    else if (strcmp(cmd, "D") == 0) {                               // if the command is "D"
        std::string fileName = std::string(cmdAr[1]);               // get the file name

        bool fileExists = false;
        int fileNo;

        for (int i = 0; i < root->files.size(); i++) {                              // check if the file exists
            if (strcmp(root->files[i].name.c_str(), fileName.c_str()) == 0) {
                fileExists = true;
                fileNo = i;
                break;
            }
        }

        if (fileExists) {                                                                       // if the file exists
            file File = root->files[fileNo];
            for (int i = 0; i < File.blockList.size(); i++) {                                   // clear all of the blocks being used by the file
                for (int j = 0; j < 128; j++) {
                    disk[File.blockList[i].cylinder][File.blockList[i].sector][j] = '\0';
                }
                freeBlockTable[File.blockList[i].cylinder][File.blockList[i].sector] = 0;       // set all the blocks being used by the file to be free
            }

            root->files.erase(root->files.begin() + fileNo);                                    // remove the file from the file list
            
            int code = 0;
            send(newSock, &code , sizeof(int), 0);                                              // send a code of 0 back to the client
            close(newSock);
        }
        else {
            int code = 1;
            send(newSock, &code , sizeof(int), 0);                                              // if the file doesn't exist send a code of 1 back to the client
            close(newSock);
        }
    }
    else if (strcmp(cmd, "L") == 0) {                                                           // if the command is "L"
        try {    
            int bFlag = std::stoi(std::string(cmdAr[1]));                                           // get the boolean flag
            std::string finalStr = "";                                                              // create a string to store the files and directories in the current directory

            if (bFlag == 0) {                                                                       // if the boolean flag is 0
                for (int i = 0; i < root->children.size(); i++) {
                    finalStr = finalStr + root->children[i]->name + " (directory)\n";               // add all the directory names to the string
                }
                for (int i = 0; i < root->files.size(); i++) {
                    finalStr = finalStr + root->files[i].name + "\n";                               // add all the filenames to the string
                }
                send(newSock, finalStr.c_str() , 1024, 0);                                          // send the string with all the directory names and filenames to the client
                close(newSock);
            }
            else {
                for (int i = 0; i < root->children.size(); i++) {
                    finalStr = finalStr + root->children[i]->name + " (directory)\n";               // add all directory names to the string
                }
                for (int i = 0; i < root->files.size(); i++) {
                    finalStr = finalStr + root->files[i].name + "\t" + std::to_string(root->files[i].length) + " bytes" + "\n";     // add all file names and their lengths to the string
                }
                send(newSock, finalStr.c_str() , 1024, 0);                                          // send the final string back to the client  
                close(newSock);
            }
        } catch (std::invalid_argument& e) {

        }
    }
    else if (strcmp(cmd, "pwd") == 0) {                          // if the command is "pwd"
        std::string dirPath = getPath(root);                     // get the path of the current directory
        send(newSock, dirPath.c_str() , 1024, 0);                // send the path back to the client
        close(newSock);
    }
    else if (strcmp(cmd, "mkdir") == 0) {                        // if the command is "mkdir"
        std::string fileName = std::string(cmdAr[1]);            // get the directory name
        
        bool dirAlreadyExists = false;                          

        for (int i = 0; i < root->children.size(); i++) {                               // check if the directory already exists
            if(strcmp(root->children[i]->name.c_str(), fileName.c_str()) == 0) {
                dirAlreadyExists = true;
            }
        }

        if (!dirAlreadyExists) {                                                // if it doesn't already exist
            directory *newDir = new directory(fileName);                        // create a new directory
            newDir->parent = root;                                              // set the parent of the new directory to the current directory
            root->children.push_back(newDir);                                   // add the new directory to the list of children of the current directory

            send(newSock, "Directory created.", 1024, 0);                       // send a directory created message back to the client
            close(newSock);
        }
        else {
            send(newSock, "Directory already exists." , 1024, 0);               // the directory already exists send a message back to the client
            close(newSock);
        }
    }
    else if (strcmp(cmd, "cd") == 0) {                                                          // if the command is "cd"
        std::string fileName = std::string(cmdAr[1]);                                           // get the directory name
        
        bool dirExists = false;
        int dirNo;
        if (strcmp(fileName.c_str(), "..") != 0) {                                              // if the directory name argument is not ".." 
            for (int i = 0; i < root->children.size(); i++) {                                   // check if the file with the directory name exists
                if(strcmp(root->children[i]->name.c_str(), fileName.c_str()) == 0) {            
                    dirExists = true;
                    dirNo = i;
                    break;
                }
            }

            if (!dirExists) {                                                                    // if a child with the name does not exist, send a message back to the client
                send(newSock, "Directory does not exist.", 1024, 0);   
                close(newSock);
            }
            else {                                                                               // if a child with the name does exist
                directory *newRoot = root->children[dirNo];                                      // change the current directory to the child
                root = newRoot;
                std::string outputString = "Directory changed to " + root->name + ".";           
                send(newSock, outputString.c_str() , 1024, 0);                                   // send a message with the directory name back to the client
                close(newSock);
            }
        }
        else {                                                                              // if the filename argument is ".."
            if (root->parent != NULL) {                                                     // if the parent of the current directory is not null (meaning it is not the root)
                root = root->parent;                                                        // change the current directory to the current directory's parent
                std::string outputString = "Directory changed to " + root->name + ".";      
                send(newSock, outputString.c_str() , 1024, 0);                              // send a message witht the directory name back to the client
                close(newSock);
            }
            else {
                send(newSock, "Cannot go back from root directory.", 1024, 0);              // if the parent of the current directory is null send a message back to the client
                close(newSock);
            }
        }
        
    }
    else if (strcmp(cmd, "rmdir") == 0)  {                                          // if the command is "rmdir"
        std::string fileName = std::string(cmdAr[1]);                               // get the directory name
        
        bool dirExists = false;
        int dirNo;

        for (int i = 0; i < root->children.size(); i++) {                           // check if the directory exists
            if(strcmp(root->children[i]->name.c_str(), fileName.c_str()) == 0) {    
                dirExists = true;
                dirNo = i;
                break;
            }
        }

        if (!dirExists) {                                                           // if the directory does not exist send a message back to the client
            send(newSock, "Directory does not exist.", 1024, 0);   
            close(newSock);
        }
        else {                                                                      // if the directory does exist call the remove directory function and remove the directory from the list of children
            removeDirectory(root->children[dirNo]);
            root->children.erase(root->children.begin() + dirNo);
            std::string outputString = "Directory removed.";
            send(newSock, outputString.c_str() , 1024, 0);   
            close(newSock);
        }
    }
    else {                                                                          // if the command isn't any of the above send a message to the client saying that it is invalid
        send(newSock, "Invalid command.", 1024, 0);                                 
        close(newSock);
    }

    pthread_exit(NULL);                                                 // exit the thread
}

int main(int argc, char* argv[])
{

    if (argc != 4) {
        printf("Server requires 3 parameters: (1) no. of cylinders (2) no. of sectors (3) track-to-track time in microseconds\n");
        exit(EXIT_FAILURE);
    }

    int cylinders = atoi(argv[1]);              // get the number of cylinders
    int sectors = atoi(argv[2]);                // get the number of sectors
    microSeconds = atoi(argv[3]);               // get the track time

    //populate disk with empty strings
    for (int i = 0; i < cylinders; i++) {
        std::vector<std::vector<char>> sectorArr;
        for (int j = 0; j < sectors; j++) {
            std::vector<char> charArr;
            for(int k = 0; k < 128; k++) {
                charArr.push_back('\0');
            }
            sectorArr.push_back(charArr);
        }
        disk.push_back(sectorArr);
    }

    //populate the free block tables with 0's
    for (int i = 0; i < cylinders; i++) {
        std::vector<int> sectorArr;
        for (int j = 0; j < sectors; j++) {
            sectorArr.push_back(0);
        }
        freeBlockTable.push_back(sectorArr);
    }


    int serverFd;                               // fd for server/socket
    int newSock;                                // fd for new socket to pass to thread
    struct sockaddr_in addr;                    // socket address struct
    int addrlen = sizeof(addr);                 // get address size
    char buf[BUFSIZ];                           // create buffer with 1024 bytes allocated
    pthread_t thread_id;                        // fd for threads

    serverFd = socket(AF_INET, SOCK_STREAM, 0); // create socket 
    if (serverFd < 0) { 
        std::cout << "Socket failed\n";         // if socket creation fails, issue error
        exit(1);                                // exit if error
    }
    
    memset(&addr, '\0', addrlen);               // clear out the struct
    addr.sin_family = AF_INET;                  // for IPv4 (from socket)
    addr.sin_addr.s_addr = INADDR_ANY;          // connect to any active local address/port
    addr.sin_port = htons(PORT);                // set port to listen to local PORT
        
    if (bind(serverFd, (struct sockaddr *)&addr, addrlen) < 0) {
        std::cout << "Binding failed\n";        // if a bind is unsuccessfull issue error
        exit(1);                                // exit in the case of an error
    }   
    if (listen(serverFd, 5) < 0) {
        std::cout << "Listening failed\n";      // if listening for connection fails (5 pending), issue error
        exit(1);                                // if listen fails, exit
    }

    while ((newSock = accept(serverFd, (struct sockaddr *)&addr, (socklen_t*)&addrlen)) > 0) {
        if (newSock < 0) { 
            std::cout << "Accept failed";       // if connection not accepted, issue error
            exit(1);                            // if can't connect, exit
        }
        pthread_create(&thread_id, NULL, connection, (void*)(size_t)newSock);   // create a thread to connection function with sockFD
		pthread_join(thread_id, NULL);                                          // join the thread after exit
    }
    close(serverFd);                            // close the socket
    return 0;                                   // exit program
}