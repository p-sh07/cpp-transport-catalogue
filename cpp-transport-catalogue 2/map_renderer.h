#pragma once
#include "geo.h"
#include "domain.h"
#include "svg.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coord*
    // NB: container of *Pointers* to geo::Coord !
    template <typename PointPtrInputIt>
    SphereProjector(PointPtrInputIt points_begin, PointPtrInputIt points_end,
                    double max_width, double max_height, double padding);
    
    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point Transform(geo::Coord coords) const;
    
private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

struct RendererSettings {
    //read from json:
    svg::Size img_size;
    double padding = 0.0;
    
    double line_width = 1.0;
    double stop_radius = 0.0;
    
    int bus_label_font_size = 1;
    svg::Point bus_label_offset;
    
    int stop_label_font_size = 1;
    svg::Point stop_label_offset;
    
    svg::Color underlayer_color;
    double underlayer_width = 0.0;
    
    std::vector<svg::Color> palette;
    
    // defaults + set by Renderer:
    svg::Color stroke_color_ = "black";
    svg::Color fill_color_ = svg::NoneColor;
    svg::StrokeLineCap line_cap_ = svg::StrokeLineCap::ROUND;
    svg::StrokeLineJoin line_join_ = svg::StrokeLineJoin::ROUND;
};

class MapRenderer {
public:
    MapRenderer() = default;
    ~MapRenderer() = default;
    
    MapRenderer(RendererSettings* settings);
    
    void LoadSettings(std::shared_ptr<RendererSettings> settings);
    //use after adding all geo::Coords to the renderer
    void MakeProjector();
    
    void AddBus(BusPtr bus);
    
    //void AddStop(std::string_view stop_name, std::string_view bus_name, geo::Coord stop_coords);
        
    void RenderOut(std::ostream& out);
    
private:
    std::shared_ptr<RendererSettings> settings_ = nullptr;
    std::unique_ptr<SphereProjector> projector_ = nullptr;
    
    svg::Color GetNextColor(size_t& counter) const;

    void StoreCoordPtr(const geo::Coord* ptr);
    
    std::unordered_set<const geo::Coord*> all_geo_points_;
    std::set<BusPtr, BusPtrSorter> buses_to_draw_;
    
    void DrawBus(svg::Document& doc, BusPtr bus, svg::Color color);
    //void DrawStop(StopPtr);
    //void DrawLabel(std::string_view text, svg::Point img_pos);
    
    void DrawAllBuses(svg::Document& doc);
};




template <typename PointPtrInputIt>
SphereProjector::SphereProjector(PointPtrInputIt points_begin, PointPtrInputIt points_end,
                                 double max_width, double max_height, double padding)
: padding_(padding) {
    // Если точки поверхности сферы не заданы, вычислять нечего
    if (points_begin == points_end) {
        return;
    }
    
    // Находим точки с минимальной и максимальной долготой
    const auto [left_it, right_it] = std::minmax_element(
                                                         points_begin, points_end,
                                                         [](auto lhs, auto rhs) { return lhs->lng < rhs->lng; });
    min_lon_ = (*left_it)->lng;
    const double max_lon = (*right_it)->lng;
    
    // Находим точки с минимальной и максимальной широтой
    const auto [bottom_it, top_it] = std::minmax_element(
                                                         points_begin, points_end,
                                                         [](auto lhs, auto rhs) { return lhs->lat < rhs->lat; });
    const double min_lat = (*bottom_it)->lat;
    max_lat_ = (*top_it)->lat;
    
    // Вычисляем коэффициент масштабирования вдоль координаты x
    std::optional<double> width_zoom;
    if (!IsZero(max_lon - min_lon_)) {
        width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }
    
    // Вычисляем коэффициент масштабирования вдоль координаты y
    std::optional<double> height_zoom;
    if (!IsZero(max_lat_ - min_lat)) {
        height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }
    
    if (width_zoom && height_zoom) {
        // Коэффициенты масштабирования по ширине и высоте ненулевые,
        // берём минимальный из них
        zoom_coeff_ = std::min(*width_zoom, *height_zoom);
    } else if (width_zoom) {
        // Коэффициент масштабирования по ширине ненулевой, используем его
        zoom_coeff_ = *width_zoom;
    } else if (height_zoom) {
        // Коэффициент масштабирования по высоте ненулевой, используем его
        zoom_coeff_ = *height_zoom;
    }
}
