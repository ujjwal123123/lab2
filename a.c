#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h> // needed for execl

#define COLOR "\x1B[31m"
#define RESET "\x1B[0m"

jmp_buf sigterm_buffer;

// do not do anything on ctrl-c
void sigint_handler() {
    // with help from
    // https://stackoverflow.com/questions/57480852/how-to-handle-control-c-signal-while-designing-a-shell
    printf("\n");
}

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

void strstrip(char *s) {
    size_t size;
    char *end;

    size = strlen(s);

    if (!size)
        return;

    end = s + size - 1;
    while (end >= s && isspace(*end))
        end--;
    *(end + 1) = '\0';

    while (*s && isspace(*s))
        s++;
}

// Return 0 on success, -1 on failure
int parse_file_redirection(int fd[], char *command) {
    int in_fd = -1;
    int out_fd = -1;

    char *in_file_name = NULL;
    char *out_file_name = NULL;

    for (char *itr = command; *itr != '\0'; itr++) {
        if (*itr == '>') {
            *itr = '\0';
            while (*(itr + 1) == ' ') {
                itr++;
            }
            out_file_name = itr + 1;
        }
        else if (*itr == '<') {
            *itr = '\0';
            while (*(itr + 1) == ' ') {
                itr++;
            }
            in_file_name = itr + 1;
        }
    }

    if (in_file_name) {
        strstrip(in_file_name);
        in_fd = open(in_file_name, O_RDONLY);
        if (in_fd < 0) {
            fprintf(stderr, "Cannot open file\n");
            return -1;
        }
    }
    if (out_file_name) {
        strstrip(out_file_name);
        out_fd = creat(out_file_name, 0644); // this is rw-r--r--
        if (out_fd < 0) {
            fprintf(stderr, "Cannot create file %d\n", __LINE__);
            return -1;
        }
    }

    fd[0] = in_fd;
    fd[1] = out_fd;
    return 0;
}

// Retuns 0 if a command was executed. The command must not
// contain a pipe or a semi-colon.
int exec_single_command(char *command) {
    // TODO: output in a file is not correct

    int fd[2]; // in, out

    if (parse_file_redirection(fd, command) != 0) {
        fprintf(stderr, "Error: Could not parse file redirections\n");
        return -1;
    };

    char **args = parse_command_with_spaces(command);

    if (exec_internal_command(args) == 0) {
        free(args);

        return 0;
    }

    int pid = fork();

    if (pid == 0) {
        if (fd[0] != -1) {
            dup2(fd[0], STDIN_FILENO);
            fsync(fd[0]);
        }
        if (fd[1] != -1) {
            dup2(fd[1], STDOUT_FILENO);
            fsync(fd[1]);
        }

        int status = execvp(args[0], args);
        if (status == -1) {
            fprintf(stderr, "Error: Could not execute the command\n");
        }
        return 0;
    }
    else {
        int status = 0;
        wait(&status);
        free(args);
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
// separated by `;`q
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

void get_commands() {}

int main() {
    char *current_dir_buf = (char *)malloc(sizeof(char) * 200);

    char *command = (char *)malloc(sizeof(char) * 500);

    sigaction(SIGINT,
              &(struct sigaction){.sa_handler = sigint_handler},
              NULL);

    while (1) {
        printf(COLOR "%s $ " RESET, getcwd(current_dir_buf, 200));
        int status = scanf(" %[^\n]s", command);

        if (status == EOF) {
            if (errno == EINTR) {
                continue; // read operation interrupted by signal
            }
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
