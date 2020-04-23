---
title: "cpmFS - A Simple File System"
author: [Jordan Sosnowski]
date: "2020-04-24"
subject: "cpmFS"
keywords: [c, cpmFS, file system, operating systems]
lang: "en"
---

## Introduction

### Problem Description

File systems are an extremely important, yet often overlooked, part of an operating system.
Without them recovering reading and writing data from a disk would be near impossible. 
A good file system is just as important as a good graphical user interface.

### Background

TODO

## Design and Implementation

### Data Flow Diagram

TODO 

### Project Design

With this project most of the design was already prederminted by the professor.
The inital files were `cpmfsys.c`, `cpmfsys.h`, `diskSimulator.c`, `diskSimulator.h`, `fysdriver.c` and `image1.img`.
The task of the project was to fill in `cpmfsys.c` with the function prototypes declared in its header file.
`diskSimulator.c` provides the functionality with interacting with the simualted hardware and `fysdriver.c` is our driver function that contains main.
`image1.img` is a "disk" image provided by the professor to be used for debugging purposes within `fsysdriver.c`.

Within `cpmfsys.c` we were tasked with implementing nine functions: `mkDirStruct`, `writeDirStruct`, `makeFreeList`, `printFreeList`, `findExtendWithName`, `checkLegalName`, `cpmDir`, `cpmRename`, and `cpmDelete`.
Most of the functions are self explanatory but we will quickly discuss them.

#### mkDirStruct

Takes in an index and an address. The index is used to determine which extend to grab, and the address is used to determine the base address.
I did not see a use for the address parameter as for the functions we needed to implement only the first block of the disk was needed.
The first block is the *directory*.
This contains extent entries of 32 bytes long that contain metadata for the files within the file system.
Note: index is multiplied by 32 as each extent entry is 32 bytes long.

Therefore, we will need to grab the first block from the disk and offset it by index*32.
This offset will allow the program to grab the extent requested by the user.
Once the address is located that points to the extent information `memcopy` is used to copy the information from the disk into a dirStruct object.
this object is then returned so the function calling `mkDirStruct` can use it.

#### writeDirStruct

This function takes in a dirStruct object, an index, and an address. 
The dirStruct object needs to be written to the disk and the index tells the program which extent it is.
However, the function that interacts with the hardware can only write 1024 bytes and each extent is 32 bytes long.
Therefore, we will need to grab all the extents in the directory block and write them all back together with the modified extent.

#### makeFreeList

This function will fill a global list to tell the program which blocks are in use or not in use.
First we will clear the existing free list and set everything to free.
After that the block 0, the directory, will be enumerated and `mkDirStruct` will create extent objects for each extent.
Each extent's block list will be enumerated to determine where the data is stored.
For example if extent zero is stored at block one, two, ten, and eleven then we will set freelist one, two, ten and eleven will be set to non-free.

#### printFreeList
 
 This function will enumerate through the global free list and print "*" if the block is in use or "." if it is not.
 Additionally, it prints it as a 16 by 16 block of "*"s or "."s for formatting purposes.
  
#### findExtentWithName

This function takes in a string and a block address this data is passed to mkDirStruct to return a directory object to be analyzed by this function.
After the directory object is returned this function will check to see if the passed string matches the directories filename and extension.

#### checkLegalName

Takes in a string and checks are performed on it to determine if it is "legal".
A file's name can only be 8 bytes long max and its extension can only be 3 bytes max.
The first character of the file name and the extension has to be either a-z, A-Z, or 0-9. 
So for example "-file.txt" is an illegal filename.

#### cpmDir

This function lists all the files on the file system.
Note, this will not print out deleted files as they are no longer active.
The filename and the length for each file / extent are listed when this function is called.

#### cpmRename

This function takes in two strings: `oldName` and `newName`. 
It will try to find an extent that matches with the `oldName` if it is found the name is set to `newName`.
After the name is set for the local instance it is written back to disk with `writeDirStruct` for persistence.
However, if the extent is not found or the `newName` is not legaly this function will return an error code.

#### cpmDelete

This function takes in a string that acts as the filename for the extent to delete.
The extent is located with `findExtentWithName` and the directory object is pulled with `mkDirStruct`.
The status of this directory is set to 0xE5 (unused) and the directory object is written back to disk.
  
## Lessons Learned

## Conclusion

## References 
