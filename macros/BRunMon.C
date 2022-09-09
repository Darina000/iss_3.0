
#include "../include/iss_sort.hh"
#include <iostream>

int Start(){
    start_monitoring();
    std::cout << "RunMon: " << bRunMon << std::endl;
    return 0;
}

int Stop(){
    stop_monitoring();
    std::cout << "RunMon: " << bRunMon << std::endl;
    return 0;
}


int Reset_event_building(){
    reset_event_building();
    std::cout << "Reset in event building: " << bReset_event_building << std::endl;
    return 0;
}

int Reset_hist(){
    reset_hist();
    std::cout << "Reset in hist: " << bReset_hist << std::endl;
    return 0;
}

int Reset(){
    reset_monitoring();
    std::cout << "Reset: " << bReset << std::endl;
    return 0;
}

