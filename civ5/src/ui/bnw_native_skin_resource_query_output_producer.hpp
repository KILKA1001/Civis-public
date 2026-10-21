#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace civ5::ui { class BnwNativePackageDiscoveryOwner; }
namespace civ5::ui::detail {
class BnwNativeGlobalColorRegistryBank;
class BnwNativeGlobalResourceWarmOwner;
class BnwNativePackageLoadActor;
class BnwNativePackageResourceCheckpoint;
class BnwNativeSelectedCategoryRecordBorrower;
struct BnwNativeArchiveRecordDiagnostic;

// Private NONNULL 54B8D0 / 5EB290 output processor. It has no public factory,
// record/MISS constructor, copied-map admission, phase argument or ready flag.
// Source is deliberately unissued until a genuine full query producer exists.
// The existing prefix selection is NOT a Source and cannot enter this method.
class BnwNativeSkinResourceQueryOutputProducer final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalColorRegistryBank;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativePackageLoadActor;
    friend class BnwNativePackageResourceCheckpoint;
    friend class BnwNativeSelectedCategoryRecordBorrower;
    BnwNativeSkinResourceQueryOutputProducer()=delete;
    class Source {
        friend class BnwNativeSkinResourceQueryOutputProducer;
        friend class BnwNativeGlobalResourceWarmOwner;
        friend class BnwNativePackageLoadActor;
    friend class BnwNativePackageResourceCheckpoint;
    friend class BnwNativeSelectedCategoryRecordBorrower;
        Source()=default;
        Source(const Source&)=delete;
        using Consumer=std::function<void(const BnwNativeArchiveRecordDiagnostic*,
                                         const std::u16string*,const std::string*)>;
        virtual void require_current()const=0;
        // Exactly one callback, including an inspected genuine MISS. All
        // pointers are borrowed from SAME native record / shared prefix / FIRST
        // storage while the actual supplier holds its operation fences. nullptr
        // prefix/FIRST means the actual indexed lookup found no such entry,
        // never "use physical AssetHandle path". No pointers escape the callback.
        virtual void with_native_record(const std::string&,const Consumer&)const=0;
        // SAME receiver+0 after actual pre-load copy, lookup/canonical rewrite
        // and descriptor read. A loaded physical XML path is not this receiver.
        virtual std::string copied_receiver_for_62ec20_fallback()const=0;
        virtual void invalidate()const noexcept=0;
      public:
        virtual ~Source()=default;
    };
    static std::string c_prefix(const std::string&);
    static std::string slash_output(std::string);
    //498940 canonical rewrite does NOT run 5EB290's final81E100 slash pass.
    //Only a SAME held native record/prefix/FIRST borrower may consume this.
    static std::string canonical_filename_839fe0(const std::u16string*,const std::string*);
    static std::string loose_output(const std::u16string*,const std::string*);
    static std::string fallback_output(const std::string&,const std::string&);
    static std::optional<std::string> produce(const std::string&,const Source&);
    // Retained declaration readers can prove empty C-prefix without a query.
    // NONEMPTY input remains unsupported until the genuine Source is issued.
    static std::optional<std::string> produce_without_source(const std::string&);
};
} // namespace civ5::ui::detail
