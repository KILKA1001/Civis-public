#pragma once
#include <array>
#include "civ5/ui/detail/notification_layout_common_child_state.hpp"
#include <bit>
#include <cstdint>
#include <memory>
#include <vector>
#include <utility>
#include <optional>
#include <string>
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationLayoutRuntimeDimensions;
class NotificationLayoutTextureKeeper;
class NotificationLayoutResizeRegistration;
class NotificationLayoutCommonChildFactoryReturn;
class NotificationLayoutBaseline;
// Private constructor DATA; no resource, Style, initializer or paint authority.
class NotificationLayoutGridState final {
    friend class NotificationLayoutBaseline;
    friend class NotificationLayoutRuntimeDimensions;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    static constexpr float zero=std::bit_cast<float>(std::uint32_t{0});
    static constexpr float one=std::bit_cast<float>(std::uint32_t{0x3f800000});
    //5D33C0 writes exactly88..C4; no constructor observation of texture84.
    std::array<float,2> field_88_8c{zero,zero};
    std::array<float,4> color_90_9c{one,one,one,one};
    std::array<float,4> kerning_a0_ac{zero,zero,zero,zero};
    std::array<float,2> increment_b0_b4{zero,zero};
    std::array<float,2> authored_b8_bc{zero,zero};
    std::array<float,2> current_c0_c4{zero,zero};
    //Six literal memset(C8+n*48,0,48) calls; distinct field blocks.
    std::array<std::array<std::uint32_t,18>,6> words_c8_274{};
    enum class Field278 {constructor_null,retained_raw};
    Field278 field_278=Field278::constructor_null;
    std::string texture_key;
    std::shared_ptr<const NotificationLayoutTextureKeeper> texture_keeper;
    std::optional<std::array<std::uint32_t,2>> texture_extent_278;
    bool texture_binding_earned=true; //earned ordinary5D33C0 +278NULL only, not84
    bool parser_returned=false,initialization_entered=false,initialization_returned=false;
    //Actual opaque suffix return and per-layer journal. These closed owners,
    //not parser serial/Boolean, are mandatory before genuine Grid init.
    std::optional<std::size_t> common_layer_;
    std::vector<std::pair<std::size_t,std::shared_ptr<const NotificationLayoutCommonChildFactoryReturn>>> common_layer_returns_;
    std::shared_ptr<const NotificationLayoutCommonChildFactoryReturn> common_return_;
    void initialize_geometry_70f8_owned(NotificationLayoutBaseline&);
    enum class Frontier {constructed,style_required,nine_slice_parser_required,common_initializer_required};
    Frontier frontier=Frontier::constructed;
    bool parser_attempted=false,failed=false,absent_style_returned=false;
    NotificationLayoutGridState()=default;
public:
    ~NotificationLayoutGridState()=default;
};
class NotificationLayoutBoxState final {
    friend class NotificationLayoutBaseline;
    friend class NotificationLayoutRuntimeDimensions;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutParentReceiver;
    static constexpr float one=std::bit_cast<float>(std::uint32_t{0x3f800000});
    std::array<float,4> color_84_90{one,one,one,one}; //58E756..58E76E
    bool color_querying=false,color_stored=false;
    std::uint64_t color_source_serial=0;
    bool common_querying=false,common_context_stored=false,common_full_flags_stored=false;
    std::uint8_t common_size_axes=0; //Retained physical progress only; no returning parser grant.
    bool post_size_querying=false,common_color_stored=false;
    std::shared_ptr<const NotificationLayoutResizeRegistration> resize_registration_{};
    //Issued only by the actual ordered common child factory/attachment producer.
    std::shared_ptr<const NotificationLayoutCommonChildFactoryReturn> common_children_return_{};
    using ID58=NotificationLayoutControlID58;
    //Actual common4F94E4 ctor58NULL; fixed selected native32-byte ID buffer.
    std::shared_ptr<const ID58> id_58{};
    enum class Frontier {constructed,style_required,color_required,common_parser_required,resize_registration_required,common_child_factory_required,common_initializer_required};
    Frontier frontier=Frontier::constructed;
    bool parser_attempted=false,failed=false,absent_style_returned=false;
    NotificationLayoutBoxState()=default;
public:
    ~NotificationLayoutBoxState()=default;
};
}
