
#include "FindLargest.h"


FindLargest::FindLargest():
  debugtime(false),
  debug(false),
  subtractMean(20)
{


}

FindLargest::~FindLargest() {

}

void FindLargest::rotate(cv::Mat& src, int direction){
  if (direction>0){
    while(direction>0){
      transpose(src, src);
      flip(src, src, direction);
      direction--;
    }
  }else{
    while(direction<0){
      transpose(src, src);
      flip(src, src, direction);
      direction++;
    }
  }
}

cv::Mat FindLargest::largestContour(std::string src){
  return largestContour(cv::imread(src, cv::IMREAD_COLOR));
}


cv::Mat FindLargest::largestContour(cv::Mat src){
  return largestSimpleContour(src);
}


cv::Mat FindLargest::largestSimpleContour(cv::Mat src){
  //_debugTime("start largestSimpleContour");

  cv::Mat thr;
  cv::threshold(src, thr,subtractMean, 255,cv::THRESH_BINARY);

  int blength = thr.cols;
  int i=blength;
  int h=0;
  int height = thr.rows;


  double lastAVG = 255;
  int currentAVG = 255;
  int divisor=0;
  int avglength = 20;
  double cAVG=0;
  char avgbuffer[avglength];
  bzero(avgbuffer,avglength);
  char cavgbuffer[avglength];
  bzero(cavgbuffer,avglength);
  for(;i>199;i--){

    divisor=0;
    currentAVG=0;
    for(h=0;h<height;h+=5){
      currentAVG += thr.at<uchar>(h,i);
      divisor++;
    }
    currentAVG /=divisor;
    lastAVG=0;
    cAVG=0;
    for(h=avglength-1;h>0;h--){
      avgbuffer[h]=avgbuffer[h-1];
      lastAVG+=avgbuffer[h];
    }
    for(h=avglength-1;h>0;h--){
      cavgbuffer[h]=cavgbuffer[h-1];
      cAVG+=cavgbuffer[h];
    }
    lastAVG=lastAVG/(avglength-1);
    cAVG=(cAVG+currentAVG)/(avglength);
    if ((i<blength-avglength*2) && (cAVG>lastAVG)) {
      break;
    }
    avgbuffer[0]=currentAVG;
    cavgbuffer[0]=currentAVG;
  }
  
  if (i<200){
    std::cerr << "this should not happen the contour is to small " << i << " use the hole image "<< std::endl;
    i=blength;
  }

  cv::Rect myROI(0, 0, i, thr.rows);
  cv::Mat result = src(myROI);
  return result;
}


std::string FindLargest::getText(cv::Mat& src){
    tesseract::TessBaseAPI *ocr = new tesseract::TessBaseAPI();
    // Initialize tesseract to use English (eng) and the LSTM OCR engine. 
    //ocr->SetDatapath();
    //ocr->Init("/usr/local/Cellar/tesseract-lang/4.0.0/share/tessdata/", "eng", tesseract::OEM_TESSERACT_LSTM_COMBINED);
    ocr->Init("/usr/local/Cellar/tesseract-lang/4.0.0/share/tessdata/", "eng", tesseract::OEM_LSTM_ONLY);
//    ocr->Init(NULL, "eng", tesseract::OEM_TESSERACT_LSTM_COMBINED);
    // Set Page segmentation mode to PSM_AUTO (3)
    //ocr->SetSourceResolution(163);
//    ocr->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
    // Open input image using OpenCV
    // Set image data
    cv::cvtColor(src, src, CV_BGR2GRAY);

    cv::adaptiveThreshold(
        src,
        src,
        255,
        CV_ADAPTIVE_THRESH_GAUSSIAN_C,
        CV_THRESH_BINARY,
        15,
        20
    );
    cv::cvtColor(src, src, CV_GRAY2BGR);

    ocr->SetImage(src.data, src.cols, src.rows, 3, src.step);
 
    // Run Tesseract OCR on image
    char* t = ocr->GetUTF8Text();

    int offset;
    float slope;

    int conf = ocr->MeanTextConf();
    ocr->GetTextDirection(&offset, &slope);
    
    std::cout << "Confidence=" << conf << "\n\n";
    std::cout << "Offset: "<< offset << " Slope: "<< slope <<"\n";
    std::cout << "OCR output:\n\n";
    std::cout << t;
    std::cout << "------\n\n";

    return std::string(t);
}