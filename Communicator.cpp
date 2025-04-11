//Модули
#include "Communicator.hpp"

//Сетевое взаимодействие
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <sys/stat.h>  // для mkdir()
#include <errno.h>      // для errno
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <fstream>

//Для хеширования
#include <sstream>
#include <cryptopp/hex.h>
#include <cryptopp/base64.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>

//Буст библиотеки для генерации SALT
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/variate_generator.hpp>

#define buff_size 4096
std::unique_ptr<char[]> buff(new char[buff_size]);
struct Client{
    int user_ID;
    std::string user_role;
    std::string client_ID;
    std::string client_type;
};

std::atomic<int> active_clients(0);
const int MAX_CLIENTS = 3;
std::mutex clients_mutex,database_mutex,log_mutex;
std::atomic<bool> server_running(true);

void Communicator::reg_user(int work_sock, const std::string& client_id, 
    std::map<int, std::pair<std::string, std::string>>& database,std::string filename,Logger* main_log) {
        std::unique_ptr<char[]> buff(new char[buff_size]);
        // Получаем данные для регистрации (логин:пароль)
        int rc = recv(work_sock, buff.get(), buff_size, 0);
        if (rc <= 0) {
        throw std::runtime_error("Failed to receive registration data");
        }

        std::string reg_data(buff.get(), rc);
        size_t delimiter_pos = reg_data.find(':');
        if (delimiter_pos == std::string::npos) {
        throw std::runtime_error("Invalid registration data format");
        }

        std::string login = reg_data.substr(0, delimiter_pos);
        std::string password = reg_data.substr(delimiter_pos + 1);

        //Проверка базы на существование такого логина
        for (const auto& entry : database) {
        if (entry.second.second == login) {
        std::string response = "User already exists";
        send(work_sock, response.c_str(), response.length(), 0);
        return;
        }
        }

        // Генерируем новый ID (просто максимальный существующий + 1) (Для оптимизации сетевого взаимодействия)
        int new_id = database.empty() ? 1 : database.rbegin()->first + 1;

        // Добавляем пользователя в базу
        database[new_id] = std::make_pair(password, "user");

        //Сохраняем базу в файл
        std::ofstream out;
        out.open(filename,std::ios::binary|std::ios::ate);//дозапись
        for (const auto& entry : database) {
            out << entry.first << ":" << entry.second.first << ":" << entry.second.second << "\n";
        }

        // Отправляем подтверждение
        std::string response = "OK";
        rc = send(work_sock, response.c_str(), response.length(), 0);
        if (rc <= 0) {
        throw std::runtime_error("Failed to send registration confirmation");
        }
        main_log->writelog(std::string("Новый пользователь зарегистрирован,Login:")+std::to_string(new_id));

}

bool Communicator::receive_file(int work_sock, const std::string& user_id,Logger* main_log) {
        std::unique_ptr<char[]> buff(new char[buff_size]);
        int rc = recv(work_sock, buff.get(), buff_size, 0);
        if (rc <= 0) {
            throw runtime_error("Failed to receive filename");
        }
        std::string filename(buff.get(), rc);
        rc = send(work_sock,"OK",2,0);
        rc = recv(work_sock, buff.get(), sizeof(size_t), 0);
        if (rc <= 0) {
            throw runtime_error("Failed to receive file size");
        }
        rc=send(work_sock,"OK",2,0);
        size_t file_size;
        memcpy(&file_size, buff.get(), sizeof(size_t));
        // Создаем папку для клиента, если ее нет
        std::string client_dir = "client_" + user_id;
        if (mkdir(client_dir.c_str(), 0777) == -1 && errno != EEXIST) {
            throw runtime_error("Failed to create client directory");
        }
        // Полный путь к файлу
        std::string filepath = client_dir + "/" + filename;
        
        // Открываем файл для записи
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            throw runtime_error("Failed to open file for writing");
        }
        
        // Получаем и записываем данные файла
        rc = recv(work_sock,buff.get(),buff_size,0);
        file.write(buff.get(),rc);
        main_log ->writelog("Файл успешно сохранён:" +filename);
        file.close();
        return true;
}
//************************************
//			Функция хеширования
//
//************************************
std::string Communicator::md5(std::string input_str)
{
    using namespace CryptoPP;
    Weak::MD5 hash;
    std::string new_hash;
    StringSource(input_str, true,
                 new HashFilter(hash, new HexEncoder(new StringSink(new_hash))));
    return new_hash;
}
//************************************
//			Генерация SALT
//
//************************************
std::string Communicator::generate_salt()
{
    time_t now = time(0);
    boost::mt19937 gen(now);
    boost::random::uniform_int_distribution<uint64_t> dist(0,std::numeric_limits<uint64_t>::max());
    boost::variate_generator<boost::mt19937&, boost::random::uniform_int_distribution<uint64_t>> getRand(gen, dist);
    std::stringstream s;
    s << std::hex << dist(gen);
    std::string results(s.str());
    while(results.length()<16) {
        results = '0' + results;
    }
    return results;
}
void Communicator::handle_client(int work_sock, std::map<int, std::pair<std::string, std::string>>& database, 
                  const std::string& path_basefile, Logger* main_log) {
    try {
        std::unique_ptr<char[]> buff(new char[buff_size]);
        Client client_data;
        
        // Получение ID клиента
        int rc = recv(work_sock, buff.get(), buff_size, 0);
        if (rc <= 0) {
            throw runtime_error("[Uncritical]ID receive error");
        }
        
        {
            std::lock_guard<std::mutex> log_lock(log_mutex);
            main_log->writelog("Client socket created");
            main_log->writelog("ID from client received");
        }
        
        buff[rc] = 0;
        std::string message(buff.get(), rc);
        
        // Проверка на пустую строку
        if (message.empty()) {
            std::string error_msg = "ERROR: Empty ID received";
            send(work_sock, error_msg.c_str(), error_msg.size(), 0);
            close(work_sock);
            main_log->writelog("Empty ID received from client");
            return;
        }
        
        std::string client_id = message.substr(1,6);
        
        // Проверка что ID не пустой
        if (client_id.empty()) {
            close(work_sock);
            main_log->writelog("Invalid ID format received: " + client_id);
            return;
        }
        client_data.client_ID=client_id;
        client_data.client_type = message[0];
        if (message.substr(7,rc).empty()){
            close(work_sock);
            main_log->writelog("Empty field user_id");
            return;
        }
        client_data.user_ID = stoi(message.substr(7,rc));

        bool check_flag = false;
        {
            std::lock_guard<std::mutex> db_lock(database_mutex);
            try {
                int client_id_num = client_data.user_ID;
                for (const auto& entry : database) {
                    if (entry.first == client_id_num) { 
                        check_flag = true; 
                        break;
                    }
                }
            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> log_lock(log_mutex);
                main_log->writelog("Error converting ID to number: " + std::string(e.what()));
                check_flag = false;
            }
        }
        
        if (!check_flag) {
            std::string message = "ID не найдено";
            rc = send(work_sock, message.c_str(), message.length(), 0);
            {
                std::lock_guard<std::mutex> log_lock(log_mutex);
                main_log->writelog("ID не найдено в БД: " + std::to_string(client_data.user_ID));
            }
            close(work_sock);
            return;
        }
        
        std::string salt_s = generate_salt();
        
        rc = send(work_sock, salt_s.c_str(), 16, 0);
        if (rc <= 0) {
            close(work_sock);
            throw runtime_error("[Uncritical]send SALT error");
        }
        
        rc = recv(work_sock, buff.get(), 32, 0);
        if (rc <= 0) {
            close(work_sock);
            throw runtime_error("[Uncritical]HASH received error");
        }
        
        buff[rc] = 0;
        std::string client_hash(buff.get(), rc);
        bool check_pass = false;
        
        {
            std::lock_guard<std::mutex> db_lock(database_mutex);
            for (const auto& entry : database) {
                if (md5(salt_s + entry.second.first) == client_hash) {
                    check_pass = true;
                }
            }
        }
        
        if (!check_pass) {
            close(work_sock);
            throw runtime_error("[Uncritical]Auth error");
        }
        
        {
            std::lock_guard<std::mutex> log_lock(log_mutex);
            main_log->writelog("Authentication success");
        }
        
        rc = send(work_sock, "OK", 2, 0);
        if (rc <= 0) {
            close(work_sock);
            throw runtime_error("[Uncritical]Send OK error");
        }
        
        
        if (client_data.client_type == "1") {
            {
                std::lock_guard<std::mutex> log_lock(log_mutex);
                main_log->writelog("Начало получения файла");
            }
            receive_file(work_sock, std::to_string(client_data.user_ID), main_log);
        } else if (client_data.client_type == "2") {
            {
                std::lock_guard<std::mutex> log_lock(log_mutex);
                main_log->writelog("Начало регистрации пользователя");
            }
            reg_user(work_sock, client_data.client_ID, database, path_basefile, main_log);
        }
        
    } catch (runtime_error& e) {
        std::lock_guard<std::mutex> log_lock(log_mutex);
        main_log->writelog(e.what());
    }
    
    close(work_sock);
}
//************************************
//		  Функция соединения
//
//************************************
int Communicator::connection(int port, std::map<int, std::pair<std::string, std::string>> database, 
    std::string path_basefile, Logger* main_log) {
    try {
        int queue_len = 100;
        sockaddr_in* addr = new (sockaddr_in);
        addr->sin_family = AF_INET;
        addr->sin_port = htons(port);
        inet_aton("127.0.0.1", &addr->sin_addr);

        int s = socket(AF_INET, SOCK_STREAM, 0);
        if (s <= 0) {
            throw domain_error("Socket created err");
        } else {
            main_log->writelog("listen socket created");
        }

        int rc = bind(s, (const sockaddr*)addr, sizeof(sockaddr_in));
        if (rc == -1) {
            throw domain_error("Socket bind err");
        } else {
            main_log->writelog("bind success");
        }

        rc = listen(s, queue_len);
        if (rc == -1) {
            throw domain_error("Socket listen err");
        }

        // Вектор для хранения потоков
        std::vector<std::thread> client_threads;

        // Основной цикл обработки клиентов
        while (server_running) {
            try {
                sockaddr_in* client_addr = new sockaddr_in;
                socklen_t len = sizeof(sockaddr_in);
                int work_sock = accept(s, (sockaddr*)(client_addr), &len);

                if (work_sock <= 0) {
                    throw runtime_error("[Uncritical]Client socket error");
                }

                // Проверяем количество активных клиентов
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    if (active_clients >= MAX_CLIENTS) {
                        std::string error_msg = "ERROR: Server busy. Maximum connections (" + 
                                              std::to_string(MAX_CLIENTS) + ") reached";
                        send(work_sock, error_msg.c_str(), error_msg.size(), 0);
                        close(work_sock);
                        main_log->writelog("Connection rejected - server busy");
                        continue;
                    }
                    active_clients++;
                }

                // Создаем новый поток для обработки клиента
                client_threads.emplace_back([this, work_sock, &database, path_basefile, main_log]() {
                    handle_client(work_sock, database, path_basefile, main_log);
                    
                    // Уменьшаем счетчик активных клиентов при завершении
                    {
                        std::lock_guard<std::mutex> lock(clients_mutex);
                        active_clients--;
                    }
                });

            } catch (runtime_error& e) {
                std::lock_guard<std::mutex> log_lock(log_mutex);
                main_log->writelog(e.what());
            }
        }

        // Ожидаем завершения всех потоков перед выходом
        for (auto& thread : client_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    } catch (domain_error& e) {
        main_log->writelog(e.what());
        return -1;
    }

    return 0;
}