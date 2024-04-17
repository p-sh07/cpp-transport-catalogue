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

inline const double EPSILON = 1e-6;
bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coord*
    // NB: container of *Pointers* to geo::Coord !
    template <typename PointPtrInputIt>
    SphereProjector(PointPtrInputIt points_begin, PointPtrInputIt points_end,
                    double max_width, double max_height, double padding);
    
    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coord coords) const;
    
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

//Unify applying renderer settings & colors to map objects -> interface class
class MapObject : public svg::Drawable {
public:
    using ColorMap = std::map<std::string_view, svg::Color>;
    
    MapObject(const SphereProjector& sp);
    virtual ~MapObject() = default;
    
    void SetParams(const RendererSettings& settings, const ColorMap& colors);
    void SetProjector(const std::shared_ptr<SphereProjector> projector_);
    
    svg::Point GetImgPoint(geo::Coord pt);
    
protected:
    const std::shared_ptr<SphereProjector> projector_ = nullptr;
    
private:
    virtual void ApplySettings(const RendererSettings& settings, const ColorMap& colors) = 0;
};


//class StopGraphic : public MapObject {
//public:
//    StopGraphic(const SphereProjector& sp, std::string_view stop_name, geo::Coord coords);
//    
//    ~StopGraphic() override = default;
//    
//    void Draw(svg::ObjectContainer& container) const override;
//    
//private:
//    void ApplySettings(const RendererSettings& settings, const ColorMap& colors) override;
//};


class BusGraphic : public MapObject {
    BusGraphic(std::string_view bus_name, const std::vector<StopPtr>& stops, bool is_roundtrip);
    
    ~BusGraphic() override = default;
    
    void Draw(svg::ObjectContainer& container) const override;
private:
    std::string_view bus_name_;
    const std::vector<StopPtr>& stops_;
    bool is_roundtrip_ = false;
    
    svg::Polyline line_;
    
    //Get Stroke-width from settings
    void ApplySettings(const RendererSettings& settings, const ColorMap& colors) override;
};


class MapRenderer {
public:
    MapRenderer() = default;
    ~MapRenderer() = default;
    
    MapRenderer(RendererSettings settings);
    
    void ApplySettings(RendererSettings settings);
    
//    void AddStop(std::string_view stop_name, std::string_view bus_name, geo::Coord stop_coords);
    
    void AddBus(std::string_view bus_name, const std::vector<StopPtr>& stops, bool is_roundtrip);
    
    void RenderOut(std::ostream& out);
    
private:
    RendererSettings settings_;
    mutable bool is_first_color_ = true;
    mutable size_t color_index_ = 0;
    
    svg::Color GetNextColor() const;
    MapObject::ColorMap BuildColorMap() const;
    void StoreBusName(std::string_view bus);
    void StoreCoordPtr(const geo::Coord* ptr);
    
    //add only bus routes with stops in them
    std::vector<std::string_view> all_bus_names_;
    std::vector<const geo::Coord*> all_geo_points_;
    std::vector<std::unique_ptr<MapObject>> objects_to_draw_;
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
