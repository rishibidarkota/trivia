// Rishi Bidarkota
// I pledge my honor that I have abided by the Stevens Honor System.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_QUESTIONS 50
#define MAX_PLAYERS 3

struct Entry {
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};

struct Player {
    int fd;
    int score;
    char name[128];
};

int read_questions(struct Entry *arr, char *filename) {

    FILE *fp = fopen(filename, "r");

    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    int count = 0;
    char line[1024];

    while (fgets(line, sizeof(line), fp) != NULL) {

        // emptyline
        if (strlen(line) == 1) continue; 

        strcpy(arr[count].prompt, line);


        fgets(line, sizeof(line), fp);



        char *option = strtok(line, " ");


        for (int i = 0; i < 3; i++) {
            strcpy(arr[count].options[i], option);
            option = strtok(NULL, " ");

        }



        fgets(line, sizeof(line), fp);
        for (int i = 0; i < 3; i++) {

            if (strncmp(line, arr[count].options[i], strlen(arr[count].options[i])) == 0) {

                arr[count].answer_idx = i;
                break;
            }
        }

        count++;
    }

    fclose(fp);

    return count;

}

int main(int argc, char *argv[]) {

    // defaults
    char *question_file = "questions.txt";
    char *ip_addr = "127.0.0.1";
    int port_num = 25555;

    int opt;

    while ((opt = getopt(argc, argv, "f:i:p:h")) != -1) {

        switch (opt) {
            case 'f':
                question_file = optarg;
                break;
            case 'i':
                ip_addr = optarg;
                break;
            case 'p':
                port_num = atoi(optarg);
                break;
            case 'h':
                printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n\n", argv[0]);
                printf("  -f question_file      Default to \"questions.txt\";\n");
                printf("  -i IP_address         Default to \"127.0.0.1\";\n");
                printf("  -p port_number        Default to 25555;\n");
                printf("  -h                    Display this help info.\n");
                return 0;
            default:
                printf("Error: Unknown option '-%c' received.\n", optopt);
                return 1;
        }
    }

    int server_fd;
    int client_fd;

    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        perror("socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, MAX_PLAYERS) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Welcome to 392 Trivia!\n");

    struct Entry questions[MAX_QUESTIONS];
    int num_questions = read_questions(questions, question_file);

    struct Player players[MAX_PLAYERS];
    int num_players = 0;

    fd_set read_fds;
    int max_fd = server_fd;

    while (num_players < MAX_PLAYERS) {

        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);

        select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &read_fds)) {

            client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);

            if (client_fd == -1) {
                perror("accept");
                continue;
            }

            

            printf("New connection detected!\n");


            write(client_fd, "Please type your name: ", 23);

            char name[128];

            int bytes_read = read(client_fd, name, sizeof(name));

            if (bytes_read == -1) {
                perror("read");
                close(client_fd);
                continue;
            } else if (bytes_read == 0) {
                printf("Lost connection!\n");
                close(client_fd);
                continue;
            }

           



            name[bytes_read - 1] = '\0'; 

            


            printf("Hi %s!\n", name);
            strcpy(players[num_players].name, name);

            players[num_players].fd = client_fd;

            players[num_players].score = 0;


            num_players++;

            // cntrl c after name input not working in client terminal
            // bytes_read = read(client_fd, name, sizeof(name));

            // if (bytes_read == -1) {
            //     perror("read");
            //     close(client_fd);
            //     continue;
            // }

            // if (bytes_read == 0) {
            //     printf("Lost connection!\n");
            //     // printf("num of players: %d\n", num_players);
            //     num_players--;
            //     close(client_fd);
            //     continue;
            // }

            if (num_players == MAX_PLAYERS) {
                printf("Max connection reached!\n");
                printf("The game starts now!\n");
                
            }

            if (client_fd > max_fd) {
                max_fd = client_fd;
            }


        }
    }

    for (int q = 0; q < num_questions; q++) {


        printf("\nQuestion %d: %s\n", q + 1, questions[q].prompt);
        printf("1: %s\n2: %s\n3: %s\n", questions[q].options[0], questions[q].options[1], questions[q].options[2]);

        char question_msg[2048];
        sprintf(question_msg, "\nQuestion %d: %s\nPress 1: %s\nPress 2: %s\nPress 3: %s\n", q + 1, questions[q].prompt, questions[q].options[0], questions[q].options[1], questions[q].options[2]);

        for (int p = 0; p < num_players; p++) {
            write(players[p].fd, question_msg, strlen(question_msg));
        }

        // reset fds
        FD_ZERO(&read_fds);
        for (int p = 0; p < MAX_PLAYERS; p++) {
            FD_SET(players[p].fd, &read_fds);
        }

        // check for if someone responds to question first
        int answered = 0;

        while (!answered) {

            select(max_fd + 1, &read_fds, NULL, NULL, NULL);

            for (int p = 0; p < MAX_PLAYERS; p++) {

                if (FD_ISSET(players[p].fd, &read_fds)) {

                    char answer[3];
                    int bytes_read = recv(players[p].fd, answer, sizeof(answer), 0);

                    if (bytes_read == -1) {
                        perror("recv");
                        continue;

                    } else if (bytes_read == 0) {

                        for (int i = 0; i < MAX_PLAYERS; i++) {
                            if (players[i].fd != -1) {
                                close(players[i].fd);
                            }

                        }
                        
                        close(server_fd);
                        return 0;
                    }

                    answer[bytes_read] = '\0'; // Null-terminate the string

                    int correct = answer[0] - '1' == questions[q].answer_idx;

                    if (correct) {
                        players[p].score++;
                        printf("Player %s answered correctly!\n", players[p].name);
                    } else {
                        players[p].score--;
                        printf("Player %s answered incorrectly.\n", players[p].name);
                    }

                    printf("The correct answer is: %s\n", questions[q].options[questions[q].answer_idx]);

                     // print all players scores
                    printf("Current scores:\n");
                        for (int i = 0; i < num_players; i++) {
                            printf("%s: %d\n", players[i].name, players[i].score);
                        }

                    char result_msg[128];

                    sprintf(result_msg, "The correct answer is: %s\n", questions[q].options[questions[q].answer_idx]);

                    for (int p = 0; p < MAX_PLAYERS; p++) {
                        write(players[p].fd, result_msg, strlen(result_msg));
                    }

                    // FD_ZERO(&read_fds); // Reset the read_fds set
                    // for (int p = 0; p < MAX_PLAYERS; p++) {
                    //     FD_SET(players[p].fd, &read_fds);
                    // }

                    // reset since someone already answered
                    answered = 1;
                    
                }
            }
        }
    }

    int max_score = 0;

    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (players[p].score > max_score) {
            max_score = players[p].score;
        }
    }

    printf("Congrats, the winner(s) is/are: ");

    int num_winners = 0;

    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (players[p].score == max_score) {
            if (num_winners > 0) {
                printf(", ");
            }
            printf("%s", players[p].name);
            num_winners++;
        }
    }

    printf("!\n");

    for (int p = 0; p < MAX_PLAYERS; p++) {
        close(players[p].fd);
    }

    close(server_fd);

    return 0;
}