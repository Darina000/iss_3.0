#include "TimeSorter.hh"

#ifndef DATASPY_HH
#define DATASPY_HH






ISSTimeSorter::ISSTimeSorter(bool myflag_spy){
    
    
    flag_spy = myflag_spy;
	// No progress bar by default
	_prog_ = false;
	
	// No input file yet
	flag_input_file = false;

}

bool ISSTimeSorter::SetInputFile( std::string input_file_name ){
	
	// Open next Root input file.
	input_file = new TFile( input_file_name.data(), "read" );
    
	if( input_file->IsZombie() ) {
		std::cout << "Cannot open " << input_file_name << std::endl;
		return false;
	}
	
	flag_input_file = true;
	// Set the input tree
	SetInputTree( (TTree*)input_file->Get("iss") );

	std::cout << "Sorting file by timestamp: " << input_file->GetName() << std::endl;

	return true;
	
}

void ISSTimeSorter::SetInputTree( TTree* user_tree ){

	// Find the tree and set branch addresses
	input_tree = user_tree;
	input_tree->SetCacheSize(200000000); // 200 MB
	input_tree->SetCacheEntryRange(0, input_tree->GetEntries()-1);
	input_tree->AddBranchToCache( "*", kTRUE );
	//input_tree->StopCacheLearningPhase();
	return;
	
}

void ISSTimeSorter::SetOutput( std::string output_file_name ){
	
    
    
    
	// Open root file
	output_file = new TFile( output_file_name.data(), "recreate", "Time sorted ISS data" );
	//output_file->SetCompressionLevel(0);

	// Create output Root file and Tree.
	output_file->cd();
	output_tree = (TTree*)input_tree->CloneTree(0);
	output_tree->SetDirectory( output_file->GetDirectory("/") );
	output_tree->SetName( "iss_sort" );
	output_tree->SetTitle( "Time sorted, calibrated ISS data" );
	//output_tree->SetBasketSize( "*", 16000 );
	//output_tree->SetAutoFlush( 30*1024*1024 );	// 30 MB
	//output_tree->SetAutoSave( 100*1024*1024 );	// 100 MB
	output_tree->AutoSave();
	//output_tree->GetBranch( "data" )->SetCompressionLevel(0);

	// Create log file.
	std::string log_file_name = output_file_name.substr( 0, output_file_name.find_last_of(".") );
	log_file_name += ".log";
	log_file.open( log_file_name.data(), std::ios::app );

	return;
	
};

unsigned long ISSTimeSorter::SortFile( unsigned long start_sort ) {

	// Start timer
	time( &t_start );
	
	// Time sort all entries of the tree
	n_entries = input_tree->GetEntries();
	std::cout << " Sorting: number of entries in input tree = " << n_entries << std::endl;
	log_file << " Sorting: number of entries in input tree = " << n_entries << std::endl;

	if( n_entries > 0 && start_sort < n_entries  ) {
		
		nb_idx = input_tree->BuildIndex( "data.GetTimeMSB()", "data.GetTimeLSB()" );
		//nb_idx = input_tree->BuildIndex( "0", "data.GetTime()" );
		//std::cout << "nb_idx " << nb_idx << std::endl;
		//nb_idx = input_tree->BuildIndex( "data.GetTime()", "0" );
		//std::cout << "nb_idx " << nb_idx << std::endl; 
		att_index = (TTreeIndex*)input_tree->GetTreeIndex();
	
		std::cout << " Sorting: size of the sorted index = " << nb_idx << std::endl;
		log_file << " Sorting: size of the sorted index = " << nb_idx << std::endl;

		// Loop on t_raw entries and fill t
		for( unsigned long i = start_sort; i < nb_idx; ++i ) {
			
			idx = att_index->GetIndex()[i];
            
			//if( idx < start_sort ) continue;
            
			input_tree->GetEntry( idx );
			output_tree->Fill();
			
			// Progress bar
			bool update_progress = false;
             
			if( nb_idx < 200 )
				update_progress = true;
			else if( i % (nb_idx/100) == 0 || i+1 == nb_idx )
				update_progress = true;
			
			if( update_progress ) {
				
				// Percent complete
				float percent = (float)(i+1)*100.0/(float)nb_idx;
				
				// Progress bar in GUI
				if( _prog_ ) prog->SetPosition( percent );

				// Progress bar in terminal
				std::cout << " " << std::setw(6) << std::setprecision(4);
				std::cout << percent << "%    \r";
				std::cout.flush();
				gSystem->ProcessEvents();
			}
		}
	}
	
	else {
		
		std::cout << " Sorting: nothing to sort " << std::endl;
		log_file << " Sorting: nothing to sort " << std::endl;
		
	}
	

	// Write histograms, trees and clean up
	if( flag_input_file ) {
		std::cout << "Read " << input_file->GetBytesRead();
		std::cout << " bytes in " << input_file->GetReadCalls();
		std::cout << " transactions" << std::endl;
		
    }else{
        input_tree->PrintCacheStats();
    }
	
	output_tree->Write( 0, TObject::kWriteDelete );
	output_file->SaveSelf();
	//output_file->Print();
	
	std::cout << "End ISSTimeSorter: time elapsed = " << time(NULL)-t_start << " sec." << std::endl;
	log_file << "End ISSTimeSorter: time elapsed = " << time(NULL)-t_start << " sec." << std::endl;
	
	
	return n_entries;
	
}


#endif /* DATASPY_HH */
