#ifdef __GNUC__
#include <unistd.h>
#endif

#include<iostream>
#include <stdio.h>
#include <list>

//headers
#include "eclat.h"
#include "timetrack.h"
#include "calcdb.h"
#include "eqclass.h"
#include "stats.h"
#include "chashtable.h"

//global vars
char infile[300];
Dbase_Ctrl_Blk *DCB;
Stats stats;

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

void parse_args(int argc, char **argv)
{
   extern char * optarg;
   int c;

   if (argc < 2){
      cout << "usage: eclat\n";
      cout << "\t-i<infile>\n";
      cout << "\t-s<support>\n";
      cout << "\t-S<absolute support> (specify -s or -S)\n";
      cout << "\t-a<algtype> (0=eclat, 1=charm, 2=basicmax, 3=maxcharm)\n";
      cout << "\t-c<closedtype> (0=cnone, 1=chash, 2=cmax)\n";
      cout << "\t-d<difftype> (0=nodiff, 1=diff, 2=diff2, 3=diffin)\n";
      cout << "\t-m<maxdifftype> (0=nodiff, 1=diff, 2=diff2, 3=diffin)\n";
      cout << "\t-p<prunetype> (0=noprune, 1=prune)\n";
      cout << "\t-z<sorttype> (0=nosort, 1=incr, 2=decr, 3=incr_noclass)\n";
      cout << "\t-o (print patterns?)\n";
      cout << "\t-l (output + output_tidlist?)\n";
      cout << "\t-b (binary input file?) (default=false)\n";
      exit(-1);
   }
   else{
     while ((c=getopt(argc,argv,"a:bc:d:i:lm:op:s:S:z:"))!=-1){
       switch(c){
       case 'a':
          alg_type = (alg_vals) atoi(optarg);
          break;
       case 'b':
          Dbase_Ctrl_Blk::binary_input = true;
          break;
       case 'c':
          closed_type = (closed_vals) atoi(optarg);
          break;
       case 'd':
          diff_type = (diff_vals) atoi(optarg);
          break;
       case 'i': //input files
	 sprintf(infile,"%s",optarg);
	 break;
       case 'l': //print idlists along with freq subtrees
          output=true;
          output_idlist = true;
          break;
       case 'm':
          max_diff_type = (diff_vals) atoi(optarg);
          break;
       case 'o': //print freq subtrees
	 output = true;
	 break;
       case 'p':
          prune_type = (prune_vals) atoi(optarg);
          break;
        case 's': //support value for L2
	 MINSUP_PER = atof(optarg);
	 break;
       case 'S': //absolute support
	 MINSUPPORT = atoi(optarg);
	 break;
       case 'z':
          sort_type = (sort_vals) atoi(optarg);
          break;
       }               
     }
   }
   //cout << "ENUM VALS " << alg_type << " " << diff_type << " " 
   //     << max_diff_type << " " << closed_type << " " << prune_type << endl;
}


ostream & operator<<(ostream& fout, idlist &vec){
  for (unsigned int i=0; i < vec.size(); ++i)
     fout << vec[i] << " ";
  return fout;
}


void get_F1()
{
  TimeTracker tt;
  double te;

  int i, j, it;
  const int arysz = 10;
  
  vector<int> itcnt(arysz,0); //count item frequency

  tt.Start();

  DBASE_MAXITEM=0;
  DBASE_NUM_TRANS = 0;
  
  DCB->Cidsum = 0;
  
   while(DCB->get_next_trans())
   {
      for (i=0; i < DCB->TransSz; ++i){
         it = DCB->TransAry[i];

         if (it >= DBASE_MAXITEM){
            itcnt.resize(it+1,0);
            DBASE_MAXITEM = it+1;
            //cout << "IT " << DBASE_MAXITEM << endl;
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
   for (i=0,j=0; i < DBASE_MAXITEM; ++i) {
      if (itcnt[i] >= MINSUPPORT) {
         if (output && alg_type == eclat) 
	   // cout << i << " - " << itcnt[i] << endl;
         DCB->FreqIdx[j] = i;
         DCB->FreqMap[i] = j;
         ++j;
      }
      else DCB->FreqMap[i] = -1;
   }
   
   //cout<< "F1 - " << DCB->NumF1 << " " << DBASE_MAXITEM << endl;  

   DCB->alloc_ParentClass(itcnt);
   
   te = tt.Stop();
   stats.add(DBASE_MAXITEM, DCB->NumF1, te);

}

list<Eqclass *> * get_F2()
{
  int i,j;
  int it1, it2;
  
  TimeTracker tt;
  double te;

  tt.Start();

  list<Eqclass *> *F2list = new list<Eqclass *>;

  //itcnt2 is a matrix of pairs p, p.first is count, p.second is flag
  int **itcnt2 = new int*[DCB->NumF1];

  //unsigned int **itcnt2 = new unsigned int *[DCB->NumF1];
  for (i=0; i < DCB->NumF1; ++i){
    itcnt2[i] = new int [DCB->NumF1];
    //cout << "alloc " << i << " " << itcnt2[i] << endl;
    for (j=0; j < DCB->NumF1; ++j){
      itcnt2[i][j] = 0;
    }
  }
    
   while(DCB->get_next_trans())
   {
      DCB->get_valid_trans();
      DCB->make_vertical();
      //DCB->print_trans();
      //count a pair only once per cid
      for (i=0; i < DCB->TransSz; ++i){
         it1 = DCB->TransAry[i];
         for (j=i+1; j < DCB->TransSz; ++j){
            it2 = DCB->TransAry[j];
            ++itcnt2[it1][it2];
         }
      }
   }                           

   //compute class size
   DCB->class_sz = new int[DCB->NumF1];
   DCB->F2sum = new int[DCB->NumF1];
   for (i=0; i < DCB->NumF1; ++i){
      DCB->class_sz[i] = 0;
      DCB->F2sum[i] = 0;
   }
   
   for (i=0; i < DCB->NumF1; ++i){
      for (j=i+1; j < DCB->NumF1; ++j){
         if (itcnt2[i][j] >= MINSUPPORT){
//              cout << "TEST " << DCB->FreqIdx[i] << " " << DCB->FreqIdx[j] 
//                   << " - " << itcnt2[i][j] << endl;
            ++DCB->class_sz[i];
            ++DCB->class_sz[j];
            DCB->F2sum[i] += itcnt2[i][j];
            DCB->F2sum[j] += itcnt2[i][j];
         }
      }
   }
   
   DCB->sort_ParentClass();
   
   //cout << "sorted order:";
   //for (i=0; i < DCB->NumF1; ++i){
   //   cout << " " << DCB->FreqIdx[DCB->ParentClass[i]->val];
   //}
   //cout << endl;
   //     for (i=0; i < DCB->NumF1; ++i)
   //        cout << " " << DCB->class_sz[DCB->ParentClass[i]->val];
   //     cout << endl;
   //     for (i=0; i < DCB->NumF1; ++i)
   //        cout << " " << DCB->F2sum[DCB->ParentClass[i]->val];
   //     cout << endl;
   
   //        cout << "CLASS SORT " << i << " " 
   //             << DCB->ParentClass[i]->val << " " 
   //             << DCB->FreqIdx[DCB->ParentClass[i]->val] << " "
   //             << DCB->ParentClass[i]->sup << endl;
   
   int F2cnt=0;

   // count frequent patterns and generate eqclass
   Eqclass *eq;
   int sup;
   for (i=0; i < DCB->NumF1; ++i) {
      eq = new Eqclass;
      eq->prefix().push_back(i);
      //if (alg_type == charm) 
      //   eq->closedsupport() = DCB->ParentClass[i]->support();
      it1 = DCB->ParentClass[i]->val;
      for (j=i+1; j < DCB->NumF1; ++j) {
	//cout << "access " << i << " " << j << endl;
         it2 = DCB->ParentClass[j]->val;
         if (it1 < it2) sup = itcnt2[it1][it2];
         else sup = itcnt2[it2][it1];
         if (sup >= MINSUPPORT){
            ++F2cnt;
	    eq->add_node(j);
	    if (output && alg_type == eclat) 
	      // cout << DCB->FreqIdx[it1] << " " << DCB->FreqIdx[it2] 
              //      << " - " << sup << endl;
         }
      }   
      F2list->push_back(eq);
   }
   
   //remap FreqIdx, FreqMap and ParentClass vals in sorted order
   //cout << "FREQMAP :\n";
   for (i=0; i < DCB->NumF1; ++i){
      DCB->FreqMap[DCB->FreqIdx[DCB->ParentClass[i]->val]] = i;
      ///cout << i << " " << DCB->ParentClass[i]->val << " " 
      //     << DCB->FreqIdx[DCB->ParentClass[i]->val] 
      //     << " " << DCB->FreqMap[DCB->FreqIdx[DCB->ParentClass[i]->val]] 
      //     << endl;
   }

   for (i=0; i < DBASE_MAXITEM; ++i)
      if (DCB->FreqMap[i] != -1){
         DCB->FreqIdx[DCB->FreqMap[i]] = i;
         //cout << i << " " << DCB->FreqMap[i] 
         //     << " " << DCB->FreqIdx[DCB->FreqMap[i]] << endl;
      }


   //cout << "ORDER :";
   for (i=0; i < DCB->NumF1; ++i){
      DCB->ParentClass[i]->val = i;
      //cout << " " << DCB->FreqIdx[i];
   }
   //cout << endl;
   
   for (i=0; i < DCB->NumF1; ++i) {
      delete [] itcnt2[i];
   }
   delete [] itcnt2;
   delete [] DCB->class_sz;
   delete [] DCB->F2sum;
   
   //cout << "F2 - " << F2cnt << " " << DCB->NumF1 * DCB->NumF1 << endl;
   te = tt.Stop();
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
          diffcnt1 <= diffmax1 && diffcnt2 <= diffmax2){
      n1 = (*l1)[i1];
      n2 = (*l2)[i2];

      //look for matching cids
      if (n1 < n2){
         ++i1;
         ++diffcnt1;
      }
      else if (n1 > n2){
         ++i2;
         ++diffcnt2;
      }
      else{
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
//     insert_iterator<idlist> differ(*join,join->begin());
//     set_difference(l1->begin(), l1->end(), 
//                      l2->begin(), l2->end(), 
//                      differ);
   int n1, n2;
   int diffcnt1 = 0, diffcnt2 = 0;
   unsigned int i1 = 0, i2 = 0;
   
   idsum = 0;

   while (i1 < l1->size() && i2 < l2->size() && diffcnt1 <= diffmax){
      n1 = (*l1)[i1];
      n2 = (*l2)[i2];

      if (n1 < n2){
         //implies that n1 is not to be found in n2
         join->push_back(n1);
         ++diffcnt1;
         idsum += n1;
         
         ++i1;
      }
      else if (n1 > n2){
         ++i2;
         ++diffcnt2;
      }
      else{
         ++i1;
         ++i2;
      }
   }   
   
   //add any remaining elements in l1 to join
   while (i1 < l1->size()){
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
   switch (diff_type){
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
      if (iter == 2){
         sval = get_intersect(&l1->tidset, &l2->tidset,
                              &join->tidset, idsum, MINSUPPORT);
         join->support() = join->tidset.size();      
         join->hashval() = idsum;
      }
      else{
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
      cout << "diff NOT HANDLED\n";
      exit(-1);
      break;
   }
}

void get_Fk(list<Eqclass *> &F2list){
   Eqclass *eq;
   idlist newmax;
   int iter = 2;
   while(!F2list.empty()){
      eq = F2list.front();

      //cout << "OUTSIDE " << *eq << "\\\\\\\\\\\\\n";
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

int main(int argc, char **argv)
{
   TimeTracker tt;
   tt.Start(); 
   parse_args(argc, argv); 

   ofstream summary("summary.out", ios::app);

   switch(alg_type){
   case basicmax: summary << "MAX "; break;
   case charm: 
      summary << "CHARM "; 
      switch(closed_type){
      case cnone: break;
      case chash: summary << "CHASH "; break;
      case cmax: summary << "CMAX "; break;
      }
      break;
   case maxcharm: summary << "MAXCHARM "; break;
   case eclat: summary << "ECLAT "; break;
   }
   switch(diff_type){
   case nodiff: summary << "NODIFF "; break;
   case diff: summary << "DIFF "; break;
   case diff2: summary << "DIFF2 "; break;
   case diffin: summary << "DIFFIN "; break;
   } 
   switch(max_diff_type){
   case nodiff: summary << "MNODIFF "; break;
   case diff: summary << "MDIFF "; break;
   case diff2: summary << "MDIFF2 "; break;
   case diffin: summary << "MDIFFIN "; break;
   }
   switch(sort_type){
   case nosort: summary << "NOSORT "; break;
   case incr: summary << "INCR "; break;
   case incr_noclass: summary << "INCRNOC "; break;
   case decr: summary << "DECR "; break;
   }
   switch(prune_type){
   case prune: summary << "PRUNE "; break;
   case noprune: break;
   }
  
   DCB = new Dbase_Ctrl_Blk(infile); 
   get_F1();
   list<Eqclass *> *F2list = get_F2();

   //DCB->print_vertical();
   get_Fk(*F2list);

   for (unsigned int i=0; i < stats.size(); ++i){
     //cout << "F" << i+1 << " - ";
     //cout << stats[i].numlarge << " " << stats[i].numcand 
     //    << " " << stats[i].nummax << endl;
   }
   
   double tottime = tt.Stop();
   stats.tottime = tottime;

   summary << infile << " " << MINSUP_PER << " "
           << DBASE_NUM_TRANS << " " << MINSUPPORT << " ";

   summary << stats << endl;
   summary.close();
   
   //cout << "TIME = " << tottime << endl;
   //cout << "NUMMAX = " << stats.summax << endl;
   if (closed_type == chash) hashtest.print_hashstats();
   exit(0);
}



