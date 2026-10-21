#pragma once
#include <memory>
#include <string>
#include <utility>
#include "civ5/content/manifest.hpp"
namespace civ5::ui {class BnwWorldViewControlTree;}
namespace civ5::runtime {class BnwSelectedWorldViewContextOwner;namespace detail {
// Actual ported selected loader request captured BEFORE AssetKey conversion.
// It is request DATA/custody, not an Original invocation or load return.
class ContextLoaderRequestCustody final {
 friend class ::civ5::runtime::BnwSelectedWorldViewContextOwner;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 enum class Origin {selected_direct_request,retained_lua_context_declaration};
 const Origin origin_;
 const std::string context_,original_utf8_;
 const std::shared_ptr<const content::XmlNode> declaration_;
 ContextLoaderRequestCustody(std::string context,std::string request,std::shared_ptr<const content::XmlNode> declaration)
  :origin_(declaration?Origin::retained_lua_context_declaration:Origin::selected_direct_request),
   context_(std::move(context)),original_utf8_(std::move(request)),declaration_(std::move(declaration)){}
};
}}
