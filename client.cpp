#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 12345 // Порт, который будет использоваться для соединения с сервером

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <filename> <server_address>\n";
        return 1;
    }

    std::string filename = argv[1];
    std::string serverAddress = argv[2];

    // Создание сокета
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Error: Unable to create socket\n";
        return 1;
    }

    // Задание адреса сервера и порта
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr);

    // Установление соединения с сервером
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: Connection failed\n";
        return 1;
    }

    // Открытие файла для чтения и отправка его содержимого на сервер
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Unable to open file\n";
        return 1;
    }

    std::stringstream fileContent;
    fileContent << file.rdbuf();
    std::string data = fileContent.str();

    if (send(clientSocket, data.c_str(), data.length(), 0) < 0) {
        std::cerr << "Error: Failed to send file content\n";
        return 1;
    }

    std::cout << "File sent successfully\n";

    // Закрытие сокета
    close(clientSocket);

    return 0;
}
