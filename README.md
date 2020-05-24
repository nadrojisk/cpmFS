# cpmFS

## Purpose

This repository is for Project 4 for the Advanced Operating Systems class offered at Auburn Univeristy (Spring 2020) by Dr. Xiao Qin.

## Summary

In this project we were required to design and implement features for the CP/M file system. 
I was tasked with implementing the following functionalities: `mkDirStruct`, `writeDirStruct`, `makeFreeList`, `printFreeList`, `checkLegalName`, `findExtentWithName`, `cpmDir`, `cpmDelete`, and `cpmRename`.

## Structure

For this project I was provided a few files: `Makefile`, `image1.img`, `fsysdriver.c` `sampleOutput.txt`, `diskSimulator.c/h`, and
`cpmfsys.h`
`cpmfsys.c` was to be filled out by the student.
I also have directory named `assets` this contains specifications and other documents provided by the professor, images, and my final report.

## Compilation

To compile this project run `make` while in the main directory. 
Most \*NIX systems that have the gcc tooltain should be able to compile this without any issue.
After the project is compiled you can run `./cpmRun` and it will print out some data that should match `sampleOutput.txt` located in `./assets/res`
