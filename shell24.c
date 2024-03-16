#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include<wordexp.h>
#define MAX_COMMAND_LENGTH 100
#define MAX_ARGS 5

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

    size_t homeDirLength = strlen(homeDir);

    for (int i = 0; argsArray[i] != NULL; i++) {
        if (strcmp(argsArray[i], "~") == 0) {
            // Expand ~ to home directory
            argsArray[i] = homeDir;
        }
        else if (strncmp(argsArray[i], "~/", 2) == 0) {
            // Expand ~/ to home directory + remaining path
            size_t pathLength = strlen(argsArray[i]) - 1; // Exclude ~
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



int main() {
    // string
    char input[MAX_COMMAND_LENGTH];
    // array of strings
    char *argsArray[MAX_ARGS + 1];
    int argsC;

    while (1) {
        printf("shell24$ ");
        fgets(input, sizeof(input), stdin);

        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }

        argsC = 0;
        char *token = strtok(input, " ");
        while (token != NULL && argsC < MAX_ARGS) {
            argsArray[argsC++] = token;
            token = strtok(NULL, " ");
        }

        if (token != NULL) {
            printf("Error: Incorrect number of arguments should be >=1 and <=5\n");
            continue; 
        }

        argsArray[argsC] = NULL;
        expandHomeDirectory(argsArray);
        printf("Arguments after expansion:\n");
        for (int i = 0; argsArray[i] != NULL; i++) {
            printf("%s\n", argsArray[i]);
        }
        executeCommand(argsArray);
    }
    return 0;
}
