#pragma once

#include "domain.h"
#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

#include <memory>
#include <string>

using namespace std::literals;

class JsonReader {
public:
    JsonReader(TransportDb& tdb, const RequestHandler& handler);
    
    void ParseInput(std::istream& in);
    void ProcessDatabaseCommands();
    void ProcessStatRequests();
    void PrintRequestAnswers(std::ostream& out) const;
    
private:
    struct StatRequest {
        std::string_view type;
        std::string_view name;
        int id;
    };
    
    TransportDb& database_;
    const RequestHandler& req_handler_;
    std::queue<StatRequest> stat_requests_;
    json::Dict parsed_json_; //TODO: Need to store dict? or just std::move strings out when processing?
    json::Array stat_request_answers_;

    json::Dict MakeStatJson(const BusStat& stat) const;
    json::Dict MakeStatJson(const StopStat& stat) const;
    
    template <typename Stat>
    void StoreRequestAnswer(const Stat& stat);
    
    svg::Point ParsePoint(json::Node point_node) const;
    svg::Color ParseColor(json::Node color_node) const;
    std::vector<svg::Color> ParsePalette(json::Node pallete_node) const;
};

using namespace std::literals;

template <typename Stat>
void JsonReader::StoreRequestAnswer(const Stat& stat) {
    json::Dict answer = { {"request_id"s, {stat.request_id}},
        {"error_message"s, {"not found"s}} };
    if(stat.exists) {
        answer = MakeStatJson(stat);
    }
    json::Node node{answer};
    stat_request_answers_.push_back(node);
}
