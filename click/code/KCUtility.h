#ifndef _KCUTILITY_H_
#define _KCUTILITY_H_

#include <iostream>
#include "KCGlobal.h"
#include "KCArray.h"

using namespace std;

void readMapping(const char* mapFile, const int attributes, const int maxValues);

ostream& operator<<(ostream& out, KCCliqueDim& c);
ostream& operator<<(ostream& out, KCClique& c);
ostream& operator<<(ostream& out, KCCliques& c);
ostream& operator<<(ostream& out, set<int>& c);
ostream& operator<<(ostream& out, vector<int>& c);
//ostream& operator<<(ostream& out, KCTransactionSet& c);

template<class T>
ostream& operator<<(ostream& out, KCSymArray<T>& o)
{
  int i, j;

  out << "Symmetric 2D Array (size " << o.getNumberOfDimensions() 
      << " X " << o.getNumberOfDimensions() << ")" << endl;

  for (i = 0; i < o.getNumberOfDimensions(); i++)
    for (j = 0; j < o.getNumberOfDimensions(); j++)
      out << "V(" << i << " " << j << ") : " << o.getValueSafe(i,j) << endl;

  
  return out;
}

#endif
