// Rishi Bidarkota
// I pledge my honor that I have abided by the Stevens Honor System.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void parse_connect(int argc, char *argv[], int *server_fd) {
    char *ip_addr = "127.0.0.1";
    int port_num = 25555;

    int opt;
    while ((opt = getopt(argc, argv, "i:p:h")) != -1) {
        switch (opt) {
            case 'i':
                ip_addr = optarg;
                break;
            case 'p':
                port_num = atoi(optarg);
                break;
            case 'h':
                printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n\n", argv[0]);
                printf("  -i IP_address         Default to \"127.0.0.1\";\n");
                printf("  -p port_number        Default to 25555;\n");
                printf("  -h                    Display this help info.\n");
                exit(0);
            default:
                printf("Error: Unknown option '-%c' received.\n", optopt);
                exit(1);
        }
    }

    struct sockaddr_in server_addr;
    socklen_t addr_size = sizeof(server_addr);

    *server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*server_fd == -1) {
        perror("socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);

    if (connect(*server_fd, (struct sockaddr *)&server_addr, addr_size) == -1) {
        perror("connect");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    int server_fd;
    parse_connect(argc, argv, &server_fd);

    char name[128];
    char buffer[1024];


    // read name and send back to server
    int bytes_read = recv(server_fd, buffer, sizeof(buffer), 0);
    if (bytes_read == -1) {
        perror("recv");
        exit(1);
    }

    // null terminate it at the end
    buffer[bytes_read] = '\0';

    printf("%s", buffer);


    fgets(name, sizeof(name), stdin);

    // replace \n with \0
    name[strcspn(name, "\n")] = '\0'; 
    write(server_fd, name, strlen(name) + 1);

    fd_set read_fds;
    int max_fd = server_fd;

    while (1) {

        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &read_fds)) {

            bytes_read = read(server_fd, buffer, sizeof(buffer));

            // handles case if a client presses cntrl c
            if (bytes_read == 0) {
                break;
                // closes server after breaking
            }

            buffer[bytes_read] = '\0';
            printf("%s", buffer);
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {

            // cntrl c case from client
            if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                break; 
            }

            buffer[strcspn(buffer, "\n")] = '\0'; 
            write(server_fd, buffer, strlen(buffer) + 1);

        }
    }

    close(server_fd);

    return 0;


}
