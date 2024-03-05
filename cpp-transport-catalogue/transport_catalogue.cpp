#include "transport_catalogue.h"

using std::string;
using std::string_view;
using std::vector;

using transport::Stop;
using transport::Route;
using transport::StopStats;
using transport::RouteStats;

//DEBUG
using std::cerr;
using std::endl;

Stop::Stop(string stop_name, geo::Coordinates coords)
: name(std::move(stop_name))
, location(coords)
{}

Route::Route(string name, vector<const Stop*>&& stops_in_order)
: name(std::move(name))
, stops(std::move(stops_in_order))
, unique_stops({stops.begin(), stops.end()}) {}

StopStats::StopStats(const std::unordered_set<std::string_view>& unique_routes)
: sorted_routes(unique_routes.begin(), unique_routes.end()) {
    std::sort(sorted_routes.begin(), sorted_routes.end());
}

void TransportCatalogue::AddStop(std::string stop_name, geo::Coordinates coords) {
    Stop* stop_to_add = new Stop(std::move(stop_name), coords);
    stop_index_[stop_to_add->name] = stop_to_add;
}
void TransportCatalogue::AddRoute(std::string route_name, const std::vector<std::string_view>& stops) {
    Route* route_to_add = new Route(std::move(route_name), GetStopPtrs(stops));
    route_index_[route_to_add->name] = route_to_add;
    AddRouteToStops(route_to_add);
}

std::optional<RouteStats> TransportCatalogue::GetRouteStats(string_view route_name) const {
    if(route_index_.count(route_name) == 0) {
        return std::nullopt;
    }
    auto route_it = route_index_.at(route_name);
    double route_length = route_it->length;
    //if route length has not yet been calculated
    if(route_length == 0.0) {
        //prev stop in route for distance calculation
        const Stop* prev_stop = nullptr;
        for(const auto& stop : route_it->stops) {
            route_length += GetDistance(prev_stop, stop);
            prev_stop = stop;
        }
    }
    return {{route_length, route_it->unique_stops.size(), route_it->stops.size()}};
}

std::optional<StopStats> TransportCatalogue::GetStopStats(std::string_view stop_name) const {
    if(stop_index_.count(stop_name) == 0) {
        return std::nullopt;
    }
    return StopStats(stop_index_.at(stop_name)->routes);
}

vector<const Stop*> TransportCatalogue::GetStopPtrs(const vector<string_view>& route_stops) const {
    vector<const Stop*> ptr_vector;
    for(const auto& stop : route_stops) {
        if(stop_index_.count(stop) > 0) {
            ptr_vector.push_back(stop_index_.at(stop));
        } //else { // TODO: throw error if stop not found?
    }
    return ptr_vector;
}

void TransportCatalogue::AddRouteToStops(const Route* route) {
    for(const auto& stop : route->unique_stops) {
        if(stop_index_.count(stop->name) > 0) {
            stop_index_.at(stop->name)->routes.insert(route->name);
        }
    }
}

double TransportCatalogue::GetDistance(const Stop* from, const Stop* to) const {
    //invalid stop pointers
    if(!from || !to) {
        return 0.0;
    }
    //use pair for convenience
    auto stop_pair = std::make_pair(from, to);
    if(distance_table_.count(stop_pair) > 0) {
        return distance_table_.at(stop_pair);
    }
    //compute if not found
    double dist = geo::ComputeDistance(from->location, to->location);
    distance_table_[stop_pair] = dist;
    return dist;
}

size_t TransportCatalogue::SPHasher::operator()(const StopPair& ptr_pair) const {
    size_t n = 37;
    std::hash<const Stop*> ptr_hash;
    return n*ptr_hash(ptr_pair.first) + ptr_hash(ptr_pair.second);
}

void TransportCatalogue::ClearData() {
    for(auto& [_, stop] : stop_index_) {
        delete stop;
    }
    
    for(auto& [_, route] : route_index_) {
        delete route;
    }
}
