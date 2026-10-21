#pragma once
#include "civ5/content/manifest.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
namespace civ5::ui::detail {
// Values of the own CPU layout. These are not native global/virtual receiver
// observations and cannot construct a LayoutOperandLoan or publish readiness.
struct NotificationCpuGeometry {
    std::uint64_t id=0,parent=0,source_serial=0,effect_revision=0,topology_revision=0,gui_frame_revision=0;
    std::array<float,4> rect{};
    std::array<float,2> offset{},intrinsic_size{};
    std::uint32_t authored_side_flags=0;
    std::string font,font_style,color_set,measurement_source;
    bool hidden=false;
};
struct NotificationCpuLeafInput {
    content::XmlNode effective;
    NotificationCpuGeometry parent;
    std::optional<std::array<double,2>> actual_offset;
    std::optional<float> actual_size_y;
    std::optional<std::array<float,2>> returned_intrinsic;
    std::uint32_t prior_authored_side_flags=0;
    bool hidden=false;
};
// Called with copied canonical attributes and a returned actual GG/resource
// measurement. The returned value grants no source identity or native borrow.
NotificationCpuGeometry notification_cpu_leaf(const NotificationCpuLeafInput&);
std::uint32_t notification_authored_side_flags(std::string_view);
}
