#pragma once
#include <memory>
#include <string>
#include <vector>
namespace civ5::runtime {
class LuaScriptHost;
class BnwWorldViewSourceOperationLoan;
// Actual selected Source/Bank ordered collection only. No caller names/path,
// sorted keys or per-context compiler permission set can issue this loan.
class BnwContextLuaIncludeCollectionLoan final {
    friend class LuaScriptHost;
    friend class BnwWorldViewSourceOperationLoan;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit BnwContextLuaIncludeCollectionLoan(std::shared_ptr<Impl>);
    void require_current_without_callbacks(const BnwWorldViewSourceOperationLoan&)const;
    void require_current_owned(const BnwWorldViewSourceOperationLoan&)const;
    std::vector<std::string> ordered_names_owned(const BnwWorldViewSourceOperationLoan&)const;
public:
    ~BnwContextLuaIncludeCollectionLoan();
    BnwContextLuaIncludeCollectionLoan(const BnwContextLuaIncludeCollectionLoan&)=delete;
    BnwContextLuaIncludeCollectionLoan&operator=(const BnwContextLuaIncludeCollectionLoan&)=delete;
};
}
