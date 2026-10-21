#pragma once
#include "bnw_native_global_color_registry_bank.hpp"

namespace civ5::ui::detail {
//Private actual actor query. No public maps/callbacks/ready or MISS issuer.
//Returned borrow never escapes the held bank Scope through this wrapper.
class BnwNativeGlobalResourceQuery final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalColorRegistryBank;
    BnwNativeGlobalResourceQuery()=delete;
    static BnwNativeGlobalColorRegistryBank::GlobalResourceSelection select(
            const std::shared_ptr<BnwNativeGlobalColorRegistryBank>& actual,const char* query) {
        if(!actual||!actual->busy_.load())throw std::runtime_error("Global query requires actual held bank Scope");
        try {return actual->select_global_resource_record(query);}
        catch(...){actual->poison();throw;}
    }
    //Actual output5EB290 is intentionally NOT exposed: Default native +8,
    //loose839FE0/820000/668FF0 and SAME Package FString fallback issuer must
    //be retained before this winner may become a filename/stream supplier.
};
} //namespace civ5::ui::detail
