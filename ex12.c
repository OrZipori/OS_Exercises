/*
 * Student name : Or Zipori
 * Student : 302933833
 * Course Exercise Group : 03
 * Exercise Name : ex1
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>

#define SIZE_MAX_MB 1000
#define MAX_REASONS 2
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef enum BOOLEAN {FALSE = 0, TRUE} BOOLEAN;
typedef enum EVALUATION {
    NO_EVAL = 0,
    NO_C_FILE,
    COMPILATION_ERROR,
    FILE_COMPILED,
    TIMEOUT,
    BAD_OUTPUT,
    SIMILAR_OUTPUT,
    MULTIPLE_DIRECTORIES,
    GREAT_JOB,
    FINISHED_RUNNING_ON_TIME,
} EVALUATION;

typedef struct {
    EVALUATION evalArr[MAX_REASONS];
    char index;
} EVAL_ADT ;

/*******************************************************************************
* function name : exitWithError
* input : message.
* output : -
* explanation : write to stderr the message and exit with code 1
*******************************************************************************/
void exitWithError(char *msg)
{
    perror(msg);
    exit(1);
}

/*******************************************************************************
* function name : parseConfigfile
* input : configDT, config[3][161] 2d array to represent config file.
* output : -
* explanation : parse the config file
*******************************************************************************/
void parseConfigfile(int configDT, char config[][161])
{
    int i;
    char c[1];
    ssize_t check;

    for (i = 0; i < 3; i++) {
        int j = 0; // length of each string

        while((check = read(configDT, c, 1)) == 1) {
            // error reading
            if (check < 0)
                exitWithError("Error reading");

            // we want to read until the \n
            if (*c == '\n') break;

            config[i][j] = *c;

            j++;
        }

        // to set an end to the string
        config[i][j] = '\0';
    }
}

/*******************************************************************************
* function name : isDir
* input : struct firent fileName
* output : true if it is a directory
* explanation : ignore directories with . and .. , and check to see if a file is a directory
*******************************************************************************/
BOOLEAN isDir(struct dirent* fileName) {
    if ((fileName->d_type == DT_DIR) &&
        (fileName->d_name[0] != '.') &&
        (fileName->d_name[1] != '.')) {
        return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
* function name : isCFile
* input : fileName.
* output : true if c file
* explanation : check if file is a c file
*******************************************************************************/
BOOLEAN isCFile(char *fileName) {
    int len = strlen(fileName);
    if ((len > 2) && (fileName[len - 2] == '.') && (fileName[len - 1] == 'c'))
        return TRUE;
    return FALSE;
}

/*******************************************************************************
* function name : compile
* input : c file.
* output : EVALUATION constant - if compiled FILE_COMPILED else COMPILATION_ERROR
* explanation : use fork and exec to run gcc and compile a c file
*******************************************************************************/
EVALUATION compile(char * cfile) {
    // create a child and make it compile
    int pid;
    int checker = 0;

    EVALUATION result = FILE_COMPILED;

    pid = fork();
    // child process
    if (pid == 0) {
        if (execlp("gcc", "gcc", cfile, NULL) < 0) {
            exitWithError("Error compiling");
        }
        exit(1);
    } else if (pid > 0) {
        // father process
        wait(&checker);

        // check to see if c file compiled
        if (WEXITSTATUS(checker) == 1) {
            result = COMPILATION_ERROR;
            //printf("compilation error : %s\n", cfile);
        }
    } else {
        exitWithError("Fork failed");
    }

    return result;
}

/*******************************************************************************
* function name : run
* input : input file and student output file.
* output : EVALUATION constant - if program timed out TIMEOUT else FINISHED_RUNNING_ON_TIME
* explanation : use fork and exec to run student a.out and check for timeout
*******************************************************************************/
EVALUATION run(char *inputFile, char *stdOutFile) {
    int sOutFd, inputFd, pid, status;
    EVALUATION result;

    if ((sOutFd = open(stdOutFile, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0)
        exitWithError("error opening output");

    if ((inputFd = open(inputFile, O_RDONLY)) < 0)
        exitWithError("error opening input");

    pid = fork();

    // first child process
    if (pid == 0) {
        // making the program get input by input file
        if ((dup2(inputFd, 0)) == -1)
            exitWithError("Error dup2");

        // making the program print to the output file
        if ((dup2(sOutFd, 1)) == -1)
            exitWithError("Error dup2");

        if (execlp("./a.out", "./a.out", NULL) < 0) {
            exitWithError("error running student a.out");
        }
        exit(1);
    } else if (pid > 0) { // father process
        int i;
        BOOLEAN hasTimedOut = TRUE;

        for (i = 0; i < 5; i++) {
            sleep(1);
            if (waitpid(pid, &status, WNOHANG) == pid) {
                hasTimedOut = FALSE;
                break;
            }
        }

        if (!hasTimedOut)
            result = FINISHED_RUNNING_ON_TIME;
        else{
            // no need for it to continue running
            if (kill(pid, SIGTERM) == -1)
                exitWithError("kill failed");
            result = TIMEOUT;
        }


        // no need for the executable
        if (unlink("a.out") == -1)
            exitWithError("unlink error");

    } else {
        exitWithError("error with fork");
    }

    close(inputFd);
    close(sOutFd);

    return result;
}

/*******************************************************************************
* function name : compare
* input : current path, student output file and correct output file.
* output : EVALUATION constant - GREAT_JOB if equals, SIMILAR if similar and BAD_OUTPUT if not similar
* explanation : use fork and exec to run comp.out
*******************************************************************************/
EVALUATION compare(char *currentPath, char *stdOutFile, char *outputFile) {
    int pid;
    EVALUATION result;
    int status;

    pid = fork();

    // child process
    if (pid == 0) {

        // we go back to the path where comp.out is
        chdir(currentPath);

        if (execlp("./comp.out", "./comp.out", stdOutFile, outputFile, NULL) < 0) {
            exitWithError("Error running comp.out");
        }
        exit(1);
    } else if (pid > 0) { // father process
        // wait for comp.out to finish
        waitpid(pid, &status, 0);

        // if comp.out finished
        if (WIFEXITED(status)) {
            // check exit code
            switch (WEXITSTATUS(status)) {
                case 1: result = GREAT_JOB;
                    break;
                case 2: result = SIMILAR_OUTPUT;
                    break;
                case 3: result = BAD_OUTPUT;
                    break;
                default: break;
            }
        }
    } else {
        exitWithError("Error with fork");
    }

    return result;
}

/*******************************************************************************
* function name : evaluateStudentWork
* input : studentFolder, penalty, depth, stdOutFile, inputFile,
          outputFile, currentPath, eval
* output : -
* explanation : evaluate student work with the help of helper functions
*******************************************************************************/
void evaluateStudentWork(char* studentFolder, int* penalty, int depth, char* stdOutFile, char* inputFile,
                               char* outputFile, char* currentPath, EVAL_ADT *eval) {
    EVALUATION result = NO_C_FILE; // we didn't find a c file yet
    DIR* currStuDir;
    struct dirent * currStuDirent;
    char nextDir[SIZE_MAX_MB] = {0};
    char cFile[SIZE_MAX_MB] = {0};
    int numOfDirs = 0;

    // open student directory
    if ((currStuDir = opendir(studentFolder) ) == NULL )
        exitWithError("Error opening student directory");

    // make this folder working directory
    chdir(studentFolder);

    // iterate student directory - BFS
    while ((currStuDirent = readdir(currStuDir)) != NULL) {
        if (isDir(currStuDirent)) {
            // save the next dir
            strcpy(nextDir, currStuDirent->d_name);
            numOfDirs++;
        } else if (isCFile(currStuDirent->d_name)) {
            strcpy(cFile, currStuDirent->d_name);
        }
    }

    // multiple directories - stop checking
    if (numOfDirs > 1 && strlen(cFile) < 2) {
        chdir("..");
        eval->evalArr[0] = MULTIPLE_DIRECTORIES;
        return;
    }

    // we found a c file and in that level there were not multiple directories
    if (strlen(cFile) > 2) {
        // set penalty
        *penalty = depth;

        // compile the file and bring back results
        result = compile(cFile);
        if (result == COMPILATION_ERROR) {
            chdir("..");
            // c file was found but didn't compile
            eval->evalArr[0] = COMPILATION_ERROR;
            closedir(currStuDir);
            return;
        }

        // run student executable
        result = run(inputFile, stdOutFile);
        if (result == TIMEOUT) {
            chdir("..");

            // c file was found but the program timed out
            eval->evalArr[0] = TIMEOUT;

            closedir(currStuDir);
            return;
        }

        result = compare(currentPath, stdOutFile, outputFile);
        eval->evalArr[0] = result;

        chdir("..");
        closedir(currStuDir);
        return;
    }

    // go deeper to the next directory
    if (eval->evalArr[0] == NO_C_FILE && strlen(nextDir) > 0)
        evaluateStudentWork(nextDir, penalty, ++depth, stdOutFile, inputFile,
                                 outputFile, currentPath, eval);

    // go back to previous folder
    chdir("..");

    closedir(currStuDir);
}

/*******************************************************************************
* function name : getPenalty
* input : grade and penalty
* output : grade after dudction of penalty
* explanation : calculate grade with penalty
*******************************************************************************/
int getPenalty(int grade, int penalty) {
    int gradeWithPenalty = grade - (penalty * 10);

    return MAX(gradeWithPenalty, 0);
}

/*******************************************************************************
* function name : gradeStudent
* input : eval, penalty, resultsFD, stdName
* output : -
* explanation : write the correct grade and reasons for a student in reslts.csv
*******************************************************************************/
void gradeStudent(EVAL_ADT * eval, int penalty, int resultsFD, char *stdName) {
    char evaluations[SIZE_MAX_MB];

    // append the results
    lseek(resultsFD, 0, SEEK_END);
    switch (eval->evalArr[0]) {
        case NO_C_FILE: sprintf(evaluations, "%s,0,NO_C_FILE", stdName);
            break;
        case COMPILATION_ERROR: sprintf(evaluations, "%s,0,COMPILATION_ERROR", stdName);
            break;
        case TIMEOUT: sprintf(evaluations, "%s,0,TIMEOUT", stdName);
            break;
        case MULTIPLE_DIRECTORIES: sprintf(evaluations, "%s,0,MULTIPLE_DIRECTORIES", stdName);
            break;
        case BAD_OUTPUT: sprintf(evaluations, "%s,0,BAD_OUTPUT", stdName);
            break;
        case SIMILAR_OUTPUT: sprintf(evaluations, "%s,%d,SIMILAR_OUTPUT", stdName, getPenalty(70, penalty));
            if (penalty > 0)
                strcat(evaluations, ",WRONG_DIRECTORY");
            break;
        case GREAT_JOB: sprintf(evaluations, "%s,%d", stdName, getPenalty(100, penalty));
            if (penalty > 0)
                strcat(evaluations, ",WRONG_DIRECTORY");
            else
                strcat(evaluations, ",GREAT_JOB");
            break;
        default: sprintf(evaluations, "ERROR");
    }

    // need to make a new line
    strcat(evaluations, "\n");

    if (write(resultsFD, evaluations, strlen(evaluations)) < 0)
        exitWithError("Error writing");
}

/*******************************************************************************
* function name : main
* input : argc, argv.
* output : 0
* explanation : main function
*******************************************************************************/
int main(int argc, char** argv) {

    int configFiledt, resultsFD;
    char configArr[3][161];
    DIR* rootDir;
    struct dirent* studentDir;
    char currentPath[SIZE_MAX_MB] = {0}, tempOut[SIZE_MAX_MB] = {0};

    // validate args
    if (argc < 3)
        exitWithError("Not enough args");

    configFiledt = open(argv[1], O_RDONLY);

    if (configFiledt < 0)
        exitWithError("Error opening config file");

    parseConfigfile(configFiledt, &configArr);

    // create the results file at this position
    resultsFD = open("results.csv", O_TRUNC | O_CREAT | O_RDWR, 0666);
    if (resultsFD < 0)
        exitWithError("Error opening results.csv");

    // open the root file of all the students
    if ((rootDir = opendir(configArr[0])) == NULL)
        exitWithError("Error opening root dir");

    // get executable path and set it to the current path
    getcwd(currentPath, SIZE_MAX_MB);

    //the temp file for every student project output
    strcpy(tempOut, currentPath);
    strcat(tempOut, "/tempOut.txt");

    // change working directory to the root directory
    chdir(configArr[0]);

    // iterate over all users.
    while ((studentDir = readdir(rootDir)) != NULL) {
        // if the c file is not in the first directory
        int penalty = 0;
        EVAL_ADT results;

        results.evalArr[0] = NO_C_FILE;
        results.index = 0;

        if (isDir(studentDir)) {
            evaluateStudentWork(studentDir->d_name, &penalty, 0, tempOut, configArr[1], configArr[2],
                                          currentPath, &results);

            gradeStudent(&results, penalty, resultsFD, studentDir->d_name);
        }

    }

    // clean
    close(configFiledt);
    closedir(rootDir);
    close(resultsFD);

    unlink(tempOut);

    return 0;
}