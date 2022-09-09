
//iss_sort.h


Bool_t bRunMon = kTRUE;
Bool_t bReset = kFALSE;
Bool_t bFirstRun = kTRUE;
Bool_t bExit = kFALSE;
Bool_t bReset_event_building = kFALSE;
Bool_t bReset_hist = kFALSE;

void start_monitoring();  //changing bRunMon to kTRUE
void stop_monitoring();  //changing bRunMon to kFALSE
void reset_monitoring();  //changing bReset to kTRUE
//void exit_monitoring();  //changing bExit to kTRUE

void reset_event_building(); //changing bReset_timing to kTRUE
void reset_hist(); //changing bReset_timing to kTRUE


