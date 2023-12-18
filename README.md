# File-System
---
A project written in C that utilizes the usage of virtual disk to create, write, read, and delete files. 

## How the code works
- First download the minGW to run GCU
    - Download here - https://sourceforge.net/projects/mingw/ 

- Then add the bin folder of that to your PATH
- Once you have done that, cd into the /apps directory 

- Then run "make" on the terminal to compile the code 

- Run this code to create a disk with 4096 data blocks to read/write on it
    - ```./fs_make.x disk.fs 4096```

- We can find the info from the disk using the reference script by using
    - ```./fs_ref.x info disk.fs```

- This should also work with "test_fs.c"

- Once that's done you can run any of the files with the "./" then the file name
    - ./api_test
    - ./not_so_simple_writer
    - ./simple_reader
    - ./simple_writer
    - ./test_fc

- To perfectly check all the attributes of this code run the api_test.c and follow it's instruction (make sure to have a disk already created using fs_make.x)

