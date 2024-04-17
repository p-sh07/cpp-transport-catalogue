#include "domain.h"

Stop::Stop(std::string stop_name, geo::Coord coords)
: name(std::move(stop_name))
, location(coords)
{}

Bus::Bus(std::string name, std::vector<StopPtr> stops_in_order, bool is_roundtrip)
: name(std::move(name))
, stops(std::move(stops_in_order))
, is_roundtrip(is_roundtrip)
{}

bool BusPtrSorter::operator()(const BusPtr& lhs, const BusPtr& rhs) const {
    return (lhs->name.compare(rhs->name) < 0);
}
