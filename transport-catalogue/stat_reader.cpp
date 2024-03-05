#include "stat_reader.h"

using std::operator""s;

void stats::ParseAndPrintStat(const TransportCatalogue& tansport_catalogue, std::string_view request,
                       std::ostream& output) {
    //Parsing
    size_t space_pos = request.find(' ');
    std::string_view id = request.substr(space_pos+1, request.npos);
    std::string_view command = request.substr(0, space_pos);
    
    if(command == stats::GET_ROUTE_INFO) {
        auto stats_optional = tansport_catalogue.GetRouteStats(id);
        
        output << "Bus "s << id << ": ";
        if(!stats_optional.has_value()) {
            output << "not found\n"s;
            return;
        }
        
        auto& stats = stats_optional.value();
        
        output << std::setprecision(6)
        << stats.total_stops << " stops on route, "s
        << stats.unique_stops << " unique stops, "s
        << stats.length << " route length\n"s;
        
    } else if(command == stats::GET_STOP_INFO) {
        auto stats_optional = tansport_catalogue.GetStopStats(id);
        
        output << "Stop "s << id << ": ";
        
        if(!stats_optional.has_value()) {
            output << "not found\n"s;
            return;
        } else if(!stats_optional.value()) {
            output << "no buses\n"s;
            return;
        }
        
        output << "buses ";
        auto& stats = stats_optional.value();
        for(const auto route_name : stats.sorted_routes) {
            output << route_name << ' ';
        }
        output << "\n";
    }
}
