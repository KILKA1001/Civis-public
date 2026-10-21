#pragma once
#include "civ5/content/manifest.hpp"
#include "civ5/ui/detail/bnw_normal_button_subobject118_backing.hpp"
#include "civ5/ui/detail/notification_layout_common_child_state.hpp"
#include "civ5/ui/detail/notification_layout_grid_box_state.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <map>
#include <optional>
#include <string>
#include <vector>
namespace civ5::runtime { class BnwWorldViewNativeContextIssuer; }
namespace civ5::ui { class BnwWorldViewControlTree; class UiScreenGlRenderer; }
namespace civ5::ui::detail {
class BnwPositionedLabelRunLoan;
class NotificationLayoutRuntimeDimensions;
class NotificationContextLoadNameLoan;
class NotificationLayoutContextRealmRef;
class NotificationLayoutParentReceiver;
class NotificationLayoutScrollReceiver;
class NotificationLayoutSliderState;
class NotificationLayoutButtonState;
class NotificationLayoutEmbeddedLabelState;
class NotificationLayoutStandaloneLabelState;
class NotificationLayoutImageState;
class NotificationContextCursorLoadPrefixReturn;
class NotificationContextInitialLuaGcReturn;
class NotificationContextCursorRestoreReturn;

// Per-canonical-node OWN state. No public constructor, numerical submission,
// live native-process pointer or LayoutOperandLoan issuance is exposed.
class NotificationLayoutBaseline final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::runtime::BnwWorldViewNativeContextIssuer;
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class NotificationLayoutParentReceiver;
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutContextOrdinaryState;
    friend class NotificationContextNativeLoadNameInputLoan;
    friend class NotificationContextLoadNameLoan;
    friend class NotificationLayoutContextRealmRef;
    friend class NotificationContextGuiPort;
    friend class NotificationLayoutResizeRegistration;
    friend class NotificationLayoutScrollReceiver;
    friend class NotificationLayoutSliderState;
    friend class NotificationLayoutGridState;
    friend class NotificationLayoutButtonState;
    friend class BnwNormalButtonSubobject118Backing;
    friend class NotificationLayoutEmbeddedLabelState;
    friend class NotificationLayoutStandaloneLabelState;
    friend class BnwStandalonePositionedLabelRunLoan;
    friend class BnwNormalLabelPaintInvocation;
    friend class BnwPositionedLabelRunLoan;
    friend class NotificationLayoutContextConstructionReturn;
    friend class NotificationLayoutImageState;
    friend class NotificationContextCursorLoadPrefixReturn;
    friend class NotificationContextInitialLuaGcReturn;
    friend class NotificationContextCursorRestoreReturn;
    enum class Kind { base, context, stack, scroll_panel, slider, button, container, image, grid, box, label };
    struct Capture;
    struct Base {
        std::array<float,4> bounds_10_1c;
        std::array<float,2> extent_20_24,offset_28_2c,size_30_34,margin_70_74;
        std::uint32_t flags_40;
        //Actual Base4F9470 NULL fields; identities are issued by real subtype construction.
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> receiver_owned{};
        std::weak_ptr<const NotificationLayoutOrdinaryReceiver> parent_38{};
        std::shared_ptr<NotificationLayoutOrdinaryChildren78> children_78{};
        std::shared_ptr<const NotificationLayoutControlID58> id_58{};
        std::shared_ptr<const NotificationLayoutCommonChildFactoryReturn> common_return_owned{};
        //4F94D4 native ctor48=0;4385EA writes raw ToolTipType CRC identity.
        //Hash storage is not a tooltip registry/control/handler-ready token.
        std::uint32_t tooltip_type_48=0;
        //4F94E1 actual ctor54=NULL;5F3B90 owns localized UTF8 buffer (notLabel178).
        std::shared_ptr<const std::string> tooltip_54{};
        //4F94C9 ctor3C=NULL;4381AC parsed secondarg store is a distinct effect.
        enum class Context3CObservation {constructor_null,parsed_realm,root_inherited_null};
        Context3CObservation context_3c_observation=Context3CObservation::constructor_null;
        std::shared_ptr<const NotificationLayoutContextRealmRef> context_realm_3c{};
        //Disengaged until the actual OWN4F9470 body stores raw1 at physical44.
        //No generic Node.alpha/double or constructor snapshot paint authority.
        std::optional<std::uint32_t> alpha_word_44;
    };
    struct Stack {
        std::uint32_t field_84,field_88;
        std::int16_t field_8c;
        float field_90,field_94,field_98,field_9c;
    };
    struct Scroll {
        std::array<float,2> calculated_84_88;
        std::array<float,4> internal_8c_98;
        std::array<float,2> offset_9c_a0;
        float fraction_a4,ratio_a8,range_ac,range_b0,step_b4;
        // Disengaged means no binding observation has been earned. A zero ID
        // inside an engaged array is an actual typed resolver NULL observation.
        std::optional<std::array<std::uint64_t,3>> handles_c8_cc_d0;
    };
    enum class Mutation { construct, parser, hide, offset_prefix, size_prefix, text_prefix, receiver_prefix, receiver_return, consume_mouse_over, context_showhide_handler, context_realm_3c };
    struct Entry {std::uint64_t sequence;Mutation command;};
    const Kind kind_;
    Base base_;
    std::optional<Stack> stack_;
    std::optional<Scroll> scroll_;
    std::shared_ptr<NotificationLayoutSliderState> slider_;
    std::shared_ptr<NotificationLayoutButtonState> button_;
    std::shared_ptr<NotificationLayoutImageState> image_;
    std::shared_ptr<NotificationLayoutStandaloneLabelState> standalone_label_;
    std::shared_ptr<NotificationLayoutGridState> grid_;
    std::shared_ptr<NotificationLayoutBoxState> box_;
    std::vector<Entry> history_;
    std::uint64_t sequence_=0;
    // Styles/receiver storage is earned later by actual construction callbacks;
    // this state is not consumed by existing LayoutOperandLoan.
    std::optional<std::uint64_t> parsed_source_serial_;
    std::string actual_type_;
    std::vector<content::XmlNode> parser_layers_;
    std::optional<std::string> actual_text_command_;
    // Borrowed only during this private staged parser; cleared before publication.
    const NotificationLayoutRuntimeDimensions* parser_runtime_dimensions_=nullptr;
    explicit NotificationLayoutBaseline(Kind,std::string);
    static NotificationLayoutBaseline construct_owned(const content::XmlNode&);
    void construct_grid_box_fields_owned();
    void parse_grid_421d70_owned(const std::vector<content::XmlNode>&);
    void parse_grid_box_prefix_until_required_owner_owned(const std::vector<content::XmlNode>&);
    static Capture capture_owned(std::uint64_t,const content::XmlNode&,
        const std::vector<content::XmlNode>&,
        const std::map<std::string,content::XmlNode,std::less<>>&,std::uint64_t,
        const NotificationLayoutRuntimeDimensions* = nullptr);
    void parse_canonical_owned(const std::vector<content::XmlNode>&,
        const std::map<std::string,content::XmlNode,std::less<>>&,std::uint64_t,
        const NotificationLayoutRuntimeDimensions*);
    static NotificationLayoutBaseline clone_owned(const NotificationLayoutBaseline&);
    void parse_stack_layer_owned(const content::XmlNode&);
    void parse_layer_owned(const content::XmlNode&);
    void finish_parser_owned(std::uint64_t);
    void record_owned(Mutation);
    void hide_owned(bool);
    void offset_prefix_owned(const std::array<double,2>&);
    void size_y_prefix_owned(float);
    void text_prefix_owned(const std::string&);
    // Owned immutable arithmetic origin: bitwise+0, deliberately independent
    // of mutable Original globals05AB6DB0/4. Original runtime equality OPEN.
    static const std::array<float,2>& arithmetic_zero_owned()noexcept;
public:
    NotificationLayoutBaseline(const NotificationLayoutBaseline&)=default;
    NotificationLayoutBaseline(NotificationLayoutBaseline&&)noexcept=default;
    NotificationLayoutBaseline&operator=(const NotificationLayoutBaseline&)=delete;
    NotificationLayoutBaseline&operator=(NotificationLayoutBaseline&&)=delete;
};
struct NotificationLayoutBaseline::Capture {
    std::uint64_t id;
    NotificationLayoutBaseline state;
};
}
