#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
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

        argsArray[argsC] = NULL;
        executeCommand(argsArray);
    }
    return 0;
}
