#include <algorithm>
#include "eclat.h"
#include "chashtable.h"
#include "../code/KCUtility.h"

cHashItem::cHashItem(vector<int> &iary, int lit, int isup, int ihval) {
   iset=iary;
   //cout << "sz " << iset.size()  << " -- " << iset << endl;
   //iset.resize(iary.size()+1);
   //if (lit != -1) iset[iset.size()-1] = lit;
   if (lit != -1) iset.push_back(lit);
   //cout << "sz2 " << iset.size()  << " -- " << iset << endl;
   sort(iset.begin(), iset.end());
   sup=isup;
   hval = ihval;
}
 
bool cHashItem::subset (cHashItem *ia)
{
   int i,j;
   if (iset.size() > ia->iset.size()) return false;
   for (i=0,j=0; i < iset.size();){
      if (iset[i] < ia->iset[j]) return false;
      else if (iset[i] > ia->iset[j]) ++j;
      else{
         ++i;
         ++j;
      }
   }
   return true;
}

int cHashItem::compare (cHashItem *ia, cHashItem *ib) {
   if (ia->support() > ib->support())  return 1;
   else if (ia->support() < ib->support()) return -1;
   else if (ia->itemset().size() > ib->itemset().size() ) return 1;
   else if( ia->subset(ib) ) return 0;
   else if (ia->itemset().size() < ib->itemset().size() ) return -1;
   else return 1;
}     

ostream& operator << (ostream& fout, cHashItem& hitem)
{
   fout << hitem.itemset() << " - " << hitem.support() << " " 
        << hitem.hashval() << endl;
   return fout;
}


//try to add new closed set to hash table
bool cHashTable::add(vector<int> &iset, int lit, int isup, int ihval)
{
   cHashItem *Hitem= new cHashItem(iset, lit, isup, ihval);
   bool res = list_add(Hitem);
   if (res){
      delete Hitem;
      return false;
   }
   else return true;
}

/*
//return false if Hitem not subsumed by another closed set
bool cHashTable::list_add(cHashItem *Hitem)
{
   //if (Hitem->hashval() == 2840690)
   //   cout << "list add " << chtable.size() << " "
   //        << chtable.bucket_count() << " -- " << *Hitem << endl;
   int hres = chtable.hash_funct()(Hitem->hashval()+Hitem->support());
   
   //cout << "HRES " << Hitem->hashval() << " "<< hres << endl;

   cHTable::iterator hi = chtable.find(hres);
   if (hi == chtable.end()){
      //cout << "NOT FOUND\n";
      chtable[hres] = new list<cHashItem *>;
      chtable[hres]->push_back(Hitem);
      return false;
   }
   else{
      //cout << " FOUND :\n";
      list<cHashItem *>::iterator li;
      
      int res;
      li = (*hi).second->begin();
      for (; li != (*hi).second->end(); ++li){
         res = cHashItem::compare(Hitem, *li);
         
         //cout << "LOOK AT " << res << " -- " << *(*li) << endl;

         if (res == 0) return true;
         else if (res == -1){
            (*hi).second->insert(li, Hitem);
            return false;
         }
      }
      (*hi).second->push_back(Hitem);
      return false;
   }
}
*/

//return false if Hitem not subsumed by another closed set
bool cHashTable::list_add(cHashItem *Hitem)
{
   //if (Hitem->hashval() == 2840690)
   //   cout << "list add " << chtable.size() << " "
   //        << chtable.bucket_count() << " -- " << *Hitem << endl;
   int hres = chtable.hash_funct()(Hitem->hashval());
   
   //cout << "HRES " << Hitem->hashval() << " "<< hres << endl;

   cHTFind p = chtable.equal_range(hres);
   cHTable:: iterator hi = p.first;
   
   int res;
   for (; hi!=p.second; ++hi){
      res = cHashItem::compare(Hitem, (*hi).second);
      
      //cout << "LOOK AT " << res << " -- " << *(*li) << endl;
      
      if (res == 0) return true;
   }
   chtable.insert(cHTPair(hres, Hitem));
   return false;
}
