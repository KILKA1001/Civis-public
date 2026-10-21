#pragma once
#include "civ5/ui/detail/notification_layout_baseline.hpp"
#include <array>
#include <memory>
#include <optional>
#include <string>
namespace civ5::ui::detail {
class NotificationLayoutTextureKeeper;
// Distinct root Image5E4D70, not Button's embedded image or NineSlice.
class NotificationLayoutImageState final {
    friend class NotificationLayoutBaseline;
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutParentReceiver;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::UiScreenGlRenderer;
    enum class TextureOrigin { constructor_null, parsed_pending, retained_raw };
    // Literal embedded40C8F0 at84 and5E4D70 tail. Private constructors alone
    // issue these OWN initial stores; no empty key can issue constructorNULL.
    std::array<float,4> field_88_94{1,0,0,1},color_98_a4{1,1,1,1};
    std::array<float,2> image_a8_ac{},image_b0_b4{},offset_b8_bc{};
    std::uint32_t sampler_c0=10,mask_mode_c4=0,field_d0=0,stretch_e4=0;
    std::array<float,2> uv_d4_d8{},uv_dc_e0{},resource_e8_ec{};
    TextureOrigin texture_origin=TextureOrigin::constructor_null;
    std::string texture_key;
    // Native resource field order114HEIGHT/118WIDTH; distinct fromE8/EC.
    std::optional<std::array<std::uint32_t,2>> texture_extent_c8;
    std::shared_ptr<const NotificationLayoutTextureKeeper> texture_keeper;
    NotificationLayoutImageState()=default;
    static void require_capture_profile_owned(const std::vector<content::XmlNode>&);
    static std::shared_ptr<NotificationLayoutImageState> capture_owned(NotificationLayoutBaseline&,
        const std::vector<content::XmlNode>&,const std::map<std::string,content::XmlNode,std::less<>>&);
    static std::shared_ptr<NotificationLayoutImageState> clone_owned(const std::shared_ptr<NotificationLayoutImageState>&);
    static void coordinates_owned(NotificationLayoutImageState&);
public:
    ~NotificationLayoutImageState()=default;
};
}
