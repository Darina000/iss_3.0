// My code include.
#include "Settings.hh"
#include "Calibration.hh"
#include "Converter.hh"
#include "TimeSorter.hh"
#include "EventBuilder.hh"
#include "Reaction.hh"
#include "Histogrammer.hh"
#include "AutoCalibrator.hh"
#include "ISSGUI.hh"

// ROOT include.
#include <TTree.h>
#include <TFile.h>
#include <THttpServer.h>
#include <TThread.h>
#include <TGClient.h>
#include <TApplication.h>

// C++ include.
#include <iostream>

#include <stdio.h>
#include <string>
#include <vector>
#include <sstream>


#include <stdlib.h>






#include <unistd.h>
//#include <dataspy.h>

//#include "dataspy.hh"


// Command line interface
#ifndef __COMMAND_LINE_INTERFACE
# include "CommandLineInterface.hh"
#endif





// Default parameters and name
std::string output_name;
std::string datadir_name = "/eos/experiment/isolde-iss/2022/ISS";
std::string name_set_file;
std::string name_cal_file;
std::string name_react_file;


std::vector<std::string> input_names;


// a flag at the input to force or not the conversion
bool flag_convert = false;
bool flag_events = false;
bool flag_source = false;
bool flag_autocal = false;

// select what steps of the analysis to be forced
std::vector<bool> force_convert;
bool force_sort = false;
bool force_events = false;

// Flag if we want to launch the GUI for sorting
bool gui_flag = false;

// Flag for somebody needing help on command line
bool help_flag = false;


//Flag for data_spy
bool flag_spy = false;
int open_spy_data = -1;



// Monitoring input file
bool flag_monitor = false;
int mon_time = -1; // update time in seconds


// Settings file
ISSSettings *myset;

// Calibration file
ISSCalibration *mycal;
bool overwrite_cal = false;

// Reaction file
ISSReaction *myreact;

// Struct for passing to the thread
typedef struct thptr {
	
	ISSCalibration *mycal;
	ISSSettings *myset;
	ISSReaction *myreact;
	
} thread_data;

// Server and controls for the GUI
THttpServer *serv;
Bool_t bRunMon = kTRUE;
Bool_t bFirstRun = kTRUE;
std::string curFileMon;
int port_num = 8030;

// Function to call the monitoring loop
void* monitor_run( void* ptr ){
//void monitor_run(){
	
	/// This function is called to run when monitoring
	
	// Get the settings, file etc.
	thptr *calfiles = (thptr*)ptr;
	
	// Setup the different steps
	ISSConverter conv_mon( calfiles->myset /*to Settings file*/ , flag_spy);
    
    /*
     here we call convertFile for setting file
     */
    /*
     int ConvertFile( std::string input_file_name,
                     unsigned long start_block = 0,
                     long end_block = -1);
     */
	ISSTimeSorter sort_mon;
	ISSEventBuilder eb_mon( calfiles->myset /*to Settings file*/);
    
	ISSHistogrammer hist_mon( calfiles->myreact /*toReaction file*/, calfiles->myset/*to Settings file*/ );
    
   // DataSpy dataspyclass;

	// Data/Event counters
	int start_block = 0;
	int nblocks = 0;
	//unsigned long start_calib = 0;
	//unsigned long ncalib = 0;
	unsigned long start_sort = 0;
	unsigned long nsort = 0;
	unsigned long start_build = 0;
	unsigned long nbuild = 0;
	unsigned long start_fill = 0;
	unsigned long nfill = 0;
    
    
	// Converter setup
	if( !flag_spy ) curFileMon = input_names.at(0); // maybe change in GUI later? // std::vector<std::string> input_names;
    //this is "at" - Returns a reference to the element at position n in the vector.
    //std::string curFileMon;
	conv_mon.AddCalibration( ((thptr*)ptr)->mycal /*to Callibrative file*/, flag_spy );
    
	conv_mon.SetOutput( "monitor_singles.root" );
	conv_mon.MakeTree();
	conv_mon.MakeHists();
	
	// Update server settings
	// title of web page
	std::string toptitle;
	if( !flag_spy ) toptitle = curFileMon.substr( curFileMon.find_last_of("/")+1,
							curFileMon.length()-curFileMon.find_last_of("/")-1 );
	else toptitle = "DataSpy ";
	toptitle += " (" + std::to_string( mon_time ) + " s)";
	serv->SetItemField("/", "_toptitle", toptitle.data() );

    
    
    // open data spy
    DataSpy dataspy_class;
    int id = 0;
    long long buffer [8*1024]; // array with 16*1024 size
    
    dataspy_class.dataSpyOpen(id);

    int data_len = 0;
    
    
    
    
//Bool_t bRunMon = kTRUE; for GUI
	while( bRunMon ) {
        
        // Convert
        //in int
        if( !flag_spy ) {
            
            nblocks = conv_mon.ConvertFile( curFileMon, start_block ); // here i need to use dataspy
            start_block = nblocks;
     
        }else {
            
		std::cout << "Looking for data from DataSpy" << std::endl;
            
           // int dataSpyRead = dataspy_class.dataSpyRead(id, (char*)nblocks, nblocks); // for 32 default
	            

	  data_len = dataspy_class.dataSpyRead(id, (char*)buffer, calfiles->myset->GetBlockSize() );
	  if( data_len == 0 && bFirstRun ) {
		std::cout << "No data yet on first pass" << std::endl;
		gSystem->Sleep( 5e3 );
		continue;
	  }


          while(data_len){	
         //       std::cout << "No data" << std::endl;
          //      gSystem->Sleep(mon_time * 1e3);
            
	//	dataspy_class.dataSpyRead()
		std::cout << "Got some data from DataSpy" << std::endl;
            	conv_mon.ConvertBlock((char*)buffer, 0 );
		data_len = dataspy_class.dataSpyRead(id, (char*)buffer, calfiles->myset->GetBlockSize() );
          
	  }
//		gSystem->Sleep(mon_time * 1e3);
	
            
        }

		// Only do the rest if it is not a source run
		if( !flag_source ) {
		std::cout << "bFirstRun1 " << bFirstRun << std::endl;
			// Sort
			if( bFirstRun ) {
				sort_mon.SetInputTree( conv_mon.GetTree() ); // use TimeSorter
				sort_mon.SetOutput( "monitor_sort.root" );  // use TimeSorter

				std::cout << "bFirstRun2 " << bFirstRun << std::endl;
				//serv->Hide("/Files/monitor_sort.root");
			}
			nsort = sort_mon.SortFile( start_sort );  // use TimeSorter
			
			std::cout << "nsort " << nsort << std::endl;

			start_sort = nsort;

			// Event builder
			if( bFirstRun ) {
				eb_mon.SetInputTree( sort_mon.GetTree() );  // in (use TimeSorter)
				eb_mon.SetOutput( "monitor_events.root" );

			
			}
			nbuild = eb_mon.BuildEvents( start_build );

			std::cout << "start build = " << start_build << std::endl;

			start_build = nbuild;

			
			// Histogrammer
			if( bFirstRun ) {
				hist_mon.SetInputTree( eb_mon.GetTree() );
				hist_mon.SetOutput( "monitor_hists.root" );
			}
			nfill = hist_mon.FillHists( start_fill );
			start_fill = nfill;
			
			// If this was the first time we ran, do stuff?
			if( bFirstRun ) {
				
				bFirstRun = kFALSE;
				
			}
		
		}
		
		// Now we can unlock the main thread again
		//TThread::UnLock();

		// Update the Canvas, but not when in a thread
		//gSystem->ProcessEvents();

		// This makes things unresponsive!
		// Unless we are threading?
		gSystem->Sleep( mon_time * 1e3 );
	
	}
	
    dataspy_class.dataSpyClose(0);
    
    
	conv_mon.CloseOutput();
	sort_mon.CloseOutput(); //use TimeSorter
	eb_mon.CloseOutput();
	hist_mon.CloseOutput();

	return 0;
	
}

//void* start_http( void* ptr ){
void start_http(){

	// Server for JSROOT
	std::string server_name = "http:" + std::to_string(port_num) + "?top=ISSDAQMonitoring";
	serv = new THttpServer( server_name.data() );
	serv->SetReadOnly(kFALSE);

	// enable monitoring and
	// specify items to draw when page is opened
	serv->SetItemField("/","_monitoring","5000");
	//serv->SetItemField("/","_layout","grid2x2");
	//serv->SetItemField("/","_drawitem","[hpxpy,hpx,Debug]");
	serv->SetItemField("/","drawopt","[colz,hist]");
	
	// register simple start/stop commands
	serv->RegisterCommand("/Start", "bRunMon=kTRUE;", "button;/usr/share/root/icons/ed_execute.png");
	serv->RegisterCommand("/Stop",  "bRunMon=kFALSE;", "button;/usr/share/root/icons/ed_interrupt.png");

	// hide commands so the only show as buttons
	serv->Hide("/Start");
	serv->Hide("/Stop");
		
	// Add data directory
	if( datadir_name.size() > 0 ) serv->AddLocation( "data/", datadir_name.data() );
	
	return;
	
}

void do_convert(){
	//------------------------//
	// Run conversion to ROOT //
	//------------------------//
    
	ISSConverter conv( myset, flag_spy );  // add flag of spy_file
	conv.AddCalibration( mycal, flag_spy);  // add flag of spy_file
    
	if( flag_source ) conv.SourceOnly();
	std::cout << "\n +++ ISS Analysis:: processing Converter +++" << std::endl;

	TFile *rtest;
	std::ifstream ftest;
	std::string name_input_file;
	std::string name_output_file;
   	
	// Check each file
    for( unsigned int i = 0; i < input_names.size(); i++ ){
                
            name_input_file = input_names.at(i);
            if( flag_source ) name_output_file = input_names.at(i) + "_source.root";
            else name_output_file = input_names.at(i) + ".root";

            force_convert.push_back( false );

            // If it doesn't exist, we have to convert it anyway
            // The convert flag will force it to be converted
            ftest.open( name_output_file.data() );
            if( !ftest.is_open() ) force_convert.at(i) = true;
            else {
                ftest.close();
                rtest = new TFile( name_output_file.data() );
                if( rtest->IsZombie() ) force_convert.at(i) = true;
                if( !flag_convert && !force_convert.at(i) )
                    std::cout << name_output_file << " already converted" << std::endl;
                rtest->Close();
            }

            if( flag_convert || force_convert.at(i) ) {
                
                std::cout << name_input_file << " --> ";
                std::cout << name_output_file << std::endl;
                
                conv.SetOutput( name_output_file );
                conv.MakeTree();
                conv.MakeHists();
                conv.ConvertFile( name_input_file );
                conv.CloseOutput();

            }
            
        }
    

	
	return;

}

void do_sort(){
	
	//-------------------------//
	// Do time sorting of data //
	//-------------------------//
    ISSTimeSorter sort(flag_spy);
    
	std::cout << "\n +++ ISS Analysis:: processing TimeSorter +++" << std::endl;

	TFile *rtest;
	std::ifstream ftest;
	std::string name_input_file;
	std::string name_output_file;
	
	// Check each file
   for( unsigned int i = 0; i < input_names.size(); i++ ){
            name_input_file = input_names.at(i) + ".root";
            name_output_file = input_names.at(i) + "_sort.root";
            // We need to time sort it if we just converted it
            if( flag_convert || force_convert.at(i) )
                force_sort = true;
            // If it doesn't exist, we have to sort it anyway
            else {

                ftest.open( name_output_file.data() );
                if( !ftest.is_open() ) force_sort = true;
                else {

                    ftest.close();
                    rtest = new TFile( name_output_file.data() );
                    if( rtest->IsZombie() ) force_sort = true;
                    if( !force_sort )
                        std::cout << name_output_file << " already sorted" << std::endl;
                    rtest->Close();
                }
            }
            if( force_sort ) {

                std::cout << name_input_file << " --> ";
                std::cout << name_output_file << std::endl;

                sort.SetInputFile( name_input_file );
                sort.SetOutput( name_output_file );
                sort.SortFile();
                sort.CloseOutput();

                force_sort = false;
            }
        }
    return;
}

void do_build(){
	
	//-----------------------//
	// Physics event builder //
	//-----------------------//
	ISSEventBuilder eb( myset ); //use  EventBuilder
	std::cout << "\n +++ ISS Analysis:: processing EventBuilder +++" << std::endl;
	
	TFile *rtest;
	std::ifstream ftest;
	std::string name_input_file;
	std::string name_output_file;
	
	// Update calibration file if given
	if( overwrite_cal ) eb.AddCalibration( mycal );

	// Do event builder for each file individually


         for( unsigned int i = 0; i < input_names.size(); i++ ){
                
            name_input_file = input_names.at(i) + "_sort.root";
            name_output_file = input_names.at(i) + "_events.root";

            // We need to do event builder if we just converted it
            // specific request to do new event build with -e
            // this is useful if you need to add a new calibration
            if( flag_convert || force_convert.at(i) || flag_events )
                force_events = true;

            // If it doesn't exist, we have to sort it anyway
            else {
                
                ftest.open( name_output_file.data() );
                if( !ftest.is_open() ) force_events = true;
                else {
                    
                    ftest.close();
                    rtest = new TFile( name_output_file.data() );
                    if( rtest->IsZombie() ) force_events = true;
                    if( !force_events )
                        std::cout << name_output_file << " already built" << std::endl;
                    rtest->Close();
                    
                }
                
            }
            
            if( force_events ) {

                std::cout << name_input_file << " --> ";
                std::cout << name_output_file << std::endl;

                eb.SetInputFile( name_input_file, flag_spy );
                eb.SetOutput( name_output_file );
                eb.BuildEvents();
                eb.CloseOutput();

                force_events = false;

            } 
        }
	return;
}

void do_hist(){
	//------------------------------//
	// Finally make some histograms //
	//------------------------------//
	ISSHistogrammer hist( myreact, myset ); //use Histogrammer
	std::cout << "\n +++ ISS Analysis:: processing Histogrammer +++" << std::endl;

	std::ifstream ftest;
	std::string name_input_file;
	std::string name_output_file;
	
	hist.SetOutput( output_name ); //use Histogrammer
	std::vector<std::string> name_hist_files;
	
	// We are going to chain all the event files now
    
    for( unsigned int i = 0; i < input_names.size(); i++ ){

            name_output_file = input_names.at(i) + "_events.root";
            name_hist_files.push_back( name_output_file );
            
        }


 
	hist.SetInputFile( name_hist_files ); 
	hist.FillHists(); 
	hist.CloseOutput(); 
	
	return;
	
}

void do_autocal(){

	//-----------------------------------//
	// Run automatic calibration routine //
	//-----------------------------------//
	ISSAutoCalibrator autocal( myset, myreact );
	autocal.AddCalibration( mycal );
	std::cout << "\n +++ ISS Analysis:: processing AutoCalibration +++" << std::endl;

	TFile *rtest;
	std::ifstream ftest;
	std::string name_input_file;
	std::string name_output_file = "autocal.root";
	std::string hadd_file_list = "";
	std::string name_results_file = "autocal_results.cal";

	// Check each file
     for( unsigned int i = 0; i < name_input_file.size(); i++ ){
                
            name_input_file = name_input_file.at(i) + "_source.root";

            // Add to list if the converted file exists
            ftest.open( name_input_file.data() );
            if( ftest.is_open() ) {
            
                ftest.close();
                rtest = new TFile( name_input_file.data() );
                if( !rtest->IsZombie() ) {
                    hadd_file_list += " " + name_input_file;
                }
                else {
                    std::cout << "Skipping " << name_input_file;
                    std::cout << ", it's broken" << std::endl;
                }
                rtest->Close();
                
            }
            
            else {
                std::cout << "Skipping " << name_input_file;
                std::cout << ", file does not exist" << std::endl;
            }
        }
	
	// Perform the hadd (doesn't work on Windows)
	std::string cmd = "hadd -k -T -v 0 -f ";
	cmd += name_output_file;
	cmd += hadd_file_list;
	gSystem->Exec( cmd.data() );
	
	// Give this file to the autocalibrator
	if( autocal.SetInputFile( name_output_file ) ) return;
	autocal.DoFits();
	autocal.SaveCalFile( name_results_file );
	
}



int main( int argc, char *argv[] ){
	// Command line interface, stolen from MiniballCoulexSort
	CommandLineInterface *interface = new CommandLineInterface();

    //daria code_ add a flag for input dataspy
    interface->Add("-spy", "Input dataspy data", &open_spy_data);
    
	interface->Add("-i", "List of input files", &input_names );
    
	interface->Add("-m", "Monitor input file every X seconds", &mon_time );
	interface->Add("-p", "Port number for web server (default 8030)", &port_num );
	interface->Add("-o", "Output file for histogram file", &output_name );
	interface->Add("-d", "Data directory to add to the monitor", &datadir_name );
	interface->Add("-f", "Flag to force new ROOT conversion", &flag_convert );
	interface->Add("-e", "Flag to force new event builder (new calibration)", &flag_events );
	interface->Add("-source", "Flag to define an source only run", &flag_source );
	interface->Add("-autocal", "Flag to perform automatic calibration of alpha source data", &flag_autocal );
	interface->Add("-s", "Settings file", &name_set_file );
	interface->Add("-c", "Calibration file", &name_cal_file );
	interface->Add("-r", "Reaction file", &name_react_file );
	interface->Add("-g", "Launch the GUI", &gui_flag );
	interface->Add("-h", "Print this help", &help_flag );

	interface->CheckFlags( argc, argv);
    
    
    
	if( help_flag ) {
		
		interface->CheckFlags( 1, argv );
		return 0;
		
	}
	// If we are launching the GUI
	if( gui_flag || argc == 1 ) {
		
		TApplication theApp( "App", &argc, argv );
		new ISSGUI();
		theApp.Run();
		
		return 0;

	}

	// Check for data spy flag
	if( open_spy_data >= 0 ) flag_spy = true;

    
    	if( !input_names.size() && !flag_spy ) { // try to get data from dataspy
        
           	std::cout << "You have to provide at least one input file" << std::endl;
        	return 1;
        
    	}

	// Check if this is a source run
	if( flag_autocal ) flag_source = true;

	// Check if we should be monitoring the input
	if( flag_spy ){
 
	        flag_monitor = true;
		if( mon_time < 0 ) mon_time = 60; // update default
        	std::cout << "Running iss_sort in a loop every " << mon_time;
        	std::cout << " seconds using DataSpy" << std::endl;

        
    	}
    
	else if( mon_time > 0 && input_names.size() == 1  ) {

            flag_monitor = true;
            std::cout << "Running iss_sort in a loop every " << mon_time;
            std::cout << " seconds\nMonitoring " << input_names.at(0) << std::endl;
		
	}
    
    	else if( mon_time > 0 ) {
		flag_monitor = false;
		std::cout << "Cannot monitor multiple input files, switching to normal mode" << std::endl;
				
	}

    
    
	// Check the ouput file name
    if( output_name.length() == 0 ){
        if (bool(input_names.size())){
            output_name = input_names.at(0) + "_hists.root";
        }
        else if( flag_spy ){
            output_name = "spy_hists.root";
        }
    }
	

    
	// Check we have a Settings file
	if( name_set_file.length() > 0 ) {
		std::cout << "Settings file: " << name_set_file << std::endl;
	}
	else {
		
		std::cout << "No settings file provided. Using defaults." << std::endl;
		name_set_file = "dummy";

        
	}

	// Check we have a calibration file
	if( name_cal_file.length() > 0 ) {
		
		std::cout << "Calibration file: " << name_cal_file << std::endl;
		overwrite_cal = true;

	}
	else {
		
		std::cout << "No calibration file provided. Using defaults." << std::endl;
		name_cal_file = "dummy";

	}
    
	// Check we have a reaction file
	if( name_react_file.length() > 0 ) {
		
		std::cout << "Reaction file: " << name_react_file << std::endl;
		
	}
	else {
		
		std::cout << "No reaction file provided. Using defaults." << std::endl;
		name_react_file = "dummy";

	}

    
    
    
	myset = new ISSSettings(name_set_file );
	mycal = new ISSCalibration(name_cal_file, myset );
	myreact = new ISSReaction(name_react_file, myset, flag_source );

	
    
    
    
	//-------------------//
	// Online monitoring //
	//-------------------//
	if( flag_monitor || flag_spy ) {
		
		// Thread for the HTTP server
		//TThread *th = new TThread( "http_server", start_http, (void*)nullptr );
		//th->Run();
		
		// Make some data for the thread
		thread_data data;
        
		data.mycal = mycal;
		data.myset = myset;
		data.myreact = myreact;

		// Start the HTTP server from the main thread (should usually do this)
		start_http();
		gSystem->ProcessEvents();


		// Thread for the monitor process
		TThread *th = new TThread( "monitor", monitor_run, (void*) &data );
		th->Run();
		
		// Just call monitor process without threading
		//monitor_run();
		
		// wait until we finish
		while( bRunMon ){
			
			gSystem->Sleep(100);
			gSystem->ProcessEvents();
			
		}
		std:cout << "until finish" << std::endl;

		return 0;
		
	}


	//------------------//
	// Run the analysis //
	//------------------//
	do_convert(); //Converter / if spy  - true
	if( !flag_source && !flag_autocal ) {
		do_sort();
		do_build();
		do_hist();
	} else if( flag_autocal ){
		do_autocal();
	}
	std::cout << "\n\nFinished!\n";
	return 0;
}
