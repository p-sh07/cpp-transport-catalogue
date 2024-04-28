#pragma once

#include "domain.h"
#include "json.h"
#include "json_builder.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

#include <memory>
#include <string>

using namespace std::literals;

namespace cmd
{
// Defines command priority
enum CommandType {
    //Database commands
    NewStop,
    AddRoadDist,
    NewBus,
    //Stats request commands
    GetStopStat,
    GetBusStat,
    Error = 100,
};

auto BaseCmdToType(std::string literal);
auto StatCmdToType(std::string literal);

class BaseCommand {
public:
    BaseCommand(CommandType type, std::string name);
    virtual ~BaseCommand() = default;
    
    virtual bool ApplyCommand(TransportDb& tdb) = 0;
    
    inline CommandType GetType() {
        return type_;
    }
    //pass by ref to move
    inline std::string MoveOutName() {
        return std::move(name_);
    }
protected:
    CommandType type_;        // Command type
    std::string name_;        // Bus or Stop name
};

class StopData : public BaseCommand {
public:
    StopData(CommandType type, std::string name,  geo::Coord coords,
             std::unordered_map<std::string, int> road_distances = {});
    ~StopData() override = default;
    bool ApplyCommand(TransportDb& database) override;
    
private:
    geo::Coord coords_;
    std::unordered_map<std::string, int> road_distances_;
};

class BusData : public BaseCommand {
public:
    BusData(CommandType type, std::string name, std::vector<std::string> stops,
             bool is_roundtrip);
    ~BusData() override = default;
    bool ApplyCommand(TransportDb& database) override;
    
private:
    std::vector<std::string> stops_;
    bool is_roundtrip_;
};

struct StatRequest {
    CommandType type;
    std::string name;
    int id = 012312;
    
};
//TODO; polymorphism?
//class StatRequest : public DbCommand {
//    StatRequest(CommandType type, std::string name, int request_id);
//        return {};
//    }
//private:
//    int id = 0;            // request id
//    
//};
} //namespace cmd

//interface, for different input methods - e.g. xml
class DatabaseWriter {
public:
    DatabaseWriter(TransportDb& database);
    
    virtual ~DatabaseWriter() = default;
    
    void ProcessDatabaseCommands();
    virtual void ProcessStatRequests() = 0;
    virtual void PrintRequestAnswers(std::ostream& out) const = 0;
    
protected:
    TransportDb& database_;
    //1.Add all stops, 2.Add road distances, 3.add routes
    std::list<std::unique_ptr<cmd::BaseCommand>> base_requests_;
    //Process stat requests last
    std::queue<std::unique_ptr<cmd::StatRequest>> stat_requests_;
};

class JsonReader : public DatabaseWriter {
public:
    JsonReader(TransportDb& tdb, const RequestHandler& handler);
    ~JsonReader() override = default;
    
    void ParseInput(std::istream& in);
    void ProcessStatRequests() override;
    void PrintRequestAnswers(std::ostream& out) const override;
    
private:
    const RequestHandler& request_handler_;

    json::Map MakeStatJson(const BusStat& stat) const;
    json::Map MakeStatJson(const StopStat& stat) const;
    
    template <typename Stat>
    void StoreRequestAnswer(const Stat& stat);
    
    json::Map parsed_json_;
    json::Array request_replies_;
    
    svg::Point ParsePoint(json::Node point_node) const;
    svg::Color ParseColor(json::Node color_node) const;
    std::vector<svg::Color> ParsePalette(json::Node pallete_node) const;
};

using namespace std::literals;

template <typename Stat>
void JsonReader::StoreRequestAnswer(const Stat& stat) {
    //make the Json document, format:
    json::Node answer;
    if(stat.exists) {
        answer = MakeStatJson(stat);
    } else {
        answer = json::Builder().StartMap()
            .Key("request_id"s).Value(stat.request_id)
            .Key("error_message"s).Value("not found"s)
            .EndMap().Build();
    }
    request_replies_.emplace_back(answer);
}
