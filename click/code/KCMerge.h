#ifndef _KCMERGE_H
#define _KCMERGE_H

#include <fstream>
#include <list>
#include <vector>
#include <set>

#include "KCDataset.h"
#include "KCGlobal.h"

void computeValueCliqueMapping(KCCliques& cliques, KCValueCliqueMap& valueCliqueMap, KCDataset& dataset);

bool pruneCliquesAndItemsets(KCCliques& cliques, KCItemsets& itemsets, 
			     float alpha, bool selectiveVertical,
			     bool subspaces, KCDataset& dataset, 
			     int& restored, bool use_frequency);

bool mergeCliques(KCDataset& d, KCCliques& cliques, 
		  KCItemsets& itemsets, 
		  float alpha, float minSup,
		  KCCliques& mergedCliques, bool use_frequency);

void getCliqueMaxSetMapping(KCFrequentSets& frequentSets, KCCliqueMaxSetMapping& map,
			    KCItemsetSupport& support, KCItemsetSupport& total,
			    KCCliques& cliques);

void sortMaximalItemsets(KCFrequentSets& frequentSets, KCCliques& cliques, 
			 KCItemsetSupport& totalSupport, KCFlagVector& erasedCliques);

#endif
