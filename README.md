# directory-sim

This set of programs simulates a disk and allows for the management of directories. 

Instructions:
in terminal
type './DirectoryServer <number of cylinders> <number of sectors> <track time in microseconds>' into terminal and press <enter>.
e.x. './DirectoryServer 10 10 300' starts a server with 10 cylinders, 10 sectors, and 300 microseconds track time.
in another terminal
type './DirectoryClient' into terminal and press <enter>.

Type commands into the client and the client will print out the output returned by the server.
The following commands are valid:
- I: information request. The disk returns two integers representing the disk geometry: the number of cylinders,
and the number of sectors per cylinder.
- R c s: read request for the contents of cylinder c sector s. The disk returns ’1’ followed by those 128 bytes of
information, or ’0’ if no such block exists. 
- W c s l data: write request for cylinder c sector s. l is the number of bytes being provided, with a maximum of 128. The data is those l bytes of data. The disk returns ’1’ to the client if it is a valid write request (legal values of c, s and l), or returns a ’0’ otherwise.
- F: format. Will format the filesystem on the disk.
- C f: create file. This will create a file named f in the filesystem. Possible return codes: 0 = successfully created the file; 1 = a file of this name already existed; 2 = some other failure (such as no space left, etc.).
- D f: delete file. This will delete the file named f from the filesystem. Possible return codes: 0 = successfully deleted the file; 1 = a file of this name did not exist; 2 = some other failure.
- L b: directory listing. This will return a listing of the files and directories in the filesystems. b is a boolean flag: if ’0’ it lists just the names of all the files, one per line; if ’1’ it includes the file lengths.
- R f: read file. This will read the entire contents of the file named f, and return the data that came from it. The message sent back to the client is, in order: a return code, the number of bytes in the file (in ASCII), a whitespace, and finally the data from the file. Possible return codes: 0 = successfully read file; 1 = no such filename exists; 2 = some other failure.
- W f l data: write file. This will overwrite the contents of the file named f with the l bytes of data. A return code is sent back to the client. Possible return codes: 0 = successfully written file; 1 = no such filename exists; 2 = some other failure (such as no space left, etc.).
- mkdir dirname : create a directory of name dirname.
- cd dirname : change current working directory to dirname
- pwd : print the working directory name.
- rmdir dirname: remove the directory given.

**The user needs to press 'ctrl c' to exit the server.** \
**The user can exit the client by typing 'exit'**
