#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/file.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

const size_t PACKET_SZ = 1024;

int get_socket(const char* ip, const int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("can't use this ip");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
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
        printf("Не удалось открыть файл");
        close(sock);
        return;
    }
    
    int descr = fileno(file);
    if (descr < 0) {
        printf("Не удалось открыть файл");
        fclose(file);
        close(sock);
        return;
    }

    struct stat statbuf = {};
    if (fstat(descr, &statbuf) != 0) {
        printf("Не удалось открыть файл");
        close(descr);
        fclose(file);
        close(sock);
        return;
    }
    int file_size = statbuf.st_size;

    char data[PACKET_SZ];

    // send filename
    send(sock, server_filename, PACKET_SZ, 0);

    // send file size 
    sprintf(data, "%d", file_size);
    send(sock, data, PACKET_SZ, 0);
    memset(data, 0, sizeof(char) * PACKET_SZ);

    while (fread(data, sizeof(char), PACKET_SZ, file) != 0) {
        send(sock, data, PACKET_SZ, 0);
        memset(data, 0, sizeof(char) * PACKET_SZ);
    }

    close(descr);
    fclose(file);
    close(sock);
}

int main(int argc, char** argv) {
    if (argc != 4 && argc != 5) {
        printf("Usage: %s <server ip> <server port> <file to send> <filename on server (optional)>", argv[0]);
        return 0;
    }

    char* ip = argv[1];

    int port;
    sscanf(argv[2], "%d", &port); //check

    char* filename = argv[3];

    char* server_filename = filename;
    if (argc == 5) {
        server_filename = argv[4];
    }

    send_file(ip, port, filename, server_filename);
    return 0;
}