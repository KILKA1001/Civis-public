#pragma once
#include "civ5/ui/detail/notification_layout_baseline.hpp"
#include "civ5/ui/notification_layout_parent_receiver.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <map>
#include <utility>
namespace civ5::ui::detail {
class BnwPositionedLabelRunLoan;
class NotificationLayoutTextureKeeper;
class NotificationLayoutEmbeddedLabelState;
class NotificationLayoutEmbeddedLabelFactoryReturn;
class NotificationLayoutButtonState final {
    friend class NotificationLayoutRuntimeDimensions;
    friend class BnwNormalButtonSubobject118Backing;
    friend class BnwPositionedLabelRunLoan;
    friend class NotificationLayoutEmbeddedLabelState;
    friend class NotificationLayoutBaseline;
    friend class NotificationLayoutScrollReceiver;
    friend class NotificationLayoutParentReceiver;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::UiScreenGlRenderer;
    std::shared_ptr<BnwNormalButtonSubobject118Backing> backing_118_;
    std::uint32_t state_104=0;
    std::array<float,2> authored_108_10c{},increment_110_114{};
    // Actual embedded40C8F0 ctor initializes destination Size to zero.
    std::array<float,2> image_13c_140{};
    //5AAA20->40C8F0 embedded Image; Button+58=5CABC0 writes these four fields.
    std::array<float,4> color_12c_138{1,1,1,1};
    std::array<float,2> image_144_148{},image_14c_150{},uv_168_16c{},uv_170_174{};
    enum class Child178Observation { constructor_null, factory_child };
    // Disengaged is UNKNOWN, never nativeNULL. Earned only by selected ctor+parser.
    std::optional<Child178Observation> child_178_observation;
    // Pending storage is not native178. Published178 remains absent until a
    // genuine full703120 return; this failed-prefix never publishes it.
    std::shared_ptr<NotificationLayoutEmbeddedLabelState> pending_label_178_;
    std::shared_ptr<NotificationLayoutEmbeddedLabelState> label_178_;
    std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn> label_178_factory_return_;
    enum class LabelRetirement {reclaimed,quarantined};
    std::optional<LabelRetirement> label_retirement_; //OWN retirement, not a native178 value
    // 5C7B50 consumer storage is {native114 height,native118 width}.
    std::optional<std::array<std::uint32_t,2>> texture_extent_15c;
    std::shared_ptr<const NotificationLayoutTextureKeeper> texture_keeper;
    bool texture_binding_earned=true; // Actual40C8F0 ctorNULL, parserinvalidates.
    std::string texture_key;
    NotificationLayoutButtonState()=default;
    static std::shared_ptr<NotificationLayoutButtonState> capture_owned(NotificationLayoutBaseline&,
        const std::vector<content::XmlNode>&,const std::map<std::string,content::XmlNode,std::less<>>&);
    static std::shared_ptr<NotificationLayoutButtonState> clone_owned(const std::shared_ptr<NotificationLayoutButtonState>&);
    static void visual_state_owned(NotificationLayoutBaseline&,std::uint32_t);
    static void coordinates_owned(NotificationLayoutButtonState&);
public:
    ~NotificationLayoutButtonState()=default;
};
// Storage belongs to the actual canonical slider baseline. Embedded native
// Shuttle(+12C) and Track(+3A8) are distinct owned subcontrols, not Lua IDs.
class NotificationLayoutSliderState final {
    friend class NotificationLayoutBaseline;
    friend class NotificationLayoutScrollReceiver;
    friend class NotificationLayoutParentReceiver;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::UiScreenGlRenderer;
    struct NineSlice {
        NotificationLayoutBaseline baseline;
        std::array<float,4> kerning_a0_ac{};
        std::array<float,2> increment_b0_b4{},authored_b8_bc{},current_c0_c4{};
        std::array<float,18> position_c8_10c{},size_110_154{},texture_start_158_19c{},texture_end_1a0_1e4{};
        std::array<float,18> uv_1e8_22c{},uv_230_274{};
        std::array<float,4> color_90_9c{1,1,1,1};
        // 4B0E90 consumer storage is {native118 width,native114 height}.
        std::optional<std::array<std::uint32_t,2>> texture_extent_278;
        std::shared_ptr<const NotificationLayoutTextureKeeper> texture_keeper;
        bool texture_binding_earned=true; // Constructor's actual NULL; parser invalidates it.
        std::string texture_key;
        explicit NineSlice(std::string);
    };
    NineSlice shuttle_,track_;
    float fraction_128,steps_628;
    std::int16_t field_624=0,travel_626=0,gutter_62c=0;
    std::uint32_t state_104=0;
    bool initialization_returned_=false;
    bool initialization_entered_=false;
    bool shuttle_parent_earned_=false;
    explicit NotificationLayoutSliderState();
    static std::shared_ptr<NotificationLayoutSliderState> capture_owned(NotificationLayoutBaseline&,
        const std::vector<content::XmlNode>&,const std::map<std::string,content::XmlNode,std::less<>>&);
    static std::shared_ptr<NotificationLayoutSliderState> clone_owned(const std::shared_ptr<NotificationLayoutSliderState>&);
    void parse_layer_owned(NotificationLayoutBaseline&,const content::XmlNode&);
    static void parse_nine_slice_layer_owned(NineSlice&,const content::XmlNode&);
    static void recache_owned(NineSlice&,bool update_coordinates=true);
    static void texture_coordinates_owned(NineSlice&);
    static void set_visual_state_owned(NineSlice&,std::uint32_t);
    static void initialize_nine_slice_owned(NineSlice&,const NotificationLayoutBaseline*);
public:
    ~NotificationLayoutSliderState()=default;
};

class NotificationLayoutScrollReceiver final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutParentReceiver;
    using Parent=NotificationLayoutParentReceiver;
    using View=Parent::TreeView;
    using Slider=NotificationLayoutSliderState;
    static void slot4c(View&,std::uint64_t);
    // Body completion is a closed pending receipt, never initialized state.
    class InitializationReturn final {
        friend class NotificationLayoutScrollReceiver;
        const std::shared_ptr<View::Binding> binding_;
        const Parent::NodeKey key_;
        std::shared_ptr<Slider> state_;
        const std::uint64_t baseline_sequence_;
        InitializationReturn(std::shared_ptr<View::Binding> binding,Parent::NodeKey key,
            std::shared_ptr<Slider> state,std::uint64_t sequence)
            :binding_(std::move(binding)),key_(key),state_(std::move(state)),baseline_sequence_(sequence){}
        InitializationReturn(const InitializationReturn&)=delete;
        InitializationReturn& operator=(const InitializationReturn&)=delete;
        InitializationReturn& operator=(InitializationReturn&&)=delete;
    public:
        InitializationReturn(InitializationReturn&&)noexcept=default;
        ~InitializationReturn()=default;
    };
    static InitializationReturn initialize_slider_owned(View&,std::uint64_t);
    // Defined after complete Binding. Only genuine Root initializer calls this
    // after successful View destruction; a receipt cannot be defaulted/replayed.
    static void publish_slider_initialization_owned(InitializationReturn&);
    static void set_size_y(View&,std::uint64_t,float);
    static void internal_size(View&,std::uint64_t);
    static void slider_set_length(View&,std::uint64_t,float);
    static void slider_set_ratio(View&,std::uint64_t,float);
    static void slider_set_fraction(View&,std::uint64_t,float);
    static void slider_slot88(View&,std::uint64_t,bool);
    static void slot88(View&,std::uint64_t,bool);
    static void reciprocal_steps(View&,std::uint64_t,float);
    NotificationLayoutScrollReceiver()=delete;
};
}
