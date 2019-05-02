// Barcode.h
#ifndef BARCODE_H
#define BARCODE_H


#include <stdio.h>
#include <cstdlib>
#include <fstream>
#include <iostream>

class Barcode{
public:
  Barcode();
  Barcode(std::string code, std::string type);
  Barcode(std::string code, std::string type, int x1, int y1,int w1, int h1);
  ~Barcode();

  void printDebug();
  std::string code();
  std::string type();
  
private:
  std::string barcode;
  std::string codetype;
  int x;
  int y;
  int w;
  int h;
};

#endif
