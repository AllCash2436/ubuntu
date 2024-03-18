#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

constexpr int MAX_THR = 10;
constexpr int MAX_BUF = 1024;

struct Args {
    int socket;
    const char* path;
    char addr[INET_ADDRSTRLEN];
};

void* handle(void* arg) {
    Args* args = reinterpret_cast<Args*>(arg);

    char buf[MAX_BUF];
    ssize_t bytes;

    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    getpeername(args->socket, reinterpret_cast<struct sockaddr*>(&client), &len);
    inet_ntop(AF_INET, &(client.sin_addr), args->addr, INET_ADDRSTRLEN);

    std::cout << "Client connected from: " << args->addr << std::endl;

    bytes = recv(args->socket, buf, MAX_BUF, 0);
    if (bytes < 0) {
        perror("Error receiving data");
        close(args->socket);
        delete args;
        pthread_exit(NULL);
    }

    FILE* file = fopen(args->path, "w");
    if (file == nullptr) {
        perror("Error opening file");
        close(args->socket);
        delete args;
        pthread_exit(NULL);
    }
    size_t written = fwrite(buf, 1, bytes, file);
    if (written != static_cast<size_t>(bytes)) {
        perror("Error writing to file");
        fclose(file);
        close(args->socket);
        delete args;
        pthread_exit(NULL);
    }
    fclose(file);

    if (written > 0) {
        std::cout << "File written successfully: " << args->path << std::endl;
    } else {
        std::cout << "Failed to write file: " << args->path << std::endl;
    }

    close(args->socket);
    delete args;
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <port> <path> <max_threads>\n";
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    const char* path = argv[2];
    int max_thr = atoi(argv[3]);

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        perror("Error binding socket");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    listen(serverSocket, 5);

    pthread_t threads[MAX_THR];
    int count = 0;

    while (true) {
        clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientLen);
        if (clientSocket < 0) {
            perror("Error accepting connection");
            continue;
        }

        Args* args = new Args;
        args->socket = clientSocket;
        args->path = path;

        pthread_create(&threads[count++ % max_thr], NULL, handle, args);
    }

    close(serverSocket);
    return EXIT_SUCCESS;
}
