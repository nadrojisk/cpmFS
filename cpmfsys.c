#include "cpmfsys.h"

int freelist[256] = {1, 0};
#define UNUSED 0xe5
#define MAX_EXTENTS 32

//function to allocate memory for a DirStructType (see above), and populate it, given a
//pointer to a buffer of memory holding the contents of disk block 0 (e), and an integer index
// which tells which extent from block zero (extent numbers start with 0) to use to make the
// DirStructType value to return.
DirStructType *mkDirStruct(int index, uint8_t *e)
{
    DirStructType *dir = malloc(sizeof(DirStructType));

    // read blob from disk
    uint8_t *blob = calloc(1, 1024);
    blockRead(blob, 0);

    // convert to DirStruct
    // each extend is 32 bytes
    memcpy(&dir->status, blob + (index * 32), sizeof(dir->status));
    memcpy(&dir->name, blob + (index * 32) + sizeof(dir->status), sizeof(dir->name) - 1);
    memcpy(&dir->extension, blob + (index * 32) + sizeof(dir->status) + sizeof(dir->name) - 1, sizeof(dir->extension) - 1);
    memcpy(&dir->XL, blob + (index * 32) + sizeof(dir->status) + sizeof(dir->name) - 1 + sizeof(dir->extension) - 1, sizeof(DirStructType) - (sizeof(dir->status) + sizeof(dir->name) - 1 + sizeof(dir->extension) - 1));

    return dir;
}

// function to write contents of a DirStructType struct back to the specified index of the extent
// in block of memory (disk block 0) pointed to by e
void writeDirStruct(DirStructType *d, uint8_t index, uint8_t *e) {}

// populate the FreeList global data structure. freeList[i] == true means
// that block i of the disk is free. block zero is never free, since it holds
// the directory. freeList[i] == false means the block is in use.
void makeFreeList()
{
    // clear freelist, skip first block as it is dir
    for (int i = 1; i < 256; i++)
    {
        freelist[i] = 0;
    }

    // recalculate free list
    for (int i = 0; i < MAX_EXTENTS; i++)
    {
        DirStructType *dir = mkDirStruct(i, NULL);
        if (dir->status != UNUSED) // unused blocks are free'd -- dont check
        {
            for (int i = 0; i < 16; i++)
            {
                int index = dir->blocks[i];
                if (index != 0)
                {
                    freelist[index] = 1;
                }
            }
        }
    }
}

// debugging function, print out the contents of the free list in 16 rows of 16, with each
// row prefixed by the 2-digit hex address of the first block in that row. Denote a used
// block with a *, a free block with a .
void printFreeList()
{
    makeFreeList();
    char output;
    printf("FREE BLOCK LIST: (* means in-use)\n");
    for (int i = 0; i < 256; i++)
    {
        if (i % 16 == 0)
        {
            printf("%2x: ", i);
        }
        if (freelist[i] == 1)
        {
            output = '*';
        }
        else
        {
            output = '.';
        }

        printf("%c ", output);
        if ((i + 1) % 16 == 0)
        {
            printf("\n");
        }
    }
}

// internal function, returns -1 for illegal name or name not found
// otherwise returns extent number 0-31
int findExtentWithName(char *name, uint8_t *block0)
{
    if (checkLegalName(name) == true)
    {
        int extent_index;
        for (int i = 0; i < MAX_EXTENTS; i++)
        {
            DirStructType *dir = mkDirStruct(i, block0);
            if (dir->status != UNUSED)
            {
                if (strcmp(dir->name, name) == 0)
                {
                    return i;
                }
            }
        }
        return extent_index;
    }
    else
    {
        return -1;
    }
}
// checks to see if the first character is A-Z a-z or 0-9
bool illegalstart(char *name)
{
    int first = (int)name[0];

    if (first < 0x30 || (first > 0x39 && first < 0x41) || (first > 0x5a && first < 0x61) || first > 0x7a)
    {
        return true;
    }

    return false;
}
// internal function, returns true for legal name (8.3 format), false for illegal
// (name or extension too long, name blank, or  illegal characters in name or extension)
bool checkLegalName(char *name)
{
    // cannot be empty
    if (strcmp(name, "") == 0)
    {
        return false;
    }

    // incorrect length
    else if (strlen(name) != 8)
    {
        return false;
    }

    // illegal first character
    else if (illegalstart(name))
    {
        return false;
    }

    else
    {
        return true;
    }
}

// print the file directory to stdout. Each filename should be printed on its own line,
// with the file size, in base 10, following the name and extension, with one space between
// the extension and the size. If a file does not have an extension it is acceptable to print
// the dot anyway, e.g. "myfile. 234" would indicate a file whose name was myfile, with no
// extension and a size of 234 bytes. This function returns no error codes, since it should
// never fail unless something is seriously wrong with the disk
void cpmDir()
{
    printf("DIRECTORY LISTING\n");
    for (int i = 0; i < MAX_EXTENTS; i++)
    {
        DirStructType *dir = mkDirStruct(i, NULL);
        if (dir->status != UNUSED)
        {
            // get dir length
            int full_blocks = 0;

            for (int j = 0; j < 16; j++)
            {
                // looks ahead by one, the last block size is calculated by RC & BC
                if (dir->blocks[j + 1] == 0)
                {
                    break;
                }
                full_blocks++;
            }
            int length = dir->RC * 128 + dir->BC + full_blocks * 1024;

            printf("%s.%s %d\n", dir->name, dir->extension, length);
        }
    }
}

// error codes for next five functions (not all errors apply to all 5 functions)
/*
    0 -- normal completion
   -1 -- source file not found
   -2 -- invalid  filename2
   -3 -- dest filename already exists
   -4 -- insufficient disk space
*/

//read directory block,
// modify the extent for file named oldName with newName, and write to the disk
int cpmRename(char *oldName, char *newName)
{

    if (checkLegalName(newName) == 0)
    { // invalid filename2
        printf("%s is an invalid filename\n", newName);
        return -2;
    }
    else if (findExtentWithName(newName, NULL) != -1)
    { // dest exists
        printf("%s already exists\n", newName);
        return -3;
    }

    uint8_t block;
    int location = findExtentWithName(oldName, &block);
    if (location != -1)
    {
        DirStructType *dir = mkDirStruct(location, NULL);
        strcpy(dir->name, newName);
        writeDirStruct(dir, block, NULL);
        return 0;
    }
    else
    {
        printf("%s does not exist\n", oldName);
        return -1;
    }
}

// delete the file named name, and free its disk blocks in the free list
int cpmDelete(char *name)
{
    if (checkLegalName(name) == 0)
    { // invalid filename2
        printf("%s is an invalid filename\n", name);
        return -2;
    }

    uint8_t block;
    int location = findExtentWithName(name, &block);
    if (location != -1)
    {
        DirStructType *dir = mkDirStruct(location, NULL);
        dir->status = UNUSED; // set status to unused -> deleted
        // if we skip unused blocks in the free list these will not be listed
        // saves time as we do not have to clear out the data, the data will
        // simply be cleared when new blocks arrive

        writeDirStruct(dir, block, NULL);
        free(dir);
        return 0;
    }
    else
    {
        printf("%s does not exist\n", name);
        return -1;
    }
}
