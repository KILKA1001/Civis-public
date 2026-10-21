#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
namespace civ5::content {struct XmlNode;}
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
// DATA only: absence is an unobserved byte, never an invented zero.
using NotificationLabelColorSetKnownBytes=std::array<std::optional<std::uint8_t>,64>;
class NotificationContextGuiPort;
class BnwCompletedFontLoan;
class BnwLabelSystemFontBorrower;
class NotificationLayoutBaseline;
class NotificationLayoutButtonState;
class BnwNormalButtonSubobject118Backing;
class NotificationLayoutImageState;
class NotificationLayoutGridState;
class NotificationLayoutContextRealmRef;
class NotificationLayoutContextRealmArgumentLoan;
class NotificationLayoutEmbeddedLabelState;
class NotificationLayoutStandaloneLabelState;
class NotificationLayoutEmbeddedLabelFactoryReturn;
class NotificationLayoutEmbeddedLabelStringLayerLoan;
// A closed parser borrower. Only the real Tree construction issuer can bind
// SAME manager/operation and canonical receiver occurrence. No numerical DTO.
class NotificationLayoutRuntimeDimensions final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutBaseline;
    friend class NotificationLayoutButtonState;
    friend class BnwNormalButtonSubobject118Backing;
    friend class NotificationLayoutImageState;
    friend class NotificationLayoutGridState;
    friend class NotificationLayoutContextRealmArgumentLoan;
    friend class NotificationLayoutEmbeddedLabelState;
    friend class NotificationLayoutStandaloneLabelState;
    friend class NotificationLayoutEmbeddedLabelFactoryReturn;
    friend class NotificationLayoutEmbeddedLabelStringLayerLoan;
    struct State;
    const std::shared_ptr<State> state_;
    explicit NotificationLayoutRuntimeDimensions(std::shared_ptr<State>);
    static std::shared_ptr<NotificationLayoutRuntimeDimensions> capture_owned(
        ::civ5::ui::BnwWorldViewControlTree&,std::uint64_t canonical_control_id,
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<NotificationContextGuiPort>&);
    // Root-owned saved-State suppliers: independent creator/alive/failure FIRST,
    // actual weak Core/node + manager get/two-way control, SAME Source occurrence.
    static void require_storage_without_callbacks(const std::shared_ptr<State>&);
    static std::array<std::int32_t,2> dimensions_from_storage_without_callbacks(const std::shared_ptr<State>&);
    // Genuine registration86AC20 port only; event0 later dispatches typed98.
    // Its body must retain saved storage through COPY/invoke/DTOR and full POST.
    static void register_resize_from_storage_owned(const std::shared_ptr<State>&,std::uint32_t);
    static void fail_storage_without_callbacks(const std::shared_ptr<State>&)noexcept;
    static std::array<float,4> image_color_from_storage_owned(const std::shared_ptr<State>&,std::string_view);
    std::array<float,4> image_color_owned(std::string_view)const;
    void construct_standalone_label_factory_owned()const;
    void enter_standalone_label_slot44_owned()const;
    void run_standalone_label_common_prefix_owned()const;
    void run_standalone_label_common_empty_children_owned()const;
    void require_standalone_label_common_return_from_parser_owned()const;
    std::uint8_t standalone_label_language_owned()const;
    NotificationLabelColorSetKnownBytes standalone_label_color_set_owned(std::string_view)const;
    std::array<float,4> standalone_label_color_owned(std::string_view)const;
    std::string standalone_label_text_owned(std::string_view)const;
    std::shared_ptr<const BnwLabelSystemFontBorrower> standalone_label_system_font_owned(
        std::shared_ptr<const BnwCompletedFontLoan>)const;
    void publish_image_constructor_owned(const std::shared_ptr<NotificationLayoutImageState>&)const;
    void load_image_texture_before_common_owned(const content::XmlNode&)const;
    void store_image_extent_before_common_owned()const;
    void store_image_common_id_58_owned(const content::XmlNode&)const;
    void run_image_common_children_owned(const content::XmlNode&)const;
    void run_ordinary_common_children_owned(const content::XmlNode&)const;
    void publish_box_constructor_owned()const;
    void require_box_common_return_from_parser_owned()const;
    void run_box_color_prefix_owned(const content::XmlNode&,std::size_t)const;
    void run_box_common_prefix_owned(const content::XmlNode&,std::size_t)const;
    void run_box_post_size_prefix_owned(std::size_t)const;
    //Closed ordinary5AAA20 receiver and actual v44/Common consumer only.
    void construct_button_receiver_owned()const;
    void bind_button_texture_before_common_owned(const content::XmlNode&,std::size_t)const;
    void store_button_common_id_58_owned(const content::XmlNode&)const;
    void run_button_common_returning_layer_owned(const content::XmlNode&,std::size_t)const;
    void require_button_common_return_from_parser_owned()const;
    //Closed physical4BDF90 Stack/actual4B02C0 consumer; no generic-kind issuer.
    void publish_stack_constructor_owned()const;
    void enter_stack_slot44_owned()const;
    void require_stack_common_operand_slot_owned(const content::XmlNode&)const;
    bool read_stack_padding_slot_owned(const content::XmlNode&)const;
    void store_stack_common_id_58_owned(const content::XmlNode&)const;
    void require_stack_common_return_from_parser_owned()const;
    // Only the real private Button per-layer PRESENT branch can enter here.
    // No supplied callback, selector-only token issuer, factory or ready grant.
    // Opaque admission is held in this actual parser State, never supplied
    // by XML/Style strings. Returned ordinal only names a retained SAME layer.
    std::optional<std::size_t> run_button_style_prefix_owned(const content::XmlNode&,std::size_t)const;
    void require_grid_parser_owned(const NotificationLayoutGridState*)const;
    std::optional<std::size_t> run_grid_style_prefix_owned(const content::XmlNode&,std::size_t)const;
    void bind_grid_texture_owned(std::size_t actual_layer_ordinal)const;
    void publish_grid_constructor_owned()const;
    void begin_grid_common_empty_layer_owned(std::size_t)const;
    void store_grid_common_id_58_owned(const content::XmlNode&)const;
    void finish_grid_common_empty_layer_owned(std::size_t)const;
    std::uint8_t label_language_980a_owned()const;
    NotificationLabelColorSetKnownBytes label_color_set_owned(std::string_view)const;
    void run_button_string_failed_prefix_owned(const content::XmlNode&,std::size_t)const;
    static void require_button_string_layer_owned(const std::shared_ptr<State>&,
        const std::shared_ptr<const NotificationLayoutEmbeddedLabelStringLayerLoan>&,std::size_t);
    static void retain_pending_label_178_prefix_owned(const std::shared_ptr<State>&,
        const std::shared_ptr<NotificationLayoutEmbeddedLabelState>&,std::size_t);
    static void require_pending_label_178_prefix_owned(const std::shared_ptr<State>&,
        const NotificationLayoutEmbeddedLabelState*,std::size_t);
    void begin_embedded_label_prefix_owned(std::size_t)const; //old failed-prefix entry remains distinct
    std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn> construct_embedded_label_factory_owned(std::size_t)const;
    void publish_embedded_label_factory_owned(const std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn>&,std::size_t)const;
    static void require_label_factory_return_owned(const std::shared_ptr<State>&,
        const std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn>&,std::size_t);
    std::shared_ptr<const BnwLabelSystemFontBorrower> label_system_font_cache_owned(
        std::shared_ptr<const BnwCompletedFontLoan>)const;
    void run_published_label_5e_font_prefix_owned(
        const std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn>&,std::size_t)const;
    static void retire_label_parent_owned(const std::shared_ptr<NotificationLayoutButtonState>&,
        const std::shared_ptr<NotificationContextGuiPort>&)noexcept;
    static void retire_failed_label_owned(const std::shared_ptr<State>&)noexcept;
    static void retire_tree_labels_owned(::civ5::ui::BnwWorldViewControlTree&)noexcept;
    static std::string tooltip_text_from_storage_owned(const std::shared_ptr<State>&,std::string_view);
    static void tooltip_changed_from_storage_owned(const std::shared_ptr<State>&);
    std::string tooltip_text_owned(std::string_view)const;
    void tooltip_changed_owned()const;
    void capture_ordinary_context_argument_owned()const;
    static void require_context_argument_owned(const std::shared_ptr<State>&,
        const std::shared_ptr<const NotificationLayoutContextRealmArgumentLoan>&);
    std::shared_ptr<const NotificationLayoutContextRealmArgumentLoan> context_realm_argument_owned()const;
    void assign_common_context_3c_owned(NotificationLayoutBaseline&,const content::XmlNode&)const;
    float dimension_owned(std::size_t)const;
    void register_resize_owned(std::uint32_t)const;
public:
    ~NotificationLayoutRuntimeDimensions(); // Root clears independent alive FIRST.
    NotificationLayoutRuntimeDimensions(const NotificationLayoutRuntimeDimensions&)=delete;
    NotificationLayoutRuntimeDimensions&operator=(const NotificationLayoutRuntimeDimensions&)=delete;
};
}
