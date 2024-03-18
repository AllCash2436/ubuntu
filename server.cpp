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

constexpr int MAX_THREADS = 10;
constexpr int MAX_BUFFER_SIZE = 1024;

struct ThreadArgs {
    int clientSocket;
    const char* savePath;
    char clientAddress[INET_ADDRSTRLEN]; // Буфер для хранения IP-адреса клиента
};

void* handleClient(void* arg) {
    ThreadArgs* threadArgs = reinterpret_cast<ThreadArgs*>(arg);

    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytesReceived;

    // Получение IP-адреса клиента
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    getpeername(threadArgs->clientSocket, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen);
    inet_ntop(AF_INET, &(clientAddr.sin_addr), threadArgs->clientAddress, INET_ADDRSTRLEN);

    std::cout << "Клиент подключился с IP-адреса: " << threadArgs->clientAddress << std::endl;

    bytesReceived = recv(threadArgs->clientSocket, buffer, MAX_BUFFER_SIZE, 0);
    if (bytesReceived < 0) {
        perror("Ошибка при приеме данных");
        close(threadArgs->clientSocket);
        delete threadArgs;
        pthread_exit(NULL);
    }

    FILE* file = fopen(threadArgs->savePath, "w");
    if (file == nullptr) {
        perror("Ошибка открытия файла");
        close(threadArgs->clientSocket);
        delete threadArgs;
        pthread_exit(NULL);
    }
    size_t bytesWritten = fwrite(buffer, 1, bytesReceived, file);
    if (bytesWritten != static_cast<size_t>(bytesReceived)) {
        perror("Ошибка записи в файл");
        fclose(file);
        close(threadArgs->clientSocket);
        delete threadArgs;
        pthread_exit(NULL);
    }
    fclose(file);

    if (bytesWritten > 0) {
        std::cout << "Файл успешно записан: " << threadArgs->savePath << std::endl;
    } else {
        std::cout << "Ошибка при записи файла: " << threadArgs->savePath << std::endl;
    }

    close(threadArgs->clientSocket);
    delete threadArgs;
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Использование: " << argv[0] << " <порт> <путь_для_сохранения> <макс_потоков>\n";
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    const char* savePath = argv[2];
    int maxThreads = atoi(argv[3]);

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Ошибка создания сокета");
        return EXIT_FAILURE;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        perror("Ошибка привязки сокета");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    listen(serverSocket, 5);

    pthread_t threads[MAX_THREADS];
    int threadCount = 0;

    while (true) {
        clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientLen);
        if (clientSocket < 0) {
            perror("Ошибка при подключении");
            continue;
        }

        ThreadArgs* args = new ThreadArgs;
        args->clientSocket = clientSocket;
        args->savePath = savePath;

        pthread_create(&threads[threadCount++ % maxThreads], NULL, handleClient, args);
    }

    close(serverSocket);
    return EXIT_SUCCESS;
}
