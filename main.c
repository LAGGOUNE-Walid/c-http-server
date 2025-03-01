#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024
#define PORT 8081

int main(void)
{
    int server_fd, client_fd;
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket failed");
        return 1;
    }

    // Configure address
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    // Start listening
    if (listen(server_fd, 10) < 0)
    {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    printf("Server running on port %d...\n", PORT);

    while (1)
    {
        // Accept client connection
        if ((client_fd = accept(server_fd, NULL, NULL)) < 0)
        {
            perror("Accept failed");
            continue;
        }

        // Receive request
        recv(client_fd, buffer, BUFFER_SIZE, 0);
        printf("Request:\n%s\n", buffer);

        // Parse GET request
        if (strncmp(buffer, "GET /", 5) != 0)
        {
            close(client_fd);
            continue;
        }

        char *f = buffer + 5;
        char *space = strchr(f, ' ');
        if (!space)
        {
            close(client_fd);
            continue;
        }
        *space = '\0'; // Null-terminate filename

        // Open requested file
        int file_fd = open(f, O_RDONLY);
        if (file_fd < 0)
        {
            char *error_response = "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found";
            send(client_fd, error_response, strlen(error_response), 0);
            close(client_fd);
            continue;
        }

        // Send HTTP header
        char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n";
        send(client_fd, header, strlen(header), 0);

        // Send file content
        off_t offset = 0;
        struct stat file_stat;
        fstat(file_fd, &file_stat);
        sendfile(client_fd, file_fd, &offset, file_stat.st_size);

        // Close connections
        close(file_fd);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}