// ExtractAddress.h
#ifndef EXCTRACTADDRESS_H
#define EXCTRACTADDRESS_H

#include <iostream>
#include <stdio.h>
//#include <regex>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

class ExtractAddress{
  public:
    void setString(std::string text);
    void extract();
    std::string getHouseNumber();
    std::string getStreetName();
    std::string getZipCode();
    std::string getTown();
  private:
    std::string orignalString;
    std::string town;
    std::string zipCode;
    std::string streetName;
    std::string housenumber;

};
#endif
