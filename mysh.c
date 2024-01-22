#include <unistd.h>     // Unix standard library
#include <stdlib.h>     // C standard library
#include <stdio.h>      // Standard input and output
#include <errno.h>      // Sets and retreives errors

#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <glob.h>

#define BUFFSIZE 5012

char* line;
char** tokens;
char** arguments;

int MAX_TOKENS;
int MAX_ARGUMENTS;
int exit_status = -1;

void startup() {

    printf("\n");   
    printf("╔══════════ ≪ * ≫ ══════════╗ \n\n"); 
    printf("     Welcome to myshell!       \n\n");
    printf("   ⚠️ Use at your own risk ⚠️    \n\n");
    printf("╚══════════ ≪ * ≫ ══════════╝ \n\n");

}

/* ============================================================ */
// Miscellaneous Helper Functions //

/* Returns 0 if the line contains a pipe, and 1 otherwise */
int hasPipe() {
    for ( int i = 0; i < strlen(line); i++ ) {
        if ( line[i] == '|' ) {
            return 0;
        }
    }
    return 1;
}

/* Returns 0 if the line contains a caret, and 1 otherwise */
int hasCaret() {
    for ( int i = 0; i < strlen(line); i++ ) {
        if ( line[i] == '<' || line[i] == '>' ) {
            return 0;
        }
    }
    return 1;
}

/* Returns 0 if the the token contains a slash, and 1 otherwise */
int hasSlash(char* token) {
    for ( int i = 0; i < strlen(token); i++ ) {
        if ( token[i] == '/' )
        return 0;
    }
    return 1;
} 

int caretCounter() {
    int count = 0;
    for ( int i = 0; i < MAX_TOKENS; i++ ) {
        if ( strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "<") == 0 )
        count++;
    }
    return count;
}

int pipeCounter() {
    int count = 0;
    for ( int i = 0; i < MAX_TOKENS; i++ ) {
        if ( strcmp(tokens[i], "|") == 0 )
        count++;
    }
    return count;
}

// Return 1 if the input is just all spaces, and 0 if a letter or symbol is found
int ifAllSpaces() {
    for (int i = 0; i < strlen(line); i++) {
        if (isspace(line[i]) == 0) {
            return 0;
        }
    }
    return 1;
}

void countTokens() {
    char* token;
    int count = 0;

    char lineCopy[strlen(line) + 1];
    memcpy(lineCopy, line, strlen(line) + 1);

    // Get the first token
    token = strtok(lineCopy, " ");

    // Walk through other tokens
    while ( token != 0 ) {
        count++;
        token = strtok(0, " ");
    }

    MAX_TOKENS = count;
    return;
}

void addToArguments(char* file_match) {

    MAX_ARGUMENTS++;

    if ( arguments == NULL ) {   // If 'arguments' has not been initialized yet

        arguments = (char**)malloc(1 * sizeof(char *));
        arguments[0] = (char*)malloc((strlen(file_match) + 1) * sizeof(char));
        arguments[0][strlen(file_match)] = '\0';
        strcpy(arguments[0], file_match);

    } else {    // Otherwise, tack on the new argument to the end

        arguments = (char**)realloc(arguments, (MAX_ARGUMENTS * sizeof(char*)));
        arguments[MAX_ARGUMENTS - 1] = (char*)malloc((strlen(file_match) + 1) * sizeof(char));
        arguments[MAX_ARGUMENTS - 1][strlen(file_match)] = '\0';
        strcpy(arguments[MAX_ARGUMENTS - 1], file_match);
    }
}

/* Reset all global variables to free space for the next command line input */
void inputReset() {

    for (int i = 0; i < MAX_TOKENS; i++) {
        free(tokens[i]); // Set pointers to NULL or to initial values
    }
    free(tokens);

    MAX_ARGUMENTS = 0;
    MAX_TOKENS = 0;
    free(line);
}

int iExist(char* program) {
    char* directory;

    char cwd[5012];
    getcwd(cwd, sizeof(cwd));
    
    if (access(program, F_OK) == 0 ) return 0;

    directory = "/usr/local/bin";  chdir(directory);
    if (access(program, F_OK) == 0 ) { chdir(cwd); return 0; }

    directory = "/usr/bin";  chdir(directory);
    if (access(program, F_OK) == 0 ) { chdir(cwd); return 0; }

    directory = "/bin";  chdir(directory);
    if (access(program, F_OK) == 0 ) { chdir(cwd); return 0; }

    chdir(cwd);
    return 1;
}


/* ================================================================================ */
/* Conditional Functions */

int thenHandler() {

    if ( exit_status == -1 ) {
        printf("Error: Conditional used without a previous command\n");
        return 1;
    }
    if ( exit_status == 1 ) {
        printf("Error: Previous command failed\n");
        printf("Cannot execute 'then' conditional\n");
        return 1;
    }
    if ( MAX_TOKENS == 1 ) {
        printf("Error: Unexpected number of arguments\n");
        return 1;
    }

    // Otherwise 'exit_status' must be 0, which represents a previous success

    // Edit the actual line. Take away the conditional
    int startIndex = 0;
    int removeLength = 5;

    int newLength = strlen(line) - removeLength;
    memmove(&line[startIndex], &line[startIndex + removeLength], newLength - startIndex + 1);
    free(tokens[0]);

    // Shift elements to the left to remove the first element
    for (int i = 0; i < MAX_TOKENS - 1; i++) {
        tokens[i] = tokens[i + 1];
    }
    MAX_TOKENS--;

    /* Now that the conditional statement is gone, it will continue through the 
    Master Directory as normal */

    return 0;
}

int elseHandler() {

    if ( exit_status == -1 ) {
        printf("Error: Conditional used without a previous command\n");
        return 1;
    }
    if ( exit_status == 0 ) {
        printf("Error: Previous command succeeded\n");
        printf("Cannot execute 'else' conditional\n");
        return 1;
    }
    if ( MAX_TOKENS == 1 ) {
        printf("Error: Unexpected number of arguments\n");
        return 1;
    }

    // Otherwise 'exit_status' must be 1, which represents a previous failure

    // Edit the actual line. Take away the conditional
    int startIndex = 0;
    int removeLength = 4;

    int newLength = strlen(line) - removeLength;
    memmove(&line[startIndex], &line[startIndex + removeLength], newLength - startIndex + 1);
    free(tokens[0]);

    // Shift elements to the left to remove the first element
    for (int i = 0; i < MAX_TOKENS - 1; i++) {
        tokens[i] = tokens[i + 1];
    }
    MAX_TOKENS--;

    // Now that the conditional statement is gone, it will continue through the 
    // Master Directory as normal.

    return 0;
}


/* ============================================================ */
// Built-In Commands Section //

int pathBuilder3000(char* program, char* directory) {

    char* pathname;
    if ( access(program, F_OK) == 0 ) { // Build a file path and print it out
        pathname = malloc(strlen(directory) + sizeof(char) + strlen(program) + sizeof(char));
        memcpy(pathname, directory, strlen(directory));
        pathname[strlen(directory)] = '/';
        memcpy(pathname + strlen(directory) + 1, program, strlen(program) + 1);
        
        printf("%s\n", pathname);
        free(pathname);
        return 0;
    }
    return 1;
}

int whichCommand() {

    if ( MAX_TOKENS != 2 ) {
        printf("Error: Unexpected number of arguments! \n");
        printf("Usage: which <program name>\n");
        return 1;
    }

    // Isolate the second argument from "line", which would be the program name.
    char* program = tokens[1];

    // Preserve the current working directory
    char cwd[5012];
    getcwd(cwd, sizeof(cwd));
    char* directory;

    if ( strcmp(program, "cd") == 0 || strcmp(program, "pwd") == 0 || strcmp(program, "which") == 0) {
        printf("Error: Unexpected argument: \"%s\"\n", program);
        printf("Usage: which <program name>\n");
        return 1;
    }

    directory = "/usr/local/bin";  chdir(directory);
    if ( pathBuilder3000(program, directory) == 0 ) { chdir(cwd); return 0; }

    directory = "/usr/bin";  chdir(directory);
    if ( pathBuilder3000(program, directory) == 0 ) { chdir(cwd); return 0; }

    directory = "/bin";  chdir(directory);
    if ( pathBuilder3000(program, directory) == 0 ) { chdir(cwd); return 0; }

    // Go back to the current working directory
    chdir(cwd);

    return 0;
}

int changeDirectory() {

    if ( MAX_TOKENS != 2 ) {
        printf("Error: Unexpected number of arguments!\n");
        printf("Usage: cd <directory name>\n");
        return 1;
    }

    // Isolate the second argument from "line", which would be the path name.
    char* path = tokens[1];

    if ( chdir(path) != 0 ) {
        printf("Error: Directory does not exist\n");
        return 1;
    }

    printf("Directory changed successfully to %s\n", path);
    return 0;
}

int printCurrDirectory() {
    char cwd[5012];

    if ( getcwd(cwd, sizeof(cwd)) != NULL ) {
        printf("Current working directory: %s\n", cwd);
    } else {
        perror("getcwd() error");
        return 1;
    }

    return 0;
}


/* ============================================================ */
// Redirection and Piping Section //

char* executablePathBuilder(char* program, char* directory) {
    char* pathname;
    pathname = malloc(strlen(directory) + sizeof(char) + strlen(program) + sizeof(char));
    memcpy(pathname, directory, strlen(directory));
    pathname[strlen(directory)] = '/';
    memcpy(pathname + strlen(directory) + 1, program, strlen(program) + 1);
    return pathname;
}

/* If the executable in question does NOT exist in the cwd, we need to build the
full pathname so execv can run it! We will replace its place in 'tokens' with
the full pathname. Tedious, but has to be done. */
void pathNameReplacer(char* program, int arrayIndex) {
    char cwd[5012];
    getcwd(cwd, sizeof(cwd));
    char* path;

    int status = 1;
    char* directory = "/usr/local/bin";  chdir(directory);
    if ( access(program, F_OK) == 0 && status == 1 ) { 
        path = executablePathBuilder(program, directory);
        status = 0;
    }
    directory = "/usr/bin";  chdir(directory);
    if ( access(program, F_OK) == 0 && status == 1 ) { 
        path = executablePathBuilder(program, directory);
        status = 0;
    }
    directory = "/bin";  chdir(directory);
    if ( access(program, F_OK) == 0 && status == 1 ) { 
        path = executablePathBuilder(program, directory);
        status = 0;
    }

    tokens[arrayIndex] = realloc(tokens[arrayIndex], strlen(path) + 1);
    memcpy(tokens[arrayIndex], path, strlen(path) + 1);
    tokens[arrayIndex][strlen(path)] = '\0';
    free(path);

    chdir(cwd);
}

/* We need to find the index of the executable in relation to the redirection symbol.
How many arguments are in between the executable and the redirection symbol? This function finds out. */
int getExecutableIndex(int caretIndex) {

    for ( int index = caretIndex - 1; index >= 0; index-- ) {
        if ( caretIndex - 1 == 0 ) {
            return 0;
        }
        if ( strcmp(tokens[index], "<") == 0 || strcmp(tokens[index], ">") == 0 
        || strcmp(tokens[index], "|") == 0 ) {
            return index + 1;
        }
    }
    // This will never trigger because at this point, there is always something before the caret
    return 0;
}

/* This function creates a custom argument list that's unique to each caret symbol, but
also formats the list correctly for execv(). The list is stored in the global array 'arguments',
and is reset after the redirection for its corresponding caret is complete */
void customArgumentList(int caretIndex) {

    // Use the index of the executable and start adding arguments after that
    for ( int index = getExecutableIndex(caretIndex); index < caretIndex; index++ ) {
        addToArguments(tokens[index]);
    }
    
    // If the 'constant' filename is the last token, we don't have any more arguments to add
    if ( caretIndex + 1 != MAX_TOKENS - 1 ) {

        // Now add any arguments after the file name
        for ( int index = caretIndex + 2; index < MAX_TOKENS; index++ ) {
            if ( strcmp(tokens[index], "<") == 0 || strcmp(tokens[index], ">") == 0 
                || strcmp(tokens[index], "|") == 0 ) break;
            addToArguments(tokens[index]);
        }
    }

    // Finally, add the null pointer at the last index to make execv() happy
    MAX_ARGUMENTS++;
    arguments = (char**)realloc(arguments, (MAX_ARGUMENTS * sizeof(char*)));
    arguments[MAX_ARGUMENTS - 1] = NULL;

    return;
}

int redirection(char* executable, char* output_file) {

    pid_t pid = fork();
    if ( pid == -1 ) {
        printf("Error forking\n");
        return 1;
    } else if ( pid == 0 ) { // We are in the child
        if ( execv(executable, arguments) == 1 ) {
            perror("execv");
            return 1;
        }
    } else { // Parent process
        int wstatus;
        wait(&wstatus);
    }

    return 0;
}   

/* Here we are just setting things up for redirection, handling any obvious errors,
creating argument lists, y'know how it is */
int redirectionWrapper(int caretIndex) {
    
    // A caret symbol cannot be the first or last token
    if ( strcmp(tokens[MAX_TOKENS - 1], "<") == 0 || strcmp(tokens[MAX_TOKENS - 1], ">") == 0 || 
         strcmp(tokens[0], "<") == 0 || strcmp(tokens[0], ">") == 0 ) {
         printf("Error: Improper use of redirection symbol\n");
         return 1;
    }

    // Caret symbols cannot be adjacent to each other
    for ( int i = 0; i < MAX_TOKENS; i++ ) {
        if ( strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 ) {
            if ( (( i != 0 ) && ( i != MAX_TOKENS - 1))
            && ( strcmp(tokens[i - 1], "<") == 0 || strcmp(tokens[i - 1], ">") == 0 
            ||   strcmp(tokens[i + 1], "<") == 0 || strcmp(tokens[i + 1], ">") == 0 ) ) {
                printf("Error: Improper use of redirection symbol\n");
                return 1;
            }
        }
    }

    if ( iExist(tokens[getExecutableIndex(caretIndex)]) == 1 ) {
        printf("Error: executable does not exist\n");
        return 1;
    }

    /* If the executable in question does NOT exist in the cwd, we need to build the
    full pathname so execv can run it! We will replace its place in 'tokens' with
    the full pathname */
    if ( access(tokens[getExecutableIndex(caretIndex)], F_OK) != 0 ) 
    pathNameReplacer(tokens[getExecutableIndex(caretIndex)], getExecutableIndex(caretIndex));

    char* caret = tokens[caretIndex];

    /* ================================================================ */
    // The real redirection starts here

    if ( strcmp(caret, ">") == 0 ) { // We want to change STDOUT

        char* executable = tokens[getExecutableIndex(caretIndex)];     
        char* output_file = tokens[caretIndex + 1];   
        int original_stdout = dup(STDOUT_FILENO);

        customArgumentList(caretIndex); // Generate an argument list custom to this particular caret
        
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if ( fd == -1 ) {
            perror("open");
            return 1;
        }
        dup2(fd, STDOUT_FILENO); // All output will now be directed to 'output_file'
        close(fd);

        // We just have to pass in the program to execute and the argument list
        if ( redirection(executable, output_file) == 1 ) return 1; 

        if (dup2(original_stdout, STDOUT_FILENO) == -1) {
            perror("Failed to reset STDOUT to terminal");
            exit(EXIT_FAILURE);
        }

    } else if ( strcmp(caret, "<") == 0 ) { // We want to change STDIN

        char* executable = tokens[getExecutableIndex(caretIndex)];   
        char* input_file = tokens[caretIndex + 1];  
        int original_stdout = dup(STDIN_FILENO);

        customArgumentList(caretIndex); // Generate an argument list custom to this particular caret

        int fd = open(input_file, O_RDONLY, 0640);
        if ( fd == -1 ) {
            perror("open");
            return 1;
        }
        dup2(fd, STDIN_FILENO); // All output will now be directed to 'output_file'
        close(fd);

        // We just have to pass in the program to execute and the argument list
        if ( redirection(executable, input_file) == 1 ) return 1; 

        if (dup2(original_stdout, STDIN_FILENO) == -1) {
            perror("Failed to reset STDIN to terminal");
            exit(EXIT_FAILURE);
        }
    }

    /* Free the argument list so a different argument list can be created if a different
    caret symbol is found */
    for (int i = 0; i < MAX_ARGUMENTS; i++) {
        free(arguments[i]); // Set pointers to NULL or to initial values
        arguments[i] = NULL;
    }
    free(arguments);
    arguments = NULL;
    MAX_ARGUMENTS = 0;
    
    return 0;
}

int pipeBuddies(char** args1, char** args2) {

    int pipefd[2];
    pid_t pid1, pid2 = 0;

    if ( pipe(pipefd) == -1 ) {
        perror("pipe");
        return 1;
    }

    pid1 = fork(); // Create the first child process
    if ( pid1 == -1 ) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if ( pid1 == 0 ) { 
        // Child process 1 - Program 1 (writes to the pipe
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);

        // Execute the first program (e.g., ls to list files)
        if ( execv(args1[0], args1) ) {
            perror("execv");
            return 1;
        }
    } else { // Back in the parent process
        
        pid2 = fork(); // Create the second child process
        if ( pid2 == -1 ) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if ( pid2 == 0 ) {
            // Child process 2 - Program 2 (reads from the pipe)
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);

            // Execute the second program (e.g., wc -l to count lines)
            if ( execv(args2[0], args2) ) {
                perror("execv");
                return 1;
            }
        }
    }
    close(pipefd[0]);
    close(pipefd[1]);

    // Wait for both child processes to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}

int pipeWrapper(int arrayIndex) {
    
    // Only one pipe symbol permitted
    if ( pipeCounter() > 1 ) {
        printf("Error: Improper use of pipe command\n");
        return 1;
    }

    // Pipes cannot be the first or last token
    if ( strcmp(tokens[0], "|") == 0 || strcmp(tokens[MAX_TOKENS - 1], "|") == 0 ) {
        printf("Error: Improper use of pipe command\n");
        return 1;
    }

    // Pipes and redirects cannot be adjacent to each other
    if ( strcmp(tokens[arrayIndex + 1], "|") == 0 || strcmp(tokens[arrayIndex - 1], "|") == 0 
    || strcmp(tokens[arrayIndex + 1], "<") == 0 || strcmp(tokens[arrayIndex - 1], "<") == 0 
    || strcmp(tokens[arrayIndex + 1], ">") == 0 || strcmp(tokens[arrayIndex - 1], ">") == 0 ) {
        printf("Error: Improper use of pipe command\n");
        return 1;
    }

    // Make sure the executables exist somewhere
    char* first_executable = tokens[getExecutableIndex(arrayIndex)];
    char* second_executable = tokens[arrayIndex + 1];

    if ( iExist(first_executable) == 1 ) {
        printf("Error: executable does not exist\n");
        return 1;
    }
    if ( iExist(second_executable) == 1 ) {
        printf("Error: executable does not exist\n");
        return 1;
    }
    if ( access(first_executable, F_OK) != 0 ) {
        pathNameReplacer(first_executable, getExecutableIndex(arrayIndex));
    }
    if ( access(second_executable, F_OK) != 0 ) {
        pathNameReplacer(second_executable, arrayIndex + 1);
    }
    
    // Create two custom argument lists for the pipe in question
    customArgumentList(arrayIndex);
    char** args1 = (char**)malloc(MAX_ARGUMENTS * sizeof(char **));

    for (int i = 0; i < MAX_ARGUMENTS - 1; ++i) {
        args1[i] = strdup(arguments[i]);
        args1[i][strlen(arguments[i])] = '\0';
        free(arguments[i]);
        arguments[i] = NULL;
    }
    free(arguments);
    arguments = NULL;
    args1[MAX_ARGUMENTS - 1] = NULL;

    // Make second list
    int length = 0;
    for (int i = arrayIndex + 1; i < MAX_TOKENS; i++) {
        if ( strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 ) break;
        length++;
    }
    char** args2 = (char**)malloc((length + 1) * sizeof(char**));
    int index = 0;
    for (int i = arrayIndex + 1; index < length; i++) {
        args2[index] = strdup(tokens[i]);
        args2[index][strlen(tokens[i])] = '\0';
        index++;
    }
    
    args2[length] = NULL;      
    length++;

    // Let's do the pipe thing
    if ( pipeBuddies(args1, args2) == 1 ) return 1;

    for (int i = 0; i < MAX_ARGUMENTS; i++) { 
        free(args1[i]);
        args1[i] = NULL;
    }
    free(args1);
    args1 = NULL;

    for (int i = 0; i < length; i++) {
        free(args2[i]);
        args2[i] = NULL;
    }
    free(args2);
    args2 = NULL;
    
    return 0;
}

int caretPipeSwitch() {

    for ( int i = 0; i < MAX_TOKENS; i++ ) {
        if ( strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 ) {
            if ( redirectionWrapper(i) == 1 ) return 1;
        }

        if ( strcmp(tokens[i], "|") == 0 ) {
            if ( pipeWrapper(i) == 1 ) return 1;
        }
    }
    // This will never trigger, so we return 0 for fun.
    return 0;
}


/* ============================================================ */
// File Execution Section //

void insertPathName(char* path, int arrayIndex) {
    arguments[arrayIndex] = realloc(arguments[arrayIndex], strlen(path) + 1);
    memcpy(arguments[arrayIndex], path, strlen(path) + 1);
    arguments[arrayIndex][strlen(path)] = '\0';
}

int executeProgram(char* program) {

    pid_t pid = fork();
    if ( pid == -1 ) {
        perror("fork");
        return 1;
    } else if ( pid == 0 ) {
        if ( execv(arguments[0], arguments) == -1 ) {
            perror("execv");
            return 1;  // Exit child process if execv fails
        }
    } else wait(NULL); // Parent process waits for the child process to finish
    return 0;
}

int executeProgramWrapper() {

    char* directory;
    char* program = tokens[0];

    char cwd[5012];
    getcwd(cwd, sizeof(cwd));

    for ( int i = 0; i < MAX_TOKENS; i++ ) {
        addToArguments(tokens[i]);
    }

    // Add a NULL terminator to the end of arguments
    MAX_ARGUMENTS++;
    arguments = (char**)realloc(arguments, (MAX_ARGUMENTS * sizeof(char*)));
    arguments[MAX_ARGUMENTS - 1] = NULL;

    
    for ( int i = 1; i < MAX_ARGUMENTS - 1; i++ ) {

        if ( strcmp(program, "echo") == 0 ) break;

        directory = "/usr/local/bin";  chdir(directory);
        if (access(arguments[i], F_OK) == 0) {
            insertPathName( executablePathBuilder(arguments[i], directory), i );
            break;
        }
        directory = "/usr/bin";  chdir(directory);
        if (access(arguments[i], F_OK) == 0) {
            insertPathName( executablePathBuilder(arguments[i], directory), i );
            break;
        }
        directory = "/bin";  chdir(directory);
        if (access(arguments[i], F_OK) == 0) {
            insertPathName( executablePathBuilder(arguments[i], directory), i );
            break;
        }       
        chdir(cwd);
        if (access(arguments[i], F_OK) == 0) {
            insertPathName( executablePathBuilder(arguments[i], cwd), i );
            break;
        }
    }

    // We must change to the directory of first argument.
    directory = "/usr/local/bin";  chdir(directory);
    if (access(program, F_OK) == 0) {
        executeProgram(program);
        chdir(cwd); return 0;
    }
    directory = "/usr/bin";  chdir(directory);
    if (access(program, F_OK) == 0) {
        executeProgram(program);
        chdir(cwd); return 0;
    }
    directory = "/bin";  chdir(directory);
    if (access(program, F_OK) == 0) {
        executeProgram(program);
        chdir(cwd); return 0;
    }       
    
    chdir(cwd);
    executeProgram(program);

    return 0; 
}


/* ============================================================ */
// Wildcard Processing //

/* All tokens must pass the following wildcard criteria, otherwise, the command will not be executed */
int wildcardCriteria(char* token) {
     
    int asterisks = 0;
    int slashes = 0;
    int last_asterisk_index = 0;
    int last_slash_index = 0;

    /* First, count up the number of asterisks and slashes, and index the last
    occurrence of each */
    for ( int i = 0; i < strlen(token); i++ ) {
        if ( token[i] == '*' ){ 
            asterisks++;
            last_asterisk_index = i;
        } if ( token[i] == '/' ) {
            slashes++;
            last_slash_index = i;
        }
    }

    // Check to make sure there is only one asterisk
    if ( asterisks > 1 ) {
        printf("Error: Improper use of '*' symbol\n");
        printf("Only one '*' symbol is permitted per file or path name\n");
        return 1;
    }

    // In the case of a path name, make sure the asterisk is in the last section
    if ( slashes > 0 && last_slash_index > last_asterisk_index ) {
        printf("Error: Improper use of '*' symbol\n");
        printf("In path names, the '*' symbol can only be used in the last section\n");
        return 1;
    }

    return 0;
}

/* IMPORTANT */
/* This function adds all wildcard matches to the official token list */
void addGlob(char* token, int arrayIndex) {

    int insertionPoint = arrayIndex + 1;
    tokens = (char**)realloc(tokens, (MAX_TOKENS + 1) * sizeof(char*)); // Make space for another token

    // Shift elements to the right
    for ( int i = MAX_TOKENS; i > insertionPoint; i-- ) {
        tokens[i] = tokens[i - 1];
    }

    // Allocate memory for the new string
    tokens[insertionPoint] = (char *)malloc((strlen(token) + 1) * sizeof(char));
    if ( tokens[insertionPoint] == 0 ) {
        printf("Memory allocation failed for new string.\n");
        return;
    }

    // Copy the new string into the array
    strcpy(tokens[insertionPoint], token);
    MAX_TOKENS++;
}

int globIt(char* token, int arrayIndex) {

    char* file_match = NULL;
    glob_t glob_result;

    int glob_status = glob(token, 0, 0, &glob_result);
    if (glob_status == 0) {

        // Successfully found matching files
        for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
        
            file_match = malloc(strlen(glob_result.gl_pathv[i]) + 1);
            memcpy(file_match, glob_result.gl_pathv[i], strlen(glob_result.gl_pathv[i]) + 1);
            file_match[strlen(glob_result.gl_pathv[i])] = '\0';
            
            /* If this is the first match, we want to first completely replace 
            the token with the wildcard character, then we'll add on its other matches */
            if ( i == 0 ) {
                tokens[arrayIndex] = realloc(tokens[arrayIndex], strlen(file_match) + 1);
                memcpy(tokens[arrayIndex], file_match, strlen(file_match) + 1);
                tokens[arrayIndex][strlen(file_match)] = '\0';
            }    
            /* If there is more than one match, tack it to the array on after 
            the original wildcard token */
            else addGlob(file_match, arrayIndex);
            free(file_match);
        }

        globfree(&glob_result);

    } else if (glob_status == GLOB_NOMATCH) {
        // printf("No files found matching the pattern.\n");
        // If there are no matches, we do nothing. The token remains the same.
        return 1;
    } else {
        printf("Error in glob.\n");
        return 1;
    }
    return 0;
}

void bareGlob(char* token, int arrayIndex) {

    char cwd[4096];
    getcwd(cwd, sizeof(cwd));

    char original_token[strlen(token) + 1];
    memcpy(original_token, token, strlen(token) + 1);
    original_token[strlen(token)] = '\0';

    char* directory;

    directory = "/usr/local/bin";  chdir(directory);
    globIt(token, arrayIndex);
    if ( strcmp(original_token, tokens[arrayIndex]) != 0 ) { chdir(cwd); return; }

    directory = "/usr/bin";  chdir(directory);
    globIt(token, arrayIndex);
    if ( strcmp(original_token, tokens[arrayIndex]) != 0 ) { chdir(cwd); return; }

    directory = "/bin";  chdir(directory);
    globIt(token, arrayIndex);
    if ( strcmp(original_token, tokens[arrayIndex]) != 0 ) { chdir(cwd); return; }

    chdir(cwd);
}

/* Check if any wildcard characters exist, and if they do, send that token to 'glob()' */
int wildcard() {

    char* token;
    int status = 0;

    for ( int i = 0; i < MAX_TOKENS; i++ ) { // Iterate through the entire array
        token = tokens[i];
        char original_token[strlen(token) + 1];
        memcpy(original_token, token, strlen(token) + 1);
        original_token[strlen(token)] = '\0';

        for ( int j = 0; j < strlen(token); j++ ) { // Iterate through each, individual token

            if ( token[j] == '*' ) { // Wildcard found!
                if ( wildcardCriteria(token) == 1 ) return 1; // Must pass criteria

                // If a match is found, return 0, if no match return 1.
                status = globIt(token, i);

                /* If we have a bare name that is not in the cwd, we need to go into 
                the three bin directories to see if the file is in there */
                int bare = 0;
                for ( int ch = 0; ch < strlen(tokens[i]); ch++ ) {
                    if ( ch == '/' ) bare = 1;
                }
                if ( bare == 0 && access(token, F_OK) != 0 && status == 1 ) 
                bareGlob(token, i); 

                status = 0;
                break; // Go to the next token
            }
        }
    }
    return status;
}


/* ============================================================ */
// Input Processing Functions //

/* This function takes the user input as a string, and indexes each token in 'tokens[]' */
void stringToArray(const char *originalString, const char *delimiter, char **stringArray, int *numSubstrings) {

    char *token;
    *numSubstrings = 0;
    int MAX_SUBSTRINGS = MAX_TOKENS;

    // Tokenize the string based on the delimiter
    token = strtok((char *)originalString, delimiter);
    
    // Store each token as a substring in the string array
    while ( token != NULL && *numSubstrings < MAX_SUBSTRINGS ) {

        stringArray[*numSubstrings] = (char *)malloc(((strlen(token)+1)) * sizeof(char));

        if ( stringArray[*numSubstrings] == NULL ) {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }

        strncpy(stringArray[*numSubstrings], token, (strlen(token)+1) - 1);
        stringArray[*numSubstrings][(strlen(token)+1) - 1] = '\0'; // Null-terminate the substring
        (*numSubstrings)++;
        token = strtok(NULL, delimiter);
    }
}

/* This function just sets things up before calling 'stringToArray()' */
void stringToArrayWrapper() {

    int numSubstrings;
    const char delimiter[] = " ";
    char originalString[strlen(line) + 1];
    memcpy(originalString, line, strlen(line) + 1);

    // Create space for each token in 'line'
    // 'tokens' is a global array that can be accessed anywhere
    tokens = (char**)malloc(MAX_TOKENS * sizeof(char *));
    
    stringToArray(originalString, delimiter, tokens, &numSubstrings);
}

/* This is an important function. If there are any carets or pipes that rub up 
against a token, it will separate them with a space */
void makeSpaceForJesus() {

    int len = strlen(line);
    int newSize = len + 2; // Increase the size of the string by 2 for two spaces

    for (int i = 0; i < len; i++) {
        if (line[i] == '<' || line[i] == '>' || line[i] == '|') {
            newSize += 2; // Increase the size by 2 for two spaces around the target character
        }
    }

    char *temp = (char *)malloc(newSize * sizeof(char));
    if (temp == NULL) {
        perror("Memory allocation failed");
        return;
    }

    int j = 0;
    for (int i = 0; i < len; i++) {
        if (line[i] == '<' || line[i] == '>' || line[i] == '|') {
            temp[j++] = ' ';      // Add space before the target character
            temp[j++] = line[i];  // Add the target character
            temp[j++] = ' ';      // Add space after the target character
        } else {
            temp[j++] = line[i];  // Copy other characters as they are
        }
    }
    temp[j] = '\0'; // Add null terminator to mark the end of the new string
    line = realloc(line, strlen(temp) + 1);

    strcpy(line, temp); // Copy the modified string back to the original

    free(temp); // Free the dynamically allocated memory
}

/* Convert user input to a char array and store in memory */
char* readInput() {

    char buffer[BUFFSIZE];
    int bytes;

    bytes = read(STDIN_FILENO, buffer, BUFFSIZE);

    if ( bytes == -1 ) {
        perror("Error reading input");
        exit(EXIT_FAILURE);
    }

    buffer[bytes - 1] = '\0';

    line = malloc(bytes);
    memcpy(line, buffer, bytes);

    return line;
}


/* ============================================================ */
// Master Directory //

int masterDirectory() {

    /* We need to find the first match, and replace it with the token with the wildcard
    character in the 'tokens' array */
    if ( wildcard() == 1 ) { 
        exit_status = 1; return 1; 
    }

    // Get the first token
    char* command = tokens[0];

    if ( strcmp(command, "then") == 0 ) {
        if ( thenHandler() == 1 ) return 1;
        exit_status = 0;
        command = tokens[1];
    }

    if ( strcmp(command, "else") == 0 ) {
        if ( elseHandler() == 1 ) return 1;
        exit_status = 0;
        command = tokens[1];
    }

    if ( strcmp(command, "pwd") == 0 && hasCaret() == 1 ) {
        if ( printCurrDirectory() == 1 ) return 1;
        exit_status = 0;
        return 0;
    }

    if ( strcmp(command, "cd") == 0 ) { // Change directories
        if ( changeDirectory() == 1 ) return 1;
        exit_status = 0;
        return 0;
    }
    
    if ( strcmp(command, "which") == 0 && hasCaret() == 1 ) {
        if ( whichCommand() == 1 ) return 1;
        exit_status = 0;
        return 0;
    }

    /* If we get to this point, we are dealing with file paths or bare names.
    This is the case where we don't have any carets or pipes, just a program name
    and potentially some arguments */
    // Ex.) ./foo arg1 arg2
    if ( hasCaret() == 1 && hasPipe() == 1 ) {
        if ( executeProgramWrapper() == 1 ) return 1;
        exit_status = 0;
        return 0;
    }

    // If we get to this point, we are dealing with redirection and piping
    if ( hasCaret() == 0 || hasPipe() == 0 ) {
        if ( caretPipeSwitch() == 1 ) return 1;
        exit_status = 0;
        return 0;
    }

    // If we get to here, user erorr
    else {
        printf("Error: Command not found\n");
        return 1;
    }
    return 0;

}

void readTextFileLine(char* inputLine) {
    line = malloc(strlen(inputLine) + 1);
    memcpy(line, inputLine, strlen(inputLine) + 1);

    if ( line[0] == '\0' ) { free(line); exit(EXIT_FAILURE); }
    if ( ifAllSpaces() == 1 ) { free(line); exit(EXIT_FAILURE); }

    if ( strcmp(line, "exit") == 0 ) {
        printf("Now leaving myshell\n");
        exit(EXIT_SUCCESS);
    }
    makeSpaceForJesus();
    countTokens();
    stringToArrayWrapper();
    masterDirectory();
    inputReset();
}


/* ============================================================ */
// Program Start //

int main(int argc, char const *argv[])
{   

    int status = 0;
    if ( argc > 2 ) {
        printf("Error: Too many arguments! \n"); 
        exit(EXIT_FAILURE);
    } 

    /* ===================================== Batch mode: */
    if ( argc == 2 ) {
                
        // Make sure the file exists
        int fd = open(argv[1], O_RDONLY);
            if (fd == -1) {
            perror("Error opening file");
            return 1;
        }
        char buffer[BUFFSIZE];
        int index = 0;
        int bytes;
        char ch;
        
        while ( ( bytes = read(fd, &ch, 1)) > 0 ) {
            if ( ch == '\n' || index >= BUFFSIZE - 1 ) {
                buffer[index] = '\0';
                readTextFileLine(buffer);
                index = 0;
            } else {
                buffer[index++] = ch;
            }
        }
        if ( index > 0 ) {
            buffer[index] = '\0';
            readTextFileLine(buffer);
        }
        close(fd);
        exit(EXIT_SUCCESS);
    } 
    /* ================================================= */

    /* =============================== Interactive mode: */
    int input = 1;
    char prompt[] = "mysh> ";

    startup();
    while (input) {
        
        write(STDOUT_FILENO, prompt, strlen(prompt));

        // readInput() should be called every iteration 
        line = readInput();

        // Edge cases
        if ( line[0] == '\0' ) { free(line); continue; }
        if ( ifAllSpaces() == 1 ) { free(line); continue; }

        // We don't want to do anything else if the input is 'exit', so check that first
        if ( strcmp(line, "exit") == 0 ) {
            printf("Now leaving myshell\n");
            exit(EXIT_SUCCESS);
        }

        // First, separate (with spaces) any input that looks like this: foo<bar
        makeSpaceForJesus();

        // Count the total amount of tokens entered by the user (including <, >, and |)
        countTokens();

        // Input all tokens into the global array called 'tokens'
        stringToArrayWrapper();

        // Now, we will enter the master directory
        status = masterDirectory();
        if (status == 1) exit_status = 1;

        inputReset();
    }
    /* ================================================= */
    
    return 0;
}