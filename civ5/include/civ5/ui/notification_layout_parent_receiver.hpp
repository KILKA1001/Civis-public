#pragma once
#include <cstdint>
#include <array>
#include <memory>
#include <optional>
#include <span>
#include <vector>
#include <deque>
namespace civ5::runtime { class BnwWorldViewNativeContextIssuer; }
namespace civ5::ui { class BnwWorldViewControlTree; class UiScreenGlRenderer; }
namespace civ5::ui::detail {
class NotificationLayoutBaseline;
class NotificationLayoutScrollReceiver;
class NotificationLayoutSliderState;
class NotificationLayoutEmbeddedLabelState;
class NotificationLayoutStandaloneLabelState;
// Private canonical state receiver, never a public rectangle or return grant.
// TreeView borrows actual node state only inside the Tree's retained Core and
// entered source operation. The enclosing Tree owns all callback bookends.
class NotificationLayoutParentReceiver final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::runtime::BnwWorldViewNativeContextIssuer;
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class NotificationLayoutScrollReceiver;
    friend class NotificationLayoutSliderState;
    friend class NotificationLayoutEmbeddedLabelState;
    friend class NotificationLayoutStandaloneLabelState;
    // Earned by the actual selected subtype constructor/vtable supplier.
    // Common base parser capture alone does not issue this observation.
    enum class BaseDispatch { base_4c_and_94 };
    // Domain is a closed Binding slot for an actual retained Core. It is not
    // a public Control ID: different canonical contexts can both contain id0.
    struct NodeKey {
        std::uint64_t domain,id;
        bool operator==(const NodeKey&)const=default;
    };
    struct NodeView {
        NodeKey key;
        std::uint64_t attached_order;
        std::optional<NodeKey> parent;
        NotificationLayoutBaseline* state;
        std::optional<BaseDispatch> base_dispatch;
    };
    struct Store {
        enum class Kind { base_anchor, stack_anchor, stack_size, bounds_union, scroll_size, scroll_internal, slider, initializer_flags, button, image, grid, standalone_label, ordinary_offset, ordinary_anchor, ordinary_hide, ordinary_no_clip, ordinary_size, ordinary_color, ordinary_alpha, ordinary_consume_mouse_over, ordinary_consume_mouse_button, ordinary_consume_mouse_wheel, ordinary_disabled, ordinary_disabled_hover_clear, ordinary_modal, ordinary_tooltip_callback, ordinary_tooltip_enable, ordinary_tooltip_type, ordinary_tooltip_text, ordinary_tooltip_clear, ordinary_children_slot, ordinary_children_count, ordinary_children_null };
        NodeKey key;
        Kind kind;
    };
    struct DerivedChildPosition {
        NodeKey stack,child;
        std::array<float,2> position;
    };
    struct FourCReturn {
        NodeKey key;
        std::uint64_t baseline_sequence;
        std::array<float,2> actual_offset;
        std::uint64_t offset_store_sequence;
    };
    class TreeView final {
        friend class NotificationLayoutParentReceiver;
        friend class NotificationLayoutScrollReceiver;
        friend class NotificationLayoutEmbeddedLabelState;
        friend class NotificationLayoutStandaloneLabelState;
        friend class ::civ5::ui::BnwWorldViewControlTree;
        friend class ::civ5::ui::UiScreenGlRenderer;
        struct Binding;
        const std::shared_ptr<Binding> binding_;
        mutable std::deque<NodeView> nodes_;
        std::vector<Store> stores_;
        std::vector<DerivedChildPosition> child_positions_;
        std::vector<FourCReturn> four_c_returns_;
        std::vector<NodeKey> parent_path_, anchor_path_;
        TreeView(std::shared_ptr<Binding>,std::vector<NodeView>);
        ~TreeView()noexcept;
        void require_pure()const;
        void require_children_owned(NodeKey)const;
        void require_box_native_common_return_owned(NodeKey)const;
        void require_full()const;
        std::array<float,2> arithmetic_pair_owned()const;
        void preflight_store_owned(NodeKey)const;
        void record_store_owned(NodeKey,Store::Kind);
        void record_four_c_return_owned(NodeKey);
        // Root-owned actual GUI/Win32 suffix. Declarations are dependencies,
        // never an inferred successful no-op or a boolean grant.
        void post_mouse_rehit_owned(std::uint64_t actual_receiver_id);
        void disable_hovered_receiver_owned(std::uint64_t actual_receiver_id);
        void initialize_slider_textures_owned(std::uint64_t actual_receiver_id);
        void initialize_button_texture_owned(std::uint64_t actual_receiver_id);
        void initialize_image_texture_owned(NodeKey actual_receiver);
        void scroll_fraction_diagnostic_owned(std::uint64_t actual_receiver_id,float actual_fraction);
        TreeView(const TreeView&)=delete;
        TreeView& operator=(const TreeView&)=delete;
        TreeView(TreeView&&)=delete;
        TreeView& operator=(TreeView&&)=delete;
    };
    static const NodeView& node(const TreeView&,NodeKey);
    static std::vector<NodeKey> children(const TreeView&,NodeKey);
    static void base_anchor(TreeView&,NodeKey);
    // Embedded slider controls use the same actual Base state arithmetic;
    // their owning typed Scroll receiver supplies its genuine enclosing scope.
    static std::array<float,2> base_anchor_coordinates(
        const NotificationLayoutBaseline&,const NotificationLayoutBaseline*);
    static std::array<float,4> base_extent_bounds(
        const NotificationLayoutBaseline&,std::span<const NotificationLayoutBaseline*const>);
    static void stack_anchor(TreeView&,NodeKey);
    static void stack_calculate(TreeView&,NodeKey);
    static void base_extent94(TreeView&,NodeKey);
    static void slot94(TreeView&,NodeKey);
    static void slot4c(TreeView&,NodeKey);
    static void reprocess_anchoring(TreeView&,NodeKey);
    // Existing selected-domain Scroll receiver calls retain their local-ID
    // spelling; cross-context traversal always uses the closed qualified key.
    static const NodeView& node(const TreeView&v,std::uint64_t id){return node(v,NodeKey{0,id});}
    static std::vector<NodeKey> children(const TreeView&v,std::uint64_t id){return children(v,NodeKey{0,id});}
    static void base_anchor(TreeView&v,std::uint64_t id){base_anchor(v,NodeKey{0,id});}
    static void slot94(TreeView&v,std::uint64_t id){slot94(v,NodeKey{0,id});}
    static void reprocess_anchoring(TreeView&v,std::uint64_t id){reprocess_anchoring(v,NodeKey{0,id});}
    NotificationLayoutParentReceiver()=delete;
};
}
