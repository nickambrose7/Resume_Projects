#include <netinet/in.h>
#include <stdint.h>
#include <talk.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <poll.h>
#include <sys/types.h>
#include <pwd.h>
#include <ctype.h>

#define TRUE 1
#define FALSE 0
#define MAXLEN 1000
#define MAXANSLEN 3
#define LOCAL 0
#define REMOTE (LOCAL + 1)
#define MAXCONNECTIONS 100

/*maximum length to which the
queue of pending connections for sockfd may grow */
#define DEFAULT_BACKLOG 100

int strcicmp(char const *a, char const *b)
/* compare two strings, case insensitive*/
{
    for (;; a++, b++) {
        int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (d != 0 || !*a)
            return d;
    }
}

void talkserver(int newsock, int vflag, int Nflag) {
    int clients[MAXCONNECTIONS];

    if (!Nflag) {
        start_windowing(); /* start the ncurses window*/
    }
    if (vflag) {
        set_verbosity(vflag);
    }
}
void talk(int newsock, int vflag, int Nflag) {
    int rlen, mlen;
    char buff[MAXLEN];
    struct pollfd fds[REMOTE + 1];
    if (!Nflag) {
        start_windowing(); /* start the ncurses window*/
    }
    if (vflag) {
        set_verbosity(vflag);
    }

    fds[LOCAL].fd = STDIN_FILENO; /* still looking for stdin*/
    fds[LOCAL].events = POLLIN;
    fds[LOCAL].revents = 0;
    fds[REMOTE] = fds[LOCAL];
    fds[REMOTE].fd = newsock; /* need to see when server sends something*/

    do {
        /* check for any events */
        if (poll(fds, sizeof(fds)/sizeof(struct pollfd), -1) == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }
        if (fds[LOCAL].revents & POLLIN) {
            update_input_buffer();
            if (has_whole_line()) { /* we have a whole line*/
                rlen = read_from_input(buff, MAXLEN); /* read the line*/
                if (send(newsock, buff, rlen, 0) < rlen) { /* send it to server*/
                    fprintf(stderr, "sent less than we wanted");
                    exit(EXIT_FAILURE);
                }
            }
            if (has_hit_eof()) {
                if (vflag){
                    printf("client: hit EOF, program should stop");
                }
                /* notify that we reached EOF*/
                if (send(newsock, "^D", strlen("^D"), 0) < rlen) { 
                    fprintf(stderr, "sent less than we wanted");
                    exit(EXIT_FAILURE);
                }
            }
        }

        if (fds[REMOTE].revents & POLLIN) {
            /* recive a line and print it to the ncurses window */
            if ((mlen =  recv(newsock, buff, sizeof(buff), 0)) == -1) {
                /* recv the message */ 
                perror("recv");
                exit(EXIT_FAILURE); 
            } 
            buff[mlen] = '\0';
            if (!strcmp(buff, "^D")) {
                fprint_to_output("Connection closed. ^C to terminate.");
            }
            else{
                write_to_output(buff, mlen); /* write it to the window*/
            }
        } 
    } while(!has_hit_eof()); /* keep talking until we hit EOF */  
    stop_windowing();
}

void mytalkserver(int port, int vflag, int aflag, int Nflag) {
    int sockfd, newsock, mlen, len;
    struct sockaddr_in sa, newsockinfo;
    socklen_t slen;
    char buff[MAXLEN + 1];
    char * ans = (char*) malloc(MAXANSLEN + 1);
    /* create socket TCP*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(EXIT_FAILURE);
    } 
    else if (vflag) {
        printf("Server: successfully created socket\n");
    }
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port); /* use gvn port*/
    sa.sin_addr.s_addr = htonl(INADDR_ANY); /* all local interfaces*/
    if (bind(sockfd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    else if (vflag) {
        printf("Server: successfully bound to address\n");
    }

    if (listen(sockfd, DEFAULT_BACKLOG) == -1) {
        perror("listen"); /* listen for a connection */
        exit(EXIT_FAILURE);
    }
    else if (vflag) {
        printf("Server: listening.....\n");
    }
    slen = sizeof(newsockinfo); /* accept the connection then check if its okay with server*/
    if ((newsock = accept(sockfd, (struct sockaddr *) &newsockinfo, &slen)) == -1) {
    /* newsock is the sd that is connected to the client that called connect*/
        perror("accept");
        exit(EXIT_FAILURE);
    }
    else if (vflag) {
        printf("Server: accepted connection, still need to check if okay with server.\n");
    }
    /* get the username*/
    mlen = recv(newsock, buff, sizeof(buff), 0); /* get name */
    buff[mlen] = '\0'; /* must null terminate */

    if (!aflag) { /*want to ask about accepting connection*/
        printf("Mytalk request from %s. Accept (y/n)? ", buff);
        scanf("%s", ans);
        if ((len = strnlen(ans, MAXANSLEN)) == MAXANSLEN) {
        ans[MAXANSLEN] = '\0'; /* null terminate at the end */
        }
        else {
            ans[len] = '\0'; /* null term at len*/
        }
    }
    else { /* autmatically accept connection */
        ans = "Yes";
    }
    
    if (!strcicmp(ans, "Yes") || !strcicmp(ans, "Y")) {
        /* send an ok to the client */
        if (send(newsock, "ok", strlen("ok"), 0) == -1) {
            perror("send");
            exit(EXIT_FAILURE);
        }
        if (vflag) {
            printf("Server: is okay with the connection, leave it.\n");
            printf("ready to chat\n");
        }
        /* send and recieve messages */
        talk(newsock, vflag, Nflag);

    }
    else {
        if (send(newsock, "not ok", strlen("not ok"), 0) == -1) {
            perror("send");
            exit(EXIT_FAILURE);
        }
    }
    free(ans);
    /* close connection */
    close(sockfd);
    close(newsock);
}



void mytalkcilent(char* hostname, int port, int vflag, int aflag, int Nflag) {
    int sockfd, mlen;
    struct sockaddr_in sa;
    struct hostent * hostent;
    char buf[MAXLEN + 1];
    uid_t userid;
    struct passwd * pw;
    if (!(hostent = gethostbyname(hostname))) {
        perror("hostent:"); /* get host info */
        exit(EXIT_FAILURE);
    }
    else if (vflag) {
        printf("client: got host name\n");
        printf("Hostname: %s\n", hostent->h_name);
    }
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(EXIT_FAILURE);
    } /* create socket TCP*/
    else if (vflag) {
        printf("client: successfully created socket\n");
    }
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = *(uint32_t*) hostent->h_addr_list[0];
    printf("Waiting for response from %s\n", hostname);
    /* connect the client socket to the server*/
    if (connect(sockfd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("connect: ");
        exit(EXIT_FAILURE);
    }
    else {
        
    }
    userid = getuid();
    pw = getpwuid(userid);
    /* send the username */
    if (send(sockfd, pw->pw_name, strlen(pw->pw_name), 0) == -1) {
            perror("send");
            exit(EXIT_FAILURE);
        }
    mlen = recv(sockfd, buf, sizeof(buf), 0); /* get the ok*/
    if (vflag) {
        printf("waiting for ok from server\n");
    }
    buf[mlen] = '\0'; /* we can't be sure of null termination.*/
    if (!strcmp(buf, "ok")) {
        /* send and recieve messages */
        talk(sockfd, vflag, Nflag);
    }
    else {
        printf("%s declined connection\n", hostname);
    }
    close(sockfd);
}

int main(int argc, char* argv[]) {
    int vflag = FALSE; /* option flags*/
    int aflag = FALSE;
    int Nflag = FALSE;
    int host = FALSE;
    struct sockaddr_in s_in;
    int o; /* option*/
    int i; /* index */
    char* hostname = NULL;
    long int port; /* holds the given port number */
    while ((o = getopt(argc, argv, "vaN")) != -1) { /* get args */
        switch (o)
        {
        case 'v':
            vflag = TRUE;
            break;
        case 'a':
            aflag = TRUE;
            break;
        case 'N':
            Nflag = TRUE;
            break;
        default:
            break;
        }
    }

    if (optind == argc) { /* no port number given */
        fprintf(stderr, "not enough arguments");
        exit(EXIT_FAILURE);
    }
    for (i = optind; i < argc; i++) { /* go through args */
        if (strtol(argv[i], NULL, 10) == 0) {
            hostname = argv[i]; /* we were given a string */
            host = TRUE;

        }
        else {
            port = strtol(argv[i], NULL, 10); /* remember to convert to a network long*/
        }
    }
    if (host) {
        mytalkcilent(hostname, port, vflag, aflag, Nflag);
    }
    else {
        mytalkserver(port, vflag, aflag, Nflag);
    }
    return 0;  
}
