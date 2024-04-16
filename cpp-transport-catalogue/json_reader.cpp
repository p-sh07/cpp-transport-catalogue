#include "json_reader.h"

using namespace std::literals;
namespace cmd {

auto BaseCmdToType(std::string literal)
{
    if(literal == "Stop"s) {
        return cmd::NewStop;
    } else if(literal == "Bus"s) {
        return cmd::NewBus;
    }
    return cmd::Error;
}

auto StatCmdToType(std::string literal)
{
    if(literal == "Stop"s) {
        return cmd::GetStopStat;
    } else if(literal == "Bus"s) {
        return cmd::GetBusStat;
    }
    return cmd::Error;
}

BaseCommand::BaseCommand(CommandType type, std::string name)
: type_(type)
, name_(std::move(name))
{}

StopData::StopData(CommandType type, std::string name,  geo::Coord coords,
                   std::unordered_map<std::string, int> road_distances)
: BaseCommand(type, name)
, coords_(coords)
, road_distances_(std::move(road_distances))
{}

bool StopData::ApplyCommand(TransportDb& database) {
    if(type_ == cmd::NewStop) {
        //will need the stop name again, so send a copy
        database.AddStop(name_, coords_);
        //if road distances are present, add them in the second pass-though
        if(!road_distances_.empty()) {
            type_ = cmd::AddRoadDist;
            return false;
        }
    } else if(type_ == cmd::AddRoadDist) {
        for(const auto& [to_stop, dist] : road_distances_) {
            database.SetRoadDistance(name_, to_stop, dist);
        }
    }
    return true;
}

BusData::BusData(CommandType type, std::string name, std::vector<std::string> stops, bool is_roundtrip)
: BaseCommand(type, name)
, stops_(std::move(stops))
, is_roundtrip_(is_roundtrip)
{}

bool BusData::ApplyCommand(TransportDb& database) {
    std::vector<std::string_view> bus_stops {stops_.begin(), stops_.end()};
    //add stops in reverse order, skipping last one
    if(!is_roundtrip_) {
        bus_stops.insert(bus_stops.end(), std::next(stops_.rbegin()), stops_.rend());
    }
    
    database.AddBus(MoveOutName(), bus_stops);
    //this object will be deleted after use
    return true;
}
}//namespace cmd;

//================ DatabaseWriter ================//
DatabaseWriter::DatabaseWriter(TransportDb& tdb)
: database_(tdb)
{}

void DatabaseWriter::ProcessDatabaseCommands() {
    //cycle throug the database commands in priority order.
    for (int i = cmd::NewStop; i < cmd::GetStopStat; ++i) {
        for(auto it = base_requests_.begin(); it != base_requests_.end(); ) {
            auto& cmnd = *it;
            //ApplyCommand returns true if no more processing required
            if(static_cast<int>(cmnd->GetType()) == i && cmnd->ApplyCommand(database_)) {
                it = base_requests_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

//================ JsonReader ================//
JsonReader::JsonReader(TransportDb& tdb, const RequestHandler& handler)
: DatabaseWriter(tdb)
, r_handler_(handler)
{}

void JsonReader::ParseInput(std::istream& in) {
    //gives a dict with 2 keys
    parsed_json_ = json::Load(in).GetRoot().AsMap();
    //process database requests; returns Array of dicts
    for(auto& entry : parsed_json_["base_requests"s].AsArray()) {
        auto& dict = entry.AsMap();
        //db_req is a map
        if(dict.at("type"s).AsString() == "Stop"s) {
            //build road distances map:
            std::unordered_map<std::string, int> distances;
            
            for(const auto& [name, dist] : dict.at("road_distances").AsMap()) {
                distances[name] = dist.AsInt();
            }
            
            //TODO: std::move on a const object, how to move string from the json?
            cmd::StopData stop_info(cmd::NewStop,
                std::move(dict.at("name"s).AsString()),
                geo::Coord{dict.at("latitude"s).AsDouble(), dict.at("longitude"s).AsDouble()},
                std::move(distances));
            
            base_requests_.push_back(std::make_unique<cmd::StopData>(std::move(stop_info)));
        }
        else if(dict.at("type"s).AsString() == "Bus"s) {
            std::vector<std::string> stops;
            for(const auto& node : dict.at("stops"s).AsArray()) {
                stops.push_back(std::move(node.AsString()));
            }
            
            cmd::BusData bus_info(cmd::NewBus, std::move(dict.at("name"s).AsString()),
                         std::move(stops), dict.at("is_roundtrip"s).AsBool());
            
            base_requests_.push_back(std::make_unique<cmd::BusData>(std::move(bus_info)));
        }
        
    }
    //Write stop & route data to database
    DatabaseWriter::ProcessDatabaseCommands();
    
    //process stat requests:
    for(auto& json_request : parsed_json_["stat_requests"].AsArray()) {
        if(json_request.AsMap().empty()) {
            throw std::runtime_error("");
        }
        auto& dict = json_request.AsMap();
        cmd::StatRequest request{cmd::StatCmdToType(dict.at("type"s).AsString()), dict.at("name"s).AsString(),
            dict.at("id"s).AsInt()};
        
        stat_requests_.push(std::make_unique<cmd::StatRequest>(std::move(request)));
    }
}

json::Dict JsonReader::MakeStatJson(const BusStat& stat) const {
    return {{"curvature"s, {stat.curvature}},
        {"request_id"s, {stat.request_id}},
        {"route_length"s, {stat.road_dist}},
        {"stop_count"s, {stat.total_stops}},
        {"unique_stop_count"s, {stat.unique_stops}} };
}
json::Dict JsonReader::MakeStatJson(const StopStat& stat) const {
    json::Array buses;
    
    for(auto bus_ptr : stat.BusesForStop) {
        buses.push_back({bus_ptr->name});
    }
    return { {"buses"s, {buses}},
        {"request_id"s, {stat.request_id}}};
}


void JsonReader::ProcessStatRequests() {
    while(!stat_requests_.empty()) {
        //TODO: std::variant<BusStat, StopStat> stats; ? maybe too complex for the task
        
        //extract the request from queue
        auto request = std::move(stat_requests_.front());
        stat_requests_.pop();
        
        if(!request) {
            throw std::runtime_error("Stat request ptr eror");
        }
        
        if(request->type == cmd::GetBusStat) {
            BusStat stat = r_handler_.GetBusStat(request->id, request->name);
            
            StoreRequestAnswer(stat);
        }
        else if(request->type == cmd::GetStopStat) {
            StopStat stat = r_handler_.GetStopStat(request->id, request->name);
            
            StoreRequestAnswer(stat);
        }
    }
}

void JsonReader::PrintRequestAnswers(std::ostream& out) const {
    json::Print(json::Document{{request_replies_}}, out);
}
