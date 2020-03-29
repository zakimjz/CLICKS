#ifndef _KCGLOBAL_H
#define _KCGLOBAL_H

#include <map>
#include <set>
#include <vector>
#include <utility>
#include <fstream>
#include <iostream>
#include <ext/hash_map>

#define KC_LEAN

using std::map;
using std::set;
using std::vector;
using std::pair;
using std::sort;
using std::ofstream;

using __gnu_cxx::hash_map;

#define INTERSECT(s1, s2, result, type) set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), insert_iterator< type >(result, result.begin()))
#define UNION(s1, s2, result, type) set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), insert_iterator< type >(result, result.begin()))

typedef unsigned int KCNodeType;
typedef pair<KCNodeType, KCNodeType> KCNodePair; //conn_nodes

typedef struct{
  KCNodeType value;
  bool deleted;
  bool processed;
} KCCliqueNode;


class StrLess
{
public:
  int operator()(KCNodePair& a, KCNodePair& b) const {
    if (a.first < b.first)
      return 1;
    else if (a.first == b.first && a.second < b.second)
      return 1;
    else return 0;
  }
};

class IntLess
{
public:
  int operator()(int a, int b) const{
    if (a > b)
      return 1;
    else
      return 0;
  }
};

typedef vector<KCCliqueNode> KCNodeList;
typedef KCNodeList::iterator KCNodeListIt;

typedef vector<KCCliqueNode> KCCliqueNodeVector;
typedef KCCliqueNodeVector::iterator KCCliqueNodeVectorIt;

typedef std::map<KCNodeType, int, StrLess> KCNodesMap;
typedef KCNodesMap::value_type KCNodesMapValue;//connAtt

typedef std::map<int, KCCliqueNodeVector, IntLess> KCOrderedNodes;
typedef KCOrderedNodes::iterator KCOrderedNodesIt;

typedef std::map<int, vector<int> > KCClusterSupport;
typedef std::map<int, int> KCDescendNodes;

typedef vector<KCNodeType> KCCliqueDim;
typedef KCCliqueDim::iterator KCCliqueDimIt;

typedef map<KCNodeType, set<int> > KCValueCliqueMap;
typedef KCValueCliqueMap::iterator KCValueCliqueMapIt;

typedef vector<int> KCFrequentSet;
typedef KCFrequentSet::iterator KCFrequentSetIt;
typedef vector<KCFrequentSet> KCFrequentSets;
typedef KCFrequentSets::iterator KCFrequentSetsIt;

class  KCTransactionSet : public set<int>
{
 public:
  KCTransactionSet() {};
};
typedef KCTransactionSet::iterator KCTransactionIt;

class KCItemset : public vector<int>
{
 public:
  KCItemset(){
    index = 0;
  }

  void serialize(ofstream& out){
    vector<int>::iterator it;
    int s;

    // This is the format in which the ECLAT algorithm that is
    // used for frequent itemset mining expects its input
    sort(begin(), end());
    /*for(int i = 0, s = 0; i < size(); i++){
      if((*this)[i] >= 0)
      break;
      else
      s++;
      }*/
    s = size();

    if(s == 0)
      return;

    out.write((char*)&index, sizeof(int));
    out.write((char*)&index, sizeof(int));
    out.write((char*)(&s), sizeof(int));

    for(it = begin(); it != end(); ++it){
      //  if((*it) >= 0)
      out.write((char*)&(*it), sizeof(int));
    }
  }

  void setIndex(const int newIndex){
    index = newIndex;
  }

 protected:
  int index;
};
typedef KCItemset::iterator KCItemsetIt;

class KCItemsets : public vector<KCItemset>
{
 public:
  void serialize(ofstream& out){
    vector<KCItemset>::iterator it;
    for(it = begin(); it != end(); ++it){
      it->serialize(out);
    }
  }
};

typedef KCItemsets::iterator KCItemsetsIt;

typedef map<int, vector<int> > KCCliqueMaxSetMapping;
typedef vector<int>::iterator KCMaxSetIt;

typedef vector<int> KCItemsetSupport;
typedef KCItemsetSupport::iterator KCItemsetSupportIt;

typedef vector<bool> KCFlagVector;

class KCClique{

public:
  KCClique(int dimensions){
    support = 0;
    numDims = dimensions;
    dims.resize(dimensions);
    bases.assign(dimensions, 0);
  }

  void reset(){
    dims.clear();
    dims.resize(numDims);
    bases.assign(numDims, 0);
  }

  KCCliqueDim& dim(const int dim){
    return dims[dim];
  }

  const KCCliqueDim& const_dim(const int dim) const{
    return dims[dim];
  }

  void sortDimensions(){
    for(int i = 0; i < numberOfDimensions(); ++i){
      sort(dims[i].begin(), dims[i].end());
    }
  }

  bool empty() const{
    for(int i = 0; i < numberOfDimensions(); ++i){
      if(!dims[i].empty())
	return false;
    }
    return true;
  }

  void sortDimension(const int dim){
    sort(dims[dim].begin(), dims[dim].end());
  }

  int numberOfDimensions() const{
    return numDims;
  }

  int getSupport(){
    return support;
  }

  void setSupport(const int supp){
    support = supp;
  }

  void incrementSupport(){
    support++;
  }

  int getBase(const int dim){
    return bases[dim];
  }

  void setBase(const int dim, const int base){
    bases[dim] = base;
  }

  KCTransactionSet& getTransactions(){
    return transactions;
  }

  void constructFullDim(const int dim, KCCliqueDim& cdim, int numAttributeValues){
    int lower, upper, j;

    cdim.clear();
    // If a dimension is empty, it supports all possible values in this dimension
    // (subspace clustering case)
    lower = getBase(dim);

    if(dim == numberOfDimensions() - 1)
      upper = numAttributeValues - 1;
    else
      upper = getBase(dim+1) - 1;

    for(j = lower; j <= upper; j++){
      cdim.push_back(j);
    }
  }

  void constructNodeList(KCNodeList& nodeList){
    vector<KCCliqueDim>::iterator dimsIt;
    KCCliqueDimIt dIt;
    KCCliqueNode node;

    nodeList.clear();
    for(dimsIt = dims.begin(); dimsIt != dims.end(); dimsIt++){
      for(dIt = dimsIt->begin(); dIt != dimsIt->end(); dIt++){
	node.value = *dIt;
	node.processed = 0;
	node.deleted = 0;
	nodeList.push_back(node);
      }
    }
  }

  int size(){
    int result = 0;
    for(int i = 0; i < numberOfDimensions(); ++i){
      result += dims[i].size();
    }
    return result;
  }


  bool operator==(const KCClique& other) const {
    for(int i = 0; i < numberOfDimensions(); ++i){
      if(!(dims[i] == other.dims[i]))
	return false;
    }
    return true;
  }

protected:
  int numDims;
  int support;

  KCTransactionSet transactions;

  // For every dimension, dims provides a vector of node entries
  // (i.e. a KCCliqueDim)
  vector<KCCliqueDim> dims;
  vector<int> bases;
};

namespace __gnu_cxx{

  template <>
  struct hash<KCClique>{
    size_t operator()(const KCClique& c) const{
      size_t result = 0;
      for(int i = 0; i < c.numberOfDimensions(); ++i){
	for(int j = 0; j < c.const_dim(i).size(); ++j){
	  result += (c.const_dim(i))[j];
	}
      }
      return result;
    }
  };

}

namespace __gnu_cxx{
  template <>
  struct hash<KCCliqueDim>{
    size_t operator()(const KCCliqueDim& c) const{
      size_t result = 0;
      for(int i = 0; i < c.size(); ++i){
	result += c[i];
      }

      return result;
    }
  };

}

typedef vector<KCClique> KCCliques;
typedef KCCliques::iterator KCCliquesIt;

typedef set<int> KCAttributes;


#endif
