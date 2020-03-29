#ifndef _KCDATASET_H_
#define _KCDATASET_H_

#include <string>

#include "KCGlobal.h"
#include "KCArray.h"

using std::string;

//typedef KCSymArray< KCTransactionSet > KCVerticalInfo;
typedef vector< KCTransactionSet > KCVerticalInfo;

class KCDataset
{
 protected:
  string file;
  KCSymArray<int> support; //MJZ -- made short into int 4/19/2006
  KCVerticalInfo verticalInfo;

  // For each attribute value, store which attribute it belongs to
  int* attributes;
  // Store for each attribute the number of distinct attribute values
  int* distAttributeValues;
  // Store for each attribute the minimum attribute value
  vector<int> bases;

  int numAttrs, numTuples, numAttributeValues, maxAttributes;

  KCOrderedNodes nodeOrder;
  bool orderNodes(KCDescendNodes&);

 public:
  bool computeSupportInfo(const float alpha, bool& supportSufficient, const bool vertical, bool use_frequency);
  bool readDataset(const bool vertical = false);
  bool calculateCliqueSupport(KCCliques& cliques, KCValueCliqueMap& map, KCItemsets& itemsets);
  bool computeItemsetsVertical(KCCliques& cliques, KCItemsets& itemsets);
  bool buildConfusionInfo(const char*, KCCliques& cliques);

 protected:
  bool readMetadata(const bool vertical, int& fileDescriptor, const bool retain = false);

 public:
  KCDataset(const char* filename){
    file = filename;
    attributes = NULL;
    distAttributeValues = NULL;
    numAttrs = numTuples = numAttributeValues = 0;
  }

  ~KCDataset(){
    if(attributes)
      delete[] attributes;

    if(distAttributeValues)
      delete[] distAttributeValues;
  }
  
  KCSymArray<int>& supportInfo(){
    return support;
  }

  const char* getDataFile(){
    return file.c_str();
  }

  int numberOfAttributes(){
    return numAttrs;
  }

  int getAttribute(int v){
    return attributes[v];
  }

  int getTuples(){
    return numTuples;
  }

  int getNumAttributeValues(){
    return numAttributeValues;
  }

  int getMaxAttributeValues(){
    return maxAttributes;
  }

  KCOrderedNodes& getNodeOrder(){
    return nodeOrder;
  }

  int getDistinctValues(int attribute){
    return distAttributeValues[attribute];
  }

  int getBase(int attribute){
    return bases[attribute];
  }

  KCTransactionSet& getVerticalInfo(const int value){
    //cout << "Getting vertical info for " << value << " (at size " << verticalInfo.size() << ")" << endl;
    return verticalInfo[value];
  }
};

#endif
