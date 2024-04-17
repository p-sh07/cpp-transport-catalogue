#include "request_handler.h"

RequestHandler::RequestHandler(const TransportDb& tdb, MapRenderer& renderer)
: tdb_(tdb)
, renderer_(renderer) 
{}

BusStat RequestHandler::GetBusStat(int request_id, const std::string_view& bus_name) const {
    //TODO - better way to assign request_id?
    auto stat = tdb_.GetBusStat(bus_name);
    stat.request_id = request_id;
    
    return stat;
}

// Возвращает маршруты, проходящие через
StopStat RequestHandler::GetStopStat(int request_id, const std::string_view& stop_name) const {
    auto stat = tdb_.GetStopStat(stop_name);
    stat.request_id = request_id;
    
    return stat;
}

void RequestHandler::UploadRendererSettings(std::shared_ptr<RendererSettings> settings) const {
    renderer_.LoadSettings(settings);
}

// Отрисовать карту в поток
void RequestHandler::RenderMap(std::ostream& out) const {
    //TODO: Add stops? OR add stops via buses in one go?
    
    for(const auto& [bus_name, bus_ptr] : tdb_.GetAllBuses()) {
        renderer_.AddBus(bus_ptr);
    }
    renderer_.RenderOut(out);
}
