/*======================================================================
| TCP/IP Server (Sockets)
|
| Name: server.c
|
|------------------------------------------------------------------
*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <libgen.h>
#include <signal.h>
#include <pthread.h>

// initializing global constants
#define MAX 2048
#define BUF 1024
#define PORT 59589

// Declare socket descriptor and datapacket struct globally
int socketd;


FILE *file_gen(char filepath[]) {
    /*
    This method uses the basename function to store a pointer for
    filename + extension. It places a pointer at the dot for a
    file extension as well. Then, it parses the string into its
    filename and extension components, recombines them, and creates a new file.
    */

    char inputfilename[BUF];
    char *base = basename(filepath);
    strncpy(inputfilename, base, sizeof(inputfilename));
    inputfilename[BUF - 1] = '\0';

    // Null-terminate the string at the dot to remove the extension    
    char *dot = strrchr(inputfilename, '.');
    char extension[BUF] = "";
    if (dot) {
        strncpy(extension, dot, sizeof(extension));
        extension[BUF - 1] = '\0';
        *dot = '\0';
    }

    snprintf(inputfilename + strlen(inputfilename),
                BUF - strlen(inputfilename),
                "_local_clone%s", extension);

    int fd = open(inputfilename, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (fd < 0) {
        perror("open");
        return NULL;
    }

    FILE *output = fdopen(fd, "wb");
    printf("A new file named %s has been created.\n", inputfilename);
    return(output);
}

void handle_sigint(int sig) {
    printf("\nCaught SIGINT (Ctrl+C). Cleaning up...\n");

    // Close the listening socket
    close(socketd);
    printf("Socket closed. Exiting.\n");

    exit(0);
}

ssize_t recv_all(int socket, void *buffer, size_t length) {
    /*
    Custom function to ensure packet reads are always complete
    */

    size_t total = 0;
    char *buf = buffer;

    while (total < length) {
        ssize_t n = recv(socket, buf + total, length - total, 0);
        if (n <= 0) return n;
        total += n;
    }
    return total;
}

void* handle_client(void* arg) {
    // Client handler for threads

    int newfd = *((int*)arg);
    free(arg);

    char buf[BUF];
    char client_ip[INET_ADDRSTRLEN];
    struct sockaddr_in cli;

    socklen_t cli_len = sizeof(cli);
    getpeername(newfd, (struct sockaddr*)&cli, &cli_len);
    inet_ntop(AF_INET, &(cli.sin_addr), client_ip, INET_ADDRSTRLEN);
    printf("Client connected from IP: %s\n", client_ip);

    char filename[BUF];

    // Send greeting to client
    snprintf((char*)buf, sizeof(buf), "\nFrom server: Hello client :)");
    send(newfd, buf, strlen(buf), 0);

    while (1) {
        // Receive filepath from client
        memset(buf, 0, sizeof(buf));
        int bytes = read(newfd, buf, sizeof(buf) - 1);
        if (bytes <= 0) {
            perror("Failed to receive filepath or empty input");
            close(newfd);
            break;
        }
        buf[bytes] = '\0';
        if (strcmp(buf, "DONE") == 0) {
            printf("Client is done sending files.\n");
            break;
        }

        strcpy(filename, buf);
        FILE *output_file = file_gen(filename);
        if (output_file == NULL) {
            perror("Failed to create file");
            close(newfd);
            return NULL;               // Continue to the next iteration to handle the next client
        }
        memset(buf, 0, sizeof(buf));

        // Send response to client that file has been initialized
        snprintf((char*)buf, sizeof(buf), "\nNew file initialized. Will now be copying original file.");
        send(newfd, (char*)buf, sizeof(buf), 0);
        memset(buf, 0, sizeof(buf));

        // Receive file data and write to output file
        int nbytes;
        while (1) {
            // Receive header of file (4 bytes)
            ssize_t hdr_bytes = recv_all(newfd, &nbytes, sizeof(int));
            if (hdr_bytes <= 0) break;

            // If nbytes is zero, treat as EOF
            if (nbytes == 0) {
                printf("End of file from client.\n");
                break;
            }

            if (nbytes < 0 || nbytes > BUF) {
                fprintf(stderr, "Corrupted packet: %d\n", nbytes);
                continue;
            }

            // Then receive payload
            char payload[BUF];
            ssize_t data_bytes = recv_all(newfd, payload, nbytes);
            if (data_bytes <= 0) break;

            fwrite(payload, 1, nbytes, output_file);
            fflush(output_file);
        }

        // Handling when client exits prematurely
        if (nbytes == 0) {
            printf("Client disconnected.\n");
        } else if (nbytes < 0) {
            perror("Error reading from client socket");
        }

        fclose(output_file);
    }
    // Close the output file
    close(newfd);
    printf("Closing connection with client...\n");
}

int main() {
    struct sockaddr_in srv, cli;    // Client and server structures
    socklen_t cli_len = sizeof(cli);
    char buf[BUF];


    // Create TCP socket
    if ((socketd=socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\nFailed to create a socket");
        exit(1);
    }

    // Set sockets to allow immediate new binds
    int enable = 1;
    if(setsockopt(socketd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("SO_REUSEADDR");
        exit(1);
    }

    #ifdef SO_REUSEPORT
    if(setsockopt(socketd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
    {
        perror("SO_REUSEPORT");
        exit(1);
    }
    #endif

    // Create server address structure
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(PORT);
    srv.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind socket to port
    if (bind(socketd, (struct sockaddr*) &srv, sizeof(srv)) < 0)
    {
        perror("\nFailed to bind");
        exit(1);
    }

    // listen on the socket
    if (listen(socketd, 1) < 0)
    {
        perror("\nFailed to listen");
        exit(1);
    }

    // Register signal handler for SIGINT
    signal(SIGINT, handle_sigint);

    while (1) {
        printf("Waiting for connection on port %d...\n", PORT);
        fflush(stdout);
        // Accept incoming connection
        int* newfd = malloc(sizeof(int));
        if ((*newfd = accept(socketd, (struct sockaddr*) &cli, &cli_len)) < 0)
        {
            perror("\nFailed to accept connection");
            continue;
        }
        
        // Create a thread to handle client communication
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, newfd) != 0) {
            perror("Failed to create thread");
            continue;
        }

        // Detach the thread
        pthread_detach(tid);
    }

    return 0;
}
