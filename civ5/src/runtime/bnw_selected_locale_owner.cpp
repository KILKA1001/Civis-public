#include "civ5/runtime/bnw_selected_locale_owner.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/runtime/lua_locale_services.hpp"
#include <thread>
namespace civ5::runtime {
namespace {
[[noreturn]] void refuse(BnwSelectedLocaleError code,const char* message){throw BnwSelectedLocaleRefusal(code,message);}
}
struct BnwSelectedLocaleOwner::Impl {
    std::thread::id thread=std::this_thread::get_id();
    std::shared_ptr<const BnwCreationConfig> config;
    const content::RulesDatabase* rules=nullptr;
    std::shared_ptr<const BnwSelectedLanguageInputOwner> selected_input;
    std::shared_ptr<const BnwSelectedLanguageInputOwner> bound_config_input;
    std::unique_ptr<ScriptLocaleSession> locale;
};
BnwSelectedLocaleRefusal::BnwSelectedLocaleRefusal(BnwSelectedLocaleError code,const std::string& message)
    :std::runtime_error(message),code_(code){}
BnwSelectedLocaleOwner::BnwSelectedLocaleOwner(std::shared_ptr<Impl> impl):impl_(std::move(impl)){}
std::shared_ptr<const BnwSelectedLocaleOwner> BnwSelectedLocaleOwner::from_creation_config(
        std::shared_ptr<const BnwCreationConfig> config,std::filesystem::path actual_selected_path) {
    const auto selected=std::filesystem::absolute(actual_selected_path).lexically_normal();
    auto input=BnwSelectedLanguageInputOwner::capture_from_creation_config(config,selected);
    return from_creation_config(std::move(config),std::move(input),selected);
}
std::shared_ptr<const BnwSelectedLocaleOwner> BnwSelectedLocaleOwner::from_creation_config(
        std::shared_ptr<const BnwCreationConfig> config,std::shared_ptr<const BnwSelectedLanguageInputOwner> input,std::filesystem::path same_selected_config_ini) {
    if(!config||!input)refuse(BnwSelectedLocaleError::parent_unavailable,"Locale requires genuine config and selected input");
    config->require_current();input->require_current();
    //A genuine early source cannot silently replace the selected late path.
    //Existing two-argument path API remains unchanged and uses its own source.
    std::error_code error;const auto selected=std::filesystem::canonical(same_selected_config_ini,error);
    if(error||selected!=input->source().config_ini)
        refuse(BnwSelectedLocaleError::wrong_config,"Early language source differs from SAME selected late INI");
    input->require_current();auto p=std::make_shared<Impl>();
    p->config=std::move(config);p->rules=&p->config->game_rules();p->selected_input=std::move(input);
    p->locale=std::make_unique<ScriptLocaleSession>(*p->rules,p->selected_input->language());
    auto result=std::shared_ptr<const BnwSelectedLocaleOwner>(new BnwSelectedLocaleOwner(std::move(p)));
    result->require_current();return result;
}
std::shared_ptr<const BnwSelectedLocaleOwner> BnwSelectedLocaleOwner::from_creation_config_and_early_input(
        std::shared_ptr<const BnwCreationConfig> config,std::shared_ptr<const BnwSelectedLanguageInputOwner> input) {
    if(!config||!input)refuse(BnwSelectedLocaleError::parent_unavailable,"Late locale requires SAME genuine early input/config");
    config->require_current();input->require_early_boot_input();
    const auto retained=config->borrow_selected_language_input();
    if(!retained||retained.get()!=input.get()||retained.owner_before(input)||input.owner_before(retained))
        refuse(BnwSelectedLocaleError::wrong_config,"Late typed locale requires SAME retained CreationConfig input");
    auto p=std::make_shared<Impl>();p->config=std::move(config);p->rules=&p->config->game_rules();p->selected_input=std::move(input);
    p->bound_config_input=retained;
    //Late activated Languages validates the SAME actual early choice again;
    //no config path is invented for the explicit process-selection branch.
    p->locale=std::make_unique<ScriptLocaleSession>(*p->rules,p->selected_input->language());
    auto result=std::shared_ptr<const BnwSelectedLocaleOwner>(new BnwSelectedLocaleOwner(std::move(p)));
    result->require_current();return result;
}
void BnwSelectedLocaleOwner::require_current()const {
    if(impl_->thread!=std::this_thread::get_id())refuse(BnwSelectedLocaleError::wrong_thread,"Selected locale constructor thread changed");
    impl_->config->require_current();if(&impl_->config->game_rules()!=impl_->rules)
        refuse(BnwSelectedLocaleError::wrong_config,"Selected locale rules allocation changed");
    impl_->selected_input->require_current();impl_->config->require_current();
    if(impl_->bound_config_input) {
        const auto retained=impl_->config->borrow_selected_language_input();
        if(!retained||retained.get()!=impl_->bound_config_input.get()||retained.owner_before(impl_->bound_config_input)||impl_->bound_config_input.owner_before(retained))
            refuse(BnwSelectedLocaleError::wrong_config,"Typed locale retained input changed");
    }
}
void BnwSelectedLocaleOwner::require_same_config(const BnwCreationConfig& config)const {
    if(impl_->thread!=std::this_thread::get_id())refuse(BnwSelectedLocaleError::wrong_thread,"Selected locale constructor thread changed");
    if(impl_->config.get()!=&config)refuse(BnwSelectedLocaleError::wrong_config,"Selected locale requires SAME CreationConfig");
    require_current();
}
std::string BnwSelectedLocaleOwner::localized_text(std::string_view tag)const {
    require_current();auto value=impl_->locale->provider().convert_text_key(tag,{});require_current();return value;
}
std::string BnwSelectedLocaleOwner::language()const {require_current();auto value=impl_->selected_input->language();require_current();return value;}
BnwSelectedLocaleSource BnwSelectedLocaleOwner::source()const {require_current();auto value=impl_->selected_input->source();require_current();return value;}
}
