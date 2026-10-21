#pragma once
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/ui/civis_input_policy_owner.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <thread>

namespace civ5::runtime {class LuaScriptHost;class BnwWorldViewSourceOperationLoan;class BnwSelectedWorldViewContextOwner;}
namespace civ5::ui {class BnwClientSceneFrame;class UiScreenGlRenderer;}
namespace civ5::graphics::detail {class BnwClearHexModelBackend;}
namespace civ5::graphics {
// OWN portable HighlightSystem storage. No public factory, event completion,
// Original x86 object identity, model readiness or rendered-frame authority.
class BnwClearHexHighlightOwner final {
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class ::civ5::graphics::detail::BnwClearHexModelBackend;
    friend class ::civ5::runtime::LuaScriptHost;
    friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
    struct State;
    const std::thread::id creator_=std::this_thread::get_id();
    const std::shared_ptr<State> state_;
    explicit BnwClearHexHighlightOwner(std::shared_ptr<State>);
    // Call597190/461400: a C string of at most127 bytes, terminated at127.
    // The unused tail is OWN initialized storage, not a claim about Original's
    // unwritten stack bytes for a null lua_tolstring result.
    struct Payload final {std::array<char,128> style{};};
    struct Queued final {
        std::uint64_t occurrence=0,source_serial=0;
        std::uint32_t record_stride=0x100,record_kind=0x74;
        Payload payload;
    };
    void first_owned()const;
    void require_current_without_callbacks_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void enqueue_owned(const runtime::BnwWorldViewSourceOperationLoan&,const Payload&);
    std::uint64_t begin_drain_owned(const runtime::BnwWorldViewSourceOperationLoan&);
    std::optional<Queued> peek_drain_owned(const runtime::BnwWorldViewSourceOperationLoan&,std::uint64_t)const;
    void dispatch_native_owned(const runtime::BnwWorldViewSourceOperationLoan&,const Queued&);
    void commit_drain_owned(const runtime::BnwWorldViewSourceOperationLoan&,const Queued&);
    void finish_drain_owned(const runtime::BnwWorldViewSourceOperationLoan&,std::uint64_t);
    void quarantine_owned()noexcept;
 public:
    ~BnwClearHexHighlightOwner();
    BnwClearHexHighlightOwner(const BnwClearHexHighlightOwner&)=delete;
    BnwClearHexHighlightOwner&operator=(const BnwClearHexHighlightOwner&)=delete;
};
}
