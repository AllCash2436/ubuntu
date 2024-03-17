#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 12345 // Порт, который будет прослушиваться сервером
#define MAX_THREADS 10 // Максимальное количество потоков

std::mutex mtx; // Мьютекс для обеспечения безопасного доступа к файлу

// Функция для обработки соединения с клиентом
void handleClient(int clientSocket) {
    char buffer[4096];
    int bytesRead;
    std::string fileData;

    // Получение данных от клиента
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        fileData.append(buffer, bytesRead);
    }

    // Обработка ошибок при получении данных
    if (bytesRead < 0) {
        std::cerr << "Error: Failed to receive data from client\n";
        close(clientSocket);
        return;
    }

    // Сохранение данных в файл
    mtx.lock();
    std::ofstream outfile("received_file.txt", std::ios::binary);
    if (!outfile) {
        std::cerr << "Error: Unable to open file for writing\n";
        mtx.unlock();
        close(clientSocket);
        return;
    }
    outfile << fileData;
    outfile.close();
    mtx.unlock();

    std::cout << "File received and saved successfully\n";

    // Закрытие сокета
    close(clientSocket);
}

int main() {
    // Создание сокета
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error: Unable to create socket\n";
        return 1;
    }

    // Настройка адреса сервера и порта
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Привязка сокета к адресу и порту
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: Bind failed\n";
        return 1;
    }

    // Начало прослушивания входящих соединений
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Error: Listen failed\n";
        return 1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    // Принятие входящих соединений и обработка каждого в отдельном потоке
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            std::cerr << "Error: Accept failed\n";
            continue;
        }

        std::thread(handleClient, clientSocket).detach(); // Создание нового потока для обработки клиента
    }

    // Закрытие сокета сервера (не достижимо в данном случае, так как сервер бесконечно ждет подключений)
    close(serverSocket);

    return 0;
}
