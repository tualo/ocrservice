#include "args.hxx"
#include <iostream>


#include "../externals/Simple-Web-Server/client_http.hpp"
#include "../externals/Simple-Web-Server/server_http.hpp"

// Added for the json-example
#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

// Added for the default_resource example
#include <algorithm>
#include <boost/filesystem.hpp>
#include <fstream>
#include <vector>
#ifdef HAVE_OPENSSL
#include "crypto.hpp"
#endif

#define MB 1024*1024


#include <opencv2/opencv.hpp>

#include "../externals/Simple-WebSocket-Server/client_ws.hpp"
#include "../externals/Simple-WebSocket-Server/server_ws.hpp"

#include "FindCodes.h"
#include "FindLargest.h"
#include "ExtractAddress.h"



#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <algorithm>    



using namespace std;
using namespace boost::property_tree;

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;



using namespace boost::archive::iterators;


double debug_start_time = (double)cv::getTickCount();
double debug_last_time = (double)cv::getTickCount();
double debug_window_offset = 0;

bool bDebugTime=false;
void debugTime(std::string str){
  if (bDebugTime){
    double time_since_start = ((double)cv::getTickCount() - debug_start_time)/cv::getTickFrequency();
    double time_since_last = ((double)cv::getTickCount() - debug_last_time)/cv::getTickFrequency();
    std::cout << str << ": " << time_since_last << "s " << "(total: " << time_since_start  << "s)" << std::endl;
  }
  debug_last_time = (double)cv::getTickCount();
}


double debugTime(double debug_start_time,double debug_last_time, stringstream &timingstream, std::string str){
    double time_since_start = ((double)cv::getTickCount() - debug_start_time)/cv::getTickFrequency();
    double time_since_last = ((double)cv::getTickCount() - debug_last_time)/cv::getTickFrequency();
    timingstream << str << ": " << time_since_last << "s " << "(total: " << time_since_start  << "s)" << std::endl;
    return (double)cv::getTickCount();
}


std::string toBase64(std::vector<uchar> binary){
    std::string message(binary.begin(), binary.end());
    std::stringstream os;
    using base64_text = insert_linebreaks<base64_from_binary<transform_width<const char *, 6, 8>>, 50000000>;

    std::copy(
        base64_text(message.c_str()),
        base64_text(message.c_str() + message.size()),
        ostream_iterator<char>(os)
    );

    return os.str();
}


int main(int argc, char* argv[])
{
    int exitCode = 0;

    args::ArgumentParser parser("OCR-Service ", "");
    args::HelpFlag help(parser, "help", "Display this help menu", { 'h',"help"});
    args::Flag debug(parser, "debug", "Show debug messages", {'d', "debug"});
    args::ValueFlag<std::string> installservice(parser, "installservice", "install as systemd service", {"installservice"});
    args::ValueFlag<int> controllport(parser, "controllport", "set the service controll port", {"controllport"});


    try{
        parser.ParseCLI(argc, argv);
    }catch (args::Help){
        std::cout << parser;
        return 0;
    }catch (args::ParseError e){
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }


    HttpServer server;
    server.config.port = (controllport)?args::get(controllport):8080;

    // Add resources using path-regex and method-string, and an anonymous function
    // POST-example for the path /string, responds the posted string
    server.resource["^/string$"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
        // Retrieve string:
        auto content = request->content.string();
        // request->content.string() is a convenience function for:
        // stringstream ss;
        // ss << request->content.rdbuf();
        // auto content=ss.str();

        *response << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n"
                    << content;


        // Alternatively, use one of the convenience functions, for instance:
        // response->write(content);
    };


    // GET-example for the path /info
    // Responds with request-information
    server.resource["^/info$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
        stringstream stream;
        stream << "<h1>Request from " << request->remote_endpoint_address() << ":" << request->remote_endpoint_port() << "</h1>";

        stream << request->method << " " << request->path << " HTTP/" << request->http_version;

        stream << "<h2>Query Fields</h2>";
        auto query_fields = request->parse_query_string();
        for(auto &field : query_fields)
        stream << field.first << ": " << field.second << "<br>";

        stream << "<h2>Header Fields</h2>";
        for(auto &field : request->header)
        stream << field.first << ": " << field.second << "<br>";

        response->write(stream);
    };

    server.resource["^/image$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
        stringstream stream;
        auto query_fields = request->parse_query_string();
        std::string filename = "";
        FindCodes* fc = new FindCodes();
        for(auto &field : query_fields){
            if (field.first=="f"){
                filename = field.second;
                fc->detect(filename);
                stream << "<br/>" << "Check:" << filename << "<br/>\n";
                auto codes = fc->codes();
                for(auto &code : codes){
                    stream << "<br/>" << "Code:" << code->code() << "<br/>\n";
                }
            }
        }
        stream << "Filename:" << filename;
        response->write(stream);
    };


    server.resource["^/imagetext$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
        stringstream stream;
        stringstream timingstream;
        double starttime = (double)cv::getTickCount();
        double lasttime = (double)cv::getTickCount();
        
        auto query_fields = request->parse_query_string();
        std::string filename = "";
        FindLargest* fl = new FindLargest();
        for(auto &field : query_fields){
            if (field.first=="f"){
                filename = field.second;
                lasttime = debugTime(starttime,lasttime,timingstream,"start");

                cv::Mat largest = fl->largestContour(filename);
                lasttime = debugTime(starttime,lasttime,timingstream,"after largest contour");

                

                fl->rotate(largest,1);
                lasttime = debugTime(starttime,lasttime,timingstream,"after rotate");
                std::string txt = fl->getText(largest);
                lasttime = debugTime(starttime,lasttime,timingstream,"after text");

                std::vector<uchar> buffer;
                buffer.resize(200* MB);
                imencode(".jpg",largest,buffer);
                lasttime = debugTime(starttime,lasttime,timingstream,"after encode");


                ExtractAddress* ea = new ExtractAddress();
                ea->setString(txt);
                ea->extract();
                lasttime = debugTime(starttime,lasttime,timingstream,"after address");


                stream << "<br/>" << "Town:" << ea->getTown() << "<br/>\n";
                stream << "<br/>" << "ZipCode:" << ea->getZipCode() << "<br/>\n";
                stream << "<br/>" << "Street:" << ea->getStreetName() << "<br/>\n";
                stream << "<br/>" << "Housenumber:" << ea->getHouseNumber() << "<br/>\n";


                stream << "<br/>" << "<img height=\"20%\" src=data:image/jpeg;base64," << toBase64(buffer) << ">"<< "<br/>\n";
                stream << "<br/><pre>" << "Text:" << txt << "</pre><br/>\n";
                stream << "<br/><h1>Zeiten</h1><pre>" << timingstream.str() << "</pre><br/>\n";

            }
        }
        stream << "Filename:" << filename;
        response->write(stream);
    };

    server.on_error = [](shared_ptr<HttpServer::Request>, const SimpleWeb::error_code& ) {
        // Handle errors here
        // Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
    };


    thread server_thread([&server]() {
        // Start server
        server.start();
    });



    std::cout << "Server running on Port: " << server.config.port << "\n";

    server_thread.join();

    return exitCode;

}