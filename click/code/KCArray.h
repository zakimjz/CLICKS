#ifndef _KCARRAY_H_
#define _KCARRAY_H_

#include <iostream>
#include <assert.h>
#include <cstring>

using namespace std;

template<class T>
class KCSymArray
{
protected:
  int dim, elements;
  T*  array;
  
  int index(int i, int j){
    if(i*(dim-1) + (i - (i*i))/2 + j < 0 ||
       i*(dim-1) + (i - (i*i))/2 + j >= elements){
      cout << "Error accessing " << i << ", " << j << "(" << i*(dim-1) + (i - (i*i))/2 + j << ")" << endl;
    }

    return i*(dim-1) + (i - (i*i))/2 + j;
  }

  int size(){ 
    return elements; 
  }

public:
  KCSymArray(){
    dim = 0;
    array = NULL;
  }

  KCSymArray(int d){
    createArray(d);
  }
  
  ~KCSymArray(){
    if(array != NULL)
      delete[] array;
  }

  void createArray(int d){
    if(array)
      delete[] array;

    dim = d;
    elements = (d * (d+1)) / 2;
    array = new T[elements];
    assert(array != NULL);    
  }

  void clear(){
    memset((void*)array, 0, size()*sizeof(T));
  }

  int getNumberOfDimensions(){
    return dim;
  }

  T& getValue(int i, int j){
    //    cout << "getValue(" << i << ", " << j << ")" << endl;
    return array[index(i,j)];
  }
  
  void setValue(int i, int j, T v){
    array[index(i,j)] = v;
  }

  void incrementValue(int i, int j){
    array[index(i,j)]++;
  }

  T& getValueSafe(int i, int j){
    return (i <= j) ? getValue(i, j) : getValue(j, i);
  }

  void setValueSafe(int i, int j, T v){
    if(i <= j)
      setValue(i, j, v);
    else
      setValue(j, i, v);
  }

  void incrementValueSafe(int i, int j, T v){
    if(i <= j)
      incrementValue(i, j, v);
    else
      incrementValue(j, i, v);
  }
  
  //friend ostream& operator<<(ostream& os, KCArray& o);
};

#endif
