#include <iostream>
#include <vector>
#include <set>

#include "KCGlobal.h"
#include "KCDataset.h"
#include "KCUtility.h"

using namespace std;

void initializeNodeList(KCDataset& dataset, KCNodeList& nodeList)
{
  KCOrderedNodesIt it;
  KCNodeListIt nodeListIt;
  
  for (it = dataset.getNodeOrder().begin(); it != dataset.getNodeOrder().end(); it ++){
    for (nodeListIt = it->second.begin(); nodeListIt != it->second.end(); nodeListIt++){
      nodeListIt->processed = 0;
      nodeList.push_back(*nodeListIt);
    }
  }
}

void initializeNodeListVertical(KCDataset& dataset, KCNodeList& nodeList)
{
  KCCliqueNode node;
  int i;

  nodeList.clear();

  for(i = 0; i < dataset.getNumAttributeValues(); i++){
    node.value = i;
    node.processed = 0;
    node.deleted = 0;
    nodeList.push_back(node);
    //cout << "Next " << node.value << " " << node.processed << " " << node.deleted << endl;
  }
}

/* 
   Recursively extend the clique given in "clique" with nodes from the "node list". 
   "cliques" contains the set of all cliques that have been detected so far and
   "coveredAttributes" is a set of all attribute indices from which at least one value
   is part of "clique".
   "subspaces" is a flag that specifies whether or not subspace cliques are to be computed, 
   while vertical indicates whether or not vertical mining information is to be used.
   Note, that in this case vertical mining information has to be present in "dataset".
   "transaction" is a list of all transactions that support the clique in "clique" and
   transactionThreshold is an integer value specifying the minimum number of transactions
   that need to support a clique (used for pruning). The latter two are only used of "vertical"
   is set to true.
*/

typedef hash_map< KCClique, KCTransactionSet > KCCliqueSupporters;
typedef hash_map< KCCliqueDim, KCTransactionSet > KCCliqueDimSupporters;

typedef KCCliqueSupporters::iterator KCCliqueSupportersIt;
typedef KCCliqueDimSupporters::iterator KCCliqueDimSupportersIt;

KCCliqueSupporters supportCache;  
KCCliqueDimSupporters dimSupportCache;

void computeMaximalCliques(KCDataset& dataset, const KCClique& in_clique, const KCNodeList& in_nodeList, KCCliques& cliques, 
			   const KCAttributes& in_coveredAttributes, const bool subspaces, const bool vertical, const int minsup)
{
  KCCliqueNode cliqueNode;
  
  KCAttributes* currentlyCovered = new KCAttributes();
  KCAttributes* potentialCover = new KCAttributes();
  KCClique* currentClique = new KCClique(dataset.numberOfAttributes());
  KCNodeList* currentNodeList = new KCNodeList(); 

  KCNodeListIt node;
  KCNodeListIt additionalNode;

  KCCliqueSupportersIt cacheIt;
  KCCliqueDimSupportersIt dimCacheIt;

  KCTransactionSet tempTrans, dimTrans, newTrans;
  bool first;

  KCClique* clique = new KCClique(in_clique.numberOfDimensions());
  (*clique) = in_clique;
  
  KCClique* tempClique = new KCClique(in_clique.numberOfDimensions());

  KCNodeList* nodeList = new KCNodeList();
  (*nodeList) = in_nodeList;

  KCAttributes* coveredAttributes = new KCAttributes();
  (*coveredAttributes) = in_coveredAttributes; 

  // If full-space clustering is enforced (subspaces == false), we check if this clique
  // spans over all attributes
  if(((!subspaces &&(signed)coveredAttributes->size() == dataset.numberOfAttributes()) || subspaces) && 
     nodeList->empty()){

    if(!vertical){
      cliques.push_back(*clique);
    }
    else if(clique->getTransactions().size() >= minsup){
      clique->setSupport(clique->getTransactions().size());
      cliques.push_back(*clique);
    }
    return;
  }     
  for(node = nodeList->begin(); node != nodeList->end(); ++node){
    if (node->deleted || node->processed)
       continue;
    
    *currentClique = *clique;
    *currentlyCovered = *coveredAttributes;
    currentNodeList->clear();
    
    // Add this node to our the new candidate clique
    currentClique->dim(dataset.getAttribute(node->value)).push_back(node->value);

    node->deleted = true;
    
    currentlyCovered->insert(dataset.getAttribute(node->value));
    // We build the potential cover further down below. While the current cover is the set
    // of all dimensions covered by current clique, potential cover also adds all dimensions
    // that could potentially be covered with the nodes remaining in the currentNodeList. That
    // way, the algorithm can purge subtrees that will not reach full dimensionality, if
    // desired
    *potentialCover = *currentlyCovered;
    
    for(additionalNode = nodeList->begin(); additionalNode != nodeList->end(); additionalNode++){       
      if(additionalNode->deleted || additionalNode->value == node->value)
	continue;
      else{
	if(dataset.supportInfo().getValueSafe(node->value, additionalNode->value) > 0){
	  cliqueNode = *additionalNode;
	  cliqueNode.deleted = false;
	  cliqueNode.processed = false;
	  currentNodeList->push_back(cliqueNode);
	
	  additionalNode->processed = 1;
	  
	  potentialCover->insert(dataset.getAttribute(additionalNode->value)); 
	}
      }
    }

    if(vertical){
      // For vertical mining we also need to compute the support at this stage.
      // If the added value was the first one for that attribute simply intersect the
      // transaction set. Otherwise unite the previous transactions with those supporting 
      // a clique where the new value is the only one for the respective attribute.
      currentClique->getTransactions().clear();
      currentClique->sortDimensions();
      cacheIt = supportCache.find(*currentClique);
      if(cacheIt == supportCache.end()){	
	if(currentClique->dim(dataset.getAttribute(node->value)).size() == 1){
	  // This is the first value for that attribute. Support calculation is easy.
	  if(clique->empty()){
	    currentClique->getTransactions() = dataset.getVerticalInfo(node->value);
	  }
	  else{
	    INTERSECT(clique->getTransactions(), dataset.getVerticalInfo(node->value), currentClique->getTransactions(), set<int>);
	  }
	}
	else{
	  // This is NOT the first value for the given attribute.
	  *tempClique = *currentClique;
	  tempClique->dim(dataset.getAttribute(node->value)).clear();
	  cacheIt = supportCache.find(*tempClique);
	  newTrans.clear();
	  if(cacheIt == supportCache.end()){
	    // No idea what the transactions for the tempClique are, so we have to compute them
	    // bottom up. We can assume that there is at least one value in this clique (otherwise we wouldn't be
	    // in this branch of the if)
	    first = true;
	    for(int i = 0; i < tempClique->numberOfDimensions(); ++i){
	      if(tempClique->dim(i).empty())
		continue;
	      
	      dimCacheIt = dimSupportCache.find(tempClique->dim(i));
	      if(dimCacheIt == dimSupportCache.end()){
		dimTrans = dataset.getVerticalInfo(tempClique->dim(i)[0]);
		for(int j = 1; j < tempClique->dim(i).size(); j++){
		  tempTrans.clear();
		  UNION(dimTrans, dataset.getVerticalInfo(tempClique->dim(i)[j]), tempTrans, set<int>);
		  dimTrans = tempTrans;
		}
		dimSupportCache[tempClique->dim(i)] = dimTrans;
	      }
	      else{
		dimTrans = dimCacheIt->second;
	      }
		
	      if(first){
		newTrans = dimTrans;
		first = false;
	      }
	      else{
		tempTrans.clear();
		INTERSECT(newTrans, dimTrans, tempTrans, set<int>);
		newTrans = tempTrans;
	      }
	    }
	    supportCache[*tempClique] = newTrans;
	    
	    tempTrans.clear();
	    INTERSECT(newTrans, dataset.getVerticalInfo(node->value), tempTrans, set<int>);
	    newTrans = tempTrans;
	  }
	  else{
	    INTERSECT(cacheIt->second, dataset.getVerticalInfo(node->value), newTrans, set<int>);
	  }
	  
	  UNION(newTrans, clique->getTransactions(), currentClique->getTransactions(), set<int>);
	}
	supportCache[*currentClique] = currentClique->getTransactions();
      }
      else
	currentClique->getTransactions() = cacheIt->second;
    } 
    
    if ((!subspaces && potentialCover->size() == dataset.numberOfAttributes()) || subspaces){
      computeMaximalCliques(dataset, *currentClique, *currentNodeList, cliques, *currentlyCovered, subspaces, vertical, minsup);
    }
  }

  delete clique;
  delete coveredAttributes;
  delete nodeList;
  delete tempClique;

  delete currentlyCovered;
  delete potentialCover;
  delete currentClique;
  delete currentNodeList;
  
}


bool computeCliques(KCDataset& dataset, KCCliques& cliques, const bool subspaces, const bool vertical, const int minsup)
{
  KCClique clique(dataset.numberOfAttributes());
  KCNodeList nodes;
  KCAttributes coveredAttributes;

  if(vertical)
    initializeNodeListVertical(dataset, nodes);
  else
    initializeNodeList(dataset, nodes);
  
  cliques.clear();

  for(int i = 0; i < dataset.numberOfAttributes(); i++){
    clique.setBase(i, dataset.getBase(i));
  }

  supportCache.clear();
  dimSupportCache.clear();

  computeMaximalCliques(dataset, clique, nodes, cliques, coveredAttributes, subspaces, vertical, minsup);

  dimSupportCache.clear();
  supportCache.clear();

  return true;
}

