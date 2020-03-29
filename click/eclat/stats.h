#ifndef __STATS_H
#define __STATS_H

#include<iostream>
#include<vector>

using namespace std;

class iterstat{
public:
   iterstat(int nc=0, int nl=0, int nm=0, double tt=0.0, double aa=0.0):
      numcand(nc), numlarge(nl), nummax(nm), time(tt), avgtid(aa){}
   int numcand;
   int numlarge;
   int nummax;
   double time;
   double avgtid;
};

class Stats: public vector<iterstat>{
public:
   static double sumtime;
   static int sumcand;
   static int sumlarge;
   static int summax;
   static int numjoin;
   static double tottime;

   //Stats();

   void add(iterstat &is);
   void add(int cand, int freq, double time, int max=0, double avgtid=0.0);
   void incrcand(unsigned int pos, int ncand=1);
   void incrlarge(unsigned int pos, int nlarge=1);
   void incrmax(unsigned int pos, int nmax=1);
   void incrtime(unsigned int pos, double ntime);
   friend ostream& operator << (ostream& fout, Stats& stats);
};

#endif
