// Grabber.h
#ifndef FINDLARGEST_H
#define FINDLARGEST_H


#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <zbar.h>

#include <stdio.h>
#include <cstdlib>
#include <fstream>

#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>

#include <boost/thread/scoped_thread.hpp>
#include <iostream>


#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

#include "Barcode.h"

class FindLargest{
public:
  FindLargest();
  ~FindLargest();

  cv::Mat largestContour(std::string src);
  cv::Mat largestContour(cv::Mat src);
  cv::Mat largestSimpleContour(cv::Mat src);
  void rotate(cv::Mat& src, int direction);
  std::string getText(cv::Mat& src);



private:
  bool debugtime;
  bool debug;
  int subtractMean;
};
#endif
