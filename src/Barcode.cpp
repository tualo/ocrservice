#include "Barcode.h"

Barcode::Barcode():
  x(0),
  y(0),
  w(0),
  h(0),
  barcode(""),
  codetype("")
{

}

Barcode::Barcode(std::string code, std::string type):
  x(0),
  y(0),
  w(0),
  h(0)
{
    barcode = code;
    codetype = type;
}
Barcode::Barcode(std::string code, std::string type, int x1, int y1,int w1, int h1){
    barcode = code;
    codetype = type;
    x = x1;
    y = y1;
    w = w1;
    h = h1;
}

Barcode::~Barcode() {

}

void Barcode::printDebug(){
    std::cout << "Barcode: " << barcode << "\t" << "Type " << codetype << "\t" << "X " << x << "\t" << "Y " << y << "\t" << "W " << w << "\t" << "H " << h << std::endl;
}
std::string Barcode::code(){
    return barcode;
}
std::string Barcode::type(){
    return codetype;
}