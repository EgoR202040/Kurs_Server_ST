#include <boost/program_options.hpp>
#include <string>
#include <iostream>
#include <fstream>

struct UI_data{
    std::string bf,lf;
    int port;
};

class Interface{
    int PORT;
    std::string path_basefile;
    std::string path_logfile;
    public:
    int Analysis(int argc,char** argv);
    UI_data get_params();
};