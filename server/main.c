#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int get_listener(const int port) {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
        perror("can't use this ip");
        return -1;
    }


    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

    listen(listener, 1);

    return listener;
}

void loop(const int port, const char* folder) {
    int listener = get_listener(port);
    if (listener < 0) {
        return;
    }

    char buf[1024];
    int bytes_read;

    while(1) {
        int sock = accept(listener, NULL, NULL);
        if (sock < 0) {
            perror("accept");
            continue;
        }

        // read filename
        bytes_read = recv(sock, buf, 1024, 0);
        if (bytes_read <= 0) {
            close(sock);
            continue;
        }

        char* full_filename = malloc(strlen(folder) + 1 + strlen(buf));
        strcpy(full_filename, folder);
        strcat(full_filename, "/");
        strcat(full_filename, buf);

        FILE* file;
        if ((file = fopen(full_filename, "w")) == NULL) {
            printf("Не удалось создать и открыть файл %s", buf);
            close(sock);
            continue;
        }
        free(full_filename);

        // read file size
        bytes_read = recv(sock, buf, 1024, 0);
        if (bytes_read <= 0) {
            fclose(file);
            close(sock);
            continue;
        }

        int file_size;
        sscanf(buf, "%d", &file_size);

        while (file_size > 0) {
            bytes_read = recv(sock, buf, 1024, 0);
            printf("%d %s %d\n", bytes_read, buf, (int)(buf[3]));
            if (bytes_read <= 0) {
                break;
            }

            if (file_size < 1024) {
                fwrite(buf, sizeof(char), file_size, file);
                file_size = 0;
            } else {
                fwrite(buf, sizeof(char), 1024, file);
                file_size -= 1024;
            }
        }

        fclose(file);
        close(sock);
    }
}


int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: %s <port> <folder>", argv[0]);
        return 0;
    }

    int port;
    sscanf(argv[1], "%d", &port);

    char* folder = argv[2];

    loop(port, folder);
    return 0;
}