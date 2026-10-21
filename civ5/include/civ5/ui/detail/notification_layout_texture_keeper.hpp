#pragma once
#include "civ5/graphics/detail/notification_raw_ui_texture_source.hpp"
#include <atomic>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class UiScreenGlRenderer;class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class NotificationLayoutParentReceiver;
class NotificationLayoutRuntimeDimensions;
class NotificationLayoutRuntimeDimensions;
class NotificationLayoutGridState;
class NotificationLayoutResourceScope;
class BnwNormalButtonSubobject118Backing;
// Immutable retained CPU raw pair, not a GL handle, initializer return or
// caller-supplied texture rectangle. Only the genuine renderer creates it.
class NotificationLayoutTextureKeeper final {
    friend class NotificationLayoutRuntimeDimensions;
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutResourceScope;
    friend class NotificationLayoutParentReceiver;
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutGridState;
    friend class BnwNormalButtonSubobject118Backing;
    const graphics::detail::NotificationRawUiTextureSource source_;
    const std::string key_;
    // Native physical fields: 114=height, 118=width. Consumer arrays below
    // are separately ordered: NineSlice278={118,114}, Button15C={114,118}.
    const std::array<std::uint32_t,2> native_114_118_;
    const std::uint64_t entry_revision_;
    const std::thread::id creator_=std::this_thread::get_id();
    mutable std::weak_ptr<const NotificationLayoutTextureKeeper> canonical_;
    NotificationLayoutTextureKeeper(graphics::detail::NotificationRawUiTextureSource source,
        std::string key,std::array<std::uint32_t,2> fields,std::uint64_t revision)
      :source_(std::move(source)),key_(std::move(key)),native_114_118_(fields),entry_revision_(revision){}
    static void require_canonical(const std::shared_ptr<const NotificationLayoutTextureKeeper>& supplied){
        if(!supplied||supplied->creator_!=std::this_thread::get_id())
            throw std::runtime_error("Layout raw texture keeper absent/foreign creator");
        {
            const auto actual=supplied->canonical_.lock();
            if(!actual||actual.get()!=supplied.get()||actual.owner_before(supplied)||supplied.owner_before(actual))
                throw std::runtime_error("Layout raw texture keeper forged or retired control");
        }
        if(supplied->canonical_.expired())throw std::runtime_error("Layout raw texture keeper retired after checker release");
    }
public:
    NotificationLayoutTextureKeeper(const NotificationLayoutTextureKeeper&)=delete;
    NotificationLayoutTextureKeeper&operator=(const NotificationLayoutTextureKeeper&)=delete;
    ~NotificationLayoutTextureKeeper()=default;
};
// This closed active lender is minted only inside the actual renderer's
// SourceOwner callback, BEFORE immutable paint revisions. No public dt/bool,
// resources table, matrix, size, SourceLoan or renderer pointer is admitted.
class NotificationLayoutResourceScope final {
    friend class NotificationLayoutRuntimeDimensions;
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationLayoutParentReceiver;
    friend class NotificationLayoutRuntimeDimensions;
    friend class NotificationLayoutGridState;
    friend class BnwNormalButtonSubobject118Backing;
    using Operation=runtime::BnwWorldViewSourceOperationLoan;
    using Tree=::civ5::ui::BnwWorldViewControlTree;
    struct State {
        const std::thread::id creator=std::this_thread::get_id();
        std::atomic<bool> alive{true},failed{false};
        bool full_entered=false,loading=false;
        void first()const{
            if(creator!=std::this_thread::get_id()||!alive.load()||failed.load())
                throw std::runtime_error("Layout resource lender retired, failed or foreign creator");
        }
    };
    const std::shared_ptr<State> state_=std::make_shared<State>();
    // All targets are fixed renderer closures. They cannot be supplied through
    // a public factory or a Tree method by application callers.
    const std::function<void(const Operation&,const Tree*)> pure_;
    const std::function<void()> full_;
    const std::function<std::shared_ptr<const NotificationLayoutTextureKeeper>(const std::string&)> load_;
    const std::function<void(const std::shared_ptr<const NotificationLayoutTextureKeeper>&)> entry_;
    NotificationLayoutResourceScope(std::function<void(const Operation&,const Tree*)> pure,
        std::function<void()> full,
        std::function<std::shared_ptr<const NotificationLayoutTextureKeeper>(const std::string&)> load,
        std::function<void(const std::shared_ptr<const NotificationLayoutTextureKeeper>&)> entry)
      :pure_(std::move(pure)),full_(std::move(full)),load_(std::move(load)),entry_(std::move(entry)){}
    void require_pure(const Operation& operation,const Tree* tree)const{
        const auto saved=state_;saved->first();
        try{pure_(operation,tree);saved->first();}catch(...){saved->failed.store(true);throw;}
    }
    void require_full(const Operation& operation,const Tree* tree)const{
        const auto saved=state_;saved->first();
        if(saved->full_entered){saved->failed.store(true);throw std::runtime_error("Layout resource lender full callback reentry");}
        saved->full_entered=true;struct End{std::shared_ptr<State>s;~End(){s->full_entered=false;}}end{saved};
        try{require_pure(operation,tree);full_();saved->first();require_pure(operation,tree);}
        catch(...){saved->failed.store(true);throw;}
    }
    std::shared_ptr<const NotificationLayoutTextureKeeper> load_owned(
        const Operation& operation,const Tree* tree,const std::string& key)const{
        const auto saved=state_;saved->first();
        if(saved->loading){saved->failed.store(true);throw std::runtime_error("Layout raw source factory reentry");}
        saved->loading=true;struct End{std::shared_ptr<State>s;~End(){s->loading=false;}}end{saved};
        try{
            require_pure(operation,tree);auto actual_key=key;
            if(actual_key.empty())throw std::runtime_error("Parsed layout Texture key absent");
            auto owner=load_(actual_key);saved->first();require_pure(operation,tree);
            NotificationLayoutTextureKeeper::require_canonical(owner);entry_(owner);saved->first();
            require_pure(operation,tree);return owner;
        }catch(...){saved->failed.store(true);throw;}
    }
    void require_entry(const Operation& operation,const Tree* tree,
        const std::shared_ptr<const NotificationLayoutTextureKeeper>& owner)const{
        const auto saved=state_;saved->first();
        try{require_pure(operation,tree);NotificationLayoutTextureKeeper::require_canonical(owner);
            entry_(owner);saved->first();require_pure(operation,tree);}
        catch(...){saved->failed.store(true);throw;}
    }
    void retire_without_callbacks()noexcept{state_->alive.store(false);}
public:
    NotificationLayoutResourceScope(const NotificationLayoutResourceScope&)=delete;
    NotificationLayoutResourceScope&operator=(const NotificationLayoutResourceScope&)=delete;
    ~NotificationLayoutResourceScope(){state_->alive.store(false);}
};
}
