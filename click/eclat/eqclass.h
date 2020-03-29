#ifndef __eqclass_h
#define __eqclass_h

#include <iostream>
#include <list>
#include <vector>
#include <set>
#include <functional>

#include "eclat.h"

template<class T>
struct delnode: public unary_function<T, void>
{
   void operator() (T x){ delete x; }
};

class Eqnode{
public:
   int val; //the item
   int sup; //support
   idlist tidset; //tidlist for this item

   int maxsup; //number of maximal itemsets
   idlist maxset; //the maximal itemsets containing this itemset
   int hval; //hash value for closed itemsets


   Eqnode(int v, int s=0, int m=0): val(v), sup(s), maxsup(m), hval(0){
      //cmaxset(1);
   }
   int &support(){ return sup; }
   int &maxsupport(){ return maxsup; }
   int &hashval(){ return hval; }
   static bool incr_cmp (Eqnode *n1, Eqnode *n2){
      //cout << "CAM<E HEREEE INCR\n";
      if ((n1)->sup < (n2)->sup) return true;
      else return false;
   }
   static bool decr_cmp (Eqnode *n1, Eqnode *n2){
      //cout << "CAM<E HEREEE DECR\n";
      if ((n1)->sup < (n2)->sup) return false;
      else return true;
   }


   friend ostream & operator<<(ostream& ostr, Eqnode& eqn);
};


class Eqclass{
private:
   vector<int> _prefix;
   //int _closedsup; //used when mining closed sets
   list<Eqnode *> _nodelist;

public:

   ~Eqclass();
   vector<int> &prefix(){ return _prefix; }
   list<Eqnode *> &nlist(){ return _nodelist; }
   //int &closedsupport(){ return _closedsup; }


   void sort_nodes(){
      if (sort_type == incr) _nodelist.sort(Eqnode::incr_cmp);
      else if (sort_type == decr) _nodelist.sort(Eqnode::decr_cmp);
   }

   vector<int> get_nodes();

   void add_node(int val);
   void add_node(Eqnode *eqn);
   int item(int n);
   void set_prefix(vector<int> &pref, Eqnode &node);
   ostream & print_prefix(bool supflg=false);
   ostream & print_node(Eqnode &node);
   friend ostream & operator<<(ostream& ostr, Eqclass& eq);
};

#endif

