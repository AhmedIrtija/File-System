# File-System
---
A project written in C that utilizes the usage of virtual disk to create, write, read, and delete files. 

## How the Code Works

1. **Download MinGW to Run GCU**
    - Download MinGW [here](https://sourceforge.net/projects/mingw/).

2. **Add MinGW Bin Folder to Your PATH**
   
3. **Change Directory to /apps**
   
4. **Compile the Code**
    - Execute the following command in the terminal:
      ```
      make
      ```

5. **Create a Disk with 4096 Data Blocks**
    - Run the command:
      ```bash
      ./fs_make.x disk.fs 4096
      ```

6. **Retrieve Disk Information Using the Reference Script**
    - Execute:
      ```bash
      ./fs_ref.x info disk.fs
      ```

7. **Works the same with "test_fs.c"**

8. **Run Individual Files**
   - Execute the following commands for each file:
      - `./api_test`
      - `./not_so_simple_writer`
      - `./simple_reader`
      - `./simple_writer`
      - `./test_fc`

9. **Check All Code Attributes**
    - Run `api_test.c` and follow its instructions. Ensure that a disk is already created using `fs_make.x`.


