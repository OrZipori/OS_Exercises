/*
 * Student name : Or Zipori
 * Student : 302933833
 * Course Exercise Group : 03
 * Exercise Name : ex1
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

typedef enum BOOLEAN {FALSE = 0, TRUE} BOOLEAN;

/*******************************************************************************
* function name : checkForEquality
* input : file1, file2.
* output : true if files are equal.
* explanation : check if both file share equal content
*******************************************************************************/
BOOLEAN checkForEquality(int file1, int file2) {
    char *c1, *c2; // buffers of size 1
    ssize_t size1, size2;
    c1 = (char *) malloc(1);
    c2 = (char *) malloc(1);

    if (c1 == NULL || c2 == NULL) {
        perror("Malloc failure");
        exit(1);
    }

    // move byte byte and check characters
    while (TRUE) {
        size1 = read(file1, c1, 1);
        size2 = read(file2, c2, 1);

        // either EOF or error.
        if (size1 != 1 || size2!= 1)
        {
            break;
        }

        if (*c1 != *c2) {
            free(c1);
            free(c2);
            return FALSE;
        }
    }

    // no need for them anymore
    free(c1);
    free(c2);

    // check whether the loop ended cause of an error
    if (size1 < 0 || size2 < 0)
    {
        perror("Error reading");
        exit(1);
    }

    // both files are equal
    if (size1 == size2)
    {
        return TRUE;
    } else { // if not equal, one has EOF and the second hasn't.
        return FALSE;
    }
}

/*******************************************************************************
* function name : checkForSimilarity
* input : file1, file2.
* output : true if files are similar.
* explanation : check if both file share similar content
*******************************************************************************/
BOOLEAN checkForSimilarity(int file1, int file2) {
    char *c1, *c2; // buffers of size 1
    ssize_t size1, size2;
    c1 = (char *) malloc(1);
    c2 = (char *) malloc(1);
    BOOLEAN file1Stop = FALSE, file2Stop = FALSE;

    if (c1 == NULL || c2 == NULL) {
        perror("Malloc failure");
        exit(1);
    }

    // reset pointer position to the begining of the files
    lseek(file1, 0, SEEK_SET);
    lseek(file2, 0, SEEK_SET);

    while (TRUE) {
        // move to the next sign
        while ((size1 = read(file1, c1, 1)) == 1) {
            if (*c1 != ' ' && *c1 != '\n') {
                break;
            }
        }

        // move to the next sign
        while ((size2 = read(file2, c2, 1)) == 1) {
            if (*c2 != ' ' && *c2 != '\n') {
                break;
            }
        }

        // error encountered
        if (size1 == -1 || size2 == -1) {
            perror("Error reading");
            free(c1);
            free(c2);
            exit(1);
        }

        // because for similarity we need to check until both files are EOF
        if (size1 == 0 && size2 == 0) {
            break;
        }

        // no similarity
        if (toupper(*c1) != toupper(*c2)) {
            free(c1);
            free(c2);
            return FALSE;
        }
    }

    // can be similar only if both ended at the same time
    return TRUE;
}

/*******************************************************************************
* function name : closeFiles
* input : file1, file2.
* output : -
* explanation : close file discriptors
*******************************************************************************/
void closeFiles(int file1, int file2) {
   if (close(file1) == -1) {
       perror("Error closing");
       exit(1);
   }
   if (close(file2) == -1) {
       perror("Error closing");
       exit(1);
   }
}

/*******************************************************************************
* function name : main
* input : argc, argv.
* output : 1 if files are equal, 2 if similar 3 if not similar
* explanation : main function
*******************************************************************************/
int main(int argc, char** argv)
{
    // are identical yes -> 1
    // if not, are alike yes -> 2
    // else 3

    int file1, file2;

    // Validation - number of inputs is correct
    if (argc < 3) {
        perror("Not enough args");
        exit(1);
    }

    // open files and enlist files in the descriptors table
    file1 = open(argv[1], O_RDONLY);
    file2 = open(argv[2], O_RDONLY);

    if (file1 < 0 || file2 < 0) {
        perror("Error opening files");
        exit(1);
    }

    // if the files are identical
    if (checkForEquality(file1, file2)) {
        closeFiles(file1, file2);
        return 1;
    }

    // if the files are not identical, check for similarity
    if (checkForSimilarity(file1, file2)) {
        closeFiles(file1, file2);
        return 2;
    }

    // not similar
    closeFiles(file1, file2);
    return 3;
}
