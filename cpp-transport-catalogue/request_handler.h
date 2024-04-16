#pragma once

#include "map_renderer.h"
#include "transport_catalogue.h"

#include <list>
#include <queue>
#include <variant>

class RequestHandler {
public:
    RequestHandler(const TransportDb& tdb, const MapRenderer& renderer);

    // Возвращает информацию о маршруте (запрос Bus)
    BusStat GetBusStat(int request_id, const std::string_view& bus_name) const;

    // Возвращает маршруты, проходящие через Stop
    StopStat GetStopStat(int request_id, const std::string_view& stop_name) const;

    // Этот метод будет нужен в следующей части итогового проекта
    svg::Document RenderMap() const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const TransportDb& tdb_;
    const MapRenderer& renderer_;
};

