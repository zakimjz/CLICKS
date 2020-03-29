#ifdef __GNUC__
#include <unistd.h>
#endif

#include<iostream>
#include <stdio.h>
#include <list>
#include <climits>

//headers
#include "eclat.h"
#include "calcdb.h"
#include "eqclass.h"
#include "stats.h"
#include "chashtable.h"
#include "../code/KCUtility.h"

//global vars
char infile[300];
Dbase_Ctrl_Blk *DCB;
Stats stats;
vector<vector<int> > v_freqsets;

double MINSUP_PER;
int MINSUPPORT=-1;
int DBASE_MAXITEM;
int DBASE_NUM_TRANS;

//default flags
bool output = false; //don't print freq itemsets
bool output_idlist = false; //don't print idlist

diff_vals diff_type = diff; //default is diffset mode
diff_vals max_diff_type = nodiff; //deafult is to use tidsets for maxtest
sort_vals sort_type = incr; //default is to sort in increasing order
alg_vals alg_type = eclat; //default is to find all freq patterns
closed_vals closed_type = cnone; //default is not to eliminate non-closed
prune_vals prune_type = noprune; //default no pruning

//extern functions
extern void enumerate_max_freq(Eqclass *eq, int iter, idlist &newmax);
extern void enumerate_max_closed_freq(Eqclass *eq, int iter, idlist &newmax);
extern void enumerate_closed_freq(Eqclass *eq, int iter, idlist &newmax);
extern void enumerate_freq(Eqclass *eq, int iter);
extern void form_closed_f2_lists(Eqclass *eq);
extern void form_f2_lists(Eqclass *eq);
extern cHashTable hashtest; //for closed (with chash) testing

//ostream & operator<<(ostream& fout, vector<int> &vec){
//  for (unsigned int i=0; i < vec.size(); ++i)
//     fout << vec[i] << " ";
//  return fout;
//}


void get_F1()
{
  double te = 0.0;

  int i, j, it;
  const int arysz = 10;
  
  vector<int> itcnt(arysz,0); //count item frequency

  DBASE_MAXITEM=0;
  DBASE_NUM_TRANS = 0;
  
  DCB->Cidsum = 0;
  
   while(DCB->get_next_trans())
   {
      for (i=0; i < DCB->TransSz; ++i)
      {
         it = DCB->TransAry[i];

         if (it >= DBASE_MAXITEM)
         {
            itcnt.resize(it+1,0);
            DBASE_MAXITEM = it+1;
         }
         ++itcnt[it];
      }
      
      if (DCB->MaxTransSz < DCB->TransSz) DCB->MaxTransSz = DCB->TransSz;     
      ++DBASE_NUM_TRANS;
      DCB->Cidsum += DCB->Cid; //used to initialize hashval for closed set mining
   }

   //set the value of MINSUPPORT
   if (MINSUPPORT == -1)
      MINSUPPORT = (int) (MINSUP_PER*DBASE_NUM_TRANS+0.5);
   
   if (MINSUPPORT<1) MINSUPPORT=1;
   //cout<<"DBASE_NUM_TRANS : "<< DBASE_NUM_TRANS << endl;
   //cout<<"DBASE_MAXITEM : "<< DBASE_MAXITEM << endl;
   //cout<<"MINSUPPORT : "<< MINSUPPORT << " (" << MINSUP_PER << ")" << endl;

   //count number of frequent items
   DCB->NumF1 = 0;
   for (i=0; i < DBASE_MAXITEM; ++i)
      if (itcnt[i] >= MINSUPPORT)
         ++DCB->NumF1;

   //construct forward and reverse mapping from items to freq items
   DCB->FreqIdx = new int [DCB->NumF1];
   DCB->FreqMap = new int [DBASE_MAXITEM];
   for (i=0,j=0; i < DBASE_MAXITEM; ++i) 
   {
      if (itcnt[i] >= MINSUPPORT) 
      {
         //if (output && alg_type == eclat) 
            //cout << i << " - " << itcnt[i] << endl;
         DCB->FreqIdx[j] = i;
         DCB->FreqMap[i] = j;
         ++j;
      }
      else DCB->FreqMap[i] = -1;
   }
   
   DCB->alloc_ParentClass(itcnt);
   
   stats.add(DBASE_MAXITEM, DCB->NumF1, te);
}

list<Eqclass *> * get_F2()
{
  int i,j;
  int it1, it2;
  vector<int> freqsets;
  
  double te = 0.0;

  list<Eqclass *> *F2list = new list<Eqclass *>;

  //itcnt2 is a matrix of pairs p, p.first is count, p.second is flag
  int **itcnt2 = new int*[DCB->NumF1];

  //unsigned int **itcnt2 = new unsigned int *[DCB->NumF1];
  for (i=0; i < DCB->NumF1; ++i)
  {
    itcnt2[i] = new int [DCB->NumF1];
    for (j=0; j < DCB->NumF1; ++j)
      itcnt2[i][j] = 0;
  }
    
   while(DCB->get_next_trans())
   {
      DCB->get_valid_trans();
      DCB->make_vertical();
      //count a pair only once per cid
      for (i=0; i < DCB->TransSz; ++i)
      {
         it1 = DCB->TransAry[i];
         for (j=i+1; j < DCB->TransSz; ++j)
         {
            it2 = DCB->TransAry[j];
            ++itcnt2[it1][it2];
         }
      }
   }                           

   //compute class size
   DCB->class_sz = new int[DCB->NumF1];
   DCB->F2sum = new int[DCB->NumF1];
   for (i=0; i < DCB->NumF1; ++i)
   {
      DCB->class_sz[i] = 0;
      DCB->F2sum[i] = 0;
   }
   
   for (i=0; i < DCB->NumF1; ++i)
   {
      for (j=i+1; j < DCB->NumF1; ++j)
      {
         if (itcnt2[i][j] >= MINSUPPORT)
         {
            ++DCB->class_sz[i];
            ++DCB->class_sz[j];
            DCB->F2sum[i] += itcnt2[i][j];
            DCB->F2sum[j] += itcnt2[i][j];
         }
      }
   }
   
   DCB->sort_ParentClass();
   
   int F2cnt=0;

   // count frequent patterns and generate eqclass
   Eqclass *eq;
   int sup;
   for (i=0; i < DCB->NumF1; ++i) 
   {
      eq = new Eqclass;
      eq->prefix().push_back(i);
      it1 = DCB->ParentClass[i]->val;
      for (j=i+1; j < DCB->NumF1; ++j) 
      {
         it2 = DCB->ParentClass[j]->val;
         if (it1 < it2) sup = itcnt2[it1][it2];
         else sup = itcnt2[it2][it1];
         if (sup >= MINSUPPORT)
         {
            ++F2cnt;
	        eq->add_node(j);
	        if (output && alg_type == eclat) 
            {
               //cout << DCB->FreqIdx[it1] << " " << DCB->FreqIdx[it2] 
                    //<< " - " << sup << endl;
               freqsets.push_back(sup);
               freqsets.push_back(DCB->FreqIdx[it1]);
               freqsets.push_back(DCB->FreqIdx[it2]);
               v_freqsets.push_back(freqsets);
               freqsets.clear();
            }
         }
      }   
      F2list->push_back(eq);
   }
   
   //remap FreqIdx, FreqMap and ParentClass vals in sorted order
   for (i=0; i < DCB->NumF1; ++i)
      DCB->FreqMap[DCB->FreqIdx[DCB->ParentClass[i]->val]] = i;

   for (i=0; i < DBASE_MAXITEM; ++i)
      if (DCB->FreqMap[i] != -1)
         DCB->FreqIdx[DCB->FreqMap[i]] = i;

   for (i=0; i < DCB->NumF1; ++i)
      DCB->ParentClass[i]->val = i;
   
   for (i=0; i < DCB->NumF1; ++i) 
      delete [] itcnt2[i];
   
   delete [] itcnt2;
   delete [] DCB->class_sz;
   delete [] DCB->F2sum;
   
   stats.add(DCB->NumF1 * DCB->NumF1, F2cnt, te);

   return F2list;
}

//performs l1 intersect l2
subset_vals get_intersect(idlist *l1, idlist *l2, idlist *join, int &idsum, 
                          int minsup=0)
{
   int diffmax1, diffmax2;
   diffmax1 = l1->size() - minsup;
   diffmax2 = l2->size() - minsup;
   
   int diffcnt1 = 0, diffcnt2 = 0;
   int n1, n2;
   unsigned int i1 = 0, i2 = 0;

   idsum = 0;
   while (i1 < l1->size() && i2 < l2->size() && 
          diffcnt1 <= diffmax1 && diffcnt2 <= diffmax2)
   {
      n1 = (*l1)[i1];
      n2 = (*l2)[i2];

      //look for matching cids
      if (n1 < n2)
      {
         ++i1;
         ++diffcnt1;
      }
      else if (n1 > n2)
      {
         ++i2;
         ++diffcnt2;
      }
      else
      {
         join->push_back(n1);
         idsum += n1;
         ++i1;
         ++i2;
      }
   }

   if (i1 < l1->size()) ++diffcnt1;
   if (i2 < l2->size()) ++diffcnt2;
   
   if (diffcnt1 == 0 && diffcnt2 == 0) return equals;
   else if (diffcnt1 == 0 && diffcnt2 > 0) return subset;
   else if (diffcnt1 > 0 && diffcnt2 == 0) return superset;
   else return notequal;   
}

//performs l1 - l2
subset_vals get_diff (idlist *l1, idlist *l2, idlist *join, 
                      int &idsum, int diffmax=INT_MAX)
{
   int n1, n2;
   int diffcnt1 = 0, diffcnt2 = 0;
   unsigned int i1 = 0, i2 = 0;
   
   idsum = 0;

   while (i1 < l1->size() && i2 < l2->size() && diffcnt1 <= diffmax)
   {
      n1 = (*l1)[i1];
      n2 = (*l2)[i2];

      if (n1 < n2)
      {
         //implies that n1 is not to be found in n2
         join->push_back(n1);
         ++diffcnt1;
         idsum += n1;
         
         ++i1;
      }
      else if (n1 > n2)
      {
         ++i2;
         ++diffcnt2;
      }
      else
      {
         ++i1;
         ++i2;
      }
   }   
   
   //add any remaining elements in l1 to join
   while (i1 < l1->size())
   {
      join->push_back((*l1)[i1]);
      idsum += (*l1)[i1];
      ++i1;
      ++diffcnt1;
   }
   
   if (i2 < l2->size()) ++diffcnt2;

   if (diffcnt1 == 0 && diffcnt2 == 0) return equals;
   else if (diffcnt1 == 0 && diffcnt2 > 0) return superset;
   else if (diffcnt1 > 0 && diffcnt2 == 0) return subset;
   else return notequal;   
}

subset_vals get_join(Eqnode *l1, Eqnode *l2, Eqnode *join, int iter)
{
   int diffmax = l1->support()-MINSUPPORT;
   int idsum;

   subset_vals sval = notequal;
 
   //compute tidset or diffset for join of l1 nd l2
   switch (diff_type)
   {
   case diff2:
      if (iter == 2) sval = get_diff(&l1->tidset, &l2->tidset, 
                                     &join->tidset, idsum, diffmax);
      else sval = get_diff(&l2->tidset, &l1->tidset, &join->tidset, 
                           idsum, diffmax);
      if (sval == subset) sval = superset;
      else if (sval == superset) sval = subset;
      join->support() = l1->support() - join->tidset.size();            
      join->hashval() = l1->hashval() - idsum;
      break;
   case nodiff:
      sval = get_intersect(&l1->tidset, &l2->tidset, 
                           &join->tidset, idsum, MINSUPPORT);
      join->support() = join->tidset.size();      
      join->hashval() = idsum;
      break;
   case diffin:
      sval = get_diff(&l2->tidset, &l1->tidset, &join->tidset, idsum, diffmax);
      if (sval == subset) sval = superset;
      else if (sval == superset) sval = subset;
      join->support() = l1->support() - join->tidset.size();            
      join->hashval() = l1->hashval() - idsum;
      break;
   case diff:
      if (iter == 2)
      {
         sval = get_intersect(&l1->tidset, &l2->tidset,
                              &join->tidset, idsum, MINSUPPORT);
         join->support() = join->tidset.size();      
         join->hashval() = idsum;
      }
      else
      {
         if (iter == 3) 
            sval = get_diff(&l1->tidset, &l2->tidset, &join->tidset, 
                            idsum, diffmax);
         else
            sval = get_diff(&l2->tidset, &l1->tidset, &join->tidset, 
                            idsum, diffmax);
         if (sval == subset) sval = superset;
         else if (sval == superset) sval = subset;
         join->support() = l1->support() - join->tidset.size();            
         join->hashval() = l1->hashval() - idsum;
      }
      break;
   }

   ++Stats::numjoin;
   return sval;
}

void get_max_join(Eqnode *l1, Eqnode *l2, Eqnode *join, int iter)
{
   int idsum;
   //find local maximal context for join
   //i.e., which maximal sets contain join as a subset
   switch(max_diff_type){
   case nodiff:
      get_intersect(&l1->maxset, &l2->maxset, &join->maxset, idsum);
      join->maxsupport() = join->maxset.size();  
      break;
   case diff2:
      if (iter == 2) get_diff(&l1->maxset, &l2->maxset, &join->maxset, idsum);
      else get_diff(&l2->maxset, &l1->maxset, &join->maxset, idsum);
      join->maxsupport() = l1->maxsupport() - join->maxset.size();
      break;
   case diffin:
      get_diff(&l2->maxset, &l1->maxset, &join->maxset, idsum);
      join->maxsupport() = l1->maxsupport() - join->maxset.size();
      break;
   case diff:
      exit(-1);
      break;
   }
}

void get_Fk(list<Eqclass *> &F2list){
   Eqclass *eq;
   idlist newmax;
   int iter = 2;
   while(!F2list.empty())
   {
      eq = F2list.front();

      switch(alg_type){
      case eclat:
         form_f2_lists(eq);
         enumerate_freq(eq, iter+1);
         break;
      case charm:
         form_closed_f2_lists(eq);
         newmax.clear();
         enumerate_closed_freq(eq, iter+1, newmax);
         break;
      case basicmax:
         form_f2_lists(eq);
         newmax.clear();
         enumerate_max_freq(eq, iter+1, newmax);
         break;
      case maxcharm:
         form_closed_f2_lists(eq);
         newmax.clear();
         enumerate_max_closed_freq(eq, iter+1, newmax);
         break;
      }      
      delete eq;
      F2list.pop_front();
   }
}                         

vector<vector<int> > get_maxfreqsets(int alnm, char * fname, int difn, double supp)
{
   vector<vector<int> >::iterator vvit;
   vector<int>::iterator vit;
   vector<int> freqsets;
   alg_type = (alg_vals) alnm;
   Dbase_Ctrl_Blk::binary_input = true;
   strcpy(infile, fname);
   diff_type = (diff_vals) difn;
   output = true;
   MINSUP_PER = supp;
   
   DCB = new Dbase_Ctrl_Blk(infile); 
   get_F1();
   list<Eqclass *> *F2list = get_F2();
   get_Fk(*F2list);

   /*for (vvit = v_freqsets.begin(); vvit != v_freqsets.end(); ++ vvit)
   {
      freqsets = *vvit;
      for (vit = freqsets.begin(); vit != freqsets.end(); ++ vit)
         cout << *vit << " ";
      cout << endl;
   }*/
   
   delete DCB;
   return v_freqsets;
}


/*int main(int argc, char **argv)
{
   vector<vector<int> >::iterator vvit;
   vector<int>::iterator vit;
   vector<int> freqsets;
   alg_type = (alg_vals) atoi(argv[1]);
   Dbase_Ctrl_Blk::binary_input = true;
   strcpy(infile, argv[2]);
   diff_type = (diff_vals) atoi(argv[3]);
   output = true;
   MINSUP_PER = atof(argv[4]);
   
   DCB = new Dbase_Ctrl_Blk(infile); 
   get_F1();
   list<Eqclass *> *F2list = get_F2();

   get_Fk(*F2list);

   for (vvit = v_freqsets.begin(); vvit != v_freqsets.end(); ++ vvit)
   {
      freqsets = *vvit;
      for (vit = freqsets.begin(); vit != freqsets.end(); ++ vit)
         cout << *vit << " ";
      cout << endl;
   }
   delete DCB;
   return 0;
}*/



