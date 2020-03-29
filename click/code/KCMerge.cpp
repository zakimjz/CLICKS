#include <iostream>

#include "KCMerge.h"
#include "KCUtility.h"
#include "KCSelective.h"

using namespace std;

#define ITEMSETSFILE "_ITEMSETS_DATA_"
extern vector<vector<int> > get_maxfreqsets(int al, char *fname, int difn, double supp);


// Create a mapping from attribute values to the indices of the cliques that they appear in
void computeValueCliqueMapping(KCCliques& cliques, KCValueCliqueMap& valueCliqueMap, KCDataset& dataset)
{
  KCCliquesIt cIt;
  KCCliqueDimIt dimIt;

  int i, cliqueIndex;
  int lower, upper, j;

  KCCliqueDim dim;

  valueCliqueMap.clear();

  for(cIt = cliques.begin(), cliqueIndex = 0; cIt != cliques.end(); cIt++, cliqueIndex++){
    for(i = 0; i < cIt->numberOfDimensions(); i++){
      // cIt->sortDimension(i);
      if(cIt->dim(i).empty())
	cIt->constructFullDim(i, dim, dataset.getNumAttributeValues());
      else
	dim = cIt->dim(i);

    
      for(dimIt = dim.begin(); dimIt != dim.end(); dimIt++){
	valueCliqueMap[*dimIt].insert(cliqueIndex);
      }
    }
  }
}

/*
  Note: The minsup value her is expected here as absolute number of transactions
  not as the fraction it is expected as below.
*/
bool pruneCliquesAndItemsets(KCCliques& cliques, KCItemsets& itemsets, 
			     float alpha, bool selectiveVertical, 
			     bool subspaces, KCDataset& dataset, 
			     int& restored, bool use_frequency)
{
  KCCliquesIt cIt;
  KCItemsetsIt itemsetsIt;
  KCItemsetIt itemsetIt;

  KCCliques prunedCliques;
  KCCliques restoredCliques;

  int i;
  bool proceed = true;

  int minsup;
  
  if (use_frequency){
    minsup = int (alpha * dataset.getTuples());
  }
  
  
  while(proceed){
    for(cIt = cliques.begin(), i = 0; cIt != cliques.end(); cIt++, i++){
      
      //use the individual dims to compute the expected support
      int countNumValidDims = 0;
      double relative_sz = 1;
      for (int j=0; j < (*cIt).numberOfDimensions(); ++j){
	if ((*cIt).dim(j).size() > 0){
	  countNumValidDims++; //how many dims the cluster spans?
	  relative_sz = relative_sz* 
	    ((*cIt).dim(j).size()/((double)dataset.getDistinctValues(j)));   
	  //cout << "DIMS " << j << " " << (*cIt).dim(j).size() << " " 
	  //	 << dataset.getDistinctValues(j) << " "
	  //	 << ((*cIt).dim(j).size()/
	  //	     ((double)dataset.getDistinctValues(j))) 
	  //	 << endl;
	}
      }
      
      if (!use_frequency){
	minsup = int (alpha* relative_sz * dataset.getTuples());
	//cout << "RELSIZ " << relative_sz << " " << minsup << " " 
	//    << cIt->getSupport() << endl;
      }
      
      
      if(cIt->getSupport() < minsup || countNumValidDims == 1){
	if(selectiveVertical && countNumValidDims > 1){
	  prunedCliques.push_back(*cIt);
	}
	cliques.erase(cIt);

	for(itemsetsIt = itemsets.begin(); itemsetsIt != itemsets.end(); itemsetsIt++){
	  for(itemsetIt = itemsetsIt->begin(); itemsetIt != itemsetsIt->end(); itemsetIt++){
	    if(*itemsetIt == i)
	      *itemsetIt = -1;
   	    else if(*itemsetIt > i)
	      (*itemsetIt)--;
	  }
	}
	break;
      }
    }
    if(cIt == cliques.end())
      break;
  }  

  for(itemsetsIt = itemsets.begin(); itemsetsIt != itemsets.end(); itemsetsIt++){
    proceed = true;
    while(proceed){
      proceed = false;
      for(itemsetIt = itemsetsIt->begin(); itemsetIt != itemsetsIt->end(); itemsetIt++){
	if(*itemsetIt == -1){
	  itemsetsIt->erase(itemsetIt);
	  proceed = true;
	  break;
	}
      }
    }
  }

  proceed = true;
  while(proceed){
    proceed = false;
    for(itemsetsIt = itemsets.begin(); itemsetsIt != itemsets.end(); itemsetsIt++){
      if(itemsetsIt->size() == 0){
	itemsets.erase(itemsetsIt);
	proceed = true;
	break;
      }
    }
  }

  restoredCliques.clear();
  if(selectiveVertical && prunedCliques.size() > 0){
    //cout << "--> There are ";
    //cout << prunedCliques.size() << " pruned cliques that need to be considered for completeness." << endl;
    // Consider every clique that was previously pruned and mine its subcliques in
    // a selective vertical manner
    for(cIt = prunedCliques.begin(), i = 0; cIt != prunedCliques.end(); cIt++, i++){
      //      cout << "Restoring completeness for " << i << endl;
      if(restoreCompleteness(dataset, *cIt, restoredCliques, subspaces, minsup)){
	/*if(!restoredCliques.empty()){
	  cout << "Pruned clique " << *cIt << " has frequent subcliques " << restoredCliques << endl;
	  }*/
      }
    }
    for(cIt = restoredCliques.begin(); cIt != restoredCliques.end(); cIt++){
      cliques.push_back(*cIt);
    }
  }
  else if(selectiveVertical && prunedCliques.size() == 0){
    //cout << "No cliques were pruned in post-processing. The result is complete." << endl;
  }

  restored = restoredCliques.size();
}

bool mergeCliques(KCDataset& d, KCCliques& cliques, KCItemsets& itemsets, 
		  float alpha, float minSup, KCCliques& mergedCliques, 
		  bool use_frequency)
{
  ofstream out;

  int i, j;
  int absoluteSupport;

  KCFrequentSets frequentSets;
  KCFrequentSetsIt setsIt;
  KCFrequentSetIt setIt;

  KCItemsetSupport totalSupport;

  int numberOfCliques;

  KCClique clique1(d.numberOfAttributes());
  KCClique clique2(d.numberOfAttributes());

  KCCliqueDimIt cIt1, cIt2;

  KCFlagVector erasedCliques;
  erasedCliques.assign(cliques.size(), false);

  mergedCliques.clear();

  absoluteSupport = int (alpha * d.getTuples());

  if(!itemsets.empty()){
    // Dump the itemsets to disk in the ECLAT format
    out.open(ITEMSETSFILE, ofstream::out | ofstream::trunc);
    if(!out.is_open())
      return false;
    itemsets.serialize(out);
    out.close();

    frequentSets = get_maxfreqsets(2, ITEMSETSFILE, 1, minSup);

    if(!frequentSets.empty()){
      sortMaximalItemsets(frequentSets, cliques, totalSupport, erasedCliques);
      
      clique1.reset();
      for (setsIt = frequentSets.begin(), numberOfCliques = 0; 
	   setsIt != frequentSets.end(); ++setsIt, numberOfCliques++){
	//first compute the merged clique, then test the support
	clique2.reset();
	for(setIt = setsIt->begin(), i = 0; setIt != setsIt->end(); 
	    ++setIt, ++i){
	  if (i == 0)
	    clique2 = cliques[*setIt];
	  else{
	    for (j = 0; j < d.numberOfAttributes(); ++j){
	      clique1.dim(j).clear();
	      
	      set_union(clique2.dim(j).begin(), clique2.dim(j).end(),
			(cliques[*setIt]).dim(j).begin(), 
			(cliques[*setIt]).dim(j).end(),
			insert_iterator<KCCliqueDim> 
			(clique1.dim(j), clique1.dim(j).begin()));
	      sort(clique1.dim(j).begin(), clique1.dim(j).end());
	      
	      for(cIt1 = clique1.dim(j).begin(); cIt1 != clique1.dim(j).end(); cIt1++){
		cIt2 = cIt1 + 1;
		if(cIt2 != clique1.dim(j).end())
		  while(*cIt2 == *cIt1)
		    cIt2 = clique1.dim(j).erase(cIt2);
	      }
	      
	      clique2.dim(j) = clique1.dim(j);
	    }
	  }
	}
	
	//compute the absolute support
	double rsz = 1;
	if (!use_frequency){
	  for (int j=0; j < clique2.numberOfDimensions(); ++j){
	    if (clique2.dim(j).size() > 0)
	      rsz = rsz * 
		(clique2.dim(j).size()/((double)d.getDistinctValues(j))); 
	  }
	  absoluteSupport = (int) (alpha * rsz * d.getTuples());
	}
	  
	//cout << "TOTAL " << totalSupport[numberOfCliques] << " -- " 
	//     << absoluteSupport << " ** "
	//     << clique2 << endl;
	if (totalSupport[numberOfCliques] >= absoluteSupport){
	  mergedCliques.push_back(clique2);
	}
	setsIt->clear();
      }
    }
    totalSupport.clear();
  }
  
  
  KCCliquesIt it = cliques.end();
  it--;
  KCCliquesIt it2;
  
  for (i = cliques.size() - 1; i >= 0; --i){
    it2 = it;
    if (erasedCliques[i]){
      it--;
      continue;
    }

    //compute the relative size of the actual dims in the clique
    double relative_sz = 1;
    if (!use_frequency){
      for (int j=0; j < cliques[i].numberOfDimensions(); ++j){
	if (cliques[i].dim(j).size() > 0)
	  relative_sz = relative_sz * 
	    (cliques[i].dim(j).size()/((double)d.getDistinctValues(j))); 
      }
      absoluteSupport = (int) (alpha * relative_sz * d.getTuples());
    }
    
    //cout << "CLIQUES: " << cliques[i] << "\n\t"
    //	 << cliques[i].getSupport() << " "<< relative_sz 
    //	 << " " << absoluteSupport << endl;
    
    if (cliques[i].getSupport() < absoluteSupport){
      it--;
      continue;
    }
    
    mergedCliques.push_back(cliques[i]);
    it--;
  }
  
  return true;
}

void getCliqueMaxSetMapping(KCFrequentSets& frequentSets, KCCliqueMaxSetMapping& map,
			    KCItemsetSupport& support, KCItemsetSupport& total,
			    KCCliques& cliques)
{
  KCFrequentSetIt setIt, setPrime;
  KCFrequentSetsIt setsIt;
  
  int totalSupport, size, setIndex;

  for(setsIt = frequentSets.begin(), setIndex = 0; setsIt != frequentSets.end(); setsIt++){
    setIt = setsIt->begin();
    setPrime = setIt;
    // First element in the frequent itemset description is the support
    // of the set
    support.push_back(*(setIt++));
    size = totalSupport = 0;

    for(; setIt != setsIt->end(); setIt++, size++){
      map[*setIt].push_back(setIndex);
      totalSupport += cliques[*setIt].getSupport();
    }

    totalSupport -= (size - 1) * support[setIndex];
    total.push_back(totalSupport);
    setIndex++;
    // Erase the support element
    setsIt->erase(setPrime);
  }
}                                

void sortMaximalItemsets(KCFrequentSets& frequentSets, KCCliques& cliques, 
			 KCItemsetSupport& totalSupport, KCFlagVector& erasedCliques)
{
  KCFrequentSetsIt setsIt;
  KCFrequentSetIt setIt, setIt2;
  
  KCCliqueMaxSetMapping map;
  KCMaxSetIt maxSetIt, maxSetIt2, maxSetIt3;

  KCItemsetSupport support;
  KCItemsetSupportIt supIt;

  int currentClique, maxIndex, setSupport, totalSetSupport;
  int j;
  
  KCFlagVector processed;
  processed.assign(frequentSets.size(), false);
    
  getCliqueMaxSetMapping(frequentSets, map, support, totalSupport, cliques);
    
  for (currentClique = 0; currentClique < cliques.size(); currentClique++){
    // If this clique does not belong to any of the itemsets or if it has been erase
    if(map[currentClique].empty() || erasedCliques[currentClique])
      continue;
    
    // Check, which itemset has the greatest support for this clique
    maxIndex = setSupport = totalSetSupport = 0;
    maxSetIt = map[currentClique].begin();
    
    while(maxSetIt != map[currentClique].end()){
      if(processed[*maxSetIt]){
	maxSetIt = map[currentClique].erase(maxSetIt);
	continue;
      }
      else if(totalSupport[*maxSetIt] > totalSetSupport ||
	      (totalSupport[*maxSetIt] == totalSetSupport && support[*maxSetIt] > setSupport)){
	totalSetSupport = totalSupport[*maxSetIt];
	setSupport = support[*maxSetIt];
	maxIndex = *maxSetIt;
      }
      
      maxSetIt++;
    }
    
    for(maxSetIt = map[currentClique].begin(); maxSetIt != map[currentClique].end(); ++maxSetIt){  
      
      if(*maxSetIt != maxIndex){
	if(frequentSets[*maxSetIt].size() == 1)
	  totalSupport[*maxSetIt] = 0;
	else
	  totalSupport[*maxSetIt] -= (cliques[currentClique].getSupport() - support[*maxSetIt]);
	
	
	for(setIt = frequentSets[*maxSetIt].begin(); setIt != frequentSets[*maxSetIt].end(); setIt++){
	  if(*setIt == currentClique){
	    setIt = frequentSets[*maxSetIt].erase(setIt);
	    break;
	  }
	}
      }
      else{
	processed[*maxSetIt] = true;
	
	for(setIt = frequentSets[*maxSetIt].begin(); setIt != frequentSets[*maxSetIt].end(); ++setIt){
	  if(erasedCliques[*setIt])
	    continue;
	  else if (*setIt == currentClique){
	    erasedCliques[*setIt] = true;
	    continue;
	  }
	  
	  erasedCliques[*setIt] = true;
	  
	  for(maxSetIt2 = map[*setIt].begin(); maxSetIt2 != map[*setIt].end(); ++maxSetIt2){
	    if(processed[*maxSetIt2])
	      continue;
	    
	    maxSetIt3 = frequentSets[*maxSetIt2].begin();
	    
	    if(frequentSets[*maxSetIt2].size() == 1)
	      totalSupport[*maxSetIt2] = 0;
	    else
	      totalSupport[*maxSetIt2] -= cliques[*setIt].getSupport() - support[*maxSetIt2];                                     
	    while(maxSetIt3 != frequentSets[*maxSetIt2].end()){
	      if(*maxSetIt3 != *setIt){
		++maxSetIt3;
		continue;
	      }
	      maxSetIt3 = frequentSets[*maxSetIt2].erase(maxSetIt3);
	      break;
	    }
	  }
	}
      }
    }
  }
  

  setsIt = frequentSets.begin();
  supIt = totalSupport.begin();
  
  while(supIt != totalSupport.end()){
    if (*supIt == 0 || setsIt->size() == 1){
      //increasing the erased element number
      ++j;
      if ((*setsIt).size() == 1)
	erasedCliques[*(setsIt->begin())] = 0; 

      supIt = totalSupport.erase(supIt);
      setsIt = frequentSets.erase(setsIt);
    } 
    else{
      ++setsIt;
      ++supIt;
    }
  }
}

