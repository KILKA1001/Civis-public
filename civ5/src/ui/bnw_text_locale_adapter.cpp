#include "civ5/ui/bnw_text_locale_adapter.hpp"
namespace civ5::ui {
BnwLabelLocaleSelectionProvider bnw_text_policy_selected_locale(
 std::shared_ptr<SelectedCivilopediaLocale> locale,std::shared_ptr<CivisLocaleTextOwner> text,
 CivilopediaLayoutOwnerKey identity){
 if(!locale||!text||identity.owner_token.empty()||identity.frame_number==0)
  throw runtime::ScriptError("Selected text-policy locale owners/identity absent");
 auto getter=[locale=std::move(locale),text=std::move(text),identity=std::move(identity)]()->std::optional<BnwLabelLocaleSelection>{
  if(!locale->current(identity))throw runtime::ScriptError("Selected text-policy locale identity is stale");
  const auto language=locale->provider().current().type;
  // provider() validates retained NLS version, locale binding, lifetime and
  // owning thread; policy() alone is deliberately not a freshness proof.
  (void)text->provider();
  if(language!=text->policy().language||language.empty())
   throw runtime::ScriptError("Selected text-policy language owners disagree");
  if(!locale->current(identity)||locale->provider().current().type!=language)
   throw runtime::ScriptError("Selected text-policy locale changed during query");
  return BnwLabelLocaleSelection{language,identity.owner_token,identity.frame_number};
 };
 (void)getter();return getter;
}
}
