#include "stats.h"

//static initializations
double Stats::tottime = 0;
double Stats::sumtime = 0;
int Stats::sumcand = 0;
int Stats::sumlarge = 0;
int Stats::summax = 0;
int Stats::numjoin = 0;

//function defs

//Stats::Stats(): vector<iterstat *>(){}

void Stats::add(iterstat &is){
   push_back(is);
   sumtime += is.time;
   sumcand += is.numcand;
   sumlarge += is.numlarge;
   summax += is.nummax;
}

void Stats::add(int cand, int freq, double time, int max, double avgtid){
  iterstat *is = new iterstat(cand, freq, max, time, avgtid);
  push_back(*is);
  sumtime += is->time;
  sumcand += is->numcand;
  sumlarge += is->numlarge;
  summax += is->nummax;
}

void Stats::incrcand(unsigned int pos, int ncand)
{
   if (pos >= size()) resize(pos+1);
   (*this)[pos].numcand += ncand;
   sumcand += ncand;
}

void Stats::incrlarge(unsigned int pos, int nlarge)
{
   if (pos >= size()) resize(pos+1);
   (*this)[pos].numlarge += nlarge;
   sumlarge += nlarge;
}

void Stats::incrmax(unsigned int pos, int nmax)
{
   if (pos >= size()) resize(pos+1);
   (*this)[pos].nummax += nmax;
   summax += nmax;
}

void Stats::incrtime(unsigned int pos, double ntime)
{
   if (pos >= size()) resize(pos+1);
   (*this)[pos].time += ntime;
   sumtime += ntime;
}


ostream& operator << (ostream& fout, Stats& stats){
  //fout << "SIZE " << stats.size() << endl;
  for (unsigned int i=0; i<stats.size(); ++i){
    fout << "[ " << i+1 << " " << stats[i].numcand << " "
	 << stats[i].numlarge << " " << stats[i].nummax << " "
         << stats[i].time << " " << stats[i].avgtid << " ] ";
  }
  fout << "[ TOT " << stats.sumcand << " " << stats.sumlarge << " "
       << stats.summax << " " << stats.sumtime << " ] ";
  fout << stats.tottime << " " << stats.numjoin;
  return fout;
}
