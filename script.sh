#!/bin/bash

# Установка зависимостей
sudo apt update
sudo apt install g++ make -y

# Создание директории и файла
sudo mkdir -p /var/server_files && sudo touch /var/server_files/file_out.txt

# Клонирование репозиториев сервера и клиента
git clone git@github.com:AllCash2436/ubuntu.git /tmp/server

# Компиляция и установка сервера
cd /tmp/server
g++ -pthread server.cpp -o server
sudo cp server /usr/local/bin/
sudo chmod +x /usr/local/bin/server

# Компиляция и установка клиента
cd /tmp/server
g++ -pthread client.cpp -o client
sudo cp client /usr/local/bin/
sudo chmod +x /usr/local/bin/client

# Создание конфигурационного файла для сервера
echo "MAX_THREADS=10" | sudo tee /etc/server_config.conf
echo "MAX_FILE_SIZE=1048576" | sudo tee -a /etc/server_config.conf
echo "SAVE_PATH='/var/server_files/'" | sudo tee -a /etc/server_config.conf

# Создание systemd unit файла для сервера
echo "[Unit]
Description=Custom Server Daemon
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/server 12345 '/var/server_files/file_out.txt' 10
Restart=on-failure
ExecReload=/bin/kill -HUP $MAINPID   # Обработчик сигнала SIGHUP
KillSignal=SIGTERM                    # Обработчик сигнала SIGTERM

[Install]
WantedBy=multi-user.target" | sudo tee /etc/systemd/system/server.service

# Перезагрузка демонов
sudo systemctl daemon-reload

# Запуск сервера и добавление его в автозагрузку
sudo systemctl start server
sudo systemctl enable server

echo "Установка завершена."
