#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>



#define READ_END	0
#define WRITE_END	1


char** splitArgument(char* str) {
    char *token;
    char** result = malloc(sizeof(char*)^2);
    char tmp[BUFSIZ];
    strcpy(tmp, str);


    token = strtok(tmp, " ");
    int index = 0;

    int inputIndex = 0;
    while (token != NULL) {
        //Removes & from the argument
        if (strncmp(token, "&", 1) == 0) {
            token = strtok(NULL, " ");
            continue;
        }

        //Removes < and output/input file directory from the argument
        if (strncmp(token, "<", 1) == 0 || strncmp(token, ">", 1) == 0 ) {
            inputIndex = 1;
            token = strtok(NULL, " ");
            continue;
        }
        if (inputIndex == 1) {
            token = strtok(NULL, " ");
            inputIndex++;
            continue;
        }
        
        result[index] = token;
        token = strtok(NULL, " ");
        index++;
    }
    return result;
    

}

char* getInputOutputPath(char* command) {
    
    char *token;
    char* result[BUFSIZ];
    char tmp[BUFSIZ];
    strcpy(tmp, command);

    token = strtok(tmp, " ");

    int running = 0;
    while (running != 2) {
        if (running == 1) {
            strcpy(result, token);
            running++;
        }
        if (strncmp(token, "<", 1) == 0 || strncmp(token, ">", 1) == 0) {
            running = 1;
        }
        token = strtok(NULL, " ");
    }
    return result;
}


int main(void) {
    int running = 1;


    char command[BUFSIZ];
	char read_cmd[BUFSIZ];
    char previousCommand[BUFSIZ];
    

    memset(read_cmd, 0, BUFSIZ * sizeof(char));
    memset(command, 0, BUFSIZ * sizeof(char));
    memset(previousCommand, 0, BUFSIZ * sizeof(char));

    int cmdIndex = 0;

    while (running) {
        printf("osh> ");
        fflush(stdout);

        fgets(command, BUFSIZ, stdin);
        command[strlen(command) - 1] = '\0';


        pid_t pid;
        int fd[2];



        /* create the pipe */
        if (pipe(fd) == -1) {
            fprintf(stderr,"Pipe failed");
            return 1;
        }

        /* now fork a child process */
        pid = fork();

        if (pid < 0) {
            fprintf(stderr, "Fork failed");
            return 1;
        }
        //Parent process
        if (pid > 0) {  

            //Handling previous command.
            if(strncmp(command, "!!", 2) == 0) {

                if (previousCommand[0]) {
                    strcpy(command, previousCommand);

                } else {
                    printf("No command in command history.\n");
                }
                
            }
            //Copy current command to previousCommand.
            strcpy(previousCommand, command);

            //Handling exit command.
            if (strncmp(command, "exit()", 6) == 0) {
                printf("Exiting Unix Shell\n");
                exit(0);
                return 1;
            }

            write(fd[WRITE_END], command, strlen(command)+1); 

            if(strstr(command, "&") == NULL) { //concurrency
                wait(NULL);
            }
        }
        else { //Child process

            //Reads read_cmd from the pipe.
            read(fd[READ_END], read_cmd, BUFSIZ);


            // printf("new previousCommand: %s\n", previousCommand);
            int file[2];
            if(strstr(read_cmd, ">") != NULL) { //output

                char* path = getInputOutputPath(read_cmd);
                file[0] = open(path, O_CREAT | O_WRONLY | O_TRUNC);
                dup2(file[0], STDOUT_FILENO);
                
            }
            if(strstr(read_cmd, "<") != NULL) { //input from file
                char* path = getInputOutputPath(read_cmd);
                file[1] = open(path, O_RDONLY);
                char input_cmd[BUFSIZ];
                memset(input_cmd, 0, BUFSIZ * sizeof(char));

                read(file[1], input_cmd, BUFSIZ);

                char** inputArgs = malloc(sizeof(char*)^2);
                inputArgs = splitArgument(input_cmd);
                close(fd[READ_END]);

                //close pipe end
                close(fd[WRITE_END]);
                close(file[0]);
                close(file[1]);
                execvp(inputArgs[0], inputArgs);
                exit(0);
            } else {
            char** args = malloc(sizeof(char*)^2);

            args = splitArgument(read_cmd);

            //close readpipe
            close(fd[READ_END]);

            //close pipe end
            close(fd[WRITE_END]);
            
            //Execute command.
            execvp(args[0], args);

            //Close file descriptors
            close(file[0]);
            close(file[1]);
            exit(0);
            }
        }
    }
    return 0;
}