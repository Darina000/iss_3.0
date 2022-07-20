#ifndef _TimeSorter_hh
#define _TimeSorter_hh

#include <bitset>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <ctime>

#include <TFile.h>
#include <TTree.h>
#include <TTreeIndex.h>
#include <TGProgressBar.h>
#include <TSystem.h>






class ISSTimeSorter {

public:

	ISSTimeSorter(bool flag_spy = 0);
	virtual ~ISSTimeSorter(){};
	
	unsigned long SortFile( unsigned long start_sort = 0 );
	
	bool	SetInputFile( std::string input_file_name );
	void	SetInputTree( TTree* user_tree );
	void	SetOutput( std::string output_file_name );
    
	inline void CloseOutput(){
		output_file->Close();
		input_file->Close(); // Close TFile
		log_file.close(); //?? to close or not to close?
	};
    
    
	inline TFile* GetFile(){ return output_file; };
	inline TTree* GetTree(){ return output_tree; };
	
	inline void AddProgressBar( std::shared_ptr<TGProgressBar> myprog ){
		prog = myprog;
		_prog_ = true;
	};


private:
		
	TFile *input_file;
	TTree *input_tree;
	TFile *output_file;
	TTree *output_tree;
	
	std::ofstream log_file;
	
	TTreeIndex *att_index;

	// Counters
	unsigned long long n_entries;
	unsigned long long nb_idx, idx;
	
	// Sort time
	time_t t_start;
	
	// Flag for input file I/O
	bool flag_input_file;
    
    //Flag for dataspy
    bool flag_spy;

	// Progress bar
	bool _prog_;
	std::shared_ptr<TGProgressBar> prog;

};

#endif
