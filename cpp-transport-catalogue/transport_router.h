#pragma once

#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"

#include <memory>

const double METERS_IN_KM = 1000.0;
const double MINUTES_IN_HOUR = 60.0;

struct BusRouterSettings {
    BusRouterSettings() = default;
    BusRouterSettings(int wait_time_at_stop, double velocity_km_per_hour);
    
    int wait_time = 0;
    double velocity_kmh = 0;
    double velocity_meters_per_minute = 0;
};

class BusRouter {
    using Graph = graph::DirectedWeightedGraph<double>;
    using Router = graph::Router<double>;
    using RouteInfo = graph::Router<double>::RouteInfo;
    
public:
    BusRouter() = default;
//    BusRouter(const BusRouterSettings& s)
//    : bus_wait_time_(s.wait_time)
//    , bus_velocity_(s.velocity)
//    {}
    
    void Init(BusRouterSettings settings, const TransportDb& tdb);
    RouteStat PlotRoute(std::string_view from, std::string_view to) const;
    
private:
    bool graph_is_init_ = false;
    bool initialized_ = false;
    BusRouterSettings settings_;
    
    struct EdgeInfo {
        BusPtr bus = nullptr;
        int span_count = 0;
        double time = 0.0;
        std::string_view from = {};
        std::string_view to = {};
    };
    
    int bus_wait_time_;
    int bus_velocity_;
    using StopIdMap = std::unordered_map<std::string_view, graph::VertexId>;
    std::unique_ptr<Router> graph_router_ = nullptr;
    Graph bus_graph_;
    StopIdMap stop_ids_;
    std::vector<EdgeInfo> edge_data_;
    
    graph::VertexId FindOrAddStopVertex(std::string_view stop_name);
    void AddStopsToGraph(BusPtr bus, StopPtr from_stop, const TransportDb& tdb, auto stops_range);
    
    //writes stop ids into map during graph building
    bool InitGraphFromDb(const TransportDb& tdb);
    RouteStat BuildRouteStat(const std::optional<RouteInfo>& info) const;
};
