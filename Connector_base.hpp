#include <map>
#include <string>
#include <fstream>
#include <utility>

class Connector_base{
    std::map<int, std::pair<std::string, std::string>> base_data;
    public:
    void connect(std::string path);
    std::map<int, std::pair<std::string, std::string>> get_data();
};