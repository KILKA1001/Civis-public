#pragma once
#include <array>
#include <cstdint>
#include <memory>
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationLayoutButtonState;
class NotificationLayoutRuntimeDimensions;
class NotificationLayoutBaseline;
class NotificationLayoutTextureKeeper;
class NotificationLayoutParentReceiver;
class NotificationLayoutOrdinaryReceiver;
// Actual ported constructor/setter destination. Unwritten words have no value.
// This object grants neither CURRENT invocation nor native32 address authority.
class BnwNormalButtonSubobject118Backing final {
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutButtonState;
    friend class NotificationLayoutBaseline;
    friend class NotificationLayoutParentReceiver;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit BnwNormalButtonSubobject118Backing(std::shared_ptr<Impl>);
    static void execute_constructor548513_state_owned(const NotificationLayoutRuntimeDimensions&,
        const std::shared_ptr<NotificationLayoutButtonState>&,
        const std::shared_ptr<const NotificationLayoutOrdinaryReceiver>&);
    static void execute_constructor40c8f0_owned(const NotificationLayoutRuntimeDimensions&,
        const std::shared_ptr<NotificationLayoutButtonState>&,
        const std::shared_ptr<const NotificationLayoutOrdinaryReceiver>&);
    static void execute_visual_state6b3610_owned(NotificationLayoutButtonState&,
        NotificationLayoutBaseline&,std::uint32_t);
    static void store_state_geometry6b368a_after_6a9610_owned(NotificationLayoutButtonState&,
        NotificationLayoutBaseline&,std::uint32_t);
    static void store_color5cabc0_owned(NotificationLayoutButtonState&,const std::array<float,4>&);
    static void invalidate_direct580780_color_before_store_owned(NotificationLayoutButtonState&);
    static void invalidate_geometry_before_store_owned(NotificationLayoutButtonState&);
    static void invalidate_coordinates_before_store_owned(NotificationLayoutButtonState&);
    static void invalidate_source44_before_store_owned(NotificationLayoutButtonState&);
    static void bind_source44_selected_keeper_owned(NotificationLayoutButtonState&,
        const std::shared_ptr<const NotificationLayoutTextureKeeper>&);
    static void initialize_6f6c13_prefix_owned(NotificationLayoutButtonState&,NotificationLayoutBaseline&);
    static void store_geometry6f6c42_owned(NotificationLayoutButtonState&,NotificationLayoutBaseline&);
    static void restore_origin6f6cd1_owned(NotificationLayoutButtonState&,NotificationLayoutBaseline&);
    static void execute_slot0_4535b0_owned(NotificationLayoutButtonState&);
    static void execute_coordinates5c7b50_owned(NotificationLayoutButtonState&);
    bool require_current_source44_null_branch_without_callbacks()const;
    void require_same_button_storage_without_callbacks(
        const std::shared_ptr<NotificationLayoutButtonState>&)const;
    std::uint32_t require_written_word_without_callbacks(std::uint32_t)const;
public:
    BnwNormalButtonSubobject118Backing(const BnwNormalButtonSubobject118Backing&)=delete;
    BnwNormalButtonSubobject118Backing&operator=(const BnwNormalButtonSubobject118Backing&)=delete;
    ~BnwNormalButtonSubobject118Backing()=default;
};
}
