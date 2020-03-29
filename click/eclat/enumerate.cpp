#include<iostream>
#include <vector>

#include "eclat.h"
#include "timetrack.h"
#include "calcdb.h"
#include "eqclass.h"
#include "stats.h"
#include "maximal.h"
#include "chashtable.h"

typedef vector<bool> bit_vector;

//extern vars
extern Dbase_Ctrl_Blk *DCB;
extern Stats stats;
//add by Xiuhong Hu
extern vector<vector<int> > v_freqsets;
//end add
MaximalTest maxtest; //for maximality & closed (with cmax) testing
cHashTable hashtest; //for closed (with chash) testing

//extern functions
extern void form_closed_f2_lists(Eqclass *eq);
extern void form_f2_lists(Eqclass *eq);
extern subset_vals get_intersect(idlist *l1, idlist *l2, 
                                 idlist *join, int minsup=0);
extern subset_vals get_diff (idlist *l1, idlist *l2, idlist *join);
extern subset_vals get_join(Eqnode *l1, Eqnode *l2, Eqnode *join, int iter);
extern void get_max_join(Eqnode *l1, Eqnode *l2, Eqnode *join, int iter);


void print_tabs(int depth)
{
   for (int i=0; i < depth; ++i)
      cout << "\t";
}

static bool notfrequent (Eqnode &n){
  //cout << "IN FREQ " << n.sup << endl;
  if (n.support() >= MINSUPPORT) return false;
  else return true;
}

void enumerate_max_freq(Eqclass *eq, int iter, idlist &newmax)
{
   int nmaxpos;

   TimeTracker tt;
   Eqclass *neq;
   list<Eqnode *>::iterator ni, nj;
   Eqnode *join;
   vector<int> freq_sets;
   vector<int>::iterator vit;

   bool extend = false;
   bool subset = false;
   
   eq->sort_nodes();
   nmaxpos = newmax.size(); //initial newmax pos

   //if ni is a subset of maxset then all elements after ni must be
   for (ni = eq->nlist().begin(); ni != eq->nlist().end() && !subset; ++ni){
      tt.Start();

      neq = new Eqclass;
      neq->set_prefix(eq->prefix(),*(*ni));

      subset = false;
      extend = false;
      bool res = maxtest.update_maxset(ni, eq, newmax, nmaxpos, subset);
      if (!subset || res){
            subset = maxtest.subset_test(ni, eq);
      }
      
      nmaxpos = newmax.size();

      if (!subset){
         nj = ni; ++nj;
         for (; nj != eq->nlist().end(); ++nj){ 
            join = new Eqnode ((*nj)->val);
            
            get_join(*ni, *nj, join, iter);
            stats.incrcand(iter-1);
            if (notfrequent(*join)) delete join;
            else{
               extend = true;
               get_max_join(*ni, *nj, join, iter);
               neq->add_node(join);
               stats.incrlarge(iter-1);
            }
         }
      
         stats.incrtime(iter-1, tt.Stop());
         if (!neq->nlist().empty())
            enumerate_max_freq(neq, iter+1, newmax);
      }
      if (!extend && (*ni)->maxsupport() == 0){
         if (output)
         {
	   //neq->print_prefix() << " - " << (*ni)->support() << endl;
            freq_sets = neq->get_nodes();
            vit = freq_sets.begin();
            freq_sets.insert(vit, (*ni)->support());
            v_freqsets.push_back(freq_sets);
            freq_sets.clear();
         }              
    
         maxtest.add(eq->prefix(), (*ni)->val, (*ni)->support());
         newmax.push_back(maxtest.maxcount-1);
         stats.incrmax((iter-2));
      }
      delete neq;
   }
}

void enumerate_max_closed_freq(Eqclass *eq, int iter, idlist &newmax)
{
   int nmaxpos;

   TimeTracker tt;
   Eqclass *neq;
   list<Eqnode *>::iterator ni, nj;
   Eqnode *join;
   subset_vals sval;

   bool extend = false;
   bool subsetflg = false;
   
   eq->sort_nodes();

   //cout << "CMAX " << *eq;
   
   nmaxpos = newmax.size(); //initial newmax pos

   //if ni is a subset of maxset then all elements after ni must be
   for (ni = eq->nlist().begin(); ni != eq->nlist().end() && !subsetflg; ++ni){
      tt.Start();

      neq = new Eqclass;
      neq->set_prefix(eq->prefix(),*(*ni));

      subsetflg = false;
      extend = false;
      bool res = maxtest.update_maxset(ni, eq, newmax, nmaxpos, subsetflg);
      if (!subsetflg || res){
         subsetflg = maxtest.subset_test(ni, eq);
      }
      
      nmaxpos = newmax.size();

      if (!subsetflg){
         nj = ni; ++nj;
         for (; nj != eq->nlist().end();){ 
            join = new Eqnode ((*nj)->val);
            
            sval = get_join(*ni, *nj, join, iter);
            //cout << "SVAL " << (int)sval;
            //eq->print_node(*(*nj));
            //cout << endl;
            //cout << "ISECT " << *join;
            stats.incrcand(iter-1);
            if (notfrequent(*join)){
               delete join;
               ++nj;
            }
            else{
               extend = true;
               //get_max_join(*ni, *nj, join, iter);
               //neq->add_node(join);
               stats.incrlarge(iter-1);

               switch(sval){
               case subset:
                  //add nj to all elements in eq by adding nj to prefix
                  neq->prefix().push_back((*nj)->val);               
                  //neq->closedsupport() = join->support();
                  delete join;
                  ++nj;
                  break;
               case notequal:
                  get_max_join(*ni, *nj, join, iter);
                  neq->add_node(join);               
                  ++nj;
                  break;
               case equals:
                  //add nj to all elements in eq by adding nj to prefix
                  neq->prefix().push_back((*nj)->val); 
                  //neq->closedsupport() = join->support();
                  delete *nj;
                  nj = eq->nlist().erase(nj); //remove nj
                  delete join;
                  break;
               case superset:
                  get_max_join(*ni, *nj, join, iter);
                  delete *nj;
                  nj = eq->nlist().erase(nj); //remove nj
                  //++nj;
                  neq->add_node(join);            
                  break;
               }
            }
         }
      
         stats.incrtime(iter-1, tt.Stop());
         if (neq->nlist().size() > 1){
            enumerate_max_closed_freq(neq, iter+1, newmax);
         }
         else if (neq->nlist().size() == 1){
            nj = neq->nlist().begin();
            if ((*nj)->maxsupport() == 0){
               if (output) 
                  neq->print_node(*(*nj)) << endl;
               maxtest.add(neq->prefix(), (*nj)->val);
               newmax.push_back(maxtest.maxcount-1);
               stats.incrmax(neq->prefix().size());
            }
         }
         else if (extend && (*ni)->maxsupport() == 0){
            if (output)
               neq->print_prefix() << endl;
            maxtest.add(neq->prefix(), -1);
            newmax.push_back(maxtest.maxcount-1);
            stats.incrmax(neq->prefix().size()-1);
         }
      }
      
      if (!extend && (*ni)->maxsupport() == 0){
         if (output)
            neq->print_prefix() << endl;
         maxtest.add(eq->prefix(), (*ni)->val);
         newmax.push_back(maxtest.maxcount-1);
         stats.incrmax(neq->prefix().size()-1);
      }

      delete neq;
   }
}

void enumerate_closed_freq(Eqclass *eq, int iter, idlist &newmax)
{
   TimeTracker tt;
   Eqclass *neq;
   int nmaxpos;
   bool cflg;
   list<Eqnode *>::iterator ni, nj;
   Eqnode *join;
   subset_vals sval;

   nmaxpos = newmax.size(); //initial newmax pos
   eq->sort_nodes();
   //print_tabs(iter-3);
   //cout << "F" << iter << " " << *eq;
   for (ni = eq->nlist().begin(); ni != eq->nlist().end(); ++ni){
      neq = new Eqclass;
      neq->set_prefix(eq->prefix(),*(*ni));

      //cout << "prefix " << neq->print_prefix() << endl;
      tt.Start();

      if (closed_type == cmax) 
         maxtest.update_maxset(ni, eq, newmax, nmaxpos, cflg);

      nmaxpos = newmax.size(); //initial newmax pos
      nj = ni;
      for (++nj; nj != eq->nlist().end(); ){
         join = new Eqnode ((*nj)->val);
         sval = get_join(*ni, *nj, join, iter);
         stats.incrcand(iter-1);
         if (notfrequent(*join)){
            delete join;
            ++nj;
         }
         else{
            stats.incrlarge(iter-1);
            switch(sval){
            case subset:
               //add nj to all elements in eq by adding nj to prefix
               neq->prefix().push_back((*nj)->val);               
               //neq->closedsupport() = join->support();
               //cout << "SUSET " << *join << endl;
               delete join;
               ++nj;
               break;
            case notequal:
               if (closed_type == cmax) get_max_join(*ni, *nj, join, iter);
               neq->add_node(join);               
               ++nj;
               break;
            case equals:
               //add nj to all elements in eq by adding nj to prefix
               neq->prefix().push_back((*nj)->val); 
               //neq->closedsupport() = join->support();
               delete *nj;
               nj = eq->nlist().erase(nj); //remove nj
               //cout << "EQUAL " << *join << endl;
               delete join;
               break;
            case superset:
               if (closed_type == cmax) get_max_join(*ni, *nj, join, iter);
               delete *nj;
               nj = eq->nlist().erase(nj); //remove nj
               //++nj;
               neq->add_node(join);            
               break;
            }
         }
      }
      
      cflg = true;
      if (closed_type == cmax){
         cflg = maxtest.check_closed(*ni);
         if (cflg){
            maxtest.add(neq->prefix(), -1, (*ni)->support());
            newmax.push_back(maxtest.maxcount-1);
         }
      }
      else if (closed_type == chash){
         cflg = hashtest.add(neq->prefix(), -1, (*ni)->support(), 
                             (*ni)->hval);
      }

      if (cflg){
         stats.incrmax(neq->prefix().size()-1);
         if (output){
            cout << "CLOSEDxy ";
            neq->print_prefix(true);
            cout << endl;
         }
      }
      
      stats.incrtime(iter-1, tt.Stop());
      //if (output) cout << *neq;
      if (neq->nlist().size() > 1){
         enumerate_closed_freq(neq, iter+1, newmax);
      }
      else if (neq->nlist().size() == 1){
         cflg = true;
         if (closed_type == cmax){
            cflg = maxtest.check_closed(neq->nlist().front());
            if (cflg){
               maxtest.add(neq->prefix(), neq->nlist().front()->val, 
                           neq->nlist().front()->sup);
               newmax.push_back(maxtest.maxcount-1);
            }
         }
         else if (closed_type == chash){
            cflg = hashtest.add(neq->prefix(), neq->nlist().front()->val, 
                                neq->nlist().front()->sup, 
                                neq->nlist().front()->hval);
         }
         
         if (cflg){
            if (output) cout << "CLOSEDy " << *neq;
            stats.incrmax(neq->prefix().size());
         }
      }
      delete neq;
   }
}

void enumerate_freq(Eqclass *eq, int iter)
{
   TimeTracker tt;
   Eqclass *neq;
   list<Eqnode *>::iterator ni, nj;
   Eqnode *join;

   for (ni = eq->nlist().begin(); ni != eq->nlist().end(); ++ni){
      neq = new Eqclass;
      neq->set_prefix(eq->prefix(),*(*ni));
      tt.Start();
      nj = ni;
      for (++nj; nj != eq->nlist().end(); ++nj){ 
         join = new Eqnode ((*nj)->val);
         get_join(*ni, *nj, join, iter);
         stats.incrcand(iter-1);
         if (notfrequent(*join)) delete join;
         else{
            neq->add_node(join);
            stats.incrlarge(iter-1);
         }
      }
      stats.incrtime(iter-1, tt.Stop());
      if (output) cout << *neq;
      if (neq->nlist().size()> 1){
         enumerate_freq(neq, iter+1);
      }
      delete neq;
   }
}

void form_closed_f2_lists(Eqclass *eq)
{
   static bit_vector *bvec = NULL;
   if (bvec == NULL){
      bvec = new bit_vector(DCB->NumF1, true);
   }
   
   subset_vals sval;
   list<Eqnode *>::iterator ni;
   Eqnode *l1, *l2;
   int pit, nit;
   TimeTracker tt;
   bool extend = false;
   bool cflg;
   
   tt.Start();
   pit = eq->prefix()[0];
   l1 = DCB->ParentClass[pit];

   if (!(*bvec)[pit]){
      eq->nlist().clear();
     return;
   }
   
   if (alg_type == maxcharm){
      cflg = maxtest.subset_test_f2(eq);
      if (cflg){
         eq->nlist().clear();
         return;
      }
   }
   
   for (ni=eq->nlist().begin(); ni != eq->nlist().end(); ){
      nit = (*ni)->val;
      if (!(*bvec)[nit]){
         ++ni;
         continue;
      }
      l2 = DCB->ParentClass[nit];
      sval = get_join(l1, l2, (*ni), 2);
      //cout << "SVAL " << (int)sval << endl;
      switch(sval){
      case subset:
         //add nj to all elements in eq by adding nj to prefix
         eq->prefix().push_back((*ni)->val);               
         extend = true;
         //eq->closedsupport() = (*ni)->support();
         delete *ni;
         ni = eq->nlist().erase(ni); //remove ni
         //cout << "CAME HERE " << eq->nlist().size() << endl;
         break;
      case notequal:
         if (alg_type == maxcharm || closed_type == cmax) 
            get_max_join(l1, l2, (*ni), 2);
         ++ni;
         break;
      case equals:
         //add nj to all elements in eq by adding nj to prefix
         eq->prefix().push_back((*ni)->val); 
         extend = true;
         //eq->closedsupport() = (*ni)->support();
         delete *ni;
         ni = eq->nlist().erase(ni); //remove ni
         (*bvec)[nit] = false;
         //cout << "CAME HERE " << eq->nlist().size() << endl;
         break;
      case superset:
         (*bvec)[nit] = false;
         if (alg_type == maxcharm || closed_type == cmax) 
            get_max_join(l1, l2, (*ni), 2);
         ++ni;
         break;
      }
   }
   
   if (alg_type == charm){

      cflg = true;
      if (closed_type == cmax){
         cflg = maxtest.check_closed(l1);
         if (cflg) maxtest.add(eq->prefix(), -1, l1->support());
      }
      else if (closed_type == chash){
         cflg = hashtest.add(eq->prefix(), -1, l1->support(), 
                             l1->hval);
      }

      if (cflg){
         stats.incrmax(eq->prefix().size()-1);
         if (output){
            cout << "CLOSEDzx ";
            eq->print_prefix(true);
            cout << endl;
         }
      }
      
      
      if (eq->nlist().size() == 1){
         cflg = true;
         if (closed_type == cmax){
            cflg = maxtest.check_closed(eq->nlist().front());
            if (cflg){
               maxtest.add(eq->prefix(), eq->nlist().front()->val, 
                           eq->nlist().front()->sup);
            }
         }
         else if (closed_type == chash){
            cflg = hashtest.add(eq->prefix(), eq->nlist().front()->val, 
                                eq->nlist().front()->sup, 
                                eq->nlist().front()->hval);
         }

         if (cflg){
            if (output){
               cout << "CLOSEDz ";
               cout << *eq;
            }
            
            stats.incrmax(eq->prefix().size());
         }
         eq->nlist().clear();
      }
   }
   else if (alg_type == maxcharm){
      if (eq->nlist().empty() && l1->maxsupport() == 0){
         if (output)
            eq->print_prefix() << endl;
         maxtest.add(eq->prefix(), -1);
         stats.incrmax(eq->prefix().size()-1);
      }
      else if (eq->nlist().size() == 1){
         l1 = eq->nlist().front();
         if (l1->maxsupport() == 0){
            if (output)
               eq->print_node(*l1) << endl;
            maxtest.add(eq->prefix(), l1->val);
            stats.incrmax(eq->prefix().size()); 
         }
         eq->nlist().clear();
      }
   }

   //cout << "F2 " << *eq << endl;
   stats.incrtime(1,tt.Stop());   
   //delete l1;
}



void form_f2_lists(Eqclass *eq)
{
   list<Eqnode *>::iterator ni;
   Eqnode *l1, *l2;
   int pit, nit;
   TimeTracker tt;
   
   tt.Start();
   pit = eq->prefix()[0];
   l1 = DCB->ParentClass[pit];
   for (ni=eq->nlist().begin(); ni != eq->nlist().end(); ++ni){
      nit = (*ni)->val;
      l2 = DCB->ParentClass[nit];

      get_join(l1, l2, (*ni), 2);
      if (alg_type == basicmax) 
         get_max_join(l1, l2, (*ni), 2);
   }
   stats.incrtime(1,tt.Stop());   
}


