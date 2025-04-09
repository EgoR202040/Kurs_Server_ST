#include "Connector_base.hpp"

std::map<int, std::pair<std::string, std::string>> Connector_base::get_data(){
    return base_data;
}

void Connector_base::connect(std::string path){
    std::ifstream fileb(path);
    if (!fileb.is_open()) return;

    std::string delimitter = ":";
    std::string s;
    
    while(getline(fileb, s)){
        size_t first_pos = s.find(delimitter);
        if(first_pos == std::string::npos) continue;

        size_t second_pos = s.find(delimitter, first_pos + 1);
        if(second_pos == std::string::npos) continue;
        try {
            int id = std::stoi(s.substr(0, first_pos));
            std::string pass = s.substr(first_pos + 1, second_pos - first_pos - 1);
            std::string role = s.substr(second_pos + 1);
            
            base_data[id] = std::make_pair(pass, role);
        } 
        catch(const std::exception& e) {
            // Пропускаем некорректные записи в случае случайных записей в БД
            continue;
        }
    }
    fileb.close();
}