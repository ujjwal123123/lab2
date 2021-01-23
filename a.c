#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h> // needed for execl

#define COLOR "\x1B[31m"
#define RESET "\x1B[0m"

// Returns array of null-terminated strings
char **parse_command_with_spaces(char *command) {
    char **args = (char **)malloc(sizeof(char *) * 100);
    char *save_ptr;
    args[0] = strtok_r(command, " ", &save_ptr);
    int i = 0;
    while (args[i] != NULL) {
        args[++i] = strtok_r(NULL, " ", &save_ptr);
    }

    return args;
}

// Returns 0 if internal command was executed
int exec_internal_command(char **parsed_command) {
    if (strcmp(parsed_command[0], "exit") == 0) {
        printf("Exiting!!\n");
        exit(0);
    }
    else if (strcmp(parsed_command[0], "test") == 0) {
        ;
        return 0;
    }
    else if (strcmp(parsed_command[0], "cd") == 0) {
        int status = chdir(parsed_command[1]);
        if (status == -1) {
            fprintf(stderr,
                    "Error: Directory could not be changed\n");
        }
        return 0;
    }

    return 1;
}

// Retuns 0 if a command was executed. The command must not contain a
// pipe or a semi-colon.
int exec_single_command(char *command) {
    char **args = parse_command_with_spaces(command);

    if (exec_internal_command(args) == 0) {
        free(args);

        return 0;
    }

    // find file redirections
    char *in = NULL;
    char *out = NULL;

    for (char *it = *args; it != NULL; it++) {
        // if (strcmp(*it, ">") == 0) {
        //     out = *it;
        //     *it = '\0';
        // }
        // else if (strcmp(*it, "<") == 0) {
        //     in = *it;
        //     *it = '\0';
        // }
    }

    int pid = fork();

    if (pid == 0) {
        fprintf(stderr, "Command: %s\n", args[0]);
        int status = execvp(args[0], args);
        // exit(0);
        if (status == -1) {
            fprintf(stderr, "Error: Could not execute the command\n");
        }
        return 0;
    }
    else {
        free(in);
        free(out);
        free(args);
        int status;
        wait(&status);
        return WEXITSTATUS(status);
    }
}

// Returns 0 if a command was executed. The command may contain a
// pipe. The command must not contain a semi-colon.
int exec_command_with_pipe(char *command) {
    char **parsed_commands = (char **)malloc(sizeof(char *) * 100);
    char *save_ptr;
    parsed_commands[0] =
        strtok_r(command, "|", &save_ptr); // without pipes
    int len = 0;
    while (parsed_commands[len] != NULL) {
        // exec_single_command(parsed_commands_without_pipe[i]);
        parsed_commands[++len] = strtok_r(NULL, "|", &save_ptr);
    }

    // int fd[2];
    for (int j = 0; j < len; j++) {
        // if (j != 0) {
        //     dup2(fd[STDIN_FILENO], STDIN_FILENO);
        // }
        // if (j != len - 1) {
        //     if (pipe(fd) == -1) {
        //         fprintf(stderr, "Pipe could not be created\n");
        //         exit(1);
        //     };
        //     dup2(fd[STDOUT_FILENO], STDOUT_FILENO);
        // }

        exec_single_command(parsed_commands[j]);

        // if (j != len - 1) {
        //     close(fd[STDOUT_FILENO]);
        // }
    }

    free(parsed_commands);
    return 0;
}

// Execute a command which may consist of a number of commands
// separated by `;`
int exec_multi_command(char *multi_command) {
    char **parsed_commands_without_semicolon =
        (char **)malloc(sizeof(char *) * 200);

    char *save_ptr;
    parsed_commands_without_semicolon[0] =
        strtok_r(multi_command, ";", &save_ptr);
    int i = 0;
    while (parsed_commands_without_semicolon[i] != NULL) {
        exec_command_with_pipe(parsed_commands_without_semicolon[i]);
        parsed_commands_without_semicolon[++i] =
            strtok_r(NULL, ";", &save_ptr);
    }
    free(parsed_commands_without_semicolon);

    return 0;
}

int main() {
    char *current_dir_buf = (char *)malloc(sizeof(char) * 200);

    char *command = (char *)malloc(sizeof(char) * 500);

    while (1) {
        printf(COLOR "%s $ " RESET, getcwd(current_dir_buf, 200));
        int status = scanf(" %[^\n]s", command);

        if (status == -1) {
            // handle Ctrl-D
            exec_multi_command("exit");
        }
        else if (exec_multi_command(command) == 0) {
            ;
        }
    }

    free(current_dir_buf);
    free(command);

    return 0;
}
