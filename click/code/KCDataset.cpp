#include <iostream>
#include <fstream>

#include <ext/hash_map>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "KCGlobal.h"
#include "KCDataset.h"
#include "KCUtility.h"

using namespace std;

bool KCDataset::orderNodes(KCDescendNodes& nlist){
  
  KCDescendNodes::iterator dit, dit1;
  KCCliqueNode cnode;
  KCCliqueNodeVector lnodes;
  KCOrderedNodes::iterator oit;
  KCCliqueNodeVector::iterator lit;   
  
  int flow;

  dit = nlist.begin();
  while (dit != nlist.end()){
    cnode.value = dit->first;
    cnode.deleted = 0;        
    nodeOrder[dit->second].push_back(cnode);

    dit1 = ++ dit;
    nlist.erase(--dit);
    dit = dit1;
  }

  return true;
}

bool KCDataset::computeSupportInfo(const float alpha, bool& supportSufficient, const bool vertical, const bool use_frequency)
{

  KCNodesMap nodes;
  KCDescendNodes nlist;
    
  int i, j, k;
  int lowerMinSup;
  int total = 0;

  if(!readDataset(vertical)){
    return false;
  }

  supportSufficient = false;

  if (use_frequency){
    lowerMinSup = (int) (alpha * numTuples);
    //cout << "TRANS " << alpha << " " << numTuples << " " << lowerMinSup << endl;
  }
  

  for(i = 0; i < numAttributeValues; ++ i){
    for (j = i + 1; j < numAttributeValues; ++ j){
      if (!use_frequency){
	lowerMinSup = (int) ((alpha * numTuples) / 
			     (distAttributeValues[attributes[i]] * 
			      distAttributeValues[attributes[j]])); 
      }
      
      if (support.getValue(i,j) < lowerMinSup){
	support.setValue(i, j, 0);
      }
      else{
	support.setValue(i, j, 1);
	nlist[i]++;
	nlist[j]++;
	//cout << "GRAPH EDGE " << i << " " << j << endl;
	supportSufficient = true;
      }
    }
  }
  
  if(supportSufficient){
    for (i = 0; i < numAttrs; i++){
      for (j = total; j < total + distAttributeValues[i] - 1; j++){
	if (nlist[j] > 0)
	  nlist[j] += (distAttributeValues[attributes[j]] - 1);
	else
	  nlist.erase(j);
	
	for (k = j + 1; k < total + distAttributeValues[i]; k++)
	  support.setValue(j, k, 1);
      }
      
      
      if (nlist[j] > 0)      
	nlist[j] += (distAttributeValues[attributes[j]] - 1);
      else
	nlist.erase(j);
      
      total += distAttributeValues[i];      
    }            
    
    orderNodes(nlist);
    nlist.clear();
  }
  
  return true;
}

// Up to this amount of bytes will be allocated for
// the read-in process (the first factor is the number of megs)
#define TUPLESPACE 10 * 1000000

bool KCDataset::readDataset(const bool vertical)
{
  int fd, nBytes;
  int tupSz;
  int* tuples;
  int actualTuples, togoTuples;

  int i, j, k;

  int trans = -1;

  int SIMTUPLES;

  if(!readMetadata(vertical, fd))
    return false;

  togoTuples = numTuples;
  actualTuples = 0;
  tupSz = 2 + numAttrs + 1;
  SIMTUPLES = TUPLESPACE / (tupSz * sizeof(int));
  tuples = new int[SIMTUPLES * tupSz];

  while(togoTuples > 0){
    if(togoTuples > SIMTUPLES){
      nBytes = read(fd, tuples, SIMTUPLES * tupSz * sizeof(int));
      assert(nBytes == SIMTUPLES * tupSz * sizeof(int));
      actualTuples = SIMTUPLES;
      togoTuples -= SIMTUPLES;
    }
    else{
      nBytes = read(fd, tuples, togoTuples * tupSz * sizeof(int));
      assert(nBytes == togoTuples * tupSz * sizeof(int));
      actualTuples = togoTuples;
      togoTuples = 0;
    }

    for(i = 0; i < actualTuples*tupSz; i+=tupSz){
      trans++;
      for(j = i + 2; j < i + tupSz - 2; j++){
	attributes[tuples[j]] = j - (i + 2);
	if(vertical){
	  (verticalInfo[tuples[j]]).insert(trans);
	}
	for(k = j + 1; k < i + tupSz - 1; k++){
	  support.incrementValue(tuples[j], tuples[k]);
	  // if(vertical){
	  // verticalInfo.getValue(tuples[j], tuples[k]).insert(trans);
	  // }
	}
      }
      if(vertical){
	(verticalInfo[tuples[i+ tupSz - 2]]).insert(trans);
      }

      attributes[tuples[i + tupSz - 2]] = tupSz - 4;
    }
  }

  close(fd);
  delete[] tuples;

  return true;
}

bool KCDataset::calculateCliqueSupport(KCCliques& cliques, KCValueCliqueMap& valueCliqueMap, KCItemsets& itemsets)
{
  int fd, nBytes;
  int tupSz;
  int* tuples;
  int actualTuples, togoTuples;

  int i, j, k, SIMTUPLES;
  int trans = -1;

  bool initialized;

  set<int> cliqueSet, intersection;
  set<int>::iterator cliqueSetIt;

  KCItemset itemset;
  int itemsetIndex = -1;

  itemsets.clear();

  for(i = 0; i < cliques.size(); i++){
    cliques[i].setSupport(0);
  }

  // Raise the retain flag to indicate that this metadata read is not
  // accompanied by a subsequent full data file read. Computed information
  // should not be deleted, accordingly.

  if(!readMetadata(false, fd, true))
    return false;

  togoTuples = numTuples;
  actualTuples = 0;
  tupSz = 2 + numAttrs + 1;
  SIMTUPLES = TUPLESPACE / (tupSz * sizeof(int));
  tuples = new int[SIMTUPLES * tupSz];

  while(togoTuples > 0){
    if(togoTuples > SIMTUPLES){
      nBytes = read(fd, tuples, SIMTUPLES * tupSz * sizeof(int));
      assert(nBytes == SIMTUPLES * tupSz * sizeof(int));
      actualTuples = SIMTUPLES;
      togoTuples -= SIMTUPLES;
    }
    else{
      nBytes = read(fd, tuples, togoTuples * tupSz * sizeof(int));
      assert(nBytes == togoTuples * tupSz * sizeof(int));
      actualTuples = togoTuples;
      togoTuples = 0;
    }

    for(i = 0; i < actualTuples*tupSz; i+=tupSz){
      trans++;
      initialized = false;
      cliqueSet.clear();
      for(j = i + 2; j < i + tupSz - 1; j++){
	if(!initialized){
	  cliqueSet = valueCliqueMap[tuples[j]];
	  initialized = true;
	}
	else{
	  intersection.clear();
	  set_intersection(cliqueSet.begin(), cliqueSet.end(),
			   valueCliqueMap[tuples[j]].begin(), valueCliqueMap[tuples[j]].end(),
			   insert_iterator<set<int> > (intersection, intersection.begin()));

	  cliqueSet = intersection;
	}
      }
      
      if(!cliqueSet.empty()){
	if(cliqueSet.size() > 1){
	  itemset.clear();
	  itemset.setIndex(++itemsetIndex);
	}
	
	for(cliqueSetIt = cliqueSet.begin(); cliqueSetIt != cliqueSet.end(); cliqueSetIt++){
	  if(cliqueSet.size() > 1)
	    itemset.push_back(*cliqueSetIt);

	  cliques[*cliqueSetIt].incrementSupport();
	} 
	
	if(cliqueSet.size() > 1){
	  itemsets.push_back(itemset);	
	}
      }
    }
  }

  close(fd);
  delete[] tuples;

  return true;
}

bool KCDataset::computeItemsetsVertical(KCCliques& cliques, KCItemsets& itemsets)
{
  int i, j, itemsetIndex;
  KCItemset set;

  itemsets.clear();
  itemsetIndex = 0;

  for(i = 0; i < getTuples(); i++){
    // For each tuple get the cliques that it supports
    set.clear();
    for(j = 0; j < cliques.size(); j++){
      if(cliques[j].getTransactions().find(i) != cliques[j].getTransactions().end())
	set.push_back(j);
    }
    if(set.size() > 1){
      set.setIndex(itemsetIndex++);
      itemsets.push_back(set);
    }
  }
  
  return true;
}



bool KCDataset::buildConfusionInfo(const char* confusionFile, KCCliques& cliques)
{
  int fd, nBytes;
  ofstream conf;

  int tupSz;
  int* tuples;
  int actualTuples, togoTuples;

  int i, j, k, SIMTUPLES;

  bool found;

  KCCliqueDimIt dimIt;
  
  vector<int> matching;
  vector<int>::iterator matchingIt;

  if(!readMetadata(false, fd))
    return false;


  conf.open(confusionFile);
  if(!conf.is_open()){
    cout << "KCDataset::buildConfusionInfo: Could not create confusion file '" << confusionFile << "'" << endl;
    close(fd);
    return false;
  }

  togoTuples = numTuples;
  actualTuples = 0;
  tupSz = 2 + numAttrs + 1;
  SIMTUPLES = TUPLESPACE / (tupSz * sizeof(int));
  tuples = new int[SIMTUPLES * tupSz];

  while(togoTuples > 0){

    if(togoTuples > SIMTUPLES){
      nBytes = read(fd, tuples, SIMTUPLES * tupSz * sizeof(int));
      assert(nBytes == SIMTUPLES * tupSz * sizeof(int));
      actualTuples = SIMTUPLES;
      togoTuples -= SIMTUPLES;
    }
    else{
      nBytes = read(fd, tuples, togoTuples * tupSz * sizeof(int));
      assert(nBytes == togoTuples * tupSz * sizeof(int));
      actualTuples = togoTuples;
      togoTuples = 0;
    }

    for(i = 0; i < actualTuples*tupSz; i+=tupSz){
      // cout << "Checking tuple " << i / tupSz << endl;
      found = false;
      matching.clear();
      for(k = 0; k < cliques.size(); k++){
	for(j = i + 2; j < i + tupSz - 1; j++){
	  for(dimIt = cliques[k].dim(j - (i+2)).begin(); dimIt != cliques[k].dim(j - (i+2)).end(); dimIt++){
	    
	    if(*dimIt == tuples[j])
	      // This dimension is covered!
	      break;
	  }

	  // Fail to cover this dimension if the attribute does not match any of the
	  // attributes that make up the clique. If the current dimension of the clique
	  // is empty we also accept, as this must be a subspace cluster then.
	  if(!cliques[k].dim(j - (i+2)).empty() && dimIt == cliques[k].dim(j - (i+2)).end())
	    // Couldn't fit the tuple into this dimension
	    break;
	  
	}
	if(j == i + tupSz - 1){
	  // A fit was found for all dimensions -> This tuple supports the clique!
	  found = true;
	  matching.push_back(k);
	}
      }
      if(!found){
	conf << "-1";
      }
      else{
	for(matchingIt = matching.begin(); matchingIt != matching.end();matchingIt++){
	  if(matchingIt != matching.begin()){
	    conf << ",";
         }
	  conf << *matchingIt;
	}
      }
      conf << endl;
    }
  }

  close(fd);
  delete[] tuples;
}

bool KCDataset::readMetadata(const bool vertical, int& fd, const bool retain)
{
  int nBytes, i;
  
  fd = open(file.c_str(), O_RDONLY);
  if(fd == -1){
    cout << "KCDataset::readMetadata: Could not open data file '" << file.c_str() << "'" << endl;
    return false;
  }

  nBytes = read(fd, &numTuples, sizeof(int));
  if(nBytes != sizeof(int)){
    cout << "KCDataset::readMetadata: Unexpected end of file." << endl;
    return false;
  }

  nBytes = read(fd, &numAttrs, sizeof(int));
  if(nBytes != sizeof(int)){
    cout << "KCDataset::readMetadata: Unexpected end of file." << endl;
    return false;
  }

  if(distAttributeValues)
    delete[] distAttributeValues;

  distAttributeValues = new int[numAttrs];
  
  nBytes = read(fd, distAttributeValues, numAttrs * sizeof(int));
  if(nBytes != numAttrs * sizeof(int)){
    cout << "KCDataset::readMetadata: Unexpected end of file." << endl;
    return false;
  }

  bases.resize(numAttrs, 0);
  maxAttributes = 0;
  for(i = 0, numAttributeValues = 0; i < numAttrs; i++){
    bases[i] = numAttributeValues;
    numAttributeValues += distAttributeValues[i];
    if(distAttributeValues[i] > maxAttributes)
      maxAttributes = distAttributeValues[i];
  }
  
  if(!retain){
    support.createArray(numAttributeValues);
    support.clear();
  
    attributes = new int[numAttributeValues];
 
    if(vertical){
      verticalInfo.resize(numAttributeValues);
    }
  }

  return true;
}




