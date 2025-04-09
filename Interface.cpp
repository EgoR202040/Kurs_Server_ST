#include "Interface.hpp"
#include "Connector_base.hpp" //Временно
#include "Communicator.hpp"

int Interface::Analysis(int argc,char** argv) {
    try{
    namespace po = boost::program_options;
    po::options_description opts("Allowed options");
    opts.add_options()
    ("help,h", "Show help")
    ("basefile,b",
     po::value<std::string>(&path_basefile)->default_value("base.txt"),
     "option is string(path to file with database(.txt))")
    ("logfile,l",
     po::value<std::string>(&path_logfile)->default_value("log.txt"),
     "option is string(path to file with logs)")
    ("PORT,p",
     po::value<int>(&PORT)->default_value(33333),
     "option is int (PORT for server)");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);
    if(vm.count("help")) {
        std::cout << opts << "\n";
        exit(0);
    }
    std::ifstream test(path_basefile);
    if(!test){
       throw domain_error("Invalid path to basefile");
    }else{
        if(test.peek()== std::ifstream::traits_type::eof()){
            throw domain_error("basefile is empty");
        }
    }
    test.close();
    Connector_base con;
    con.connect(path_basefile);
    auto data = con.get_data();
    /*
    for (const auto& entry : data) {
        int id = entry.first;
        std::string password = entry.second.first;
        std::string role = entry.second.second;
        
        std::cout << "ID: " << id 
                  << " | Password: " << password 
                  << " | Role: " << role 
                  << "\n";
    }*/
    Communicator tcp_ip;
    Logger main_log(path_logfile);
    tcp_ip.connection(PORT,data,path_basefile,&main_log);
    }catch(boost::program_options::error& e){
        std::cerr << e.what() << std::endl;
    }catch(domain_error& e){
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

UI_data Interface::get_params() {
    UI_data data;
    data.bf=path_basefile;
    data.lf = path_logfile;
    data.port = PORT;
    return data;
}