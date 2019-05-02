#include "FindCodes.h"


FindCodes::FindCodes():
  startSubtractMean(15),
  stepSubtractMean(5),
  
  startBlocksize(45),
  stepBlocksize(10),
  debugtime(false)
{
  maxTasks = boost::thread::hardware_concurrency();



 debug_start_time = (double)cv::getTickCount();
 debug_last_time = (double)cv::getTickCount();
 debug_window_offset = 0;

}

FindCodes::~FindCodes() {

}

void FindCodes::debugTime(std::string str){
    mutex.lock();
    if (debugtime){
        double time_since_start = ((double)cv::getTickCount() - debug_start_time)/cv::getTickFrequency();
        double time_since_last = ((double)cv::getTickCount() - debug_last_time)/cv::getTickFrequency();
        std::cout << str << ": " << time_since_last << "s " << "(total: " << time_since_start  << "s)" << std::endl;
    }
    debug_last_time = (double)cv::getTickCount();
    mutex.unlock();
}

void FindCodes::detect(std::string filename){

    const char* fname = "";
    fname= filename.c_str();
    cv::Mat img = imread(fname, cv::IMREAD_COLOR );
    

    cv::Mat useimage;
    cv::cvtColor(img, useimage, CV_BGR2GRAY);

    detectCodes(useimage);

}

void FindCodes::setDebugTiming(bool v){
    debugtime=v;
}

void FindCodes::detectCodes(cv::Mat image){
    std::list<boost::thread*> threadList;

    debugTime("start detectCodes");

    boost::thread* meanLoopThread = new boost::thread(&FindCodes::findBlured, this, image);  
    threadList.push_back(meanLoopThread);



    int i_bc_thres_stop=10;
    int i_bc_thres_start=200;
    int i_bc_thres_step=20;

    for (int thres=i_bc_thres_stop;(thres>=i_bc_thres_start);thres-=i_bc_thres_step){
        boost::thread* meanLoopThread = new boost::thread(&FindCodes::detectByThreshold, this, image, thres);  
        threadList.push_back(meanLoopThread);
    }

    for (int bs=startBlocksize;((bs>=0));bs-=stepBlocksize){
        for (int sm=startSubtractMean;((sm>=0));sm-=stepSubtractMean){
            boost::thread* meanLoopThread = new boost::thread(&FindCodes::detectByAdaptiveThreshold, this, image, bs, sm);  
            threadList.push_back(meanLoopThread);
        }
    }
    
    
    std::list<boost::thread*>::const_iterator it;
    for (it = threadList.begin(); it != threadList.end(); ++it){
        ((boost::thread*)*it)->join();
    }

    delete meanLoopThread;
    
    debugTime("stop detectCodes");
    //debugCodes();
}
void FindCodes::debugCodes(){
    std::list<Barcode*>::const_iterator it;
    for (it = barcodes.begin(); it != barcodes.end(); ++it){
        ((Barcode*)*it)->printDebug();
    }
}

bool FindCodes::hasCode(std::string code, std::string type){
    std::list<Barcode*>::const_iterator it;
    for (it = barcodes.begin(); it != barcodes.end(); ++it){
        if (
         ((Barcode*)*it)->code()==code
        ){
            if (
            ((Barcode*)*it)->type()==type
            ){
                return true;
            }
        }
        
    }
    return false;
}

std::list<Barcode*> FindCodes::codes(){
    return barcodes;
}



void FindCodes::detectByThreshold(cv::Mat image,int thres) {
    cv::Mat gray;

    mutex.lock();
    cv::Mat useimage = image.clone();
    mutex.unlock();

    cv::threshold(useimage,gray,thres,255, CV_THRESH_BINARY );
    findCodeInImage(gray);

//    gray.release();
//    useimage.release();
}

void FindCodes::detectByAdaptiveThreshold(cv::Mat image,int blocksize, int subtractmean) {
        /*
        mutex.lock();
        std::cout << "blocksize: " << blocksize << " " << " subtractmean " << subtractmean << std::endl;
        mutex.unlock();
        */
        cv::Mat gray;
        mutex.lock();
        cv::Mat useimage = image.clone();
        mutex.unlock();

        cv::adaptiveThreshold(
            useimage,
            gray,
            255,
            CV_ADAPTIVE_THRESH_GAUSSIAN_C,
            CV_THRESH_BINARY,
            blocksize,
            subtractmean
        );
        findCodeInImage(gray);

//        gray.release();
//        useimage.release();

}

void FindCodes::findBlured(cv::Mat image){
    cv::Mat gray;
    mutex.lock();
    cv::Mat useimage = image.clone();
    mutex.unlock();
    cv::GaussianBlur(useimage,gray,cv::Size(3,3),2,2);
    findCodeInImage(gray);

//    gray.release();
//    useimage.release();

}

void FindCodes::findCodeInImage(cv::Mat gray) {

        zbar::Image* _image;
        zbar::ImageScanner* _imageScanner;

        _image = new zbar::Image(gray.cols, gray.rows, "Y800", nullptr, 0);
        _imageScanner = new zbar::ImageScanner();
        _imageScanner->set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);
        _image->set_data((uchar *)gray.data, gray.cols * gray.rows);
        int n = _imageScanner->scan(*_image);
        for(zbar::Image::SymbolIterator symbol = _image->symbol_begin(); symbol != _image->symbol_end(); ++symbol) {

            int n = symbol->get_location_size();
            int xmin = 999999;
            int xmax = 0;
            int ymin = 999999;
            int ymax = 0;
            if(n <= 0){
            //    return(null);
            }else{
                cv::Rect* bounds = new cv::Rect();

                for(int i = 0; i < n; i++) {
                    int x = symbol->get_location_x( i);
                    if(xmin > x) xmin = x;
                    if(xmax < x) xmax = x;

                    int y = symbol->get_location_y( i);
                    if(ymin > y) ymin = y;
                    if(ymax < y) ymax = y;
                }
                bounds->x = xmin;
                bounds->y = ymin;
                bounds->width = xmax - xmin;
                bounds->height = ymax - ymin;
                mutex.lock();
                if (hasCode(symbol->get_data().c_str(), symbol->get_type_name().c_str())){

                }else{
                    Barcode* bc = new Barcode(
                        symbol->get_data().c_str(),
                        symbol->get_type_name().c_str(),
                        bounds->x ,
                        bounds->y ,
                        bounds->width ,
                        bounds->height 
                    );
                    Barcode* bc_find = isPartialCode(bc);
                    if (bc_find==NULL){
                        barcodes.push_back( bc );
                    }
                    if (bc_find!=bc){
                        barcodes.remove(bc_find);
                    }

                }

                //std::cout << "bounds->x: " << bounds->x << " " << "bounds->y: " << bounds->y << " bounds->width " << bounds->width << " bounds->height " << bounds->height << std::endl;
                mutex.unlock();
            }

        


        }
        _image->set_data(NULL, 0);
}


Barcode* FindCodes::isPartialCode(Barcode* code) {
    std::list<Barcode*>::const_iterator it;
    for (it = barcodes.begin(); it != barcodes.end(); ++it){
        Barcode* it_code = ((Barcode*)*it);
        if ( it_code->type()==code->type() ){
            std::string c1= it_code->code();
            std::string c2= code->code();
            if ( c1.find(c2)>=0){
                return code;
            }
            if ( c2.find(c1)>=0){
                return it_code;
            }
        }
    }
    return NULL;
}
