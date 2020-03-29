
#include <string>

#include "KCGlobal.h"
#include "KCDataset.h"
#include "KCUtility.h"
#include "KCCliques.h"
#include "KCMerge.h"

#include "timeutil.h"

using std::string;

int main(int argc, char *argv[]) {
  string inputFile;
  string benchmarkFile;

  float alpha, minsup;
  
  bool   subspaces = false;
  bool   vertical = false;
  bool   selectiveVertical = false;
  bool   quiet = false;
  bool   confusion = false;
  bool   mapping = false;
  bool   use_frequency = false;
  bool   merge=false;

  string mapFile;

  int    k = -1;
  int    restored = 0;


  if (argc < 5) {
    cout << "Usage: " << argv[0] << endl;
    cout << "\t <input file> //input file name (after mconvert)\n";
    cout << "\t <alpha> //alpha value\n";
    cout << "\t <minsup> //minsup for clique merging\n";
    cout << "\t <benchmarkFile> //file where performance into is written\n";
    cout << "\t[MAP <mapfile>] //map file from mconvert to remap attributes to original\n";
    cout << "\t Optional Flags:\n";
    cout << "\t\t[FREQ] //treat alpha as relative min support of the cluster\n";
    cout << "\t\t[FULL|SUB] //full or subspace mining (default is full)\n";
    cout << "\t\t[VERTICAL] //perform vertical mining. *caution* slow\n";
    cout << "\t\t[MERGE] //merge cliques\n";
    cout << "\t\t[CONFUSION] //output confusion matrix\n";
    //cout << "\t\t[K<n>] //limit num cliques to n\n";
    cout << "\t\t[SELECTIVE] //perform selective expansion for completeness\n";
    cout << "\t\t[QUIET] //suppress output\n";
    cout << "Defaults are full-dimensional clustering without vertical extension and no merging." << endl;
    exit(1);
  }

  inputFile = argv[1];
  alpha = atof(argv[2]);
  minsup = atof(argv[3]);
  benchmarkFile = argv[4];

  for(int i = 5; i < argc; i++){
    if(!strcmp(argv[i], "FULL")){
      subspaces = false;
    }
    else if(!strcmp(argv[i], "SUB")){
      subspaces = true;
    }
    else if(!strcmp(argv[i], "VERTICAL")){
      vertical = true;
    }
    else if(!strcmp(argv[i], "SELECTIVE")){
      selectiveVertical = true;
    }
    else if(!strcmp(argv[i], "CONFUSION")){
      confusion = true;
    }
    else if(!strcmp(argv[i], "MERGE")){
      merge = true;
    }
    else if(!strcmp(argv[i], "QUIET")){
      quiet = true;
    }
    else if(!strcmp(argv[i], "MAP")){
      mapping = true;
      mapFile.assign(argv[++i]);
    }
    else if((argv[i])[0] == 'K'){
      k = atoi(&((argv[i])[1]));
    }
    else if (!strcmp(argv[i], "FREQ")){
      use_frequency = true;
    }
  }

  cout << "Mining categorical clusters using k-partite maximal cliques" << endl;
  cout << "-----------------------------------------------------------" << endl << endl;

#ifdef KC_LEAN
  cout << "--> CLICK is compiled in LEAN mode." << endl;
#endif

  if(subspaces){
    cout << "--> SUBSPACE option enabled." << endl;
  }
  if(use_frequency){
    cout << "--> USE FREQUENCY option enabled." << endl;
  }
  if(merge){
    cout << "--> MERGE option enabled." << endl;
  }

  if(vertical){
    cout << "--> VERTICAL mining enabled." << endl;
  }
  else if(selectiveVertical){
    cout << "--> SELECTIVE VERTICAL mining is enabled." << endl;
  }

  if(k > 0){
    cout << "--> Reducing result to " << k << " clusters." << endl;
  }

  KCDataset dataset(inputFile.c_str());
  bool supportSufficient;
  
  Timer t1("Total Clustering time");
  Timer t2("Pre-Processing time");
  Timer t3("Clique building time");
  Timer t4("Merging time");
  
  t1.Start();
  
  cout << "--> Generating support information from '" << inputFile.c_str() << "'." << endl;
  t2.Start(); 

#ifndef KC_LEAN
  if(vertical){
    if(!dataset.computeSupportInfo(alpha, supportSufficient, vertical, use_frequency)){
      return -1;
    }
  }
  else{
#endif
    if(selectiveVertical){
      // Vertical information needs to be generated in the dataset for selective
      // vertical mining
      if(!dataset.computeSupportInfo(alpha, supportSufficient, true, use_frequency)){ 
	return -1;
      } 
    }
    else{
      if(!dataset.computeSupportInfo(alpha, supportSufficient, vertical, use_frequency)){
	return -1;
      }
    }
#ifndef KC_LEAN
  }
#endif

  t2.Stop();

  if(mapping){
    readMapping(mapFile.c_str(), dataset.numberOfAttributes(), dataset.getMaxAttributeValues());
  }

  cout << "--> Computing candidate clusters." << endl;

  t3.Start();
  KCCliques cliques;
  int initialCliques;


  computeCliques(dataset, cliques, subspaces, vertical, int(alpha * dataset.getTuples()));
  initialCliques = cliques.size();

  t3.Stop();

  cout << "--> Verifying & Merging candidates." << endl;
  t4.Start();
  
  KCValueCliqueMap valueCliqueMap;
  KCItemsets itemsets;
  KCCliques mergedCliques;
  KCCliques generatedCliques;
  
  if(!cliques.empty()){
#ifndef KC_LEAN
    if(vertical){
      // The vertical method does prune low-support cliques, but is does not
      // compute the itemsets used in the merging process. So that we need to
      // do separately.
      dataset.computeItemsetsVertical(cliques, itemsets);
    }
    else{
#endif
      cout << "--> Computing value clique mapping." << endl;
      computeValueCliqueMapping(cliques, valueCliqueMap, dataset);
      cout << "--> Calculating clique support." << endl;
      dataset.calculateCliqueSupport(cliques, valueCliqueMap, itemsets);
      cout << "--> Pruning Cliques and Itemsets." << endl;
      generatedCliques = cliques;
      pruneCliquesAndItemsets(cliques, itemsets, alpha, selectiveVertical,
			      subspaces, dataset, restored, use_frequency);
      // If the selective vertical mining restored induced subcliques, do it again 
      if(restored > 0){
	cout << "--> Computing value clique mapping." << endl;
	computeValueCliqueMapping(cliques, valueCliqueMap, dataset);
	cout << "--> Calculating clique support." << endl;
	dataset.calculateCliqueSupport(cliques, valueCliqueMap, itemsets);
      }

      if (!quiet){
	cout << "--> Initial Cliques after Support Pruning" << endl;
	cout << cliques;
      }
  
      
#ifndef KC_LEAN
    }
#endif
    
    if(cliques.size() > 1 && merge){
      cout << "--> Merging Cliques. " << cliques.size() << endl;
      mergeCliques(dataset, cliques, itemsets, alpha, minsup, mergedCliques, use_frequency);
    }
    else{
      mergedCliques = cliques;
    }
  }

  t4.Stop();
  t1.Stop();
  
  cout << endl;
  cout << "Clustering summary" << endl;
  cout << "------------------" << endl;

  cout << "A total of " << initialCliques << " cluster candidates were originally detected." << endl;
  if(restored > 0){
    cout << "Selective vertical mining recovered " << restored << " frequent induced subcliques." << endl;
  }
  if (merge)
    cout << "After support validation and merging " << mergedCliques.size() << " clusters remained." << endl;  
  
  cout << t1;
  cout << t2;
  cout << t3;
  cout << t4;
  
  if(!quiet){
    cout << mergedCliques;
  }
  
  if(confusion){
    cout << "--> Generating confusion information." << endl;
    dataset.buildConfusionInfo("click_confusion.txt", mergedCliques);    
  }

  cout << "--> Done." << endl; 

  double avgInitialClusterSize;
  int sizeSum;
  KCCliquesIt cIt;
  if(generatedCliques.size() == 0){
    avgInitialClusterSize = 0.0;
  }
  else{
    for(cIt = generatedCliques.begin(), sizeSum = 0; cIt != generatedCliques.end(); cIt++){
      sizeSum += cIt->size();
    }
    avgInitialClusterSize = ((double)sizeSum / (double)generatedCliques.size());
  }

  ofstream ofile;
  ofile.open(argv[4], ios::app);
  ofile << t1.UserTime() + t1.SystemTime() << " " << dataset.getTuples() << " " 
        << dataset.numberOfAttributes() << " " << dataset.getMaxAttributeValues() << " "
	<< mergedCliques.size() << " " << initialCliques << " " 
	<< t2.UserTime() + t2.SystemTime() << " " 
	<< t3.UserTime() + t3.SystemTime() << " " 
	<< t4.UserTime() + t4.SystemTime() << " " 
	<< restored << " " 
	<< avgInitialClusterSize << " "
	<< dataset.getNumAttributeValues() << endl;

  ofile.close();
  
  return 0;
}
