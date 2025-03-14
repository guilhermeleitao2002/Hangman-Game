// Author: Guilherme Leitão 99951 & Diogo Costa 96182

// Libraries
# include <unistd.h>
# include <string.h>
# include <stdlib.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <sys/time.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <stdio.h>
# include <signal.h>
# include <time.h>
# include <sys/stat.h>
# include <sys/dir.h>

// Constants
# define MAX_BUFFER_SIZE 1024
# define SIZE 128
# define PORT_SIZE 6
# define PLAYER_ID 7
# define MAX_PLAYERS 30
# define RANDOM 0

// Global variables
int fd_udp, fd_tcp, counter = 1;

// Structs
typedef struct {

    int score[10];
    char PLID[10][PLAYER_ID];
    char word[10][30];
    int n_succ[10];
    int n_tot[10];

} SCORELIST;

/*
*   Handles the SIGINT signal
*/
void leave() {

    printf("\nServer is shutting down...\n");
    close(fd_tcp);
    close(fd_udp);

    // Remove all files in GAMES folder and SCORES folder
    system("rm -rf GAMES/*");
    system("rm -rf SCORES/*");

    exit(0);

}

/*
*   Updates the game word with the letter in the respective positions
*   word: the game word
*   positions: the number of positions
*   letter: the letter to be inserted
*   buffer: the buffer containing the positions
*/
void update_game_word(char* word, int positions, char letter, char *buffer){

    int i = 0;
    char *token = strtok(buffer, " ");

    // inserts the letter in every respective position
    while(i < positions) {

        int position;

        sscanf(token, "%d", &position);
        word[position - 1] = letter;
        i++;

        token = strtok(NULL, " ");

    }
}

/*
*   Checks if the letter is in the word
*   letter: the letter to be checked
*   word: the word to be checked
*/
int letter_in_word(char letter, char *word) {

    int n = strlen(word);

    for(int i = 0; i < n; i++)

        if(word[i] == letter)

            return 1;

    return 0;


}

/*
*   Reads the initial start of the program
*   argc: number of arguments
*   argv: arguments
*   port: port to be used
*   word_file: file containing the words
*   verbose: verbose mode
*/
void read_entry(int argc, char **argv, char port[PORT_SIZE], char word_file[SIZE], int *verbose) {

    if (argc < 2)
    {

        printf("Invalid argument\n");
        leave();
    }

    strcpy(word_file, argv[1]);

    if (argc == 2)
        return;

    if (argc == 4)

        if (strcmp(argv[2], "-p") == 0)

            strcpy(port, argv[3]);

        else
        {

            printf("Invalid argument\n");
            leave();
        }

    else if (argc == 3)

        if (strcmp(argv[2], "-v") == 0)

            *verbose = 1;

        else
        {

            printf("Invalid argument\n");
            leave();
        }

    else if (argc == 5)

        if (strcmp(argv[2], "-v") == 0)
        {

            *verbose = 1;

            if (strcmp(argv[3], "-p") == 0)

                strcpy(port, argv[4]);

            else
            {

                printf("Invalid argument\n");
                leave();
            }
        }
        else if (strcmp(argv[2], "-p") == 0)
        {

            strcpy(port, argv[3]);

            if (strcmp(argv[4], "-v") == 0)

                *verbose = 1;

            else
            {

                printf("Invalid argument\n");
                leave();
            }
        }
        else
        {

            printf("Invalid argument\n");
            leave();
        }

    else
    {

        printf("Invalid argument\n");
        leave();
    }
}

/*
*   Counts the number of lines in a file
*   filename: the file to be counted
*/
int count_lines(char filename[SIZE]) {

    FILE *fp = fopen(filename, "r");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int n_lines = 0;

    while ((read = getline(&line, &len, fp)) != -1) n_lines++;

    fclose(fp);

    return n_lines;
}

/*
*   Checks if the player already exists
*   plid: the player id
*/
int player_already_exists(char plid[PLAYER_ID]) {

    // Checks if the player already exists
    char file_name[SIZE];
    sprintf(file_name, "GAMES/GAME_%s.txt", plid);

    if(access(file_name, F_OK) == 0)
        
        if(count_lines(file_name) == 1) return 2;
        else  return 1;

    return 0;
}

/*
*   Grabs a random word from a file
*   wordfile: the file containing the words
*   image: the image associated with the word
*/
char *pick_random_word_from_file(char wordfile[SIZE], char *image) {

    FILE *fp = fopen(wordfile, "r");
    char *word = NULL, *line = NULL;
    size_t len = 0;
    ssize_t read;
    int n_lines = 0, line_number = 0;

    while ((read = getline(&line, &len, fp)) != -1) n_lines++;

    rewind(fp);

    // get a TRULY random number
    srand(time(NULL));

    if(counter == 27) counter = 1;

    if(RANDOM)

        line_number = rand() % n_lines + 1;

    else

        line_number = counter++;

    for(int i = 0; i < line_number; i++)

        getline(&line, &len, fp);

    word = strtok(line, " ");
    strcpy(image, strtok(NULL, " "));

    fclose(fp);

    return word;

}

/*
*   Counts the number of different letters in a word
*   word: the word to be counted
*/
int count_different_letters_in_word(char word[SIZE]) {

    int n_letters = 0;
    char *letters = (char*) malloc(strlen(word) * sizeof(char));

    for(int i = 0; i < strlen(word); i++) {

        int found = 0;

        for(int j = 0; j < n_letters; j++)

            if(word[i] == letters[j]) found = 1;

        if(!found) letters[n_letters++] = word[i];
    }

    free(letters);

    return n_letters;
}

/*
*   Checks if the word is the final word
*   filename: the file containing the game state
*   word: the word to be checked
*/
int check_if_final_word(char filename[SIZE], char word[SIZE]) {

    int n_diff = count_different_letters_in_word(word);
    FILE *fp = fopen(filename, "r");
    char *line = NULL, car1, car2;
    size_t len = 0;
    int n_T = 0;

    // Skips the first line
    getline(&line, &len, fp);

    while((fgets(line, SIZE, fp)) != NULL) {

        sscanf(line, "%c %c", &car1, &car2);

        if(car1 == 'T' && letter_in_word(car2, word)) n_T++;

    }

    return (n_T == n_diff);

}

/*
*   Fetches the total errors that were given
*   filename: the file containing the game state
*   word: the game word
*/
int get_errors_given(char filename[SIZE], char word[SIZE]) {

    int n_diff = count_different_letters_in_word(word);
    FILE *fp = fopen(filename, "r");
    char *line = NULL, car1, car2;
    size_t len = 0;
    int errors = 0;
    char final_word[SIZE];

    getline(&line, &len, fp);

    while((fgets(line, SIZE, fp)) != NULL) {

        sscanf(line, "%c", &car1);

        if(car1 == 'T') {                                   // letter trial

            sscanf(line, "%c %c\n", &car1, &car2);

            if(!letter_in_word(car2, word)) errors++;

        } else {                                            // word trial

            sscanf(line, "%c %s\n", &car1, final_word);

            if(strcmp(final_word, word) == 0) errors++; 

        }

    }

    return --errors;

}

/*
*   Creates the vector with the positions of the letter in the word
*   word: the word
*   letter: the letter
*   vector: the vector
*/
int positions_vector(char *word, char letter, char vector[31]) {

    int n = strlen(word), positions = 0, j = 0;

    for(int i = 0; i < n; i++)

        if(word[i] == letter)

            positions++;

    for(int i = 0; i < n; i++)

        if(word[i] == letter){

            sprintf(vector + j, "%d ", i + 1);

            if(i + 1 > 9) {

                j += 3;
                continue;

            }

            j += 2;

        }
    
    return positions;

}

/*
*   Own implementation of the write function to avoid the problem of partial writes
*   fd: the file descriptor
*   buff: the buffer to be written
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
*   Own implementation of the write function to avoid the problem of partial writes (2)
*   fd: the file descriptor
*   buff: the buffer to be written
*   n: the number of bytes to be written
*/
void my_write2(int fd, char *buff, size_t n) {

    ssize_t nleft = n, nwritten;

    // loop until there is nothing left to write
    while(nleft > 0) {

        nwritten = write(fd, buff, nleft);
        if(nwritten <= 0) leave();                              // Error

        nleft -= nwritten;
        buff += nwritten;

    }

}

/*
*   Own implementation of the read function to avoid the problem of partial reads
*   fd: the file descriptor
*   buff: the buffer to be read
*/
void my_read(int fd, char buff[MAX_BUFFER_SIZE]) {

    int i = 0;
    char c;

    // loop until there is nothing left to read
    while(1) {

        read(fd, &c, 1);
        buff[i++] = c;

        if(c == '\n') {

            buff[i] = '\0';
            break;

        }

    }

}

/*
*   Fetches the current date and time
*   year: the year
*   month: the month
*   day: the day
*   hour: the hour
*   minute: the minute
*   second: the second
*/
void get_current_time(int *year, int *month, int *day, int *hour, int *minute, int *second) {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    *year = tm.tm_year + 1900;
    *month = tm.tm_mon + 1;
    *day = tm.tm_mday;
    *hour = tm.tm_hour;
    *minute = tm.tm_min;
    *second = tm.tm_sec;

}

/*
*   Writes the game state to the file
*   code: the code (T or G)
*   plid: the player id
*   word: the word
*   letter: the letter
*/
void write_to_games(char code, char plid[PLAYER_ID], char word[SIZE], char letter) {

    char buffer[SIZE];
    sprintf(buffer, "GAMES/GAME_%s.txt", plid);

    FILE *fp = fopen(buffer, "a");
    if(fp == NULL) leave();                                                 // Error

    fseek(fp, 0, SEEK_END);

    if(code == 'T')                                                         // letter trial

        fprintf(fp, "T %c\n", letter);

    else                                                                    // word trial
            
        fprintf(fp, "G %s\n", word);


    fclose(fp);

}

/*
*   Archives the game
*   outcome: the outcome (W or L or Q)
*   plid: the player id
*/
void archive_game(char outcome, char plid[PLAYER_ID]) {

    int year, month, day, hour, minute, second;

    char buffer[SIZE], buffer1[SIZE], buffer2[SIZE];
    sprintf(buffer, "GAMES/GAME_%s.txt", plid);

    sprintf(buffer1, "GAMES/%s", plid);
    mkdir(buffer1, 0777);

    get_current_time(&year, &month, &day, &hour, &minute, &second);
    sprintf(buffer2, "GAMES/%s/%04d%02d%02d_%02d%02d%02d_%c.txt", plid, year, month, day, hour, minute, second, outcome);

    rename(buffer, buffer2);

}

/*
*   Checks if the letter or word has already been guessed
*   filename: the filename
*   letter: the letter
*   word: the word
*/
int is_duplicate_guess(char filename[SIZE], char letter, char *word) {

    FILE *fp = fopen(filename, "r");
    if(fp == NULL) leave();                                                 // Error

    char buffer[SIZE], code, c, guess_word[SIZE];

    while(fgets(buffer, SIZE, fp) != NULL) {

        if(letter != ' ') {
            sscanf(buffer, "%c %c", &code, &c);

            if(code == 'T' && c == letter) {

                fclose(fp);
                return 1;

            }

        } else {

            sscanf(buffer, "%c %s", &code, guess_word);

            if(code == 'G' && strcmp(word, guess_word) == 0) {

                fclose(fp);
                return 1;

            }

        }

    }

    fclose(fp);
    return 0;

}

/*
*   Gets the game word from the file
*   filename: the filename
*   word: the word
*/
void get_gameword(char filename[SIZE], char word[SIZE]) {

    FILE *fp = fopen(filename, "r");
    char buffer[MAX_BUFFER_SIZE], ignore[SIZE];
    if(fp == NULL) leave();                                                 // Error

    fgets(buffer, SIZE, fp);
    sscanf(buffer, " %s %s", word, ignore);

    fclose(fp);

}

/*
*   Gets the image from the file
*   filename: the filename
*   image: the image
*/
void get_image(char filename[SIZE], char image[SIZE]) {

    FILE *fp = fopen(filename, "r");
    char buffer[MAX_BUFFER_SIZE], ignore[SIZE];
    if(fp == NULL) leave();                                                 // Error

    fgets(buffer, SIZE, fp);
    sscanf(buffer, " %s %s", ignore, image);

    fclose(fp);

}

/*
*   Gets the number of errors to give from a word
*   word: the word
*/
int get_max_errors(char word[SIZE]) {

    int n = strlen(word);

    if(n <= 6) return 7;
    else if(n <= 10) return 8;
    
    return 9;

}

/*
*   Gets the top 10 scores from all games
*   list: the list that will contain the scores and player ids
*/
int get_10_top_scores(SCORELIST *list) {

    struct dirent **filelist;
    int n_entries, i_file;
    char fname[MAX_BUFFER_SIZE];
    FILE *fp;

    n_entries = scandir("SCORES/", &filelist, 0, alphasort);

    i_file = 0;
    if(n_entries < 0)
    
        return 0;

    else {

        while(n_entries--) {

            if(filelist[n_entries] -> d_name[0] != '.') {

                sprintf(fname, "SCORES/%s", filelist[n_entries] -> d_name);
                fp = fopen(fname, "r");
                if(fp != NULL) {

                    fscanf(fp, "%d %s %s %d %d", &list -> score[i_file], list -> PLID[i_file], list -> word[i_file], &list -> n_succ[i_file], &list -> n_tot[i_file]);
                    fclose(fp);
                    i_file++;

                }

            }

            free(filelist[n_entries]);
            if(i_file == 10) break;

        }

        free(filelist);

    }
    
    return (i_file);

}

/*
*   Writes the formatted scoreboard to the buffer
*   buffer: the buffer
*/
int write_scoreboard(char buffer[MAX_BUFFER_SIZE]) {

    int i, n_scores;
    char temp[SIZE];
    SCORELIST list;

    n_scores = get_10_top_scores(&list);

    if(n_scores == 0) return -1;

    // Write the scoreboard to the buffer
    sprintf(buffer, "-------------------------------- TOP 10 SCORES --------------------------------\n\n    SCORE PLAYER        WORD\t\t\t      GOOD TRIALS  TOTAL TRIALS\n\n");

    for(i = 0; i < n_scores; i++) {

        sprintf(temp, "%2d - %03d  %s  %s\t\t\t\t  %d\t        %d\n", i + 1, list.score[i], list.PLID[i], list.word[i], list.n_succ[i], list.n_tot[i]);
        strcat(buffer, temp);

    }

    return strlen(buffer);

}

/*
*   Finds the last game played by the player
*   PLID: the player id
*   fname: the filename
*/
int FindLastGame(char *PLID, char *fname) {

    struct dirent **filelist;
    int n_entries, found;
    char dirname[20];

    sprintf(dirname, "GAMES/%s/", PLID);

    n_entries = scandir(dirname, &filelist, 0, alphasort);
    found = 0;

    if(n_entries <= 0)
    
        return 0;

    else {

        while(n_entries--) {

            if(filelist[n_entries] -> d_name[0] != '.') {

                sprintf(fname, "GAMES/%s/%s", PLID, filelist[n_entries] -> d_name);
                found = 1;

            }

            free(filelist[n_entries]);

            if(found) break;

        }

        free(filelist);

    }

    return (found);

}

/*
*   Writes the formatted state of the game to the buffer
*   plid: the player id
*   buffer: the buffer
*/
int write_state(char plid[PLAYER_ID], char buffer[MAX_BUFFER_SIZE]) {

    int i, found, lines, finished = 1;
    char temp[SIZE], fname[SIZE], word[SIZE], code, ignore[SIZE], letter, guess_word[31], current_word[31];
    SCORELIST list;
    FILE *fp;

    if(player_already_exists(plid) != 0) {

        sprintf(fname, "GAMES/GAME_%s.txt", plid);
        found = 1;
        finished = 0;

    } else found = FindLastGame(plid, fname);

    if(found == 0) return -1;

    lines = count_lines(fname) - 1;

    // Write the state of the game to the buffer
    sprintf(buffer, "Active game found for player %s\n--- Transactions found: %d ---\n", plid, lines);

    fp = fopen(fname, "r");

    get_gameword(fname, word);

    int n = strlen(word);
    for(i = 0; i < n; i++)

        current_word[i] = '-';

    current_word[n] = '\0';

    fgets(temp, SIZE, fp);

    for(i = 0; i < lines; i++) {

        memset(temp, 0, sizeof(temp));
        fgets(temp, SIZE, fp);
        sscanf(temp, "%c", &code);

        if(code == 'T') {

            sscanf(temp, "%c %c", ignore, &letter);
            if(letter_in_word(letter, word)){

                sprintf(temp, "Letter trial: %c - TRUE\n", letter);

                char pos_vector[SIZE];
                memset(pos_vector, 0, sizeof(pos_vector));
                int pos = positions_vector(word, letter, pos_vector);
                update_game_word(current_word, pos, letter, pos_vector);

            } else

                sprintf(temp, "Letter trial: %c - FALSE\n", letter);

            strcat(buffer, temp);

        } else {

            sscanf(temp, "%c %s", ignore, guess_word);
            sprintf(temp, "Word guess: %s\n", guess_word);
            strcat(buffer, temp);

        }

    }

    memset(temp, 0, sizeof(temp));
    if(!finished)

        sprintf(temp, "Solved so far: %s\n", current_word);

    else {

        sprintf(temp, "Termination\n");

    }

    strcat(buffer, temp);

    return strlen(buffer);

}

/*
*   Writes the formatted hint to the buffer
*   buffer: the buffer
*   plid: the player id
*   n: the number of bytes to read
*   image: the image name
*   str: the string to write
*   fp: the file pointer
*/
size_t write_hint_image(unsigned char *buffer, char plid[PLAYER_ID], size_t n, char image[31], char str[SIZE], FILE *fp) {

    sprintf(buffer, "RHL OK %s %s ", image, str);
    buffer += 9 + strlen(image) + strlen(str);

    size_t read = fread(buffer, sizeof(unsigned char), n, fp);

    // rewind the pointer
    buffer = buffer - 9 - strlen(image) - strlen(str);
    
    fclose(fp);

    return read;

}

/*
*   main function
*   argc: number of arguments
*   argv: array of arguments
*/
int main(int argc, char **argv) {

    // copilot help me comment things

    int newfd_tcp, errcode_udp, errcode_tcp, word_len, verbose = 0, trial;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints_udp, *res_udp = NULL, hints_tcp, *res_tcp = NULL;
    struct sockaddr_in addr;
    char buffer[MAX_BUFFER_SIZE], port[PORT_SIZE] = "58062";
    char ignore[SIZE], message[4], word[31], plid[PLAYER_ID] = "099951", word_file[SIZE], letter;
    struct sigaction act;

    read_entry(argc, argv, port, word_file, &verbose);

    // set the flags and create the udp socket
    fd_udp = socket(AF_INET, SOCK_DGRAM, 0);                                            // UDP socket
    if(fd_udp == -1) leave();                                                           // Error

    memset(&hints_udp, 0, sizeof(hints_udp));
    hints_udp.ai_family = AF_INET;                                                      // IPv4
    hints_udp.ai_socktype = SOCK_DGRAM;                                                 // UDP socket
    hints_udp.ai_flags = AI_PASSIVE;

    errcode_udp = getaddrinfo(NULL, port, &hints_udp, &res_udp);
    if(errcode_udp != 0) leave();                                                       // Error

    n = bind(fd_udp, res_udp->ai_addr, res_udp->ai_addrlen);
    if(n == -1) leave();                                                                // Error

    // create the "mother" process to handle the tcp connections
    if(fork() == 0) {

        fd_tcp = socket(AF_INET, SOCK_STREAM, 0);                                               // TCP socket
        if(fd_tcp == -1) exit(1);                                                               // Error

        memset(&hints_tcp, 0, sizeof(hints_tcp));
        hints_tcp.ai_family = AF_INET;                                                      // IPv4
        hints_tcp.ai_socktype = SOCK_STREAM;                                                // TCP socket
        hints_tcp.ai_flags = AI_PASSIVE;                                                    // Fill in my IP for me        

        errcode_tcp = getaddrinfo(NULL, port, &hints_tcp, &res_tcp);
        if(errcode_tcp != 0) exit(1);                                                           // Error

        n = bind(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
        if(n == -1) exit(1);                                                                // Error

        if(listen(fd_tcp, MAX_PLAYERS) == -1) exit(1);                                      // Error

        while(1) {

            addrlen = sizeof(addr);
            if((newfd_tcp = accept(fd_tcp, (struct sockaddr*) &addr, &addrlen)) == -1) exit(1); // Error

            // create a child process to handle the tcp connection
            if(fork() == 0) {

                memset(buffer, 0, sizeof(buffer));

                my_read(newfd_tcp, buffer);

                memset(plid, 0, sizeof(plid));
                memset(message, 0, sizeof(message));

                // read the message and the player id
                sscanf(buffer, "%s %s\n", message, plid);

                if(strcmp(message, "GSB") == 0) {

                    memset(buffer, 0, sizeof(buffer));
                    char buffer2[MAX_BUFFER_SIZE];

                    // write the scoreboard to the buffer
                    int size = write_scoreboard(buffer2);

                    if(size == -1)

                        sprintf(buffer, "RSB EMPTY\n");

                    else {

                        sprintf(buffer, "RSB OK TOPSCORES_012345 %d ", size);
                        strcat(buffer, buffer2);

                    }
                    
                    my_write(newfd_tcp, buffer);
                    
                } else if(strcmp(message, "STA") == 0) {

                    memset(buffer, 0, sizeof(buffer));
                    char buffer2[MAX_BUFFER_SIZE];

                    // write the state to the buffer
                    int size = write_state(plid, buffer2);

                    if(size == -1)

                        sprintf(buffer, "RST NOK\n");

                    else {

                        if(player_already_exists(plid) != 0)

                            sprintf(buffer, "RST ACT STATE_%s %d ", plid, size);

                        else

                            sprintf(buffer, "RST FIN STATE_%s %d ", plid, size);
                        
                        strcat(buffer, buffer2);

                    }
                    
                    my_write(newfd_tcp, buffer);
                    
                } else if(strcmp(message, "GHL") == 0) {

                    int i;
                    char temp[SIZE], image[31], str[SIZE];
                    FILE *fp;
                    size_t read;

                    sprintf(temp, "GAMES/GAME_%s.txt", plid);
                    get_image(temp, image);

                    memset(temp, 0, sizeof(temp));
                    sprintf(temp, "IMAGES/%s", image);

                    fp = fopen(temp, "r");
                    if(fp != NULL) {

                        fseek(fp, 0, SEEK_END);
                        size_t n = ftell(fp);
                        rewind(fp);

                        snprintf(str, sizeof(str), "%zu", n);
                        char buffer2[9 + strlen(image) + strlen(str) + n + 1];
                        memset(buffer2, 0, n + 1);

                        memset(buffer2, 0, sizeof(buffer2));

                        // write the hint image to the buffer
                        read = write_hint_image(buffer2, plid, n, image, str, fp);

                        buffer2[read] = '\0';
                        my_write2(newfd_tcp, buffer2, read);

                    } else {

                        char buffer2[9];
                        sprintf(buffer2, "RHL NOK\n");
                        my_write2(newfd_tcp, buffer2, read);

                    }
                    
                }

                close(newfd_tcp);
                return 0;

            }

        }

        // wait for all the child processes to finish
        while(wait(NULL) > 0);        

        return 0;

    }

    // when ctrl+c is pressed, the server shuts down and closes the sockets
    act.sa_handler = leave;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

    while(1) {

        memset(buffer, 0, sizeof(buffer));

        addrlen = sizeof(addr);
        n = recvfrom(fd_udp, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *) &addr, &addrlen);
        buffer[n] = '\0';
        if(n == -1) leave();                                                            // Error

        memset(plid, 0, sizeof(plid));
        memset(message, 0, sizeof(message));

        // read the message and the player id
        sscanf(buffer, "%s %s\n", message, plid);

        // get the ip and the port of the client to verbose mode
        char ip[INET_ADDRSTRLEN];
        int port = ntohs(addr.sin_port);
        inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);

        if(strcmp(message, "SNG") == 0) {

            if(player_already_exists(plid) == 1) {

                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "RSG NOK\n");

                if(verbose == 1) printf("PLID=%s: game already exists (%s:%d)\n", plid, ip, port);

                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

                continue;

            }

            char word[SIZE], file_name[SIZE], image[SIZE];

            sprintf(file_name, "GAMES/GAME_%s.txt", plid);

            if(player_already_exists(plid) == 0) {

                strcpy(word, pick_random_word_from_file(word_file, image));
                int n_letters = strlen(word);

                memset(buffer, 0, sizeof(buffer));

                sprintf(buffer, "RSG OK %d %d\n", n_letters, get_max_errors(word));

                if(verbose == 1) printf("PLID=%s: new game; word = \"%s\" (%d letters) (%s:%d)\n", plid, word, n_letters, ip, port);

                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

                FILE *fp = fopen(file_name, "w");
                if(fp == NULL) leave();                                                 // Error

                fprintf(fp, "%s %s", word, image);

                fclose(fp);

            } else {

                get_gameword(file_name, word);
                int n_letters = strlen(word);

                memset(buffer, 0, sizeof(buffer));

                sprintf(buffer, "SNG OK %d %d\n", n_letters, get_max_errors(word));

                if(verbose == 1) printf("PLID=%s: game confirmation resent (%s:%d)\n", plid, ip, port);

                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

            }

        } else if(strcmp(message, "PLG") == 0) {

            // read the letter and the trial number
            sscanf(buffer, "%s %s %c %d\n", ignore, ignore, &letter, &trial);

            if(player_already_exists(plid) == 0) {

                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "RLG ERR\n");

                if(verbose == 1) printf("PLID=%s: something happenned (%s:%d)\n", plid, ip, port);

                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

                continue;

            }

            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "GAMES/GAME_%s.txt", plid);

            if(trial != count_lines(buffer)) {

                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "RLG INV %d\n", count_lines(buffer));

                if(verbose == 1) printf("PLID=%s: invalid play (%s:%d)\n", plid, ip, port);

                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

                continue;

            }

            if(is_duplicate_guess(buffer, letter, "___")) {

                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "RLG DUP %d\n", trial);

                if(verbose == 1) printf("PLID=%s: duplicate guess (%s:%d)\n", plid, ip, port);

                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

                continue;

            }

            write_to_games('T', plid, NULL, letter);

            char word[31];
            get_gameword(buffer, word);

            char file_name[SIZE];
            sprintf(file_name, "GAMES/GAME_%s.txt", plid);

            if(letter_in_word(letter, word)) {

                char vec[31];
                int pos = positions_vector(word, letter, vec);

                if(check_if_final_word(buffer, word)) {

                    memset(buffer, 0, sizeof(buffer));

                    sprintf(buffer, "RLG WIN %d\n", trial);

                    if(verbose == 1) printf("PLID=%s: play letter '%c' - WIN (game ended) (%s:%d)\n", plid, letter, ip, port);

                    n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                    if(n == -1) leave();                                                    // Error

                    float score = (float)((float)(count_lines(file_name) - 1 - get_errors_given(file_name, word)) / ((float)(count_lines(file_name) - 1))) * 100;
                    int year, month, day, hour, minute, second;

                    get_current_time(&year, &month, &day, &hour, &minute, &second);

                    memset(buffer, 0, sizeof(buffer));
                    sprintf(buffer, "SCORES/%03.0f_%s_%02d%02d%04d_%02d%02d%02d.txt", score, plid, day, month, year, hour, minute, second);

                    FILE *fp = fopen(buffer, "w");
                    if(fp == NULL) leave();                                                 // Error

                    fprintf(fp, "%03.0f %s %s %d %d", score, plid, word, count_lines(file_name) - 1 - get_errors_given(file_name, word), count_lines(file_name) - 1);

                    archive_game('W', plid);

                    fclose(fp);

                    continue;

                }

                vec[strlen(vec) - 1] = '\0';
                sprintf(buffer, "RLG OK %d %d %s\n", trial, pos, vec);

                if(verbose == 1) printf("PLID=%s: play letter '%c' - %d hits; word not guessed (%s:%d)\n", plid, letter, count_lines(file_name) - 2 - get_errors_given(file_name, word), ip, port);

                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

            } else {

                memset(buffer, 0, sizeof(buffer));

                if(get_errors_given(file_name, word) == get_max_errors(word)) {

                    sprintf(buffer, "RLG OVR %d\n", trial);

                    if(verbose == 1) printf("PLID=%s: play letter '%c' - LOST (game ended) (%s:%d)\n", plid, letter, ip, port);

                    archive_game('F', plid);

                    n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                    if(n == -1) leave();                                                    // Error

                    continue;

                }

                sprintf(buffer, "RLG NOK %d\n", trial);

                if(verbose == 1) printf("PLID=%s: play letter '%c' - wrong guess (%s:%d)\n", plid, letter, ip, port);

                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

            }

        } else if(strcmp(message, "PWG") == 0) {

            // reads the word and the trial number
            sscanf(buffer, "%s %s %s %d\n", ignore, ignore, word, &trial);

            if(player_already_exists(plid) == 0) {

                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "RWG ERR\n");

                if(verbose == 1) printf("PLID=%s: something happenned (%s:%d)\n", plid, ip, port);

                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

                continue;

            }

            char final_word[SIZE], file_name[SIZE];
            sprintf(file_name, "GAMES/GAME_%s.txt", plid);
            get_gameword(file_name, final_word);

            if(trial != count_lines(file_name)) {

                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "RWG INV %d\n", count_lines(file_name));

                if(verbose == 1) printf("PLID=%s: invalid play (%s:%d)\n", plid, ip, port);

                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

                continue;

            }

            if(is_duplicate_guess(file_name, ' ', word)) {

                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "RWG DUP %d\n", trial);

                if(verbose == 1) printf("PLID=%s: duplicate word guess (%s:%d)\n", plid, ip, port);

                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

                continue;

            }

            write_to_games('G', plid, word, ' ');

            if(strcmp(final_word, word) == 0) {

                memset(buffer, 0, sizeof(buffer));

                sprintf(buffer, "RWG WIN %d\n", trial);

                if(verbose == 1) printf("PLID=%s: guess word \"%s\" - WIN (game ended) (%s:%d)\n", plid, word, ip, port);


                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

                float score = (float)((float)(count_lines(file_name) - 1 - get_errors_given(file_name, final_word)) / ((float)(count_lines(file_name) - 1))) * 100;
                int year, month, day, hour, minute, second;

                get_current_time(&year, &month, &day, &hour, &minute, &second);

                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "SCORES/%03.0f_%s_%02d%02d%04d_%02d%02d%02d.txt", score, plid, day, month, year, hour, minute, second);

                FILE *fp = fopen(buffer, "w");
                if(fp == NULL) leave();                                                 // Error

                fprintf(fp, "%03.0f %s %s %d %d", score, plid, word, count_lines(file_name) - 1 - get_errors_given(file_name, word), count_lines(file_name) - 1);
                archive_game('W', plid);

                fclose(fp);

            } else {

                memset(buffer, 0, sizeof(buffer));

                if(get_errors_given(file_name, final_word) == get_max_errors(final_word)) {

                    sprintf(buffer, "RWG OVR %d\n", trial);

                    if(verbose == 1) printf("PLID=%s: guess word \"%s\" - LOST (game ended) (%s:%d)\n", plid, word, ip, port);

                    archive_game('F', plid);

                    n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                    if(n == -1) leave();                                                    // Error

                    continue;

                }

                sprintf(buffer, "RWG NOK %d\n", trial);

                if(verbose == 1) printf("PLID=%s: guess word \"%s\" - wrong guess (%s:%d)\n", plid, word, ip, port);

                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

            }

        } else if(strcmp(message, "QUT") == 0) {

            if(player_already_exists(plid) == 0) {

                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "RQT ERR\n");

                if(verbose == 1) printf("PLID=%s: something happenned (%s:%d)\n", plid, ip, port);

                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

                continue;

            }

            // archive game
            archive_game('Q', plid);

            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "RQT OK\n");

            if(verbose == 1) printf("PLID=%s: game quitted (%s:%d)\n", plid, ip, port);

            n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
            if(n == -1) leave();                                                    // Error

        } else if(strcmp(message, "REV") == 0) {

            if(player_already_exists(plid) == 0) {

                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "RRV ERR\n");

                if(verbose == 1) printf("PLID=%s: something happenned (%s:%d)\n", plid, ip, port);

                n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
                if(n == -1) leave();                                                    // Error

                continue;

            }

            char final_word[SIZE];
            get_gameword(plid, final_word);

            // archive game
            archive_game('Q', plid);

            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "RRV %s\n", final_word);

            if(verbose == 1) printf("PLID=%s: word revealed and game quitted (%s:%d)\n", plid, ip, port);

            n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
            if(n == -1) leave();                                                    // Error

        } else {                                                                    // an error with the command was detected

            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "ERR\n");

            if(verbose == 1) printf("PLID=%s: something happenned (%s:%d)\n", plid, ip, port);

            n = sendto(fd_udp, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
            if(n == -1) leave();                                                    // Error

        }

    }

    freeaddrinfo(res_udp);

    wait(NULL);

    return 0;
}