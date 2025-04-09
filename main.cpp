#include "Interface.hpp"

int main(int argc,char** argv) {
    Interface UI;
    UI.Analysis(argc,argv);
    auto result=UI.get_params();
    return 0;
}