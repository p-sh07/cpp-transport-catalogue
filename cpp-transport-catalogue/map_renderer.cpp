#include "map_renderer.h"


//=================== SphereProjector ===================//
// Проецирует широту и долготу в координаты внутри SVG-изображения
svg::Point SphereProjector::operator()(geo::Coord coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}


//=================== MapObject ===================//
void MapObject::SetParams(const RendererSettings& settings, const ColorMap& colors) {
    ApplySettings(settings, colors);
}
void SetProjector(const std::shared_ptr<SphereProjector> projector_);

svg::Point MapObject::GetImgPoint(geo::Coord pt) {
    return projector_(pt);
}


//=================== StopGraphic ===================//
//StopGraphic::StopGraphic(const SphereProjector& sp, std::string_view stop_name, geo::Coord coords)
//: MapObject(sp) //TODO
//{}
//
//void StopGraphic::Draw(svg::ObjectContainer& container) const {
//    //TODO
//}
//
//void StopGraphic::ApplySettings (const RendererSettings& settings, const ColorMap& colors) {
//    //TODO
//}


//=================== BusGraphic ===================//
BusGraphic::BusGraphic(const SphereProjector& sp, std::string_view bus_name, const std::vector<StopPtr>& stops, bool is_roundtrip )
: MapObject(sp)
, bus_name_(bus_name)
, stops_(stops)
, is_roundtrip_(is_roundtrip)
{}

void BusGraphic::Draw(svg::ObjectContainer& container) const {
    //draw polyline
    svg::Polyline route_line;

    for(const auto& stop : stops_) {
        //project stop geo coords to img pixel coords
        route_line.AddPoint(projector_(stop->location));
    }

    container.Add(route_line);
}

void BusGraphic::ApplySettings(const RendererSettings& stngs, const ColorMap& colors) {
    line_.SetFillColor(stngs.fill_color_)
        .SetStrokeColor(colors.at(bus_name_))
        .SetStrokeWidth(stngs.line_width)
        .SetStrokeLineCap(stngs.line_cap_)
        .SetStrokeLineJoin(stngs.line_join_);
}


//=================== MapRenderer ===================//
MapRenderer::MapRenderer(RendererSettings settings)
: settings_(std::move(settings))
{}

void MapRenderer::ApplySettings(RendererSettings settings) {
    settings_ = std::move(settings);
}

//void MapRenderer::AddStop(std::string_view stop_name, std::string_view bus_name, geo::Coord stop_coords) {
//    //TODO
//    
//    //Bus name for color selection
//    //StoreCoordPtr(pt);
//}


void MapRenderer::AddBus(std::string_view bus_name, const std::vector<StopPtr>& stops, bool is_roundtrip) {
    //TODO
    if(bus_name == "smth") {
        throw std::runtime_error("");
    }
    //StoreCoordPtr(pt);
}

void MapRenderer::RenderOut(std::ostream& out) {
    svg::Document doc;
    SphereProjector sp(all_geo_points_.begin(), all_geo_points_.end(), settings_.img_size.width, settings_.img_size.height, settings_.padding);
    
    const auto color_map = BuildColorMap();
    
    for(const auto& obj : objects_to_draw_) {
        obj->SetParams(settings_, color_map);
        obj->Draw(doc);
    }
    doc.Render(out);
}

svg::Color MapRenderer::GetNextColor() const {
    if(is_first_color_) {
        is_first_color_ = false;
        return settings_.palette[0];
    }
    if(color_index_ == settings_.palette.size()) {
        color_index_ = 0;
    } else {
        ++color_index_;
    }
    return settings_.palette[color_index_];
}

MapObject::ColorMap MapRenderer::BuildColorMap() const {
    MapObject::ColorMap cm;
    for(const auto& bus_name : all_bus_names_) {
        cm[bus_name] = svg::NoneColor;
    }
    
    for(auto it = cm.begin(); it != cm.end(); ++it) {
        it->second = GetNextColor();
    }
    //reset first color flag
    is_first_color_ = true;
    
    return cm;
}

void MapRenderer::StoreBusName(std::string_view bus) {
    all_bus_names_.push_back(bus);
}


void MapRenderer::StoreCoordPtr(const geo::Coord* ptr) {
    all_geo_points_.push_back(ptr);
}
