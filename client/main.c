#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

const size_t PACKET_SZ = 1024;

void close_file_socket_descr(FILE* file, int sock, int descr) {
    if (file != NULL) {
        fclose(file);
    }

    if (sock >= 0) {
        close(sock);
    }

    if (descr >= 0) {
        close(descr);
    }
}

int get_socket(const char* ip, const int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "Socket was not created\n");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    errno = 0;
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        close_file_socket_descr(NULL, sock, -1);
        fprintf(stderr, "Problems with ip\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close_file_socket_descr(NULL, sock, -1);
        fprintf(stderr, "Problems with connection\n");
        perror("connect");
        return -1;
    }

    return sock;
}



void send_file(const char* ip, const int port, const char* filename, const char* server_filename) {
    int sock = get_socket(ip, port);
    if (sock < 0) {
        return;
    }

    FILE* file;
    if ((file = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "File cannot be open");
        close_file_socket_descr(NULL, sock, -1);
        return;
    }
    
    int descr = fileno(file);
    if (descr < 0) {
        fprintf(stderr, "Problems with file desctiptor");
        close_file_socket_descr(file, sock, -1);
        return;
    }

    struct stat statbuf = {};
    if (fstat(descr, &statbuf) != 0) {
        fprintf(stderr, "Problems with file descriptor");
        close_file_socket_descr(file, sock, descr);
        return;
    }
    int file_size = statbuf.st_size;

    // send filename
    if (send(sock, server_filename, PACKET_SZ, 0) < 0) {
        fprintf(stderr, "Problems with data sending");
        close_file_socket_descr(file, sock, descr);
        return;   
    }

    // send file size
    char data[PACKET_SZ];
    sprintf(data, "%d", file_size);
    if (send(sock, data, PACKET_SZ, 0) < 0) {
        fprintf(stderr, "Problems with data sending");
        close_file_socket_descr(file, sock, descr);
        return;   
    }
    memset(data, 0, sizeof(char) * PACKET_SZ);

    while (fread(data, sizeof(char), PACKET_SZ, file) != 0) {
        if (send(sock, data, PACKET_SZ, 0) < 0) {
            fprintf(stderr, "Problems with data sending");
            close_file_socket_descr(file, sock, descr);
            return;   
        }
        memset(data, 0, sizeof(char) * PACKET_SZ);
    }

    close_file_socket_descr(file, sock, descr);
}

int main(int argc, char** argv) {
    if (argc != 4 && argc != 5) {
        printf("Usage: %s <server ip> <server port> <file to send> <filename on server (optional)>", argv[0]);
        return 0;
    }

    char* ip = argv[1];

    int port;
    if (sscanf(argv[2], "%d", &port) != 1) {
        fprintf(stderr, "%s is not a valid port", argv[2]);
        return 0;
    }

    char* filename = argv[3];

    char* server_filename = filename;
    if (argc == 5) {
        server_filename = argv[4];
    }

    send_file(ip, port, filename, server_filename);
    return 0;
}