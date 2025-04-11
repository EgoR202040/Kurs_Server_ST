#pragma once
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include "Logger.hpp"
#include "Connector_base.hpp"

using namespace std;
struct Communicator
{
    void handle_client(int work_sock, std::map<int, std::pair<std::string, std::string>>& database, 
        const std::string& path_basefile, Logger* main_log);
    int connection(int port,std::map<int,std::pair<std::string,std::string>> database,std::string path_basefile,Logger* main_log);
    static std::string md5(std::string input_str);
    static std::string generate_salt();
    void reg_user(int work_sock, const std::string& client_id, std::map<int, std::pair<std::string, std::string>>& database,std::string path_basefile,Logger* main_log);
    //void service(int s,std::map<int,std::pair<std::string,std::string>> database); Заготовка под многопоточность
    bool receive_file(int work_sock, const std::string& user_id,Logger* main_log,std::string client_id);
};
