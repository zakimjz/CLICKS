#include <algorithm>
#include "calcdb.h"
#include "eqclass.h"

ostream & operator<<(ostream& ostr, Eqnode& eqn){
   int *fidx = Dbase_Ctrl_Blk::FreqIdx;
   ostr << fidx[eqn.val] << " - " << eqn.support(); // << " " << eqn.hashval();
   if (output_idlist){
      //ostr << " [" << eqn.tidset << "]";
      if (alg_type == basicmax || alg_type == maxcharm || closed_type == cmax) 
         ostr << " -- " << eqn.maxsup << " {" << eqn.maxset << "}";
   }
   return ostr;
}

Eqclass::~Eqclass()
{
   //list<Eqnode *>::iterator ni = _nodelist.begin();
   for_each(nlist().begin(), nlist().end(), delnode<Eqnode *>());
}

void Eqclass::add_node(int val)
{
   Eqnode *eqn = new Eqnode(val);
   _nodelist.push_back(eqn);
}

void Eqclass::add_node(Eqnode *eqn)
{
   _nodelist.push_back(eqn);
}


void Eqclass::set_prefix(vector<int> &pref, Eqnode &node)
{
   _prefix = pref;
   _prefix.push_back(node.val);
}

ostream & Eqclass::print_prefix(bool supflg)
{
   for (unsigned int i=0; i < _prefix.size(); ++i){
      cout << Dbase_Ctrl_Blk::FreqIdx[_prefix[i]] << " ";
   }
   //if (supflg) cout << " - " << _closedsup << " ";
   return cout;
}

vector<int> Eqclass::get_nodes()
{
   vector<int> v;
   v.clear();
   for (unsigned int i=0; i < _prefix.size(); ++i)
      v.push_back(Dbase_Ctrl_Blk::FreqIdx[_prefix[i]]);
   return v;
}      

ostream & Eqclass::print_node(Eqnode &node)
{
   print_prefix();
   cout << node;
   return cout;
}


//print with items remapped to their original value
ostream& operator << (ostream& fout, Eqclass& eq)
{
  list<Eqnode *>::iterator ni = eq._nodelist.begin();
  for (; ni != eq._nodelist.end(); ++ni){
     eq.print_node(*(*ni)) << endl;
  }
  return fout;
}
 
