#pragma once
#include <string>
#include <stdexcept>
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"

namespace civ5::ui { class BnwNativePackageDiscoveryOwner; }
namespace civ5::ui::detail {
// Actual fixed startup callers, independent of any Package XML/FString input.
// Only the Package's Default/Common operation can construct this source. The
// native debug-byte operand is deliberately not inferred from high quality.
class BnwNativeDefaultLooseQueryFolder final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    enum class Caller {overrides,assets,shared_speech};
    const Caller caller_;
    const BnwNativeResourceBootSettingsOwner& settings_;
    const BnwNativePackageDiscoveryOwner& parent_;
    explicit BnwNativeDefaultLooseQueryFolder(Caller c,const BnwNativeResourceBootSettingsOwner& settings,
        const BnwNativePackageDiscoveryOwner& parent):caller_(c),settings_(settings),parent_(parent){}
    std::string original_query()const {
        switch(caller_) {
            case Caller::overrides:return "Resource\\Overrides";
            case Caller::assets:return "Assets";
            case Caller::shared_speech:return "Assets\\Sounds\\Speech\\Shared";
        }
        throw std::runtime_error("Unknown genuine Default loose caller");
    }
    unsigned native_mode()const {
        if(caller_==Caller::overrides)return 4; //54CC94 PUSH4 before82A470.
        //Actual54CAE0 negative retained command branch skips54CB4B's write1.
        //Only SAME fresh settings owner can read its private loader-zero operand.
        return settings_.normal_default_loose_mode(parent_);
    }
    bool base_filters()const noexcept{return caller_==Caller::assets;}
    bool xml_override()const noexcept{return caller_==Caller::assets;}
};
} // namespace civ5::ui::detail
