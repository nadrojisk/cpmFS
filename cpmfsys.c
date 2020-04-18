#include "cpmfsys.h"

int freelist[256] = {1, 0};
#define UNUSED 0xe5
#define MAX_EXTENTS 32

char **split_name(char *name);
char **strip_name(DirStructType *dir);

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

    blob += index * 32;
    int blob_offset = 0;

    // copy status over
    memcpy(&dir->status, blob + blob_offset, sizeof(dir->status));
    blob_offset += sizeof(dir->status);

    // copy the strings seperately as they need to bring over the null byte
    // this null byte is not stored on disk... for some reason
    memcpy(&dir->name, blob + blob_offset, sizeof(dir->name) - 1);
    blob_offset += sizeof(dir->name) - 1;
    memcpy(&dir->extension, blob + blob_offset, sizeof(dir->extension) - 1);
    blob_offset += sizeof(dir->extension) - 1;

    // bring over rest of dir
    memcpy(&dir->XL, blob + blob_offset, sizeof(DirStructType) - blob_offset);

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
        char **names = split_name(name);
        char *filename = names[0];
        char *extension = names[1];

        for (int i = 0; i < MAX_EXTENTS; i++)
        {
            DirStructType *dir = mkDirStruct(i, block0);
            if (dir->status != UNUSED)
            {
                // strip filename and extension of spaces
                char **name = strip_name(dir);

                if (strcmp(name[0], filename) == 0 && strcmp(name[1], extension) == 0)
                {
                    return i;
                }
            }
        }

        return -1;
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

// split name into filename and extension
char **split_name(char *name)
{
    char str[80];
    strcpy(str, name);

    // split name from extension
    char *filename = strtok(str, ".");
    char *extension = strtok(NULL, ".");

    char **output = malloc(16);
    output[0] = malloc(9);
    output[1] = malloc(4);
    strcpy(output[0], filename);

    // extensions can be null, so if it is dont copy it over
    if (extension != NULL)
    {

        strcpy(output[1], extension);
    }
    else
    {
        strcpy(output[1], "");
    }
    return output;
}

// as we created the split names with malloc we need to free
// the data when we are done with it
void free_names(char **names)
{
    free(names[0]);
    free(names[1]);
    free(names);
}

// Strips filename and extension of spaces
char **strip_name(DirStructType *dir)
{
    char name[9];
    char extension[4];
    strcpy(name, dir->name);
    strcpy(extension, dir->extension);

    // split name from extension
    char *stripped_name = strtok(name, " ");
    char *stripped_extension = strtok(extension, " ");

    char **output = malloc(16);
    output[0] = malloc(9);
    output[1] = malloc(4);
    strcpy(output[0], stripped_name);

    // extensions can be null, so if it is dont copy it over
    if (stripped_extension != NULL)
    {

        strcpy(output[1], stripped_extension);
    }
    else
    {
        strcpy(output[1], "");
    }
    return output;
}

// internal function, returns true for legal name (8.3 format), false for illegal
// (name or extension too long, name blank, or  illegal characters in name or extension)
bool checkLegalName(char *name)
{
    bool return_code;
    // name cannot be empty
    if (strcmp(name, "") == 0)
    {
        return_code = false;
    }
    else
    {
        char **names = split_name(name);
        char *filename = names[0];
        char *extension = names[1];

        // incorrect filename length
        if (strlen(filename) > 8)
        {
            return_code = false;
        }
        // incorrect extension length
        else if (strlen(extension) > 3)
        {
            return_code = false;
        }

        // illegal first character
        else if (illegalstart(filename))
        {
            return_code = false;
        }

        // if extension isnt empty check for legal first character
        else if ((strcmp(extension, "") != 0) && illegalstart(extension))
        {
            return_code = false;
        }

        // legal name
        else
        {
            return_code = true;
        }
        free_names(names);
    }

    return return_code;
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
                if (dir->blocks[j] == 0)
                {
                    break;
                }
                full_blocks++;
            }

            full_blocks--;
            int length = (dir->RC * 128) + dir->BC + (full_blocks * 1024);
            char **names = strip_name(dir);
            char filename[14];
            sprintf(filename, "%s.%s", names[0], names[1]);
            printf("%-12s %d\n", filename, length);
            free_names(names);
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
    int return_code;
    if (checkLegalName(newName) == 0)
    { // invalid filename2
        printf("%s is an invalid filename\n", newName);
        return_code = -2;
    }
    else if (findExtentWithName(newName, NULL) != -1)
    { // dest exists
        printf("%s already exists\n", newName);
        return_code = -3;
    }
    else
    {
        uint8_t block;
        int location = findExtentWithName(oldName, NULL);
        if (location != -1)
        {
            DirStructType *dir = mkDirStruct(location, NULL);
            strcpy(dir->name, newName);
            writeDirStruct(dir, block, NULL);
            return_code = 0;
        }
        else
        {
            printf("%s does not exist\n", oldName);
            return_code = -1;
        }
    }

    return return_code;
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
