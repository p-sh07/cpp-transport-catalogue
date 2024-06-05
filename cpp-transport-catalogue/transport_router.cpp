#include "transport_router.h"

#include <ranges>
using namespace std::literals;

BusRouterSettings::BusRouterSettings(int wait_time_at_stop, double velocity_km_per_hour)
: wait_time(wait_time_at_stop)
, velocity_kmh(velocity_km_per_hour)
, velocity_meters_per_minute(velocity_kmh * METERS_IN_KM / MINUTES_IN_HOUR)
{}

void BusRouter::Init(BusRouterSettings settings, const TransportDb& tdb) {
    //1.Upload settings
    settings_ = std::move(settings);
    //2.Initialize graph
    graph_is_init_ = InitGraphFromDb(tdb);
    //3.Initialize router
    graph_router_ = std::make_unique<Router>(bus_graph_);
    //Complete!
    initialized_ = true;
}

RouteStat BusRouter::PlotRoute(std::string_view from, std::string_view to) const {
    if(!initialized_ || !graph_router_) {
        throw std::runtime_error("BusRouter not initialized!");
    }
    if(stop_ids_.count(from) == 0 || stop_ids_.count(to) == 0) {
        CERR << "*Warning: Router cannot find stop [" << (stop_ids_.count(from) == 0 ? from : to) << "]\n";
        return {};
    }
    auto route_info = graph_router_->BuildRoute(stop_ids_.at(from), stop_ids_.at(to));
    return BuildRouteStat(route_info);
}

graph::VertexId BusRouter::FindOrAddStopVertex(std::string_view stop_name) {
    //Inserts new value or returns id of existing entry pair<it,bool>->pair<name, VertexId>;
    return (stop_ids_.insert( {stop_name, stop_ids_.size()} )).first->second;
}

void BusRouter::AddStopsToGraph(BusPtr bus, StopPtr from_stop, const TransportDb& tdb, auto stops_range) {
    auto from_id = FindOrAddStopVertex(from_stop->name);
    StopPtr prev_stop = from_stop;
    CERR << from_id << ".[" << from_stop->name << "]:\n";
    
    bool adjacent_stops = true;
    size_t prev_edge = 0;
    
    //Iterate though all remaining stops -> start at stop which is after from_stop
    for(const auto& to_stop : stops_range) {
        CERR << " ->[" << to_stop->name << "] ";
        
        //Additional travel time from this stop to next
        double add_travel_time = 1.0 * tdb.GetRoadDistance(prev_stop, to_stop)
        / settings_.velocity_meters_per_minute;
        //Get previous edge travel time from graph,
        double edge_weight //if adjacent stops -> gives Bus wait time
        = bus_graph_.GetEdge(adjacent_stops ? 0 : prev_edge).weight
        + add_travel_time;
        
        int spans = edge_data_.at(prev_edge).span_count + 1;
        
        prev_edge = bus_graph_.AddEdge({
            //starting vertex, graph::VertexId
            from_id,
            //destination vertex, graph::VertexId
            FindOrAddStopVertex(to_stop->name),
            //total time taken, double
            edge_weight
        });
        
        //Store info for building Item Array (in request response)
        edge_data_.push_back({
            bus,
            spans,
            add_travel_time,
            from_stop->name,
            to_stop->name
        });
        adjacent_stops = false;
        prev_stop = to_stop;
    }
}

//writes stop ids into map during graph building
bool BusRouter::InitGraphFromDb(const TransportDb& tdb) {
    //1.Get buses -> set<BusPtr>, init Graph;
    BusSet buses = tdb.GetAllBusesWithStops();
    bus_graph_ = Graph(buses.size());
    if(buses.size() == 0) {
        CERR << "Graph: No buses with stops in db\n";
        return true;
    }
    
    //1.1.Add waiting time edge
    bus_graph_.AddEdge({0, 0, 1.0 * settings_.wait_time});
    //1.2.Init Edge Data with a 0-span Info for EdgeId 0
    edge_data_.push_back({});
    
    //2.Add each bus to graph
    for(const auto& bus : buses) {
        CERR << "$Bus [" << bus->name << "]\n";
        //take only half of the stops for iterating if not a roundtrip
        size_t no_repeats = bus->is_roundtrip ? bus->stops.size() : bus->stops.size() / 2 + 1;
        size_t processed_stops = 0;
        
        //2.1.Iterate through all stops, until final stop is reached (middle of vector for !is_roundtrip buses)
        for(const auto& from_stop : bus->stops | std::views::take(no_repeats)) {
            // 0 1 2 3 2 1 0 -> take first half, 0 1 2 3, drop passed stops, e.g. from 1 -> range is: 2, 3;
            AddStopsToGraph(bus, from_stop, tdb, bus->stops
                            | std::views::take(no_repeats)
                            | std::views::drop(processed_stops + 1));
            
            if(!bus->is_roundtrip) {
                CERR << "<-Reversing -- \n";
                //add previous bus stops backwards it not roundtrip
                AddStopsToGraph(bus, from_stop, tdb,  bus->stops
                                | std::views::take(processed_stops)
                                | std::views::reverse);
            }
            ++processed_stops;
            CERR << std::endl;
        }
    }
    return true;
}

RouteStat BusRouter::BuildRouteStat(const std::optional<RouteInfo>& info) const {
    RouteStat stat;
    if(!info.has_value()) {
        return {};
    }
    stat.exists = true;
    stat.total_time = info->weight;
    
    for(auto edge_id : info->edges) {
        if(edge_data_.size() <= edge_id) {
            continue;
        }
        const auto& edge_info = edge_data_.at(edge_id);
        stat.items.push_back({"Wait"s, edge_info.from, 1.0 * settings_.wait_time});
        stat.items.push_back({"Bus"s, edge_info.bus->name, edge_info.time, edge_info.span_count});
    }
    
    return stat;
}
