#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const size_t PACKET_SZ = 1024;

void close_file_socket(FILE* file, int sock) {
    if (file != NULL) {
        fclose(file);
    }

    if (sock >= 0) {
        close(sock);
    }
}

int get_listener(const int port) {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        fprintf(stderr, "Socket was not created\n");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
        close_file_socket(NULL, listener);
        fprintf(stderr, "Problems with ip\n");
        return -1;
    }

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close_file_socket(NULL, listener);
        fprintf(stderr, "Problems with binding\n");
        return -1;
    }

    if (listen(listener, 1) < 0) {
        close_file_socket(NULL, listener);
        fprintf(stderr, "Problems with listening\n");
        return -1;
    }

    return listener;
}

void loop(const int port, const char* folder) {
    int listener = get_listener(port);
    if (listener < 0) {
        return;
    }

    char buf[PACKET_SZ];
    int bytes_read;
    while (1) {
        int sock = accept(listener, NULL, NULL);
        if (sock < 0) {
            fprintf(stderr, "Problems with socket accepting");
            continue;
        }

        // read filename
        bytes_read = recv(sock, buf, PACKET_SZ, 0);
        if (bytes_read <= 0) {
            close(sock);
            continue;
        }

        char* full_filename = malloc(strlen(folder) + 1 + strlen(buf));
        if (full_filename == NULL) {
            fprintf(stderr, "Maybe the memory has run out");
            close_file_socket(NULL, sock);
            continue;
        }
        strcpy(full_filename, folder);
        strcat(full_filename, "/");
        strcat(full_filename, buf);

        FILE* file;
        if ((file = fopen(full_filename, "w")) == NULL) {
            fprintf(stderr, "Problems with file opening : %s", buf);
            close_file_socket(NULL, sock);
            continue;
        }
        free(full_filename);

        // read file size
        bytes_read = recv(sock, buf, PACKET_SZ, 0);
        if (bytes_read <= 0) {
            fprintf(stderr, "Problems with getting data : %s", buf);
            close_file_socket(file, sock);
            continue;
        }

        int file_size;
        sscanf(buf, "%d", &file_size);

        while (file_size > 0) {
            bytes_read = recv(sock, buf, PACKET_SZ, 0);
            if (bytes_read <= 0) {
                fprintf(stderr, "Problems with getting data : %s", buf);
                break;
            }

            size_t cur_sz = PACKET_SZ;
            if (file_size < cur_sz) {
                cur_sz = file_size;
            }

            if (fwrite(buf, sizeof(char), cur_sz, file) != cur_sz) {
                fprintf(stderr, "Problems with saving data : %s", buf);
                break;                
            }
            file_size -= cur_sz;
        }

        close_file_socket(file, sock);
    }
}


int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: %s <port> <folder>", argv[0]);
        return 0;
    }

    int port;
    if (sscanf(argv[1], "%d", &port) != 1) {
        fprintf(stderr, "%s is not a valid port", argv[1]);
        return 0;
    }

    char* folder = argv[2];

    loop(port, folder);
    return 0;
}