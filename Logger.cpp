#include "Logger.hpp"
#include <mutex>
#include <stdexcept>
#include <experimental/filesystem>
#include <iomanip>
#include <iostream>

using namespace std;

namespace fs = std::experimental::filesystem;

// Мьютекс для потокобезопасности
static std::mutex log_mutex;

Logger::Logger(const string& path_file) {
    set_path(path_file);
}

int Logger::set_path(const string& path_file) {
    lock_guard<mutex> lock(log_mutex);

    // Проверка расширения файла
    if (path_file.find('.') == string::npos) {
        throw invalid_argument("File must have an extension");
    }

    // Проверка и создание директорий при необходимости
    fs::path log_path(path_file);
    if (log_path.has_parent_path() && !fs::exists(log_path.parent_path())) {
        try {
            fs::create_directories(log_path.parent_path());
        } catch (const fs::filesystem_error& e) {
            throw runtime_error("Failed to create log directory: " + string(e.what()));
        }
    }

    // Проверка возможности записи в файл
    ofstream test_stream(path_file, ios_base::out | ios_base::app);
    if (!test_stream) {
        throw runtime_error("Cannot open log file for writing: " + path_file);
    }
    test_stream.close();

    path_to_logfile = path_file;
    return 0;
}

string Logger::getCurrentDateTime(const string& format){
    time_t now = time(nullptr);
    tm tstruct{};
    char buf[80];

    localtime_r(&now, &tstruct); // Безопасная версия для многопоточности

    const char* fmt = "%Y-%m-%d %T"; // формат по умолчанию
    if (format == "date") {
        fmt = "%Y-%m-%d";
    } else if (format == "time") {
        fmt = "%T";
    } else if (format == "short") {
        fmt = "%Y%m%d_%H%M%S";
    }

    strftime(buf, sizeof(buf), fmt, &tstruct);
    return string(buf);
}


int Logger::writelog(string message,std::string client_ID) {
    lock_guard<mutex> lock(log_mutex); //Блокировка логирования

    ofstream filelog(path_to_logfile, ios_base::out | ios_base::app);
    if (!filelog) {
        try {
            set_path(path_to_logfile);
            filelog.open(path_to_logfile, ios_base::out | ios_base::app);
            if (!filelog) throw runtime_error("");
        } catch (...) {
            cerr << "[" << getCurrentDateTime("now") 
                 << "] [ERROR] Failed to open log file: " << path_to_logfile << endl;
            return -1;
        }
    }

    try {
        if(client_ID=="None"){
            filelog << "[" << getCurrentDateTime("now") << "] "
               << message << endl;
        }else{
            filelog << "[" << getCurrentDateTime("now") << "] " <<"["<<client_ID <<"]"
               << message << endl;
        }
        // Проверяем, что запись прошла успешно
        if (filelog.fail()) {
            throw runtime_error("Write operation failed");
        }
    } catch (const exception& e) {
        cerr << "[" << getCurrentDateTime("now") << "] [ERROR] Log write failed: " 
             << e.what() << endl;
        return -1;
    }

    return 0;
}
