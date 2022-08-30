
#include "../include/iss_sort.hh"
#include <iostream>


int Reset(){
    reset_monitoring();
    std::cout << "Reset: " << bReset << std::endl;
    return 0;
}

int Exit(){
    exit_monitoring();
    std::cout << "Exit: " << bExit << std::endl;
    return 0;
}
