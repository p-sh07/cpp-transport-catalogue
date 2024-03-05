#pragma once
#include "geo.h"

#include <algorithm>
#include <list>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

//DEBUG
#include <iostream>
#include <iomanip>

namespace transport {
struct Stop {
public:
    explicit Stop(std::string stop_name, geo::Coordinates coords);
    std::string name;
    geo::Coordinates location;
    mutable std::unordered_set<std::string_view> routes;
};

struct Route {
    //move constructor, to use with input_reader functions
    explicit Route(std::string name, std::vector<const Stop*>&& stops);
    
    std::string name;
    std::vector<const Stop*> stops; //in order
    //TODO: May be inefficient
    std::unordered_set<const Stop*> unique_stops;
    double length = 0.0;
};

struct RouteStats {
    double length = 0.0;
    size_t unique_stops = 0;
    size_t total_stops = 0;
};

struct StopStats {
    StopStats(const std::unordered_set<std::string_view>& unique_routes);
    
    // check if route contains buses
    explicit operator bool() const {
        return !sorted_routes.empty();
    }
    
    bool operator!() const {
        return !operator bool();
    }
    
    std::vector<std::string_view> sorted_routes;
};

}

class TransportCatalogue {
public:
    ~TransportCatalogue() {
        ClearData();
    }
	//use string here, and make functions in i/o move the string
    void AddStop(std::string stop_name, geo::Coordinates coords);
    void AddRoute(std::string route_name, const std::vector<std::string_view>& stops);
    
    std::optional<transport::RouteStats> GetRouteStats(std::string_view route_name) const;
    std::optional<transport::StopStats> GetStopStats(std::string_view stop_name) const;
    
private:
    using StopPair = std::pair<const transport::Stop*,const transport::Stop*>;
    
    std::vector<const transport::Stop*> GetStopPtrs(const std::vector<std::string_view>& route_stops) const;
    double GetDistance(const transport::Stop* from, const transport::Stop* to) const;
    void AddRouteToStops(const transport::Route* route);
    void ClearData();
    
    std::unordered_map<std::string_view, transport::Stop*> stop_index_;
    std::unordered_map<std::string_view, transport::Route*> route_index_;
    
    struct SPHasher {
        size_t operator()(const StopPair& ptr_pair) const;
    };
    //can be modified by const member functions (e.g.GetStats)
    mutable std::unordered_map<StopPair, double, SPHasher> distance_table_;
};
