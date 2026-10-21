#include "bnw_native_primary32_record_snapshot.hpp"
#include <iostream>
#include <type_traits>
using namespace civ5::ui::detail;
namespace {
void need(bool v, const char* m) { if (!v) throw std::runtime_error(m); }
template<class F> void refuses(F&& f) { bool failed = false; try { f(); } catch (const std::exception&) { failed = true; } need(failed, "Unsupported record shape accepted"); }
template<class T> concept PublicAcquire = requires { T::acquire({}); };
template<class T> concept PublicValidatedPrefixIndex = requires(const T& value) {
    value.source_index_after_validation_without_callbacks();
};
template<class T> concept PublicValidatedAdmissionCount = requires(const T& value) {
    value.admitted_count_after_validation_without_callbacks();
};
}
int main() { try {
    static_assert(!std::is_default_constructible_v<BnwNativePrimary32RecordSourceBindings> &&
                  !std::is_copy_constructible_v<BnwNativePrimary32RecordSourceBindings> &&
                  !std::is_default_constructible_v<BnwNativePrimary32RecordSnapshotOwner> &&
                  !PublicAcquire<BnwNativePrimary32RecordSnapshotOwner>);
    static_assert(!PublicValidatedPrefixIndex<BnwNativeGlobalArchivePrefixRecord> &&
                  !PublicValidatedAdmissionCount<BnwNativeArchiveV6EntryAdmissionOwner>);
    const std::string first = "explosion200poundwater-002.wav";
    BnwNativeArchiveRecordDiagnostic source{0x6b70aef3u, 17, 4096, 31u << 20, 28672};
    bnw_primary32_record_shape_diagnostic(source, first, 31);
    const auto a = bnw_default_namespace_ascii_query_diagnostic(u"sounds/explosion200poundwater-002.wav");
    const auto b = bnw_default_namespace_ascii_query_diagnostic(u"art/work_boat_death.dge");
    const std::map<std::uint32_t,BnwNativeArchiveRecordDiagnostic> selected{{source.crc,source}};
    need(a.crc == b.crc && bnw_default_namespace_crc_selection_diagnostic(selected,a) ==
         bnw_default_namespace_crc_selection_diagnostic(selected,b), "CRC-only selected source record changed by spelling");
    need(!bnw_default_namespace_crc_selection_diagnostic(selected,bnw_default_namespace_ascii_query_diagnostic(u"Config.ini")), "Missing archived CRC manufactured hit");
    const auto prepared = [] {
        std::u16string caller = u"sounds/explosion200poundwater-002.wav";
        const auto owned = bnw_primary32_prepare_ascii_query_diagnostic(caller);
        // Mirrors an external callback replacing and destroying the borrowed buffer.
        caller.assign(10000,u'X'); caller.clear(); caller.shrink_to_fit();
        return owned;
    }(); // original string has also been destroyed before selection.
    need(prepared.raw_basename == first && prepared.crc == source.crc &&
         bnw_default_namespace_crc_selection_diagnostic(selected,prepared),
         "Prepared query retained borrowed or changed caller memory");
    refuses([&] { (void)bnw_primary32_prepare_ascii_query_diagnostic(u""); });
    refuses([&] { (void)bnw_primary32_prepare_ascii_query_diagnostic(u"path/"); });
    refuses([&] { (void)bnw_primary32_prepare_ascii_query_diagnostic(u"unicode/\u0430.ini"); });
    refuses([&] { (void)bnw_primary32_prepare_ascii_query_diagnostic(std::u16string(260,u'A')); });
    refuses([&] { (void)bnw_primary32_prepare_ascii_query_diagnostic(std::u16string(128,u'A')); });
    auto bad = source; bad.offset = 0; refuses([&] { bnw_primary32_record_shape_diagnostic(bad,first,31); });
    bad = source; bad.packed_prefix |= 1; refuses([&] { bnw_primary32_record_shape_diagnostic(bad,first,31); });
    refuses([&] { bnw_primary32_record_shape_diagnostic(source,first,30); });
    bad = source; bad.symbolic_slot = 0; refuses([&] { bnw_primary32_record_shape_diagnostic(bad,first,31); });
    bad = source; bad.symbolic_slot = 28673; refuses([&] { bnw_primary32_record_shape_diagnostic(bad,first,31); });
    refuses([&] { bnw_primary32_record_shape_diagnostic(source,"another_filename",31); });
    source.length = 0; bnw_primary32_record_shape_diagnostic(source,first,31); // empty payload != zero loader.
    const std::string zero = "zero_479_nu[w";
    BnwNativeArchiveRecordDiagnostic zero_crc{0,0,14,0,1};
    bnw_primary32_record_shape_diagnostic(zero_crc,zero,0); // actual CRC0 valid.
    std::cout << "PASS closed snapshot bindings/factory, finite native record shape/CRC-only/FIRST/CRC0/zero payload; actual Root issuer pending\n";
    return 0;
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; } }
