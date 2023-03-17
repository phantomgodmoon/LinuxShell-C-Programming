/*
        COMP3511 Spring 2023 
        PA1: Simplified Linux Shell (myshell)

        Your name:Leung Pak Hei
        Your ITSC email: phleungaf@connect.ust.hk 

        Declaration:

        I declare that I am not involved in plagiarism
        I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks. 

    */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LEN 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"

#define MAX_PIPE_SEGMENTS 8

#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the  Standard file descriptors here
#define STDIN_FILENO 0  // Standard input
#define STDOUT_FILENO 1 // Standard output

// Define some templates for printf
#define TEMPLATE_MYSHELL_START "Myshell (pid=%d) starts\n"
#define TEMPLATE_MYSHELL_END "Myshell (pid=%d) ends\n"

void process_cmd(char *cmdline);

void show_prompt();

int get_cmd_line(char *cmdline);

void read_tokens(char **argv, char *line, int *numTokens, char *token);

int main()
{
    char cmdline[MAX_CMDLINE_LEN];
    printf(TEMPLATE_MYSHELL_START, getpid());
    while (1)
    {
        show_prompt();
        if (get_cmd_line(cmdline) == -1)
            continue;

        if (strcmp(cmdline, "exit") == 0)
        {
            printf(TEMPLATE_MYSHELL_END, getpid());
            exit(0);
        }
        pid_t pid = fork();
        if (pid == 0)
        {
            process_cmd(cmdline);
            exit(0);
        }
        else
        {
            wait(0);
        }
    }
    return 0;
}

char *trim(char *str)
{
    // Remove leading spaces
    while (*str == ' ')
        str++;

    // Remove trailing spaces
    char *end = str + strlen(str) - 1;
    while (end > str && *end == ' ')
        end--;
    *(end + 1) = '\0';

    return str;
}

void removeChar(char *str, char charToRemmove)
{
    int i, j;
    int len = strlen(str);
    for (i = 0; i < len; i++)
    {
        if (str[i] == charToRemmove)
        {
            for (j = i; j < len; j++)
            {
                str[j] = str[j + 1];
            }
            len--;
            i--;
        }
    }
}

int find_arg_index(char **args, char *arg_to_find)
{
    int index = -1;
    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], arg_to_find) == 0)
        {
            index = i;
            break;
        }
    }
    return index;
}

int parse_args(char *cmdline, char **args)
{
    int nargs = 0;
    char *token = strtok(cmdline, " \t\n");
    while (token != NULL && nargs < 9 - 1)
    {
        args[nargs++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[nargs] = NULL;
    return nargs;
}

void process_cmd(char *cmdline)
{

    char *pipe_segments[MAX_ARGUMENTS_PER_SEGMENT];
    int num_pipe_segments = 0;

    // Pipe case
    if (strstr(cmdline, "|") != NULL) {
        read_tokens(pipe_segments, cmdline, &num_pipe_segments, "|");

        int pfds[num_pipe_segments - 1][2];
        for (int i = 0; i < num_pipe_segments; i++) {
            if (pipe(pfds[i]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        char *arg_content[num_pipe_segments][MAX_ARGUMENTS_PER_SEGMENT];
        int num_subsegments[num_pipe_segments];
        for (int i = 0; i < num_pipe_segments; i++) {
            num_subsegments[i] = 0;
            read_tokens(arg_content[i], pipe_segments[i], &num_subsegments[i], " ");
            arg_content[i][num_subsegments[i]] = '\0';
        }

        for (int i = 0; i < num_pipe_segments; i++) {
            int num_subsegments_executed = 0;
            while (num_subsegments_executed < num_subsegments[i]) {
                int num_args_executed = 0;
                char *subsegment[MAX_ARGUMENTS_PER_SEGMENT];
                while (num_args_executed < MAX_ARGUMENTS_PER_SEGMENT) {
                    if (num_subsegments_executed >= num_subsegments[i]) {
                        break;
                    }
                    subsegment[num_args_executed] = arg_content[i][num_subsegments_executed];
                    num_args_executed++;
                    num_subsegments_executed++;
                }
                subsegment[num_args_executed] = NULL;

                if (i == 0) {
                    pid_t pid = fork();
                    if (pid == 0) {
                        close(1);
                        dup2(pfds[i][1], 1);
                        close(pfds[i][0]);
                        execvp(subsegment[0], subsegment);
                    }
                    else {
                        close(0);
                        dup2(pfds[i][0], 0);
                        close(pfds[i][1]);
                        wait(0);
                    }
                }
                else if (i == num_pipe_segments - 1) {
                    close(0);
                    dup2(pfds[i - 1][0], 0);
                    close(pfds[i - 1][1]);
                    execvp(subsegment[0], subsegment);
                }
                else {
                    pid_t pid = fork();
                    if (pid == 0) {
                        close(0);
                        dup2(pfds[i - 1][0], 0);
                        close(pfds[i - 1][1]);
                        close(1);
                        dup2(pfds[i][1], 1);
                        close(pfds[i][0]);
                        execvp(subsegment[0], subsegment);
                    }
                    else {
                        close(0);
                        dup2(pfds[i][0], 0);
                        close(pfds[i][1]);
                        wait(0);
                    }
                }
            }
        }
    }
    // Other cases, including file operation and the native commands
    else
    {
        char *args[9];
        int nargs = parse_args(cmdline, args);

        // Output
        int output_fd = STDOUT_FILENO;
        for (int i = 0; i < nargs; i++)
        {
            if (strcmp(args[i], ">") == 0)
            {
                output_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (output_fd == -1)
                {
                    perror(args[i + 1]);
                    return;
                }
                args[i] = NULL;
                args[i + 1] = NULL;
                nargs -= 2;
                break;
            }
        }

        // Input
        int input_fd = STDIN_FILENO;
        for (int i = 0; i < nargs; i++)
        {
            if (strcmp(args[i], "<") == 0)
            {
                // Open the input file
                input_fd = open(args[i + 1], O_RDONLY);
                if (input_fd == -1)
                {
                    perror(args[i + 1]);
                    return;
                }

                args[i] = NULL;
                args[i + 1] = NULL;
                nargs -= 2;
                break;
            }
        }

        if (input_fd != STDIN_FILENO)
        {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != STDOUT_FILENO)
        {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

            // Execute the command
        execvp(args[0], args);
           
    }
}

void read_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

void show_prompt()
{
    printf("phleungaf> ");
}

int get_cmd_line(char *cmdline)
{
    int i;
    int n;
    if (!fgets(cmdline, MAX_CMDLINE_LEN, stdin))
        return -1;
    // Ignore the newline character
    n = strlen(cmdline);
    cmdline[--n] = '\0';
    i = 0;
    while (i < n && cmdline[i] == ' ')
    {
        ++i;
    }
    if (i == n)
    {
        // Empty command
        return -1;
    }
    return 0;
}
