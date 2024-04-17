#include <iostream>
#include <fstream>
#include <string>

#include "json_reader.h"
#include "request_handler.h"

using namespace std;

int main() {
    
    TransportDb database;
    MapRenderer map_renderer;
    RequestHandler request_handler(database, map_renderer);
    JsonReader jreader(database, request_handler);
    
    //auto& in = std::cin;
    ifstream in("/Users/ps/Docs/input.json"s);
    
    if(!in.is_open()) {
        cerr << "Error opening input file" << endl;
    }
    
    jreader.ParseInput(in);
    //jreader.ProcessStatRequests();
    //jreader.PrintRequestAnswers(std::cout);
    request_handler.RenderMap(std::cout);
}
