#ifndef _CHASH_
#define _CHASH_

using namespace std;
#define HASHNS __gnu_cxx

#include <list>
//#include <map.h>
#include <ext/hash_map>
#include <vector>
#include <iostream>


class cHashItem{
private:
   vector<int> iset;
   int sup;
   int hval;
public:
   cHashItem(vector<int> &iary, int lit, int isup, int ihval);
   int hashval() { return hval;}
   int support() { return sup; }
   vector<int> &itemset() { return iset; }
   bool subset (cHashItem *ia);
   static int compare (cHashItem *ia, cHashItem *ib);
   friend ostream& operator << (ostream& fout, cHashItem& lst);
};


#define HASHSIZE 100
struct cHash
{
   size_t operator() (int x) const { return (x%HASHSIZE); }
};


//typedef map<int, list<cHashItem *> *, less<int> > cHTable;
//typedef hash_map<int, list<cHashItem *> *, cHash, equal_to<int> > cHTable;
typedef HASHNS::hash_multimap<int, cHashItem *,
   HASHNS::hash<int>, equal_to<int> > cHTable;
typedef pair<cHTable::iterator, cHTable::iterator> cHTFind;
typedef cHTable::value_type cHTPair;


class cHashTable{
public:
   cHTable chtable;
   cHashTable(int sz=HASHSIZE): chtable(sz){}

   bool add(vector<int> &iset, int lit, int isup, int ihval);
   bool list_add(cHashItem *Hitem);

   void print_hashstats(){
      cout << "HASHSTATS " << chtable.size() << " "
           << chtable.bucket_count() << endl;
   }

};

#endif

