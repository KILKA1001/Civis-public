#pragma once
#include <cstdint>
#include <memory>
namespace civ5::runtime {
class BnwSelectedWorldViewContextOwner;class BnwWorldViewSourceOperationLoan;class LuaScriptHost;
// Actual selected own Bank/INI numeric operand only, never Original cfg object,
// manager readiness, profile61 return, Controls table or RootRead authority.
class BnwContextLuaProfileConfigLoan final {
    friend class BnwSelectedWorldViewContextOwner;
    friend class LuaScriptHost;
    struct Impl;const std::shared_ptr<const Impl> impl_;
    explicit BnwContextLuaProfileConfigLoan(std::shared_ptr<const Impl>);
    void require_current_without_callbacks(const BnwWorldViewSourceOperationLoan&)const;
    void require_current_owned(const BnwWorldViewSourceOperationLoan&)const;
    std::int32_t logging_enabled_value_owned(const BnwWorldViewSourceOperationLoan&)const;
    std::int32_t debug_library_value_owned(const BnwWorldViewSourceOperationLoan&)const;
public:
    ~BnwContextLuaProfileConfigLoan();
    BnwContextLuaProfileConfigLoan(const BnwContextLuaProfileConfigLoan&)=delete;
    BnwContextLuaProfileConfigLoan&operator=(const BnwContextLuaProfileConfigLoan&)=delete;
};
}
