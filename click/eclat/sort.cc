#include <errno.h>
#include <iostream.h>
#include <malloc.h>
#include <stdio.h>
#include <fstream.h>
#include <strstream.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

char freqf[300];
const int lineSize=1024;
const int wdSize=256;

class Itemset{
private:
   int *iary;
   int sz;
   int sup;
public:
   Itemset(int sze, int sp){ sz = sze; sup = sp; iary = new int[sz]; }
   ~Itemset(){ delete [] iary; }
   int size(){ return sz; }
   int *iset(){ return iary; }
   int& operator [] (unsigned int index){ return iary[index]; }   
   friend ostream& operator << (ostream& fout, Itemset& iset);
};

ostream& operator << (ostream& fout, Itemset& iset){
   for (int i=0; i < iset.sz; i++)
      fout << iset.iary[i] << " ";
   fout << "- " << iset.sup << endl;
   return fout;
}

Itemset **isetary;
int isettotsz=1000;
int isetarysz=0;

int cmpiset(const void *a, const void *b)
{
   Itemset *ia = *(Itemset **)a;
   Itemset *ib = *(Itemset **)b;

   if (ia->size() > ib->size()) return 1;
   else if (ia->size() < ib->size()) return -1;
   
   for (int i=0; i < ia->size() && i < ib->size(); i++){
      if ((*ia)[i] > (*ib)[i]) return 1;
      else if ((*ia)[i] < (*ib)[i]) return -1;
   }


   //   if (ia->size() > ib->size()) return 1;
   //else if (ia->size() < ib->size()) return -1;

   return 0;
}

int cmpint(const void *a, const void *b)
{
   int ia = *(int *)a;
   int ib = *(int *)b;

   if (ia > ib) return 1;
   else if (ia < ib) return -1;
   else return 0;
}
 

void getitemset(char *inBuf, int inSize)
{
   char inStr[wdSize];
   static int tmpary[1000];
   int tmpsz;
   
   int it, sup;
   istrstream ist(inBuf, inSize);

   if (inBuf[0] != '0' && 
       inBuf[0] != '1' &&
       inBuf[0] != '2' &&
       inBuf[0] != '3' &&
       inBuf[0] != '4' &&
       inBuf[0] != '5' &&
       inBuf[0] != '6' &&
       inBuf[0] != '7' &&
       inBuf[0] != '8' &&
       inBuf[0] != '9'){
      cout << inBuf << endl;
      return;
   }
   
   
   // it is a digit, i.e., start of itemset
   tmpsz = 0;
   while(ist >> inStr){
      if (strcmp(inStr, "-") == 0){
         ist >> inStr;
         sup = atoi(inStr);
      }
      else{
         it = atoi(inStr);
         tmpary[tmpsz++] = it;
      }
      
      
      //it = atoi(inStr);
      //cout << it << endl;
      //tmpary[tmpsz++] = it;
   }
   
   Itemset *fit = new Itemset(tmpsz,sup);
   for (int i=0; i < tmpsz; i++)
      (*fit)[i] = tmpary[i];
   //cout << "BEFORE " << *fit;
   qsort(fit->iset(), fit->size(), sizeof(int), cmpint);
   cout << *fit;
   if (isetarysz+1 > isettotsz){
      isettotsz = 2*isettotsz;
      isetary = (Itemset **) realloc(isetary, isettotsz*sizeof(Itemset *));
   }
   isetary[isetarysz++] = fit;
}

int main(int argc, char **argv)
{
   int i;
   
   sprintf(freqf,"%s", argv[1]);
   
   isetary = (Itemset **) malloc (sizeof(Itemset*)*isettotsz);
   
   char inBuf[lineSize];
   int inSize;
   
   ifstream fin(freqf, ios::in);
   if (!fin){
      perror("cannot open freq seq file");
      exit(errno);
   }

   while(fin.getline(inBuf, lineSize)){
      inSize = fin.gcount();
      getitemset(inBuf, inSize);
   }
   
   cerr << "\n\n<<<<<<<<<<<<<< COMPLETE SORT <<<<<<<<<<<<<<<<<<<<<<<<<<\n";
   isetary = (Itemset **) realloc(isetary, isetarysz*sizeof(Itemset *));
   
   qsort(isetary, isetarysz, sizeof(Itemset *), cmpiset);
   
   for (i=0; i < isetarysz; i++)
      cerr << *isetary[i];
}

