#pragma once
#include "geo.h"

#include <string>
#include <set>
#include <vector>

struct Stop {
    explicit Stop(std::string stop_name, geo::Coord coords);
    std::string name;
    geo::Coord location;
};

using StopPtr = const Stop*;

struct Bus {
    //move string during construction
    explicit Bus(std::string name, std::vector<StopPtr> stops);
    bool is_roundtrip = false;
    std::string name;
    std::vector<StopPtr> stops;
};

using BusPtr = const Bus*;

struct BusPtrSorter {
    bool operator()(const BusPtr& lhs, const BusPtr& rhs) const;
};

struct StopStat {
    int request_id = 0;
    bool exists = false;
    std::set<BusPtr, BusPtrSorter> BusesForStop;
};

struct BusStat {
    int request_id = 0;
    bool exists = false;
    int total_stops = 0;
    int unique_stops = 0;
    double road_dist = 0.0;
    double curvature = 0.0;
};
