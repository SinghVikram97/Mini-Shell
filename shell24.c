#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include<wordexp.h>
#define MAX_COMMAND_LENGTH 100

void executeCommand(char *argsArray[]) {
    int pid = fork();
    int status;

    if (pid < 0) {
        printf("Fork failed\n");
        exit(1);
    }

    if (pid > 0) {
        waitpid(pid, &status, 0);
    } else {
        int resultOfExec = execvp(argsArray[0], argsArray);
        if (resultOfExec == -1) {
            printf("Execution of command failed\n");
            exit(1);
        }
    }
}

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
            int pathLength = strlen(argsArray[i]) - 1; // Exclude ~
            char *expandedPath = malloc(homeDirLength + pathLength + 1); // +1 for null terminator
            if (expandedPath == NULL) {
                printf("Error: Memory allocation failed\n");
                return;
            }
            strcpy(expandedPath, homeDir);
            strcat(expandedPath, argsArray[i] + 1); // Skip '~'
            argsArray[i] = expandedPath;
        }
    }
}

void executeCatCommand(char *fileName) {
    
    char *argsArrayCatCommand[3];
    argsArrayCatCommand[0]="cat";
    argsArrayCatCommand[1]=fileName;
    argsArrayCatCommand[2]=NULL;

    executeCommand(argsArrayCatCommand);

}

void processPipeOperation(char input[]) {
    const int MAX_PIPES = 6; // Maximum number of pipes
    const int MAX_COMMANDS = MAX_PIPES + 1; // Maximum number of commands including the first one
    const int MAX_ARGS = 5; // Maximum number of arguments per command

    char *commands[MAX_COMMANDS]; // Array to store commands

    // Tokenize input based on "|"
    int numCommands = 0;
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

    // Execute commands
    for (int i = 0; i < numCommands; i++) {
        int argsCount = 0;
        char *argsArray[MAX_ARGS + 1]; // Argument array for execvp

        // Tokenize the command based on spaces
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
            if (i != 0) {
                // Redirect stdin from the read end of the previous pipe
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("Dup2 failed");
                    exit(EXIT_FAILURE);
                }
            }

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
                perror("Execution of command failed");
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

void processFileConcatenation(char input[]){
    // Max 5 operations 
    // ie. max 5 #
    // file1 # file2 # file3 # file4 # file5 # file6
    // ie. upto 6 files

    int MAX_ARGS=6;
    char *argsArray[MAX_ARGS + 1];
    int argsC;

    argsC = 0;
    char *token = strtok(input, " # ");
    while (token != NULL && argsC < MAX_ARGS) {
        argsArray[argsC++] = token;
        token = strtok(NULL, " # ");
    }

    if (token != NULL) {
        printf("Error: Text file (.txt) concatenation (upto 5 operations / 6 Files)\n");
        return;
    }

    argsArray[argsC] = NULL;
    //expandHomeDirectory(argsArray);
    for(int i=0;argsArray[i]!=NULL;i++){
        executeCatCommand(argsArray[i]);
    }
}

void processNormalCommand(char input[]){
    int MAX_ARGS=5;
    char *argsArray[MAX_ARGS + 1];
    int argsC;

    argsC = 0;
    char *token = strtok(input, " ");
    while (token != NULL && argsC < MAX_ARGS) {
        argsArray[argsC++] = token;
        token = strtok(NULL, " ");
    }

    if (token != NULL) {
        printf("Error: Incorrect number of arguments should be >=1 and <=5\n");
        return;
    }

    argsArray[argsC] = NULL;
    expandHomeDirectory(argsArray);
    executeCommand(argsArray);
}

int main() {
    // string
    char input[MAX_COMMAND_LENGTH];

    while (1) {
        printf("shell24$ ");
        fgets(input, sizeof(input), stdin);

        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }

        int concatenate = 0;
        int piping=0;
        for(int i=0;i<strlen(input);i++){
            if (input[i]=='#') {
                concatenate = 1;
                break;
            }else if(input[i]=='|'){
                piping=1;
                break;
            }
        }
        if(concatenate==1){
            processFileConcatenation(input);
        }else if(piping==1){
            processPipeOperation(input);
        }else{
            processNormalCommand(input);
        }
    }
    return 0;
}