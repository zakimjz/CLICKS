#ifndef __DATABASE_H
#define __DATABASE_H

#include <iostream>
#include <fstream>
#include <vector>
#include "eclat.h"
#include "eqclass.h"

#define ITSZ sizeof(int)
#define DCBBUFSZ 2048
#define TRANSOFF 3

class Dbase_Ctrl_Blk{
private:

  //vars related to the horizontal format
   ifstream fd;
   int buf_size;
   int * buf;
   int cur_blk_size;
   int cur_buf_pos;
   int endpos;
   char readall;
   static int *PvtTransAry;

public:
   static int NumF1;   //number of freq items
   static int *FreqMap; //mapping of freq items, i.e., item to freq_idx
   static int *FreqIdx; //freq_idx to original item value
   static bool binary_input;
   //vars related to the horizontal format
   static int *TransAry;
   static int TransSz;
   static int Tid;
   static int Cid;
   static int MaxTransSz;

   static int Cidsum; //used for closed sets

   //vars related to vertical format
   static vector<Eqnode *> ParentClass;
   static int *class_sz;
   static int *F2sum;

   //function definitions
   Dbase_Ctrl_Blk(const char *infile, const int buf_sz=DCBBUFSZ);
   ~Dbase_Ctrl_Blk();

   //functions for horizontal format
   void get_next_trans_ext();
   void get_first_blk();
   int get_next_trans();
   void get_valid_trans();
   void print_trans();
   int eof(){return (readall == 1);}

   //functions for vertical format
   void make_vertical();
   void print_vertical();
   void alloc_ParentClass(vector<int> &itcnt);
   void sort_ParentClass();
   static bool incr_cmp(Eqnode *n1, Eqnode *n2);
   static bool decr_cmp(Eqnode *n1, Eqnode *n2);

};

#endif //__DATABASE_H





