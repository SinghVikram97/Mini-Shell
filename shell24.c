#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include<wordexp.h>
#include<fcntl.h>
#include<ctype.h>
#include <sys/stat.h>

// I have taken maximum command length as 100 which can be modified
#define MAX_COMMAND_LENGTH 100
// I have taken maximum processes that can be pushed to background to be 100 which can modified
// Array to store background process IDs
int background_processes[100];
// Counter to keep track of number of processes in background
int background_process_count = 0;

// Function to trim leading and trailing whitespace from a string
// Parameters:
// - str: The string to be trimmed
// Example:
//   Input: "    abc     "
//   Output: "abc"
void trimWhitespace(char *str) {
    if (str == NULL) {
        return;
    }

    int len = strlen(str);
    int start = 0;
    int end = len - 1;

    // Trim leading whitespace
    while (isspace(str[start])) {
        start++;
    }

    // Trim trailing whitespace
    while (end > start && isspace(str[end])) {
        end--;
    }

    // Shift characters to the beginning of the string
    int shift = 0;
    for (int i = start; i <= end; i++) {
        str[i - start] = str[i];
        shift++;
    }

    // Null-terminate the trimmed string
    str[shift] = '\0';
}

// Function to execute a command
// Parameters:
// - argsArray: Array containing the command and its arguments
// Example:
//   executeCommand(["ls", "-l", NULL]);
void executeCommand(char *argsArray[]) {
    int pid = fork();
    int status;

    if (pid < 0) {
        printf("Fork failed\n");
        exit(1);
    }

    if (pid > 0) {
        // Wait for child to finish executing command
        waitpid(pid, &status, 0);
    } else {
        // execvp returns only incase of failure
        // it returns -1
        int resultOfExec = execvp(argsArray[0], argsArray);
        if (resultOfExec == -1) {
            printf("Execution of command failed %s\n",argsArray[0]);
            exit(1);
        }
    }
}

// Function to expand the home directory in a given set of arguments
// Parameters:
// - argsArray: Array of arguments
// Example:
//   expandHomeDirectory(["ls", "~/Documents", NULL]);
// Modifies ~/ and replaces it with /Users/vikramsinghbedi if $HOME is /Users/vikramsinghbedi 
void expandHomeDirectory(char *argsArray[]) {
    char *homeDir = getenv("HOME");
    if (homeDir == NULL) {
        printf("Error: HOME environment variable not set\n");
        return;
    }

    int homeDirLength = strlen(homeDir);

    for (int i = 0; argsArray[i] != NULL; i++) {
        if (strcmp(argsArray[i], "~") == 0) {
            // Expand ~ to home directory
            argsArray[i] = homeDir;
        }
        else if (strncmp(argsArray[i], "~/", 2) == 0) {
            // Expand ~/ to home directory + remaining path
            // Exclude ~
            int pathLength = strlen(argsArray[i]) - 1; 
            // +1 for null terminator
            char *expandedPath = malloc(homeDirLength + pathLength + 1); 
            if (expandedPath == NULL) {
                printf("Error: Memory allocation failed\n");
                return;
            }
            strcpy(expandedPath, homeDir);
            // Skip '~'
            strcat(expandedPath, argsArray[i] + 1); 
            argsArray[i] = expandedPath;
        }
    }
}

// Function to execute the "cat" command with a specified file
// Parameters:
// - fileName: The name of the file to be displayed
// Example:
//   executeCatCommand("example.txt");
void executeCatCommand(char *fileName) {
    
    char *argsArrayCatCommand[3];
    argsArrayCatCommand[0]="cat";
    argsArrayCatCommand[1]=fileName;
    argsArrayCatCommand[2]=NULL;

    // Pass argsArrayCatCommand={"cat","fileName",NULL} 
    executeCommand(argsArrayCatCommand);

}

// Function to process pipe operations
// Parameters:
// - input: The input string containing pipe-separated commands
// Example:
//   processPipeOperation("ls | grep example | cat one.txt");
void processPipeOperation(char input[]) {
    // Maximum number of pipes
    const int MAX_PIPES = 6; 
    // Maximum number of commands - would be one + number of pipes
    const int MAX_COMMANDS = MAX_PIPES + 1; 
    // Maximum number of arguments per command
    const int MAX_ARGS = 5; 

    // Array to store indiviual commands extracted b/w pipes
    char *commands[MAX_COMMANDS]; 

    int numCommands = 0;
    // Tokenize input based on "|" - we get indiviual commands
    char *token = strtok(input, "|");
    while (token != NULL && numCommands < MAX_COMMANDS) {
        // Trim leading and trailing whitespace from the command
        char *trimmedCommand = token;
        while (*trimmedCommand == ' ' || *trimmedCommand == '\t')
            ++trimmedCommand;
        size_t length = strlen(trimmedCommand);
        while (length > 0 && (trimmedCommand[length - 1] == ' ' || trimmedCommand[length - 1] == '\t'))
            trimmedCommand[--length] = '\0';
        if (length > 0) {
            commands[numCommands++] = trimmedCommand;
        }
        // Get pointer to remaining token
        token = strtok(NULL, "|");
    }

    // Check if more commands present after tokenization limit
    if (token != NULL) {
        printf("Error: Too many pipe commands (up to 6 operations)\n");
        return;
    }

    // Create pipes
    int pipes[MAX_PIPES][2];
    for (int i = 0; i < MAX_PIPES; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }
    }

    // Execute commands - one by one
    for (int i = 0; i < numCommands; i++) {
        int argsCount = 0;
        // Argument array for execvp
        char *argsArray[MAX_ARGS + 1]; 

        // Tokenize the command based on spaces - we get indiviual command and its arguments
        char *arg = strtok(commands[i], " ");
        while (arg != NULL && argsCount < MAX_ARGS) {
            argsArray[argsCount++] = arg;
            arg = strtok(NULL, " ");
        }

        // Null-terminate the argument array
        argsArray[argsCount] = NULL;

        // Check if the number of arguments exceeds the limit
        if (argsCount >= MAX_ARGS) {
            printf("Error: Too many arguments for command %d (up to 5 arguments allowed)\n", i + 1);
            return;
        }

        // Fork a child process for each command
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            // For i=0 we need input from stdin
            // Otherwise from previous pipe
            if (i != 0) {
                // Redirect stdin from the read end of the previous pipe
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("Dup2 failed");
                    exit(EXIT_FAILURE);
                }
            }

            // For last command we need to direct output to stdout
            // Otherwise to current pipe
            if (i != numCommands - 1) {
                // Redirect stdout to the write end of the current pipe
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("Dup2 failed");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipe ends
            for (int j = 0; j < MAX_PIPES; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command
            if (execvp(argsArray[0], argsArray) == -1) {
                printf("Execution of command failed %s\n",argsArray[0]);
                exit(EXIT_FAILURE);
            }
        }
    }

    // Close all pipe ends in the parent process
    for (int i = 0; i < MAX_PIPES; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < numCommands; i++) {
        wait(NULL);
    }
}

// Function to process file concatenation operations
// Parameters:
// - input: The input string containing file names separated by "#"
// Example:
//   processFileConcatenation("file1 # file2 # file3");
void processFileConcatenation(char input[]){
    // Max 5 operations 
    // ie. max 5 #
    // file1 # file2 # file3 # file4 # file5 # file6
    // ie. upto 6 files
    int MAX_ARGS=6;
    char *argsArray[MAX_ARGS + 1];
    int argsC;

    argsC = 0;
    // Tokenize based on # to get indiviual filenames
    char *token = strtok(input, " # ");
    while (token != NULL && argsC < MAX_ARGS) {
        argsArray[argsC++] = token;
        token = strtok(NULL, " # ");
    }

    // Check if more files remaining after tokenization - it means more than 5 operations/6
    if (token != NULL) {
        printf("Error: File concatenation (upto 5 operations / 6 Files)\n");
        return;
    }

    argsArray[argsC] = NULL;

    // Expand ~ sign in filepath if needed
    expandHomeDirectory(argsArray);

    // Execute cat command for each file
    for(int i=0;argsArray[i]!=NULL;i++){
        executeCatCommand(argsArray[i]);
    }
}

// Function to process normal commands without any special characters
// Parameters:
// - input: The input string containing the command and its arguments
// Example:
//   processNormalCommand("ls -l")
void processNormalCommand(char input[]){
    // Maximum number of arguments per command
    int MAX_ARGS=5;
    char *argsArray[MAX_ARGS + 1];
    int argsC;

    argsC = 0;
    // Tokenize input based on spaces to get command and its arguments
    char *token = strtok(input, " ");
    while (token != NULL && argsC < MAX_ARGS) {
        argsArray[argsC++] = token;
        token = strtok(NULL, " ");
    }

    // Check if more arguments present after tokenization limit
    if (token != NULL) {
        printf("Error: Incorrect number of arguments should be >=1 and <=5\n");
        return;
    }

    argsArray[argsC] = NULL;

    // Replace ~ with $HOME in commands
    expandHomeDirectory(argsArray);
    executeCommand(argsArray);
}

// Function to process a normal command with input taken from a file
// Parameters:
// - command: The command to execute
// - inputFile: The file from which to take input
// Example:
//   processNormalCommandWithInputFile("sort", "input.txt")
void processNormalCommandWithInputFile(char *command, char *inputFile){
    // Maximum number of arguments per command
    int MAX_ARGS=5;
    char *argsArray[MAX_ARGS + 1];
    int argsC;

    argsC = 0;

    // Tokenize the command based on spaces
    char *token = strtok(command, " ");
    while (token != NULL && argsC < MAX_ARGS) {
        argsArray[argsC++] = token;
        token = strtok(NULL, " ");
    }

    // Check if more arguments present after tokenization limit
    if (token != NULL) {
        printf("Error: Incorrect number of arguments should be >=1 and <=5\n");
        return;
    }

    argsArray[argsC] = NULL;

    // change ~ to $HOME in command
    expandHomeDirectory(argsArray);

    // Open input file in read only mode
    int fd = open(inputFile, O_RDONLY);
    if (fd == -1) {
        printf("Error opening input file");
        exit(1);
    }

    // Create a child process
    int pid = fork();
    if (pid == -1) {
        printf("Fork failed");
        close(fd);
        exit(1);
    }

    if (pid == 0) {
        // Child process
        // Redirect standard input to the input file
        if (dup2(fd, STDIN_FILENO) == -1) {
            printf("Error redirecting input");
            close(fd);
            exit(EXIT_FAILURE);
        }

        // Close the file descriptor since it's no longer needed in the child process
        close(fd);

        // Execute the command
        if (execvp(argsArray[0], argsArray) == -1) {
            printf("Execution of command failed %s\n",argsArray[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        int status;
        // Wait for child process
        waitpid(pid, &status, 0);
        close(fd);
    }
}

// Function to process a normal command with output redirection to a file
// Parameters:
// - command: The command to execute
// - outputFile: The file to which the output should be redirected
// Example:
//   processNormalCommandWithOutputFile("ls -l", "output.txt")
void processNormalCommandWithOutputFile(char *command, char *outputFile){
    // Maximum number of arguments per command
    int MAX_ARGS=5;
    char *argsArray[MAX_ARGS + 1];
    int argsC;

    argsC = 0;
    // Tokenize the command based on spaces
    char *token = strtok(command, " ");
    while (token != NULL && argsC < MAX_ARGS) {
        argsArray[argsC++] = token;
        token = strtok(NULL, " ");
    }

    // Check if more arguments present after tokenization limit
    if (token != NULL) {
        printf("Error: Incorrect number of arguments should be >=1 and <=5\n");
        return;
    }

    argsArray[argsC] = NULL;

    // Replace ~ with $HOME
    expandHomeDirectory(argsArray);
    
    // Set umask as 0
    umask(0);

    // Open the output file for writing, create if not exists, truncate if exists
    int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd == -1) {
        perror("Error opening output file");
        exit(1);
    }

    // Fork a child process
    int pid = fork();
    if (pid == -1) {
        printf("Fork failed");
        close(fd);
        exit(1);
    }

    if (pid == 0) {
        // Child process
        // Redirect standard output to the output file
        if (dup2(fd, STDOUT_FILENO) == -1) {
            printf("Error redirecting input\n");
            close(fd);
            exit(EXIT_FAILURE);
        }

        // Close the file descriptor since it's no longer needed in the child process
        close(fd);

        // Execute the command
        if (execvp(argsArray[0], argsArray) == -1) {
            printf("Execution of command failed %s\n",argsArray[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        int status;
        // Wait for Child process
        waitpid(pid, &status, 0);
        close(fd);
    }
}

// Function to process a normal command with output redirection (append mode) to a file
// Parameters:
// - command: The command to execute
// - outputFile: The file to which the output should be appended
// Example:
//   processNormalCommandWithAppendOutputFile("ls -l", "output.txt")
void processNormalCommandWithAppendOutputFile(char *command, char *outputFile){
    // Maximum number of arguments per command
    int MAX_ARGS=5;
    char *argsArray[MAX_ARGS + 1];
    int argsC;

    argsC = 0;
    // Tokenize the command based on spaces
    char *token = strtok(command, " ");
    while (token != NULL && argsC < MAX_ARGS) {
        argsArray[argsC++] = token;
        token = strtok(NULL, " ");
    }

    // Check if more arguments present after tokenization limit
    if (token != NULL) {
        printf("Error: Incorrect number of arguments should be >=1 and <=5\n");
        return;
    }

    argsArray[argsC] = NULL;

    // Replace ~ with $HOME
    expandHomeDirectory(argsArray);
    
    // Set umask as 0
    umask(0);

    // Open the output file for writing in append mode, create if not exists
    int fd = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0777);
    if (fd == -1) {
        perror("Error opening output file");
        exit(1);
    }

    // Fork a child process
    int pid = fork();
    if (pid == -1) {
        printf("Fork failed");
        close(fd);
        exit(1);
    }

    if (pid == 0) {
        // Child process
        // Redirect standard output to the output file
        if (dup2(fd, STDOUT_FILENO) == -1) {
            printf("Error redirecting input\n");
            close(fd);
            exit(EXIT_FAILURE);
        }

        // Close the file descriptor since it's no longer needed in the child process
        close(fd);

        // Execute the command
        if (execvp(argsArray[0], argsArray) == -1) {
            printf("Execution of command failed %s\n",argsArray[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        int status;
        // Wait for child process
        waitpid(pid, &status, 0);
        close(fd);
    }
}

// Function to process command redirection (input, output, or output append)
// Parameters:
// - input: The command with redirection symbols
// Example:
//   processRedirection("ls > output.txt")
void processRedirection(char input[]){
    int inputRedirection=0;
    int outputRedirection=0;
    int outputAppendRedirection=0;

    // Detect redirection type
    for(int i = 0; i < strlen(input); i++){
        if(input[i] == '<'){
            inputRedirection = 1;
        } else if(input[i] == '>'){
            if(i+1 < strlen(input) && input[i + 1] == '>'){
                outputAppendRedirection = 1;
                i++;
            } else {
                outputRedirection = 1;
            }
        }
    }

    // Process input redirection
    if(inputRedirection==1){
        int MAX_ARGS=5;
        int argsCount = 0;
        char *command = strtok(input, "<");
        char *file = strtok(NULL, "<");

        trimWhitespace(command);
        trimWhitespace(file);

        processNormalCommandWithInputFile(command,file);
    }else if(outputRedirection==1){
        // Output redirection
        int MAX_ARGS=5;
        int argsCount = 0;
        char *command = strtok(input, ">");
        char *file = strtok(NULL, ">");

        trimWhitespace(command);
        trimWhitespace(file);

        processNormalCommandWithOutputFile(command,file);
    }else if(outputAppendRedirection==1){
        // Output append redirection
        int MAX_ARGS=5;
        int argsCount = 0;
        char *command = strtok(input, ">>");
        char *file = strtok(NULL, ">>");

        trimWhitespace(command);
        trimWhitespace(file);

        processNormalCommandWithAppendOutputFile(command,file);
    }
}

// Function to execute a command
// Parameters:
// - argsArray: Array of strings where the first element is the command and the rest are command arguments
// Returns:
//  1 if the command is executed successfully
// -1 if there is an error in forking or executing the command
int executeCommand2(char *argsArray[]){

    // Fork a child
    int pid = fork();
    int status;

    if (pid < 0) {
        printf("Fork failed\n");
        return -1;
    }

    if (pid > 0) {
        // Wait for child to execute
        waitpid(pid, &status, 0);
        return 1;
    } else {
        // execvp returns only if command fails and returns -1
        int resultOfExec = execvp(argsArray[0], argsArray);
        if (resultOfExec == -1) {
            printf("Execution of command failed %s\n",argsArray[0]);
            return -1;
        }
    }
}

// Function to execute a command with return status
// Parameters:
// - command: String containing the command and its arguments separated by spaces
// Returns:
//  1 if the command is executed successfully
//  -1 if there is an error in parsing the command or executing it
int executeCommandWithReturnStatus(char command[]) {
    int MAX_ARGS=5;
    char *argsArray[MAX_ARGS + 1];
    int argsC;

    argsC = 0;
    // Split by space, separating the command and its arguments
    char *token = strtok(command, " ");
    while (token != NULL && argsC < MAX_ARGS) {
        argsArray[argsC++] = token;
        token = strtok(NULL, " ");
    }

    // Check if the token is not NULL, indicating incorrect number of arguments
    if (token != NULL) {
        printf("Error: Incorrect number of arguments should be >=1 and <=5\n");
        return -1;
    }

    argsArray[argsC] = NULL;

    // replace ~ by $HOME ie. user home
    expandHomeDirectory(argsArray);

    // Execute the command and return its status
    return executeCommand2(argsArray);
}

// Function to process commands separated by && and || operators
// Parameters:
// - input: String containing commands and operators separated by && and || symbols
void processAndOr(char input[]) {
    // Since max 5 operations are allowed 5 &&/|| means 6 commands
    const int MAX_COMMANDS = 6;
    // Maximum number of arguments per command
    const int MAX_ARGS = 5;

    char *commands[MAX_COMMANDS];
    // There will be one less operator than commands
    char operators[MAX_COMMANDS - 1]; 

    int numCommands = 0;
    int numOperators = 0;

    // Identify and store operators
    for (int i = 0; input[i] != '\0'; i++) {
        if ((input[i] == '&' && input[i + 1] == '&') || (input[i] == '|' && input[i + 1] == '|')) {
            operators[numOperators++] = input[i];
            i++; // Skip the second character of the operator
        }
    }

    // Tokenize the input to extract commands
    char *token = strtok(input, "&|");
    while (token != NULL) {
        commands[numCommands++] = token;
        token = strtok(NULL, "&|");
    }

    // If number of commands > max commands print error and exit
    if(numCommands>MAX_COMMANDS){
        printf("Maximum of 5 operations / 6 commands are allowed\n");
        return;
    }

    // Trim whitespace from each command
    for (int i = 0; i < numCommands; i++) {
       trimWhitespace(commands[i]);
    }

    int status = 0; 
    // Execute the first command and get its status
    if (numCommands > 0) {
        status = executeCommandWithReturnStatus(commands[0]);
    }

    // Process rest of the commands based on operators and previous command status
    for (int i = 1; i < numCommands; i++) {
        if (status == 1 && operators[i - 1] == '&') {
            // Execute the command and update status if the previous command succeeded and operator is &&
            status = executeCommandWithReturnStatus(commands[i]);
        } else if(status!=1 && operators[i-1]=='&'){
            // Skip the command if the previous command failed and operator is &&
            continue;
        } else if (status == 1 && operators[i - 1] == '|') {
            // Skip the command if the previous command succeeded and operator is ||
            continue;
        } else if (status != 1 && operators[i - 1] == '|') {
            // Execute the command and update status if the previous command failed and operator is ||
            status = executeCommandWithReturnStatus(commands[i]);
        }
    }
}

// Function to process sequential commands separated by semicolons
// Parameters:
// - input: String containing sequential commands separated by semicolons
void processSequentialCommands(char input[]){
    // Max commands will be 5
    const int MAX_COMMANDS = 5;
    int numCommands = 0;

    char *commands[MAX_COMMANDS];

     // Tokenize the input to extract commands
    char *token = strtok(input, ";");
    while (token != NULL) {
        commands[numCommands++] = token;
        token = strtok(NULL, ";");
    }

    // If number of commands > 5 then print error
    if(numCommands>MAX_COMMANDS){
        printf("Maximum of 5 commands are allowed\n");
        return;
    }

    // Trim whitespace from each command
    for (int i = 0; i < numCommands; i++) {
       trimWhitespace(commands[i]);
    }

    // Execute each command sequentially
    for(int i=0;i<numCommands;i++){
        executeCommandWithReturnStatus(commands[i]);
    }

}

// Function to execute a command in the background
// Parameters:
// - argsArray: Array of strings containing the command and its arguments
void executeCommandInBackground(char *argsArray[], int isShell){
    // Fork new process
    int pid = fork();
    int status;

    if (pid < 0) {
        printf("Fork failed\n");
        exit(1);
    }

    if (pid > 0) {
       // Parent process
       // Add child process ID to the background_processes array
       // Don't wait for child - keep it running in background
       if(isShell!=1){
            background_processes[background_process_count++] = pid;
       }
    } else {
        // Child process
        // Execute the command
        int resultOfExec = execvp(argsArray[0], argsArray);
        if (resultOfExec == -1) {
            printf("Execution of command failed %s\n",argsArray[0]);
            exit(1);
        }
    }
}

// Function to process background execution of a command
// Parameters:
// - input: String containing the command and its arguments
void processBackgroundExecution(char input[]){
    int MAX_ARGS=5;
    char *argsArray[MAX_ARGS + 1];
    int argsC;

    argsC = 0;
    // Tokenize by space to extract command and its arguments
    char *token = strtok(input, " ");
    while (token != NULL && argsC < MAX_ARGS) {
        argsArray[argsC++] = token;
        token = strtok(NULL, " ");
    }

    // Check if more arguments present after tokenization
    if (token != NULL) {
        printf("Error: Incorrect number of arguments should be >=1 and <=5\n");
        return;
    }

    argsArray[argsC] = NULL;

    // Replace ~ with $HOME ie. user home
    expandHomeDirectory(argsArray);

    // Execute command in background
    executeCommandInBackground(argsArray,0);
}

// Function to bring the last background process to the foreground
void bringLastBackgroundProcessToForeground() {
    // Function to bring the last background process to the foreground
    if (background_process_count == 0) {
        printf("No background processes to bring to foreground\n");
        return;
    }

    // Get the PID of the last background process
    int pid = background_processes[background_process_count - 1];
    int status;

    // Wait for the background process to finish
    waitpid(pid, &status, 0);

    // Remove the background process from the list
    background_process_count--;
}

// Function to start a new shell in the background
void startNewShell(){
    // Arguments to start a new shell using xterm
    char *args[] = {"xterm", "-e", "./shell24", NULL};

    // Start new shell
    executeCommandInBackground(args,1);
}

int main() {
    // get user input in an array
    char input[MAX_COMMAND_LENGTH];

    // infinite loop for shell
    while (1) {
        // print shell prompt and wait for user input
        printf("shell24$ ");

        // take user input
        fgets(input, sizeof(input), stdin);

        // Remove trailing newline character
        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }

        // Flags to identify the type of command
        int concatenate = 0; // File concatenation
        int piping = 0;  // Pipe operation
        int redirect = 0; // Input/Ouput redirection
        int and_or=0; // && and || 
        int sequential=0; // ; Sequential execution
        int backgroundProcess=0; // Execute in background

        // Traverse through input
        for (int i = 0; i < strlen(input); i++) {
            if (input[i] == '|') {
                // if double '|' then it is OR
                if(i+1<strlen(input) && input[i+1]=='|'){
                    and_or=1;
                }else if(i-1>0 && input[i-1]=='|'){
                    and_or=1; 
                }else{
                    // Single '|' means pipe
                    piping = 1;
                }
            } else if (input[i] == '#') {
                // # means file concatenation
                concatenate = 1;
            } else if (input[i] == '>' || input[i] == '<') {
                // if > or < or >> it means input/output redirection
                redirect = 1;
            } else if(input[i]=='&'){
                // Double '&' means AND
                 if(i+1<strlen(input) && input[i+1]=='&'){
                    and_or=1;
                }else if(i-1>0 && input[i-1]=='&'){
                    and_or=1; 
                }else{
                    // Single '&' means run in background
                    backgroundProcess = 1;
                }
            }else if(input[i]==';'){
                // ; means run sequentially
                sequential=1;
            }
        }

        // if newt it means open a new shell
        if(strcmp("newt",input)==0){
            startNewShell();
        }
        // bring last background process to foreground
        else if(strcmp("fg",input)==0){
            bringLastBackgroundProcessToForeground();
        }
        // Execute functions based on their type of input
        else if(concatenate==1){
            processFileConcatenation(input);
        }else if(piping==1){
            processPipeOperation(input);
        }else if(redirect==1){
            processRedirection(input);
        }else if(and_or==1){
            processAndOr(input);
        }else if(sequential==1){
            processSequentialCommands(input);
        }else if(backgroundProcess==1){
            processBackgroundExecution(input);
        }
        else{
            processNormalCommand(input);
        }
    }
    return 0;
}