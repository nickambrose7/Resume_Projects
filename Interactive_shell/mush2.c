#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "mush.h"
#include <sys/types.h>
#include <pwd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>


#define TRUE 1
#define FALSE 0
#define READ_END 0 
#define WRITE_END 1

#define MAX_LINE_LEN 100
int interrupt = FALSE;
void handler(int sig_num) {
    interrupt = TRUE;
    fflush(stdout);
}

void cd(clstage s) {
    /* 0 on success, -1 on failure */
    struct passwd* pw;
    char * home = (char *) malloc(MAX_LINE_LEN);
    if (s->argc == 1) { /* only given one arg.*/
        if ((home = getenv("HOME")) != NULL) { /* try getting env var*/
            if (chdir(home) == -1) {
                perror("cd");
            }
        }
        else if ((pw = getpwuid(getuid()))) {
            if (chdir(pw->pw_dir) == -1) {
                perror("cd");
            } 
        }
        else {
            fprintf(stderr, "unable to determine home directory\n");
            return;
        }
    }
    else if (s->argc == 2) { /* two args, we were given the path*/
        if (chdir(s->argv[1]) == -1) {
            perror("cd");
        }
    }   
    else {
        fprintf(stderr, "Wrong number of arguments for cd\n");
    }

}

void executeProg(clstage c) {
    pid_t pid = 0;
    int wstatus;
    struct stat s;
    int inputfd, outputfd, svstdin, svstdout;
    if (c->inname) { /* not taking input from stdin */
        if ((inputfd = open(c->inname, O_RDONLY, 0666)) < 0){ /* open input file*/
            perror("open"); /* invalid input file, return and next prompt*/
            return;
        }
        /* save our stdin before replacing it with the desired outfile */
        if ((svstdin = dup(STDIN_FILENO)) < 0 ) { perror("dup"); }
        /* below, dup2 will close our stdin fd, and make is so our new stdin 
        is now the inputfd */
        if (dup2(inputfd, STDIN_FILENO) < 0) { perror("dup2");}
    }
    if (c->outname) { /* not putting output to stdout */
        if ((outputfd = open(c->outname, O_RDWR | O_TRUNC | O_CREAT, 0666)) < 0){
            perror("open"); /* invalid output file*/
            return;
        }
        /* save our stdout */
        if ((svstdout = dup(STDOUT_FILENO)) < 0) {perror("dup"); }
        if (dup2(outputfd, STDOUT_FILENO) < 0) { perror("dup2"); }

    }
    if ((pid = fork()) < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0){ /* parent */
        pid = wait(&wstatus);
    }
    else { /* child */
        if (execvp(c->argv[0], c->argv) < 0) {
            if (stat(c->argv[0], &s) < 0) {
                fprintf(stderr, "%s: No such file or directory\n", c->argv[0]);
                exit(EXIT_FAILURE); /* exit from child */
            }
            else {
                fprintf(stderr, "%s: Permission denied\n", c->argv[0]);
                exit(EXIT_FAILURE); /* exit from child */
            }
        }
    }
    if (c->inname) { /* must reset our correct stdin */
        close(inputfd);
        dup2(svstdin, STDIN_FILENO);
        close(svstdin);
    }
    if (c->outname) { /* must reset our correct stdout */
        close(outputfd);
        fflush(stdout);
        dup2(svstdout, STDOUT_FILENO);
        close(svstdin);
    }
}

void executePipeline(pipeline p, int pipelen) {
    /* we know we have at least two programs that need to be executed
    via a pipeline if we are in this function */
    int old[2], next[2]; /* old pipe and next pipe */
    pid_t pid; 
    int i, inputfd, outputfd, svstdin, svstdout;
    clstage c = p->stage;
    sigset_t mask;
    
    /* ensure we are taking input from the correct file */
    if (c->inname) { /* not taking input from stdin */
        if ((inputfd = open(c->inname, O_RDONLY, 0666)) < 0){ /* open input file*/
            perror("open"); /* invalid input file, return and next prompt*/
            return;
        }
        /* save our stdin before replacing it with the desired outfile */
        if ((svstdin = dup(STDIN_FILENO)) < 0 ) { perror("dup"); }
        /* below, dup2 will close our stdin fd, and make is so our new stdin 
        is now the inputfd */
        if (dup2(inputfd, STDIN_FILENO) < 0) { perror("dup2");}
    }
    if (c->outname) { /* not putting output to stdout */
        if ((outputfd = open(c->outname, O_RDWR | O_TRUNC | O_CREAT, 0666)) < 0){
            perror("open"); /* invalid output file*/
            return;
        }
        /* save our stdout */
        if ((svstdout = dup(STDOUT_FILENO)) < 0) {perror("dup"); }
        if (dup2(outputfd, STDOUT_FILENO) < 0) { perror("dup2"); }

    }
    /* block all signals before we launch children */
    sigfillset(&mask);
    sigprocmask(SIG_SETMASK, &mask, NULL);
    
    /* get the pipes going */
    
    if (pipe(old)) { 
        perror("old pipe");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < pipelen; i++) { /* pipelen is the amt of progs to run*/
        c = p->stage + i; /* make sure we are on right stage, SKETCHY*/
        if (i < pipelen - 1) { 
        /* we have not reached the last prog yet, so we can create the next pipe.*/
            if (pipe(next)) {
                perror("next pipe");
                exit(EXIT_FAILURE);
            } /* we have two pipes open after this stage*/
        }

        /* launch our process */

        if ((pid = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (!pid) { /* child */
            if (-1 == dup2(old[READ_END], STDIN_FILENO)) {
            /* make pipe read output of this program */
                perror("dup2 old");
            }
            /* if we are NOT one away from the end */
            if (i < pipelen - 1) {
                if (-1 == dup2(next[WRITE_END], STDOUT_FILENO)) {
                    perror("dup2 new");
                }
            }
            close(old[0]); /* clean up */
            close(old[1]);
            close(next[0]);
            close(next[1]);
            /* exec the program */
            if (execvp(c->argv[0], c->argv) == -1) {
                perror("execvp");
            }
        }
        /* parent */
        close(old[0]);
        close(old[1]);
        old[0] = next[0];
        old[1] = next[1];
    }
    /* launched all children, so unblock signals */
    sigemptyset(&mask);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    while(i--) {
        if (-1 == wait(NULL)) { /* wait for children to terminate*/
            if (errno == EINTR) {
                fprintf(stderr, "Interrupted wait, do not count as a terminated child\n");
                i++;
            }
            else {
                perror("Wait");
            }
        }
    }

    /* reset our stdin and out in the case we were given input or output files*/

    if (c->inname) { /* must reset our correct stdin */
        close(inputfd);
        dup2(svstdin, STDIN_FILENO);
        close(svstdin);
    }
    if (c->outname) { /* must reset our correct stdout */
        close(outputfd);
        fflush(stdout);
        dup2(svstdout, STDOUT_FILENO);
        close(svstdin);
    }

}

void run_programs(pipeline p) {
    /* runs all the programs in the pipeline, is sure to close all fds
    that need to be closed, free everything that needs to be freed, and 
    report on any erros in the programs that we try to run. */
    if (p->length > 1) { /* we are working with a pipeline, need to handle that*/
        executePipeline(p, p->length); /* offset to the right stage */
    }
    else if (p->length == 1) { /* just need to execute one program*/
        executeProg(p->stage);
    }
    else {
        fprintf(stderr, "error with pipeline");
    }   
}

int main(int argc, char * argv[]) {
    char* line = NULL; /* parser will malloc for me */
    pipeline p; /* this is a pointer, just used typedef to hide that*/
    FILE* file = NULL; /* used for stdio, the arg for our readLongLine function*/
    struct sigaction sa;
    if (argc == 1) {
        /* interactive mode */
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = handler;
        sigaction(SIGINT, &sa, NULL);
        if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) {
            printf("8-P ");
        }
        else {
            fprintf(stderr, "No file given, but not ttys so idk what to do");
        }
        while ((!feof(stdin))) {
            /* functions to do what we need for the given command */
            if (((line = readLongString(stdin)) != NULL) && !interrupt) {
                if ((p = crack_pipeline(line)) != NULL) {
                    if (strcmp(p->stage->argv[0], "cd") == 0) { /* check for cd only */
                    cd(p->stage); 
                    }
                    else if (p) { /* if p is not null*/
                        run_programs(p);
                    }
                    else {
                        fprintf(stderr, "invalid command\n");
                    }
                    free(line);
                    free_pipeline(p);
                    fflush(stdout);
                    printf("8-P ");
                }
                else { /* we were just given a new line */
                    printf("8-P ");
                    free(line);
                }
            }
            else if (!line && interrupt) { /* interrup was gvn on command line*/
                printf("\n8-P ");
                interrupt = FALSE; /* caught the interrupt */
            }
            else {
                if (!feof(stdin)) { /* don't print promt after EOF*/
                    printf("8-P ");
                }
                interrupt = FALSE; /* interrupted sleep, dont need to handle */
            }
        }  
    }
    else if (argc == 2) {
        /* batch mode (read from file) */
        /* make sure file is valid */
        if (!(file = fopen(argv[1], "r"))) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        while ((line = readLongString(file)) != NULL) { /* Not EOF or ^C*/
            if ((p = crack_pipeline(line)) != NULL) {
                if (strcmp(p->stage->argv[0], "cd") == 0) { /* check for cd only */
                    cd(p->stage); 
                }
                else if (p) { /* if p is not null*/
                    run_programs(p);
                }
                else {
                    fprintf(stderr, "invalid command\n");
                }
                free(line);
                free_pipeline(p);
                fflush(stdout);
            }
            else { /* only given new line, just read next line*/
                free(line);
            }
        }
        fclose(file);   
    }
    else {
        fprintf(stderr, "Too many arguments\n");
        exit(EXIT_FAILURE);
    }
    return 0; /* reached EOF, so we are shell can quit*/
}
