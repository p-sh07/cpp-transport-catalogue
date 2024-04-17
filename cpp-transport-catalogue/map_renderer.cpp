#include "map_renderer.h"

using namespace std::literals;

void MapRenderer::LoadSettings(std::shared_ptr<RendererSettings> settings) {
    settings_ = settings;
}

void MapRenderer::MakeProjector() {
    if(settings_) {
        projector_ = std::make_unique<SphereProjector>(all_geo_points_.begin(), all_geo_points_.end(), settings_->img_size.width, settings_->img_size.height, settings_->padding);
    }
}

void MapRenderer::AddBus(BusPtr bus) {
    buses_to_draw_.insert(bus);
    for(const auto& stop : bus->stops) {
        all_geo_points_.insert(&(stop->location));
    }
}

void MapRenderer::DrawBus(svg::Document& doc, BusPtr bus, svg::Color color) {
    svg::Polyline line;
    //TODO: error handling?
    if(!settings_ || !projector_) {
        throw std::runtime_error("Renderer is not initialised!"s);
    }
    if(!bus) {
        throw std::runtime_error("Renderer tried to accessed a null bus pointer"s);
    }
    auto& stops = bus->stops;
    
    //forward draw
    for(const auto& stop : stops) {
        line.AddPoint(projector_->Transform(stop->location));
    }
    
    //backward draw, if not roundtrip
    if(!bus->is_roundtrip) {
        for(auto it = stops.rbegin(); it != stops.rend(); ++it) {
            //TODO: Possibly store the projected points on the way up?
            line.AddPoint(projector_->Transform((*it)->location));
        }
    }
    
    line.SetFillColor(settings_->fill_color_)
        .SetStrokeColor(color)
        .SetStrokeWidth(settings_->line_width)
        .SetStrokeLineCap(settings_->line_cap_)
        .SetStrokeLineJoin(settings_->line_join_);
    
    doc.Add(line);
}

void MapRenderer::DrawAllBuses(svg::Document& doc) {
    size_t counter = 0;
    for(const auto& bus : buses_to_draw_) {
        if(!bus->stops.empty()) {
            DrawBus(doc, bus, GetNextColor(counter));
            //advance to next color if bus had stops
            ++counter;
        }
    }
}

void MapRenderer::RenderOut(std::ostream& out) {
    svg::Document doc;

    DrawAllBuses(doc);
    //Can add drawing other objects here later:
    //e.g. DrawStops, DrawLabels, etc.
    
    doc.Render(out);
}

svg::Color MapRenderer::GetNextColor(size_t& counter) const {
    return settings_->palette[counter % settings_->palette.size()];
}

void MapRenderer::StoreCoordPtr(const geo::Coord* ptr) {
    all_geo_points_.insert(ptr);
}


//=================== SphereProjector ===================//
// Проецирует широту и долготу в координаты внутри SVG-изображения
svg::Point SphereProjector::Transform(geo::Coord coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}
