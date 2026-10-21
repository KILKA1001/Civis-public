#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "../ui/bnw_native_pre_dx9_discovery_owner.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include <optional>
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include "civ5/platform/windows_path.hpp"
#include <array>
#include <atomic>
#include <span>
#include <thread>
#include <variant>
#include <type_traits>
#include <clocale>
#include <cctype>
#include <cstring>
#include <locale>
#include <fstream>
#include <algorithm>
#include <map>
#include "civ5/content/manifest.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
namespace civ5::runtime {
namespace {
[[noreturn]] void refuse(BnwSelectedLocaleError code, const char* message) {
    throw BnwSelectedLocaleRefusal(code, message);
}
constexpr std::size_t input_budget = 1024 * 1024;
std::string read_input(const std::filesystem::path& path) {
    std::ifstream stream(platform::extended_windows_path(path), std::ios::binary);
    if (!stream) refuse(BnwSelectedLocaleError::input_unavailable, "Selected language INI cannot be opened");
    std::string result;
    std::array<char, 4096> buffer{};
    while (stream) {
        stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = static_cast<std::size_t>(stream.gcount());
        if (count > input_budget - result.size())
            refuse(BnwSelectedLocaleError::invalid_language_input, "Selected language INI exceeds bounded input");
        result.append(buffer.data(), count);
    }
    if (!stream.eof()) refuse(BnwSelectedLocaleError::input_unavailable, "Selected language INI read failed");
    return result;
}
std::string_view trim(std::string_view text) {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t' || text.front() == '\r')) text.remove_prefix(1);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r')) text.remove_suffix(1);
    return text;
}
bool equal_ascii(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A')) : c; };
        if (lower(a[i]) != lower(b[i])) return false;
    }
    return true;
}
std::string selected_language(std::string_view bytes) {
    if (bytes.find('\0') != bytes.npos)
        refuse(BnwSelectedLocaleError::invalid_language_input, "Selected INI contains unsupported NUL/encoding");
    if (bytes.starts_with("\xef\xbb\xbf")) bytes.remove_prefix(3);
    std::string_view section;
    std::optional<std::string> language;
    while (!bytes.empty()) {
        const auto end = bytes.find('\n');
        const auto line = trim(bytes.substr(0, end));
        if (end == bytes.npos) bytes = {}; else bytes.remove_prefix(end + 1);
        if (line.empty() || line.front() == ';' || line.front() == '#') continue;
        if (line.front() == '[') {
            if (line.back() != ']')
                refuse(BnwSelectedLocaleError::invalid_language_input, "Selected INI section is malformed");
            section = trim(line.substr(1, line.size() - 2));
            continue;
        }
        const auto separator = line.find('=');
        if (separator == line.npos || !equal_ascii(trim(line.substr(0, separator)), "Language")) continue;
        if (language || !equal_ascii(section, "User Settings"))
            refuse(BnwSelectedLocaleError::invalid_language_input, "Selected Language is duplicate or outside User Settings");
        const auto value = trim(line.substr(separator + 1));
        if (value.empty() || value.size() > 63)
            refuse(BnwSelectedLocaleError::invalid_language_input, "Selected Language is empty or unsupported");
        for (const char c : value) {
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '-'))
                refuse(BnwSelectedLocaleError::invalid_language_input, "Selected Language token is unsupported");
        }
        language = std::string(value);
    }
    if (!language) refuse(BnwSelectedLocaleError::invalid_language_input, "Selected INI has no Language input");
    return *language;
}
std::filesystem::path resolved_input(const std::filesystem::path& input) {
    std::error_code error;
    auto path = std::filesystem::canonical(input, error);
    if (error || !std::filesystem::is_regular_file(path, error) || error)
        refuse(BnwSelectedLocaleError::input_unavailable, "Selected INI is not a readable regular file");
    return path;
}
} // namespace
namespace {
//Concrete standalone product source before package parsing/rules. The small
//Languages metadata documents are not Package resource/query authority.
std::wstring current_product_command() {
    const auto* value=GetCommandLineW();if(!value)
        refuse(BnwSelectedLocaleError::input_unavailable,"Actual product command line unavailable");
    std::wstring command(value);if(command.size()>65536)
        refuse(BnwSelectedLocaleError::invalid_language_input,"Product command exceeds bounded selection source");
    return command;
}
struct ProductOptions {std::wstring command;std::optional<std::filesystem::path> settings;std::optional<std::string> language;};
ProductOptions product_options() {
    ProductOptions result;result.command=current_product_command();int count=0;
    auto** arguments=CommandLineToArgvW(result.command.c_str(),&count);
    if(!arguments||count<1||count>4096) {
        if(arguments)LocalFree(arguments);
        refuse(BnwSelectedLocaleError::invalid_language_input,"Actual product argv unavailable/unsupported");
    }
    struct Release{wchar_t** value;~Release(){LocalFree(value);}}release{arguments};
    for(int i=1;i<count;++i) {
        const std::wstring_view option(arguments[i]);
        // Frontend language aliases retain the same actual product argv source.
        if(option!=L"--civis-settings"&&option!=L"--ui-language"&&option!=L"--language")continue;
        if(++i>=count||!arguments[i][0])
            refuse(BnwSelectedLocaleError::invalid_language_input,"Product language option has no value");
        if(option==L"--civis-settings") {
            if(result.settings)refuse(BnwSelectedLocaleError::invalid_language_input,"Duplicate selected product settings path");
            result.settings=std::filesystem::absolute(arguments[i]).lexically_normal();
        }else {
            if(result.language)refuse(BnwSelectedLocaleError::invalid_language_input,"Duplicate product UI language");
            std::string value;
            for(const auto byte:std::wstring_view(arguments[i])) {
                if(byte<1||byte>127)refuse(BnwSelectedLocaleError::invalid_language_input,"Product UI language is not a supported ASCII Type");
                value.push_back(static_cast<char>(byte));
            }
            if(value.empty()||value.size()>63)refuse(BnwSelectedLocaleError::invalid_language_input,"Product UI language Type has unsupported extent");
            result.language=std::move(value);
        }
    }
    if(current_product_command()!=result.command)
        refuse(BnwSelectedLocaleError::input_changed,"Actual product command changed during selection");
    return result;
}
struct ProductLanguageSource {
    struct File {std::filesystem::path requested,canonical;std::string bytes;};
    const std::wstring command;
    const std::filesystem::path game_root,directory;
    const std::vector<File> files;
    const std::vector<ScriptLanguage> available;
    ProductLanguageSource(std::wstring actual,std::filesystem::path root,std::filesystem::path folder,
        std::vector<File> sources,std::vector<ScriptLanguage> choices)
        :command(std::move(actual)),game_root(std::move(root)),directory(std::move(folder)),files(std::move(sources)),available(std::move(choices)){}
    static std::vector<std::filesystem::path> paths(const std::filesystem::path& folder) {
        std::vector<std::filesystem::path> result;
        for(const auto& entry:std::filesystem::directory_iterator(folder)) {
            if(entry.path().extension()!=L".xml"||!entry.is_regular_file())continue;
            if(result.size()>=32)refuse(BnwSelectedLocaleError::invalid_language_input,"Early local Languages metadata file count exceeds budget");
            result.push_back(entry.path());
        }
        std::sort(result.begin(),result.end());return result;
    }
    void require_current()const {
        if(current_product_command()!=command)
            refuse(BnwSelectedLocaleError::input_changed,"Actual product language argv changed");
        std::error_code error;const auto folder=std::filesystem::canonical(game_root/L"Assets/Gameplay/XML/NewText",error);
        if(error||folder!=directory)refuse(BnwSelectedLocaleError::input_changed,"Actual local Languages directory routing changed");
        const auto selected=paths(directory);
        if(selected.size()!=files.size())refuse(BnwSelectedLocaleError::input_changed,"Actual local Languages source set changed");
        for(std::size_t i=0;i<files.size();++i)if(selected[i]!=files[i].requested||
                resolved_input(selected[i])!=files[i].canonical||read_input(files[i].canonical)!=files[i].bytes)
            refuse(BnwSelectedLocaleError::input_changed,"Actual local Languages metadata path/bytes changed");
        if(current_product_command()!=command)
            refuse(BnwSelectedLocaleError::input_changed,"Actual product command retired during Languages check");
    }
    static std::shared_ptr<const ProductLanguageSource> capture(const std::filesystem::path& root,const std::wstring& command) {
        std::error_code error;const auto directory=std::filesystem::canonical(root/L"Assets/Gameplay/XML/NewText",error);
        if(error)refuse(BnwSelectedLocaleError::input_unavailable,"Local Languages metadata directory unavailable");
        std::vector<File> sources;std::map<std::string,ScriptLanguage> choices;std::size_t total=0;
        for(const auto& path:paths(directory)) {
            const auto canonical=resolved_input(path);auto bytes=read_input(canonical);
            if(bytes.size()>2*input_budget-total)refuse(BnwSelectedLocaleError::invalid_language_input,"Local Languages metadata exceeds total budget");
            total+=bytes.size();const auto parsed=content::read_xml_metadata(bytes);
            if(parsed.name!="GameData")refuse(BnwSelectedLocaleError::invalid_language_input,"Local Languages metadata root unsupported");
            for(const auto& table:parsed.children)if(table.name=="Languages")for(const auto& row:table.children) {
                if(row.name!="Row")refuse(BnwSelectedLocaleError::invalid_language_input,"Local Languages row operation unsupported");
                const auto* type=row.child("Type");const auto* name=row.child("Name");const auto* table_name=row.child("TableName");
                if(!type||!name||!table_name||type->text.empty()||name->text.empty()||table_name->text.empty())
                    refuse(BnwSelectedLocaleError::invalid_language_input,"Local Languages row is incomplete");
                if(type->text.size()>63||type->text.find('\0')!=type->text.npos)
                    refuse(BnwSelectedLocaleError::invalid_language_input,"Local Languages Type unsupported");
                for(unsigned char byte:type->text)if(!((byte>='a'&&byte<='z')||(byte>='A'&&byte<='Z')||
                    (byte>='0'&&byte<='9')||byte=='_'||byte=='-'))
                    refuse(BnwSelectedLocaleError::invalid_language_input,"Local Languages Type is not an ASCII locale token");
                if(!choices.emplace(type->text,ScriptLanguage{type->text,name->text}).second)
                    refuse(BnwSelectedLocaleError::invalid_language_input,"Local Languages Type is ambiguous");
            }
            sources.push_back({path,canonical,std::move(bytes)});
        }
        if(choices.empty())refuse(BnwSelectedLocaleError::input_unavailable,"No actual local Languages are available");
        std::vector<ScriptLanguage> available;for(const auto& [type,value]:choices){(void)type;available.push_back(value);}
        auto selected=std::make_shared<const ProductLanguageSource>(command,root,directory,std::move(sources),std::move(available));
        selected->require_current();return selected;
    }
};
std::string actual_ctype_locale() {
    const auto* value=std::setlocale(LC_CTYPE,nullptr);
    if(!value)refuse(BnwSelectedLocaleError::input_unavailable,"Actual standalone CRT CTYPE source unavailable");
    return value;
}
}
struct BnwSelectedLanguageInputOwner::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    const std::string ctype_locale=actual_ctype_locale();
    const std::locale stream_locale=std::locale();
    using Boot=ui::BnwNativeResourceBootSettingsOwner;
    const std::variant<std::weak_ptr<const Boot>,std::weak_ptr<const BnwCreationConfig>> parent;
    struct OwnBankOrigin {
        const std::shared_ptr<assets::CivisResourceBankOwner> bank;
        const std::filesystem::path game_root;
        const std::string recipe_identity;
    };
    const std::optional<OwnBankOrigin> own_bank;
    struct InitialPackageOrigin {
        std::weak_ptr<ui::detail::BnwNativePreDx9DiscoveryOwner> source;
        std::filesystem::path game_root;
    };
    struct InitialSelection {
        std::filesystem::path requested;
        std::shared_ptr<const ProductLanguageSource> product;
        BnwSelectedLocaleSource captured;
        std::string bytes;
    };
    const std::optional<InitialPackageOrigin> initial_package;
    mutable std::optional<InitialSelection> initial_selection;
    mutable std::weak_ptr<const Boot> initial_boot_consumer;
    const std::filesystem::path requested,boot_profile;
    const std::shared_ptr<const ProductLanguageSource> product;
    const BnwSelectedLocaleSource captured;
    const std::string bytes;
    std::weak_ptr<const BnwSelectedLanguageInputOwner> self;
    std::atomic<bool> retired{false},busy{false};
    Impl(std::weak_ptr<const Boot> actual,std::filesystem::path path,BnwSelectedLocaleSource frame,std::string input,
         std::shared_ptr<const ProductLanguageSource> source={},std::filesystem::path profile={})
        :parent(std::move(actual)),requested(std::move(path)),boot_profile(profile.empty()?requested:std::move(profile)),
         product(std::move(source)),captured(std::move(frame)),bytes(std::move(input)){}
    Impl(std::weak_ptr<const BnwCreationConfig> actual,std::filesystem::path path,BnwSelectedLocaleSource frame,std::string input)
        :parent(std::move(actual)),requested(std::move(path)),boot_profile(),product(),captured(std::move(frame)),bytes(std::move(input)){}
    Impl(OwnBankOrigin origin,std::filesystem::path path,BnwSelectedLocaleSource frame,std::string input,
         std::shared_ptr<const ProductLanguageSource> source)
        :parent(std::weak_ptr<const Boot>{}),own_bank(std::move(origin)),requested(std::move(path)),
         boot_profile(),product(std::move(source)),captured(std::move(frame)),bytes(std::move(input)){}
    Impl(InitialPackageOrigin origin)
        :parent(std::weak_ptr<const Boot>{}),initial_package(std::move(origin)),requested(),boot_profile(),
         product(),captured(),bytes(){}
    static void require_initial_current(const std::shared_ptr<Impl>&);
    static void resolve_initial_language(const std::shared_ptr<Impl>&);
    static void require_own_current(const std::shared_ptr<Impl>&);
    void live()const {
        if(thread!=std::this_thread::get_id())refuse(BnwSelectedLocaleError::wrong_thread,"Selected early language foreign thread");
        if(retired.load()||self.expired())refuse(BnwSelectedLocaleError::input_changed,"Selected early language source retired");
    }
    struct Scope {
        const std::shared_ptr<Impl> owned;
        explicit Scope(std::shared_ptr<Impl> actual):owned(std::move(actual)) {
            owned->live();if(owned->busy.exchange(true)){owned->retired.store(true);refuse(BnwSelectedLocaleError::input_changed,"Selected language source reentry");}
        }
        ~Scope(){owned->busy.store(false);}
    };
};
void BnwSelectedLanguageInputOwner::Impl::require_initial_current(const std::shared_ptr<Impl>& owned) {
    Scope operation(owned);
    try {
        if(!owned->initial_package)refuse(BnwSelectedLocaleError::parent_unavailable,"Initial package input has no typed issuer");
        const auto source=owned->initial_package->source.lock();
        if(!source||source.use_count()<=1)refuse(BnwSelectedLocaleError::parent_unavailable,"Initial package platform issuer retired");
        source->require_current();
        if(source->game_root_for_language_source()!=owned->initial_package->game_root)
            refuse(BnwSelectedLocaleError::input_changed,"Initial package platform root changed");
        if(owned->initial_selection) {
            const auto& selection=*owned->initial_selection;
            selection.product->require_current();
            if(selection.captured.kind==BnwSelectedLanguageSourceKind::selected_ini&&
               (resolved_input(selection.requested)!=selection.captured.config_ini||
                read_input(selection.captured.config_ini)!=selection.bytes||
                selected_language(selection.bytes)!=selection.captured.language))
                refuse(BnwSelectedLocaleError::input_changed,"Initial package actual selected language source changed");
        }
        if(actual_ctype_locale()!=owned->ctype_locale||std::locale()!=owned->stream_locale)
            refuse(BnwSelectedLocaleError::input_changed,"Initial package actual CRT/stream locale changed");
        source->require_current();owned->live();
        if(source.use_count()<=1)refuse(BnwSelectedLocaleError::parent_unavailable,"Initial package source lost actual owner during callback");
    }catch(...){owned->retired.store(true);throw;}
}
void BnwSelectedLanguageInputOwner::Impl::resolve_initial_language(const std::shared_ptr<Impl>& owned) {
    require_initial_current(owned);if(owned->initial_selection)return;
    Scope operation(owned);
    try {
        const auto source=owned->initial_package->source.lock();
        if(!source)refuse(BnwSelectedLocaleError::parent_unavailable,"Initial package metadata source retired");
        const auto creation=[&]{
            if(const auto boot=owned->initial_boot_consumer.lock())source->require_actual_boot_consumer(*boot);
            else source->require_metadata_language_observation();
        };
        creation();const auto options=product_options();
        auto product=ProductLanguageSource::capture(owned->initial_package->game_root,options.command);
        InitialSelection staged;staged.product=product;
        if(options.settings) {
            staged.requested=*options.settings;staged.captured.config_ini=resolved_input(staged.requested);
            staged.bytes=read_input(staged.captured.config_ini);staged.captured.language=selected_language(staged.bytes);
            if(options.language&&*options.language!=staged.captured.language)
                refuse(BnwSelectedLocaleError::invalid_language_input,"Actual package language conflicts with actual selected settings");
        }else if(options.language) {
            staged.captured.kind=BnwSelectedLanguageSourceKind::explicit_process_language;
            staged.captured.language=*options.language;
            staged.captured.selection_identity=identify_bytes(std::span<const std::uint8_t>(
                reinterpret_cast<const std::uint8_t*>(options.command.data()),options.command.size()*sizeof(wchar_t)));
        }else throw CivisLanguageSelectionRequired(product->available);
        if(std::none_of(product->available.begin(),product->available.end(),[&](const auto& row){return row.type==staged.captured.language;}))
            refuse(BnwSelectedLocaleError::invalid_language_input,"Actual initial package language is absent from local Languages");
        if(staged.captured.kind==BnwSelectedLanguageSourceKind::selected_ini)
            staged.captured.file=identify_bytes(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(staged.bytes.data()),staged.bytes.size()));
        product->require_current();creation();owned->live();owned->initial_selection.emplace(std::move(staged));
    }catch(...){owned->retired.store(true);throw;}
}
std::shared_ptr<const BnwSelectedLanguageInputOwner> BnwSelectedLanguageInputOwner::capture_from_initial_package_core(
    const std::shared_ptr<ui::detail::BnwNativePreDx9DiscoveryOwner>& source) {
    if(!source)refuse(BnwSelectedLocaleError::parent_unavailable,"Actual initial package Core issuer absent");
    source->require_core_platform_capture();
    auto state=std::make_shared<Impl>(Impl::InitialPackageOrigin{source,source->game_root_for_language_source()});
    auto result=std::shared_ptr<const BnwSelectedLanguageInputOwner>(new BnwSelectedLanguageInputOwner(state));
    state->self=result;result->require_current();source->require_core_platform_capture();return result;
}
void BnwSelectedLanguageInputOwner::bind_actual_initial_boot(const std::shared_ptr<const ui::BnwNativeResourceBootSettingsOwner>& boot)const {
    impl_->live();if(!impl_->initial_package||!boot)
        refuse(BnwSelectedLocaleError::parent_unavailable,"Initial language needs its actual Source23 Boot consumer");
    require_current();const auto source=impl_->initial_package->source.lock();
    if(!source)refuse(BnwSelectedLocaleError::parent_unavailable,"Initial language issuer retired before Boot");
    source->require_actual_boot_consumer(*boot);
    if(const auto previous=impl_->initial_boot_consumer.lock()) {
        if(previous.get()!=boot.get()||previous.owner_before(boot)||boot.owner_before(previous))
            refuse(BnwSelectedLocaleError::input_changed,"Initial language already consumed by another Boot");
    }
    impl_->initial_boot_consumer=boot;require_current();
}
void BnwSelectedLanguageInputOwner::Impl::require_own_current(const std::shared_ptr<Impl>& owned) {
    Scope operation(owned);
    try {
        if(!owned->own_bank||!owned->product)
            refuse(BnwSelectedLocaleError::parent_unavailable,"Own selected language has no genuine Bank/product origin");
        const auto& origin=*owned->own_bank;
        const auto pure_origin=[&] {
            owned->live();const auto canonical=origin.bank->retain_owner();
            if(!canonical||canonical.get()!=origin.bank.get()||canonical.owner_before(origin.bank)||origin.bank.owner_before(canonical))
                refuse(BnwSelectedLocaleError::parent_unavailable,"Own selected language retained Bank control changed");
            origin.bank->require_current();
            const auto recipe=origin.bank->source_recipe();
            if(recipe.game_root!=origin.game_root||recipe.recipe_identity!=origin.recipe_identity||
               owned->product->game_root!=origin.game_root)
                refuse(BnwSelectedLocaleError::input_changed,"Own selected language Bank recipe/root changed");
            owned->live();
        };
        const auto boundary=[&] {
            pure_origin();origin.bank->require_boundary();pure_origin();
        };
        const auto source_checks=[&] {
            pure_origin();owned->product->require_current();pure_origin();
            if(owned->captured.kind==BnwSelectedLanguageSourceKind::selected_ini) {
                if(resolved_input(owned->requested)!=owned->captured.config_ini||
                   read_input(owned->captured.config_ini)!=owned->bytes||
                   selected_language(owned->bytes)!=owned->captured.language)
                    refuse(BnwSelectedLocaleError::input_changed,"Own selected language settings path/bytes changed");
            }else if(owned->captured.kind!=BnwSelectedLanguageSourceKind::explicit_process_language)
                refuse(BnwSelectedLocaleError::input_changed,"Own selected language source kind unsupported");
            if(actual_ctype_locale()!=owned->ctype_locale||std::locale()!=owned->stream_locale)
                refuse(BnwSelectedLocaleError::input_changed,"Own selected language CRT/stream locale changed");
            pure_origin();
        };
        boundary();source_checks();boundary();source_checks();
    }catch(...){owned->retired.store(true);throw;}
}
std::shared_ptr<const BnwSelectedLanguageInputOwner> BnwSelectedLanguageInputOwner::capture_from_civis_resources(
        const std::shared_ptr<assets::CivisResourceBankOwner>& input) {
    const auto supplied=input;
    if(!supplied)refuse(BnwSelectedLocaleError::parent_unavailable,"Own selected language Bank missing");
    // retain_owner checks actual Bank creator before its mutable weak identity.
    const auto bank=supplied->retain_owner();
    if(!bank||bank.get()!=supplied.get()||bank.owner_before(supplied)||supplied.owner_before(bank))
        refuse(BnwSelectedLocaleError::parent_unavailable,"Own selected language Bank control is not canonical");
    bank->require_current();const auto recipe=bank->source_recipe();
    const auto boundary=[&] {
        const auto canonical=bank->retain_owner();
        if(!canonical||canonical.get()!=bank.get()||canonical.owner_before(bank)||bank.owner_before(canonical))
            refuse(BnwSelectedLocaleError::parent_unavailable,"Own selected language capture Bank control changed");
        bank->require_current();const auto current=bank->source_recipe();
        if(current.game_root!=recipe.game_root||current.recipe_identity!=recipe.recipe_identity)
            refuse(BnwSelectedLocaleError::input_changed,"Own selected language Bank origin changed");
        bank->require_boundary();const auto post=bank->retain_owner();
        if(!post||post.get()!=bank.get()||post.owner_before(bank)||bank.owner_before(post))
            refuse(BnwSelectedLocaleError::parent_unavailable,"Own selected language capture Bank control retired");
        bank->require_current();
        const auto after=bank->source_recipe();
        if(after.game_root!=recipe.game_root||after.recipe_identity!=recipe.recipe_identity)
            refuse(BnwSelectedLocaleError::input_changed,"Own selected language Bank origin retired in callback");
    };
    boundary();const auto options=product_options();
    auto product=ProductLanguageSource::capture(recipe.game_root,options.command);
    BnwSelectedLocaleSource frame;std::filesystem::path requested;std::string bytes;
    if(options.settings) {
        requested=*options.settings;frame.config_ini=resolved_input(requested);bytes=read_input(frame.config_ini);
        frame.language=selected_language(bytes);
        if(options.language&&*options.language!=frame.language)
            refuse(BnwSelectedLocaleError::invalid_language_input,"Explicit product language conflicts with own selected settings file");
    }else if(options.language) {
        frame.kind=BnwSelectedLanguageSourceKind::explicit_process_language;frame.language=*options.language;
        const auto raw=std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(options.command.data()),options.command.size()*sizeof(wchar_t));
        frame.selection_identity=identify_bytes(raw);
    }else {
        boundary();product->require_current();bank->require_current();
        throw CivisLanguageSelectionRequired(product->available);
    }
    if(std::none_of(product->available.begin(),product->available.end(),[&](const auto& value){return value.type==frame.language;}))
        refuse(BnwSelectedLocaleError::invalid_language_input,"Own selected UI language is absent from actual local Languages");
    if(frame.kind==BnwSelectedLanguageSourceKind::selected_ini)
        frame.file=identify_bytes(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()));
    product->require_current();boundary();
    auto state=std::make_shared<Impl>(Impl::OwnBankOrigin{bank,recipe.game_root,recipe.recipe_identity},requested,
        std::move(frame),std::move(bytes),std::move(product));
    auto result=std::shared_ptr<const BnwSelectedLanguageInputOwner>(new BnwSelectedLanguageInputOwner(state));
    state->self=result;Impl::require_own_current(state);return result;
}
BnwSelectedLanguageInputOwner::BnwSelectedLanguageInputOwner(std::shared_ptr<Impl> impl):impl_(std::move(impl)){}
BnwSelectedLanguageInputOwner::~BnwSelectedLanguageInputOwner(){impl_->retired.store(true);}
std::shared_ptr<const BnwSelectedLanguageInputOwner> BnwSelectedLanguageInputOwner::capture_from_early_boot(
        const std::shared_ptr<const ui::BnwNativeResourceBootSettingsOwner>& input,
        const std::filesystem::path& actual_profile_config) {
    const auto actual=input;if(!actual)refuse(BnwSelectedLocaleError::parent_unavailable,"Actual early settings source missing");
    const auto profile=actual->selected_language_profile_path_without_parent_callbacks();
    if(profile!=actual_profile_config)refuse(BnwSelectedLocaleError::input_unavailable,"Early language profile differs from SAME Boot");
    const auto options=product_options();
    auto product=ProductLanguageSource::capture(actual->selected_language_game_root_without_parent_callbacks(),options.command);
    BnwSelectedLocaleSource frame;std::filesystem::path requested=profile;std::string bytes;
    const bool optional_original_import=!options.settings&&!options.language;
    if(options.settings) {
        requested=*options.settings;frame.config_ini=resolved_input(requested);bytes=read_input(frame.config_ini);
        frame.language=selected_language(bytes);
        if(options.language&&*options.language!=frame.language)
            refuse(BnwSelectedLocaleError::invalid_language_input,"Explicit product language conflicts with selected settings file");
    }else if(options.language) {
        frame.kind=BnwSelectedLanguageSourceKind::explicit_process_language;frame.language=*options.language;
        const auto raw=std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(options.command.data()),options.command.size()*sizeof(wchar_t));
        frame.selection_identity=identify_bytes(raw); //Actual command identity, not a manufactured file.
    }else {
        //Original profile is an optional read-only import, never a mandatory
        //Steam/default source. Absence or unusable import asks real selection.
        try {frame.config_ini=resolved_input(profile);bytes=read_input(frame.config_ini);frame.language=selected_language(bytes);}
        catch(const BnwSelectedLocaleRefusal&){throw CivisLanguageSelectionRequired(product->available);}
    }
    if(std::none_of(product->available.begin(),product->available.end(),[&](const auto& value){return value.type==frame.language;})) {
        if(optional_original_import)throw CivisLanguageSelectionRequired(product->available);
        refuse(BnwSelectedLocaleError::invalid_language_input,"Selected UI language is absent from actual local Languages");
    }
    if(frame.kind==BnwSelectedLanguageSourceKind::selected_ini)
        frame.file=identify_bytes(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()));
    if(actual->selected_language_profile_path_without_parent_callbacks()!=profile)
        refuse(BnwSelectedLocaleError::input_changed,"Same Boot profile changed during product selection");
    product->require_current();auto state=std::make_shared<Impl>(std::weak_ptr<const Impl::Boot>(actual),requested,
        std::move(frame),std::move(bytes),std::move(product),profile);
    auto result=std::shared_ptr<const BnwSelectedLanguageInputOwner>(new BnwSelectedLanguageInputOwner(state));
    state->self=result;result->require_current();return result;
}
std::shared_ptr<const BnwSelectedLanguageInputOwner> BnwSelectedLanguageInputOwner::capture_from_creation_config(
        const std::shared_ptr<const BnwCreationConfig>& input,std::filesystem::path selected_path) {
    const auto actual=input;if(!actual)refuse(BnwSelectedLocaleError::parent_unavailable,"Actual CreationConfig missing");
    actual->require_current();const auto requested=std::filesystem::absolute(selected_path).lexically_normal();
    BnwSelectedLocaleSource frame;frame.config_ini=resolved_input(requested);const auto bytes=read_input(frame.config_ini);
    frame.language=selected_language(bytes);
    frame.file=identify_bytes(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()));
    actual->require_current();auto state=std::make_shared<Impl>(std::weak_ptr<const BnwCreationConfig>(actual),requested,std::move(frame),bytes);
    auto result=std::shared_ptr<const BnwSelectedLanguageInputOwner>(new BnwSelectedLanguageInputOwner(state));
    state->self=result;result->require_current();return result;
}
void BnwSelectedLanguageInputOwner::require_current()const {
    impl_->live();
    const auto own_state=impl_;
    if(own_state->initial_package){Impl::require_initial_current(own_state);return;}
    if(own_state->own_bank){Impl::require_own_current(own_state);return;}
    const auto owned=impl_;Impl::Scope operation(owned);
    try {
        //Exactly one typed parent facade pin in this operation. Currentness
        //does not call the Parent resource validator during a held Warm loan.
        const auto parent=std::visit([](const auto& weak){return std::variant<std::shared_ptr<const Impl::Boot>,
            std::shared_ptr<const BnwCreationConfig>>(weak.lock());},owned->parent);
        const auto current_parent=[&] {
            std::visit([&](const auto& actual){
                if(!actual||actual.use_count()<=1)refuse(BnwSelectedLocaleError::parent_unavailable,"Selected input lost externally owned actual parent");
                using Actual=typename std::decay_t<decltype(actual)>::element_type;
                if constexpr(std::is_same_v<Actual,const Impl::Boot>) {
                    if(actual->selected_language_profile_path_without_parent_callbacks()!=owned->boot_profile)
                        refuse(BnwSelectedLocaleError::input_changed,"Selected Boot profile routing changed");
                }else actual->require_current();
                owned->live();if(actual.use_count()<=1)refuse(BnwSelectedLocaleError::parent_unavailable,"Selected input parent retired in callback");
            },parent);
        };
        current_parent();
        if(owned->product) {
            owned->product->require_current();
            const auto* boot=std::get_if<std::shared_ptr<const Impl::Boot>>(&parent);
            if(!boot||(*boot)->selected_language_game_root_without_parent_callbacks()!=owned->product->game_root)
                refuse(BnwSelectedLocaleError::input_changed,"Local Languages source differs from SAME Boot game root");
        }
        if(owned->captured.kind==BnwSelectedLanguageSourceKind::selected_ini) {
            if(resolved_input(owned->requested)!=owned->captured.config_ini||read_input(owned->captured.config_ini)!=owned->bytes)
                refuse(BnwSelectedLocaleError::input_changed,"Selected language input path/bytes changed");
            if(selected_language(owned->bytes)!=owned->captured.language)
                refuse(BnwSelectedLocaleError::input_changed,"Selected language no longer belongs to actual retained bytes");
        }else if(!owned->product)
            refuse(BnwSelectedLocaleError::input_changed,"Explicit process language has no actual product selection source");
        current_parent();
        if(actual_ctype_locale()!=owned->ctype_locale||std::locale()!=owned->stream_locale)
            refuse(BnwSelectedLocaleError::input_changed,"Actual standalone CRT/stream locale changed");
    }catch(...){owned->retired.store(true);throw;}
}
#include "bnw_selected_lua_debug_ini_core.inc"
#include "bnw_selected_lua_logging_ini_core.inc"
std::string BnwSelectedLanguageInputOwner::language()const {
    impl_->live();
    if(impl_->initial_package){Impl::resolve_initial_language(impl_);require_current();
        auto value=impl_->initial_selection->captured.language;require_current();return value;}
    const auto own_state=impl_;
    if(own_state->own_bank){
        Impl::require_own_current(own_state);auto value=own_state->captured.language;
        Impl::require_own_current(own_state);return value;
    }
    require_current();auto value=impl_->captured.language;require_current();return value;
}
BnwSelectedLocaleSource BnwSelectedLanguageInputOwner::source()const {
    impl_->live();
    if(impl_->initial_package){Impl::resolve_initial_language(impl_);require_current();
        auto value=impl_->initial_selection->captured;require_current();return value;}
    const auto own_state=impl_;
    if(own_state->own_bank){
        Impl::require_own_current(own_state);auto value=own_state->captured;
        Impl::require_own_current(own_state);return value;
    }
    require_current();auto value=impl_->captured;require_current();return value;
}
void BnwSelectedLanguageInputOwner::require_early_boot_input()const {
    impl_->live();
    if(impl_->initial_package){
        require_current();const auto boot=impl_->initial_boot_consumer.lock();
        const auto source=impl_->initial_package->source.lock();
        if(!source||!boot||boot.use_count()<=1)refuse(BnwSelectedLocaleError::parent_unavailable,"Initial package source has no actual returned Source23 Boot");
        source->require_actual_boot_consumer(*boot);require_current();return;
    }
    const auto own_state=impl_;own_state->live();
    if(own_state->own_bank)
        refuse(BnwSelectedLocaleError::parent_unavailable,"Own Bank language is not an Original early Boot input");
    require_current();if(!std::holds_alternative<std::weak_ptr<const Impl::Boot>>(impl_->parent))
        refuse(BnwSelectedLocaleError::parent_unavailable,"Typed early locale input is not Boot-owned");
    require_current();
}
void BnwSelectedLanguageInputOwner::require_same_early_game_root(const std::filesystem::path& actual_root)const {
    require_early_boot_input();
    if(impl_->initial_package){std::error_code error;const auto actual=std::filesystem::canonical(actual_root,error);
        if(error||actual!=std::filesystem::canonical(impl_->initial_package->game_root))
            refuse(BnwSelectedLocaleError::wrong_config,"Initial selected language belongs to another actual game root");
        require_current();return;}
    std::error_code error;const auto canonical=std::filesystem::canonical(actual_root,error);
    if(error||!impl_->product||canonical!=std::filesystem::canonical(impl_->product->game_root))
        refuse(BnwSelectedLocaleError::wrong_config,"Early selected input differs from actual activated rules game root");
    impl_->product->require_current();require_current();
}
void BnwSelectedLanguageInputOwner::require_same_boot(const ui::BnwNativeResourceBootSettingsOwner& actual)const {
    impl_->live();
    if(impl_->initial_package){require_early_boot_input();const auto boot=impl_->initial_boot_consumer.lock();
        if(!boot||boot.get()!=&actual)refuse(BnwSelectedLocaleError::parent_unavailable,"Initial package language consumed by another Boot");
        const auto source=impl_->initial_package->source.lock();source->require_actual_boot_consumer(actual);require_current();return;}
    const auto own_state=impl_;own_state->live();
    if(own_state->own_bank)
        refuse(BnwSelectedLocaleError::parent_unavailable,"Own Bank language has no Original Boot affinity");
    //Concrete affinity to the same upstream Boot, never a caller path/language.
    require_current();const auto owned=impl_;
    {Impl::Scope operation(owned);
    try {
        const auto* weak=std::get_if<std::weak_ptr<const Impl::Boot>>(&owned->parent);
        if(!weak)refuse(BnwSelectedLocaleError::parent_unavailable,"Package metadata input is not the actual early Boot source");
        const auto parent=weak->lock();
        if(!parent||parent.use_count()<=1||parent.get()!=&actual)
            refuse(BnwSelectedLocaleError::parent_unavailable,"Package metadata input belongs to a different actual Boot");
        if(parent->selected_language_profile_path_without_parent_callbacks()!=owned->boot_profile)
            refuse(BnwSelectedLocaleError::input_changed,"Same Boot selected profile routing changed");
        owned->live();if(parent.use_count()<=1)
            refuse(BnwSelectedLocaleError::parent_unavailable,"Actual selected Boot retired during affinity check");
    }catch(...){owned->retired.store(true);throw;}
    }
    //Release the affinity Scope before the ordinary source bookend.
    require_current();
}
int BnwSelectedLanguageInputOwner::platform_ascii_compare(std::string_view left,std::string_view right)const {
    require_current();for(unsigned char byte:left)if(byte&128)refuse(BnwSelectedLocaleError::invalid_language_input,"Package ASCII compare input is not native835330 value");
    for(unsigned char byte:right)if(byte&128)refuse(BnwSelectedLocaleError::invalid_language_input,"Package ASCII compare selector is not ASCII");
    const auto value=_stricmp(std::string(left).c_str(),std::string(right).c_str());require_current();return value;
}
std::string BnwSelectedLanguageInputOwner::platform_lowercase_ascii(std::string_view text)const {
    require_current();std::string value(text.substr(0,text.find('\0')));
    for(unsigned char byte:value)if(byte&128)
        refuse(BnwSelectedLocaleError::invalid_language_input,"Selected manager basename lacks bounded ASCII source");
    //Actual current standalone CRT _strlwr, not guessed ASCII folding. The
    //native83B4E0 C-prefix stops at NUL; emitted bytes may legitimately be high.
    _strlwr(value.data());require_current();return value;
}
std::string BnwSelectedLanguageInputOwner::platform_uppercase_ascii(std::string_view text)const {
    require_current();std::string value(text);
    for(auto& byte:value) {
        const auto actual=static_cast<unsigned char>(byte);
        if(actual&128)refuse(BnwSelectedLocaleError::invalid_language_input,"Native File writer input lacks835330 ASCII source");
        byte=static_cast<char>(std::toupper(actual));
    }
    require_current();return value;
}
std::locale BnwSelectedLanguageInputOwner::platform_stream_locale()const {
    require_current();auto value=impl_->stream_locale;require_current();return value;
}
} //namespace
