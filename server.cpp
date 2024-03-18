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

constexpr int MAX_CONCURRENCY = 10;
constexpr int MAX_BUFF_SIZE = 1024;

struct ClientData {
    int socket;
    const char* save_path;
};

void* clientHandler(void* arg) {
    ClientData* data = reinterpret_cast<ClientData*>(arg);

    char buffer[MAX_BUFF_SIZE];
    ssize_t bytes_received;

    bytes_received = recv(data->socket, buffer, MAX_BUFF_SIZE, 0);
    if (bytes_received < 0) {
        perror("Error receiving data");
        close(data->socket);
        delete data;
        pthread_exit(NULL);
    }

    FILE* file = fopen(data->save_path, "w");
    if (file == nullptr) {
        perror("Error opening file");
        close(data->socket);
        delete data;
        pthread_exit(NULL);
    }
    size_t bytes_written = fwrite(buffer, 1, bytes_received, file);
    if (bytes_written != static_cast<size_t>(bytes_received)) {
        perror("Error writing to file");
        fclose(file);
        close(data->socket);
        delete data;
        pthread_exit(NULL);
    }
    fclose(file);

    if (bytes_written > 0) {
        std::cout << "File written successfully: " << data->save_path << std::endl;
    } else {
        std::cout << "Failed to write file: " << data->save_path << std::endl;
    }

    close(data->socket);
    delete data;
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <port> <save_path> <max_concurrency>\n";
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    const char* save_path = argv[2];
    int max_concurrency = atoi(argv[3]);

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(server_socket);
        return EXIT_FAILURE;
    }

    listen(server_socket, 5);

    pthread_t threads[MAX_CONCURRENCY];
    int thread_count = 0;

    while (true) {
        client_socket = accept(server_socket, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
        if (client_socket < 0) {
            perror("Error accepting connection");
            continue;
        }

        ClientData* client_data = new ClientData;
        client_data->socket = client_socket;
        client_data->save_path = save_path;

        pthread_create(&threads[thread_count++ % max_concurrency], NULL, clientHandler, client_data);
    }

    close(server_socket);
    return EXIT_SUCCESS;
}
