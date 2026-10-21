#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
namespace civ5::ui { class BnwNativeDatabaseValidationSettings; }
namespace civ5::ui::detail {
struct BnwConfigBoolOperation { std::string section,key; bool default_value=false; };
struct BnwConfigBoolDiagnostic {
    bool found=false,value=false,inserted=false;
    bool section_inserted=false;
};
struct BnwConfigEntryDiagnostic { std::string section,key,text;std::int32_t integer=0;float numeric=0; };
// PURE test/source result. It carries no path, existence, namespace, Package,
// lease, generation permission, native cached predicate or readiness.
struct BnwConfigDictionaryDiagnostic {
    bool auto_save=false,dirty=true,load_attempted=false,leaf_complete=false;
    std::uint64_t load_generation=0;
    std::string refusal;
    std::vector<BnwConfigBoolDiagnostic> getters;
    std::vector<BnwConfigEntryDiagnostic> entries;
};
BnwConfigDictionaryDiagnostic diagnose_bnw_config_dictionary(
    std::string_view,std::vector<BnwConfigBoolOperation>);
// Mutable, noncopyable source leaf. ONLY Settings can instantiate/use the
// production bridge; its391 fence and genuine routing must enclose each call.
// No public factory or scalar receipt can bind this to native ConfigLoad.
class BnwNativeConfigDictionary final {
  public:
    ~BnwNativeConfigDictionary()=default;
  private:
    friend class ::civ5::ui::BnwNativeDatabaseValidationSettings;
    friend BnwConfigDictionaryDiagnostic diagnose_bnw_config_dictionary(
        std::string_view,std::vector<BnwConfigBoolOperation>);
    struct Entry {std::string key,text;std::int32_t integer=0;float numeric=0;};
    struct Section {std::string name;std::vector<Entry> entries;};
    std::vector<Section> sections_;
    bool auto_save_=false,dirty_=true,load_attempted_=false,failed_=false;
    std::uint64_t load_generation_=0;
    BnwNativeConfigDictionary()=default;
    BnwNativeConfigDictionary(const BnwNativeConfigDictionary&)=delete;
    BnwNativeConfigDictionary& operator=(const BnwNativeConfigDictionary&)=delete;
    BnwNativeConfigDictionary(BnwNativeConfigDictionary&&)=delete;
    BnwNativeConfigDictionary& operator=(BnwNativeConfigDictionary&&)=delete;
    void load_finite_stream_payload(std::string_view);
    BnwConfigBoolDiagnostic get_bool(const BnwConfigBoolOperation&);
    BnwConfigBoolDiagnostic get_bool_impl(const BnwConfigBoolOperation&);
};
}
