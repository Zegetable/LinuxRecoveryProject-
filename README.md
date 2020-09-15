don't run the project unless you have a linux partition you don't care about

# Final Project
## Compilation
To compile enter: 
`make build` 
or
`gcc -lm -o recovery src/recovery.c src/functions.c`
## Running
To run enter:
`sudo ./recovery /dev/sdX# filename.mkv root_directory_inode_number`

For example:
  `sudo ./recovery /dev/sda1 lost_file.mkv 643`
## Issues
Populating new inode not 100% success rate
