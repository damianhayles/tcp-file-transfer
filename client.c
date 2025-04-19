/*======================================================================
| TCP/IP Client (Sockets)
|
| Name: client.c
|
|------------------------------------------------------------------
*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>


// initializing global constants
#define BUF 1024
#define PORT 59589

struct datapacket {
    int nbytes;
    char payload[BUF];
} __attribute__((packed));


int main() {
    int socketd;
    struct sockaddr_in srv;
    char buf[BUF];


    // Create TCP socket
    if ((socketd=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("\nFailed to create a socket");
        printf("\nProgram will now terminate.");
        exit(1);
    }

    // Create server address structure
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(PORT);
    srv.sin_addr.s_addr = htonl(INADDR_ANY); // Connect to localhost

    // Connect to server
    if (connect(socketd, (struct sockaddr*) &srv, sizeof(srv)) < 0) {
        perror("Failed to connect to server");
        exit(1);
    }

    printf("Connected to server :)");

    // Read greeting from server :)
    if (read(socketd, buf, sizeof(buf)) > 0) {
        printf("%s\n", buf);
    }

    while (1) {
        // Ask for filepath from user
        printf("\nPlease input the filepath of the file\nyou would like to copy to the server (or type DONE to finish):\n");
        fgets(buf, sizeof(buf), stdin);
        buf[strcspn(buf, "\n")] = 0;    // Remove newline if present

        // Send 'DONE' to tell server to close connection
        if (strcmp(buf, "DONE") == 0) {
            write(socketd, buf, strlen(buf));
            break;
        }

        // Send file path to server
        write(socketd, (char*)buf, strlen(buf));

        // Open file on client side
        int input_fd = open(buf, O_RDONLY);
        if (input_fd < 0) {
            perror("Failed to open input file");
            close(socketd);
            exit(1);
        }

        // Get total file size for a progress bar
        struct stat st;
        if (stat(buf, &st) != 0) {
            perror("Failed to create stat struct properly...\n");
            close(input_fd);
            close(socketd);
            exit(1);
        }
        off_t total_size = st.st_size;
        off_t total_sent = 0;

        // Read response from server
        memset(buf, 0, sizeof(buf));
        if (read(socketd, buf, sizeof(buf)) < 0)
        {
            perror("Read error");
            exit(1);
        }
        printf("%s", buf);
        memset(buf, 0, sizeof(buf));


        // Read data packets and send them to the server
        printf("\nCopying...\n");

        struct datapacket data;
        ssize_t bytes_read;

        while ((bytes_read = read(input_fd, data.payload, BUF)) > 0) {
            data.nbytes = bytes_read;

            // Send nbytes first
            if (send(socketd, &data.nbytes, sizeof(int), 0) < 0) {
                perror("Send failed\n");
                close(input_fd);
                close(socketd);
                exit(1);
            }

            // Then send payload
            if (send(socketd, &data.payload, data.nbytes, 0) < 0) {
                perror("Send failed\n");
                close(input_fd);
                close(socketd);
                exit(1);
            }

            total_sent += bytes_read;

            // Print progress bar in terminal
            int bar_width = 50;
            float progress = (float)total_sent / total_size;
            int pos = (int)(bar_width * progress);
            printf("\r[");
            for (int i = 0; i < bar_width; ++i) {
                if (i < pos) printf("=");
                else if (i == pos) printf(">");
                else printf(" ");
            }
            printf("] %3.0f%%", progress * 100);
            fflush(stdout);
        }

        // Send packet of 0 bytes to signify EOF
        data.nbytes = 0;
        if (send(socketd, &data.nbytes, sizeof(int), 0) < 0) {
            perror("Send EOF failed\n");
            close(input_fd);
            close(socketd);
            exit(1);
        }
        printf("\nTransfer complete!\n");

        close(input_fd);
    }
    close(socketd);
    printf("Bye!\n");

    return 0;

}
