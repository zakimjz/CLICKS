#include <iostream>

#include "KCUtility.h"

using namespace std;

bool hasMapping = false;
vector<vector<string> > attrMap;

void readMapping(const char* mapFile, const int attributes, const int maxValues)
{
  ifstream in;
  string token, value;
  int currentAttribute = 0;

  vector<string> dummy;
  dummy.resize(maxValues + 1, string(""));
  attrMap.resize(attributes, dummy);

  in.open(mapFile);

  while(!in.eof()){
    in >> token;
    // New attribute description
    if(token == "*A*"){
      in >> value;
      currentAttribute = atoi(value.c_str());
    }
    else{
      in >> value;
      attrMap[currentAttribute][atoi(value.c_str())].assign(token);
    }
  }
  in.close();
  hasMapping = true;
}

ostream& operator<<(ostream& out, KCClique& c){
  int i;
  KCCliqueDimIt it;

  for(i = 0; i < c.numberOfDimensions(); ++i){
    c.sortDimension(i);
      if (c.dim(i).size() > 0){
      out << "A" << i << " {";
      for(it = c.dim(i).begin(); it != c.dim(i).end(); it++){
	if(!hasMapping){
	  out << *it - c.getBase(i) + 1 << " ";
	}
	else{
	  out << attrMap[i][*it - c.getBase(i) + 1] << " ";
	}
	//out << *it << " ";
      }
      out << "}" << endl;
    }
  }

  return out;
}

ostream& operator<<(ostream& out, KCCliqueDim& c){
  int i;
  KCCliqueDimIt it;

  out << "{";
  for(it = c.begin(); it != c.end(); it++){
    out << *it << " ";
  }
  out << "}" << endl;

  return out;
}

ostream& operator<<(ostream& out, set<int>& c){
  int i;
  set<int>::iterator it;

  out << "{";
  for(it = c.begin(); it != c.end(); it++){
    out << *it << " ";
  }
  out << "}" << endl;

  return out;
}

ostream& operator<<(ostream& out, vector<int>& c){
  int i;
  vector<int>::iterator it;

  out << "{";
  for(it = c.begin(); it != c.end(); it++){
    out << *it << " ";
  }
  out << "}" << endl;

  return out;
}

ostream& operator<<(ostream& out, KCCliques& c){
  KCCliquesIt it;
  KCCliqueDimIt dimIt;

  int i = 1, j = 0;

  out << endl;
  out << "**************** Cliques Dump ****************" << endl;

  for (it = c.begin(); it != c.end(); ++it, ++i){
    out << "---------------- Clique " << i << "(" << it->getSupport() << ")----------------" << endl;
    out << *it;
  }
  
  out << "**********************************************" << endl;
    
  return out;
}

// ostream& operator<<(ostream& out, KCTransactionSet& c)
// {
//   KCTransactionSet::iterator it;

//   out << "{";
//   for(it = c.begin(); it != c.end(); it++){
//     out << *it << " ";
//   }
//   out << "}";

//   return out;
// }



