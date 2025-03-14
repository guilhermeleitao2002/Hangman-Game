// Author: Guilherme Leitão 99951 & Diogo Costa 96182

// Libraries
# include <unistd.h>
# include <string.h>
# include <stdlib.h>
# include <sys/types.h>
# include <sys/time.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <stdio.h>
# include <signal.h>

// Constants
# define MAX_BUFFER_SIZE 1024
# define STATUS_SIZE 4
# define SIZE 128
# define PORT_SIZE 6
# define PLAYER_ID 7
# define ATTEMPTS_SIZE 2
# define ZERO 0
# define ONE 1
# define MAX_TIME 5

// Global variables
int fd_udp, fd_tcp;

/*
*   Handles the SIGINT signal
*/
void leave() {

    printf("\nPlayer is exitting...\n");

    close(fd_tcp);
    close(fd_udp);

    exit(0);

}

/*
*   Reads the initial start of the program
*   argc: number of arguments
*   argv: arguments
*   port: port to be used
*   gsip: game server ip
*/
void read_entry(int argc, char **argv, char port[PORT_SIZE], char gsip[SIZE]) {

    if(argc == 1)

        return;

    else if(argc == 3)

        if(strcmp(argv[1], "-p") == 0)

            strcpy(port, argv[2]);

        else if(strcmp(argv[1], "-n") == 0)

            strcpy(gsip, argv[2]);

        else {

            printf("Invalid argument\n");
            leave();

        }

    else if(argc == 5)

        if(strcmp(argv[1], "-p") == 0) {

            strcpy(port, argv[2]);

            if(strcmp(argv[3], "-n") == 0)

                strcpy(gsip, argv[4]);

            else {

                printf("Invalid argument\n");
                leave();

            }

        } else if(strcmp(argv[1], "-n") == 0) {

            strcpy(gsip, argv[2]);

            if(strcmp(argv[3], "-p") == 0)

                strcpy(port, argv[4]);

            else {

                printf("Invalid argument\n");
                leave();

            }

        } else {

            printf("Invalid argument\n");
            leave();

        }

}

/*
*   Returns a word in lowercase
*   word: string to be converted
*/
char* to_lowercase(char word[SIZE]) {

    int i = 0;

    while(word[i] != '\0'){

        if(word[i] >= 'A' && word[i] <= 'Z')

            word[i] = word[i] + 32;

        i++;

    }

    return word;

}

/*
*   formats the game word with a given number of underscores
*   len: number of underscores
*/
void build_word(int len, char *word) {

    for(int i = 0; i < len; i++)

        word[i] = '_';

    word[len] = '\0';

}

/*
*   updates the game word with the letter in the respective positions
*   positions: number of positions where the letter appears
*   letter: letter to be inserted
*   buffer: buffer with the positions where the letter appears
*/
void update_game_word(char* word, int positions, char letter, char *buffer, int final){

    int i = 0;
    char *token = strtok(buffer, " ");

    if(!final){
    
        // "breaks" the string into several words and "jumps" to the forth
        for(int j = 0; j < 4; j++)

            token = strtok(NULL, " ");

        // inserts the letter in every respective position
        while(i < positions) {

            int position;

            sscanf(token, "%d", &position);
            word[position - 1] = letter;
            i++;

            token = strtok(NULL, " ");

        }

    } else {

        int n = strlen(word);
        for(i = 0; i < n; i++)

            if(word[i] == '_')
            
                word[i] = letter;

    }

}

/*
*   implements a personalized write function to guarantee that every bit of information is sent
*   fd: file descriptor
*   buff: buffer to be sent
*/
void my_write(int fd, char buff[MAX_BUFFER_SIZE]) {

    ssize_t nleft = strlen(buff), nwritten;
    char *ptr = strcpy(buff, buff);

    // loop until there is nothing left to write
    while(nleft > 0) {

        nwritten = write(fd, ptr, nleft);
        if(nwritten <= 0) leave();                              // Error

        nleft -= nwritten;
        ptr += nwritten;

    }

}

/*
*   implements a personalized read function to guarantee that every bit of information is received,
*   and to read the first four arguments of the message
*   fd: file descriptor
*   buff: buffer to be received
*/
void my_read1(int fd, char buff[MAX_BUFFER_SIZE]) {

    ssize_t nread = 0;
    int space_count = 0;
    char *ptr = buff;

    // loop until the first four arguments are read
    while(1) {

        nread = read(fd, ptr, 1);
        if(nread < 0) leave();                                                          // Error

        if(ptr[0] == ' ') space_count++;

        ptr += nread;

        if(space_count == 4) break;

    }

}

/*
*   implements a personalized read function to guarantee that every bit of information is received
*   and to read the rest of the message content (text or image)
*   fd: file descriptor
*   size: size of the buffer to be received
*   filename: name of the file to be received
*   ptr: pointer to the buffer to be received
*/
void my_read2(int fd, int size, char filename[SIZE], char *ptr) {

    ssize_t nread = 0;
    int space_count = 0, initial_size = size;

    // loop until the whole message is read
    while(size > 0) {

        nread = read(fd, ptr, size);
        if(nread < 0) leave();                                                          // Error

        ptr += nread;
        size -= nread;

    }

    ptr -= initial_size;

}

/*
*   checks if the connection has timed out
*   fd: file descriptor
*/
int time_limit_exceeded(int fd) {

    fd_set set;
    struct timeval timeout;

    // initialize the set and the timeout for connection listening
    FD_ZERO(&set);
    FD_SET(fd, &set);
    timeout.tv_sec = MAX_TIME;
    timeout.tv_usec = ZERO;

    if(select(fd + 1, &set, NULL, NULL, &timeout) == 0) {

        printf("Something happened, repeat the last command!\n");
        return 1;

    }

    return 0;

}

/*
*   prints a manual like description of the game
*/
void help() {

    printf("\n\tstart/sg <plid> - to start a game\n");
    printf("\tplay/pl <letter> to guess a letter\n");
    printf("\tguess/gw <word> to guess a word\n");
    printf("\tscoreboard/sb - to see the top 10 plays\n");
    printf("\thint/h - to request a visual aid\n");
    printf("\tstate/st - to ask for the status of the last game\n");
    printf("\tquit - to quit the game\n");
    printf("\texit - quits the game and application\n");
    printf("\trev - to reveal the word; quits afterwards\n\n");

}


/*
*   main function
*   argc: number of arguments
*   argv: array of arguments
*/
int main(int argc, char **argv) {

    int errcode_udp, errcode_tcp, word_len, errors_given = 0;
    int max_errors, attempts = 0, positions, quitted = 1, size;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints_udp, *res_udp = NULL, hints_tcp, *res_tcp = NULL;
    struct sockaddr_in addr;
    FILE *fp = NULL;
    char buffer[MAX_BUFFER_SIZE], gsip[SIZE] = "tejo.tecnico.ulisboa.pt", port[PORT_SIZE] = "58062";
    char command[SIZE], word1[SIZE], word2[SIZE], status[STATUS_SIZE], *game_word = NULL, ignore[SIZE];
    char plid[PLAYER_ID] = "------", attempts_c[ATTEMPTS_SIZE], rev_word[SIZE], filename[SIZE];
    struct sigaction act;

    read_entry(argc, argv, port, gsip);

    // set the flags and create the udp socket
    fd_udp = socket(AF_INET, SOCK_DGRAM, 0);                                            // UDP socket
    if(fd_udp == -1) leave();                                                           // Error

    memset(&hints_udp, 0, sizeof(hints_udp));
    hints_udp.ai_family = AF_INET;                                                      // IPv4
    hints_udp.ai_socktype = SOCK_DGRAM;                                                 // UDP socket

    memset(&hints_tcp, 0, sizeof(hints_tcp));
    hints_tcp.ai_family = AF_INET;                                                      // IPv4
    hints_tcp.ai_socktype = SOCK_STREAM;                                                // TCP socket

    errcode_udp = getaddrinfo(gsip, port, &hints_udp, &res_udp);
    if(errcode_udp != 0) leave();                                                       // Error
    
    errcode_tcp = getaddrinfo(gsip, port, &hints_tcp, &res_tcp);
    if(errcode_tcp != 0) leave();                                                       // Error

    // prints the manual
    printf("\n Manual:\n");
    help();

    // when ctrl+c is pressed or segfault detected, the server shuts down and closes the sockets
    act.sa_handler = leave;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

    while(1) {

        // receives command from user
        printf(" > ");
        fgets(command, SIZE, stdin);
        sscanf(command, "%s %s", word1, word2);

        if(strcmp(to_lowercase(word1), "start") == 0 || strcmp(to_lowercase(word1), "sg") == 0){

            if(strlen(word2) != 6){

                printf("Insert a valid student id\n");
                continue;

            }

            memset(buffer, 0, sizeof(buffer));

            // build message
            strcpy(buffer, "SNG ");
            strcat(buffer, word2);
            strcat(buffer, "\n");

            // send message
            n = sendto(fd_udp, buffer, 11, 0, res_udp->ai_addr,res_udp->ai_addrlen);
            if(n == -1) leave();                                                         // Error

            memset(buffer, 0, sizeof(buffer));

            // wait for the server's response
            if(time_limit_exceeded(fd_udp)) continue;

            addrlen = sizeof(addr);
            n = recvfrom(fd_udp, buffer, 14, 0, (struct sockaddr*) &addr, &addrlen);
            if(n == -1) leave();                                                          // Error
            
            // extrapolate parameters from response
            sscanf(buffer, "%s %s %d %d\n", ignore, status, &word_len, &max_errors);

            if(strcmp(status, "OK") == 0) {

                strcpy(plid, word2);
                game_word = (char *) malloc(word_len * sizeof(char) + 1);
                build_word(word_len, game_word);
                quitted = 0;
                printf("New game started. Guess %d letter word: %s\n", word_len, game_word);

            } else

                printf("Game already started\n");

            continue;

        } else if(strcmp(to_lowercase(word1), "exit") == 0) {

            if(quitted) break;

            memset(buffer, 0, sizeof(buffer));

            // build message
            strcpy(buffer, "QUT ");
            strcat(buffer, plid);
            strcat(buffer, "\n");

            // send message
            n = sendto(fd_udp, buffer, 11, 0, res_udp->ai_addr, res_udp->ai_addrlen);
            if(n == -1) leave();                                                         // Error

            memset(buffer, 0, sizeof(buffer));

            // wait for the server's response
            if(time_limit_exceeded(fd_udp)) continue;

            addrlen = sizeof(addr);
            n = recvfrom(fd_udp, buffer, 8, 0, (struct sockaddr*) &addr, &addrlen);
            if(n == -1) leave();                                                          // Error

            // extrapolate parameters from response
            sscanf(buffer, "%s %s\n", ignore, status);

            if(strcmp(status, "OK") == 0) {

                attempts = 0;
                
                free(game_word);
                break;

            }

            printf("Something went wrong\n");

            continue;

        }
        
        // check if a game has been started
        if(strcmp(plid, "------") == 0) {

            printf("You must start a game first\n");
            continue;

        }

        if(strcmp(to_lowercase(word1), "play") == 0 || strcmp(to_lowercase(word1), "pl") == 0) {

            if(strlen(word2) != 1) {

                printf("Must be a letter\n");
                continue;

            }

            memset(buffer, 0, sizeof(buffer));

            // build message
            strcpy(buffer, "PLG ");
            strcat(buffer, plid);
            strcat(buffer, " ");
            strcat(buffer, to_lowercase(word2));
            strcat(buffer, " ");
            sprintf(attempts_c, "%d", attempts + 1);
            strcat(buffer, attempts_c);
            strcat(buffer, "\n");

            // send message
            if(attempts + 1 >= 10)

                n = sendto(fd_udp, buffer, 16, 0, res_udp->ai_addr, res_udp->ai_addrlen);

            else
            
                n = sendto(fd_udp, buffer, 15, 0, res_udp->ai_addr, res_udp->ai_addrlen);

            if(n == -1) leave();                                                         // Error

            memset(buffer, 0, sizeof(buffer));

            // wait for the server's response
            if(time_limit_exceeded(fd_udp)) continue;

            addrlen = sizeof(addr);
            n = recvfrom(fd_udp, buffer, 74, 0, (struct sockaddr*) &addr, &addrlen);
            if(n == -1) leave();                                                          // Error

            // extrapolate parameters from response
            sscanf(buffer, "%s %s %s %d %s\n", ignore, status, ignore, &positions, ignore);

            if(strcmp(status, "OK") == 0) {

                attempts++;

                update_game_word(game_word, positions, word2[0], buffer, ZERO);

                printf("Yes, '%c' is part of the word: %s\n", word2[0], game_word);

            } else if(strcmp(status, "NOK") == 0) {

                attempts++;

                printf("No, '%c' is not part of the word. %d errors remaining.\n", word2[0], max_errors - ++errors_given);

            } else if(strcmp(status, "WIN") == 0) {

                update_game_word(game_word, positions, word2[0], buffer, ONE);

                printf("WELL DONE! You guessed the word %s\n", game_word);

                attempts = 0;
                quitted = 1;
                errors_given = 0;

                free(game_word);

            } else if(strcmp(status, "OVR") == 0) {

                printf("GAME OVER!\n");

                attempts = 0;
                quitted = 1;
                errors_given = 0;

                free(game_word);

            } else if(strcmp(status, "DUP") == 0)

                printf("You already tried '%c'\n", word2[0]);

            else if(strcmp(status, "INV") == 0)

                printf("Repeat the last guess\n");

            else

                printf("Something went wrong!\n");
                    
        } else if(strcmp(to_lowercase(word1), "guess") == 0 || strcmp(to_lowercase(word1), "gw") == 0) {

            memset(buffer, 0, sizeof(buffer));

            // build message
            strcpy(buffer, "PWG ");
            strcat(buffer, plid);
            strcat(buffer, " ");
            strcat(buffer, to_lowercase(word2));
            strcat(buffer, " ");
            sprintf(attempts_c, "%d", attempts + 1);
            strcat(buffer, attempts_c);
            strcat(buffer, "\n");

            // send message
            if(attempts + 1 >= 10)

                n = sendto(fd_udp, buffer, 15 + word_len, 0, res_udp->ai_addr, res_udp->ai_addrlen);

            else

                n = sendto(fd_udp, buffer, 14 + word_len, 0, res_udp->ai_addr, res_udp->ai_addrlen);
            if(n == -1) leave();                                                         // Error

            memset(buffer, 0, sizeof(buffer));

            // wait for the server's response
            if(time_limit_exceeded(fd_udp)) continue;

            addrlen = sizeof(addr);
            n = recvfrom(fd_udp, buffer, 11, 0, (struct sockaddr*) &addr, &addrlen);
            if(n == -1) leave();                                                          // Error

            // extrapolate parameters from response
            sscanf(buffer, "%s %s %s\n", ignore, status, ignore);

            if(strcmp(status, "WIN") == 0) {

                printf("WELL DONE! You guessed: %s\n", word2);

                attempts = 0;
                quitted = 1;
                errors_given = 0;

                free(game_word);

            } else if(strcmp(status, "NOK") == 0) {

                printf("Wrong! %d errors remaining.\n", max_errors - ++errors_given);

                attempts++;

            } else if(strcmp(status, "OVR") == 0) {

                printf("GAME OVER!\n");

                attempts = 0;
                quitted = 1;
                errors_given = 0;

                free(game_word);

            } else if(strcmp(status, "INV") == 0)

                printf("Invalid attempt\n");

            else if(strcmp(status, "DUP") == 0)

                printf("You already tried \"%s\"\n", word2);

            else

                printf("Something went wrong!\n");

        } else if(strcmp(to_lowercase(word1), "scoreboard") == 0 || strcmp(to_lowercase(word1), "sb") == 0) {

            memset(buffer, 0, sizeof(buffer));

            // build message
            strcpy(buffer, "GSB\n");

            // open tcp socket
            fd_tcp = socket(AF_INET, SOCK_STREAM, 0);                                   // TCP socket
            if(fd_tcp == -1) leave();                                                   // Error

            n = connect(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
            if(n == -1) leave();                                                        // Error

            // send message
            my_write(fd_tcp, buffer);

            memset(buffer, 0, sizeof(buffer));

            // wait for the server's response
            if(time_limit_exceeded(fd_tcp)) {

                close(fd_tcp);
                continue;

            }

            my_read1(fd_tcp, buffer);

            // extrapolate parameters from response
            sscanf(buffer, "%s %s %s %d\n", ignore, status, filename, &size);

            if(strcmp(status, "OK") == 0) {

                char file_contents[MAX_BUFFER_SIZE];

                memset(file_contents, 0, sizeof(file_contents));

                // read scoreboard information
                my_read2(fd_tcp, size, filename, file_contents);

                fp = fopen(filename, "w");
                if(fp == NULL) leave();                                                     // Error

                fprintf(fp, "%s", file_contents);
                printf("%s\n", file_contents);
                printf("\nScoreboard saved in file: %s\n", filename);

                fclose(fp);

            } else if(strcmp(status, "EMPTY") == 0)

                printf("No scores yet!\n");

            else

                printf("Something went wrong!\n");

            close(fd_tcp);
        
        } else if(strcmp(to_lowercase(word1), "hint") == 0 || strcmp(to_lowercase(word1), "h") == 0) {

            memset(buffer, 0, sizeof(buffer));

            // build message
            strcpy(buffer, "GHL ");
            strcat(buffer, plid);
            strcat(buffer, "\n");

            // open tcp socket
            fd_tcp = socket(AF_INET, SOCK_STREAM, 0);                                   // TCP socket
            if(fd_tcp == -1) leave();                                                   // Error

            n = connect(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
            if(n == -1) leave();                                                        // Error

            // send message
            my_write(fd_tcp, buffer);

            memset(buffer, 0, sizeof(buffer));

            // wait for the server's response
            if(time_limit_exceeded(fd_tcp)) {

                close(fd_tcp);
                continue;
                
            }

            my_read1(fd_tcp, buffer);

            // extrapolate parameters from response
            sscanf(buffer, "%s %s %s %d", ignore, status, filename, &size);

            if(strcmp(status, "OK") == 0) {

                char *file_contents = (char *) malloc(size * sizeof(char));

                // read image information
                my_read2(fd_tcp, size, filename, file_contents);

                fp = fopen(filename, "w");
                if(fp == NULL) leave();                                                     // Error

                fwrite(file_contents, 1, size, fp);
                printf("\nImage saved in file: %s (%d Bytes)\n", filename, size);

                free(file_contents);
                fclose(fp);

            } else

                printf("Something went wrong!\n");

            close(fd_tcp);

        } else if(strcmp(to_lowercase(word1), "state") == 0 || strcmp(to_lowercase(word1), "st") == 0) {

            memset(buffer, 0, sizeof(buffer));

            // build message
            strcpy(buffer, "STA ");
            strcat(buffer, plid);
            strcat(buffer, "\n");

            // open tcp socket
            fd_tcp = socket(AF_INET, SOCK_STREAM, 0);                                   // TCP socket
            if(fd_tcp == -1) leave();                                                   // Error

            n = connect(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
            if(n == -1) leave();                                                        // Error

            // send message
            my_write(fd_tcp, buffer);

            memset(buffer, 0, sizeof(buffer));

            // wait for the server's response
            if(time_limit_exceeded(fd_tcp)) {

                close(fd_tcp);
                continue;
                
            }

            my_read1(fd_tcp, buffer);

            // extrapolate parameters from response
            sscanf(buffer, "%s %s %s %d\n", ignore, status, filename, &size);

            if(strcmp(status, "ACT") == 0) {

                char file_contents[MAX_BUFFER_SIZE];

                memset(file_contents, 0, sizeof(file_contents));

                // read ongoing game information
                my_read2(fd_tcp, size, filename, file_contents);

                fp = fopen(filename, "w");
                if(fp == NULL) leave();                                                     // Error

                fprintf(fp, "%s", file_contents);
                printf("%s\n", file_contents);
                printf("Ongoing game saved in file: %s\n", filename);

                fclose(fp);

            } else if(strcmp(status, "FIN") == 0) {

                char file_contents[MAX_BUFFER_SIZE];

                memset(file_contents, 0, sizeof(file_contents));

                // read last game information
                my_read2(fd_tcp, size, filename, file_contents);

                fp = fopen(filename, "w");
                if(fp == NULL) leave();                                                     // Error

                fprintf(fp, "%s", file_contents);
                printf("%s\n", file_contents);
                printf("Most recent game saved in file: %s\n", filename);

                fclose(fp);

            } else if(strcmp(status, "NOK") == 0)

                printf("No games yet!\n");

            else

                printf("Something went wrong!\n");

            close(fd_tcp);
        
        } else if(strcmp(to_lowercase(word1), "quit") == 0) {

            memset(buffer, 0, sizeof(buffer));

            // build message
            strcpy(buffer, "QUT ");
            strcat(buffer, plid);
            strcat(buffer, "\n");

            // send message
            n = sendto(fd_udp, buffer, 11, 0, res_udp->ai_addr, res_udp->ai_addrlen);
            if(n == -1) leave();                                                         // Error

            memset(buffer, 0, sizeof(buffer));

            // wait for the server's response
            if(time_limit_exceeded(fd_udp)) continue;

            addrlen = sizeof(addr);
            n = recvfrom(fd_udp, buffer, 8, 0, (struct sockaddr*) &addr, &addrlen);
            if(n == -1) leave();                                                          // Error

            // extrapolate parameters from response
            sscanf(buffer, "%s %s\n", ignore, status);

            if(strcmp(status, "OK") == 0) {

                attempts = 0;
                quitted = 1;

                free(game_word);
                continue;

            } else

                printf("Something went wrong\n");

        } else if(strcmp(to_lowercase(word1), "rev") == 0) {

            memset(buffer, 0, sizeof(buffer));

            // build message
            strcpy(buffer, "REV ");
            strcat(buffer, plid);
            strcat(buffer, "\n");

            // send message
            n = sendto(fd_udp, buffer, 11, 0, res_udp->ai_addr, res_udp->ai_addrlen);
            if(n == -1) leave();                                                         // Error

            memset(buffer, 0, sizeof(buffer));

            // wait for the server's response
            if(time_limit_exceeded(fd_udp)) continue;

            addrlen = sizeof(addr);
            n = recvfrom(fd_udp, buffer, SIZE, 0, (struct sockaddr*) &addr, &addrlen);
            if(n == -1) leave();                                                          // Error

            // extrapolate parameters from response
            sscanf(buffer, "%s %s/%s\n", ignore, rev_word, status);

            if(strcmp(status, "OK") == 0) {

                printf("The word is %s\n", rev_word);
                continue;

            }

            printf("Something went wrong\n");

        } else {
        
            printf("Invalid command\n");
            help();

        }
        
    }

    freeaddrinfo(res_udp);
    freeaddrinfo(res_tcp);
    leave();

    return 0;

}