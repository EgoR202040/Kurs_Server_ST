#include <string>
#include <chrono>
#include <time.h>
#include <fstream>
using namespace std;
class Logger
{
    static std::string getCurrentDateTime(const string& format);
    std::string path_to_logfile;
public:
    int writelog(string message);
    int writelog(string level,string message);
    int operator()(const string& message);
    int set_path(const string& path_file);
    Logger(const string& path_file);
};