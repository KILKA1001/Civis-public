#pragma once
#include "civ5/runtime/detail/context_loader_request_custody.hpp"
#include <memory>
#include <string>
#include <cstdint>
#include <utility>
namespace civ5::runtime {
class BnwCivisOwnedStockSourceBundle;class ScriptUiChildSource;
class BnwSelectedWorldViewContextOwner;class BnwWorldViewSourceOperationLoan;class LuaScriptHost;
namespace detail {
// An operand of the active selected Civis loader. Neither a retail allocation
// receipt nor permission to execute a context on another Source operation.
class ContextLoaderResourceOperand final {
 friend class ::civ5::runtime::BnwSelectedWorldViewContextOwner;
 friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
 friend class ::civ5::runtime::LuaScriptHost;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 const std::shared_ptr<const ContextLoaderRequestCustody> request_;
 const std::shared_ptr<const BnwCivisOwnedStockSourceBundle> bundle_;
 const std::shared_ptr<const ScriptUiChildSource> source_;
 const std::string context_,lua_path_,xml_path_,stem_,lua_sha_,xml_sha_;
 const std::uint64_t serial_,generation_;
 ContextLoaderResourceOperand(std::shared_ptr<const BnwCivisOwnedStockSourceBundle> b,
   std::shared_ptr<const ScriptUiChildSource> s,std::string c,std::string l,std::string x,
   std::string stem,std::string ls,std::string xs,std::uint64_t op,std::uint64_t gen,
   std::shared_ptr<const ContextLoaderRequestCustody> request)
  :request_(std::move(request)),bundle_(std::move(b)),source_(std::move(s)),context_(std::move(c)),lua_path_(std::move(l)),
   xml_path_(std::move(x)),stem_(std::move(stem)),lua_sha_(std::move(ls)),xml_sha_(std::move(xs)),serial_(op),generation_(gen){}
};
}}
