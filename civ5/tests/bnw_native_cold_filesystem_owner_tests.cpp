#include "bnw_native_cold_filesystem_owner.hpp"
#include "civ5/ui/detail/bnw_native_original_stat_path.hpp"
#include <iostream>
#include <limits>
#include <type_traits>
using namespace civ5::ui::detail;
namespace {
void need(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
template<class F>void refuses(F&& f) {
    bool refused=false;try{f();}catch(const std::exception&){refused=true;}
    need(refused,"Cold pure source scope accepted unsupported input");
}
template<class T>concept PublicAcquire = requires(std::shared_ptr<const civ5::ui::BnwNativeEarlyInputLease> lease) { T::acquire(lease); };
template<class T>concept PublicPrefixClaim = requires(const T& owner) { owner.claim_initial_prefix_registry(); };
template<class T>concept PublicHeaderMark = requires(const T& owner) { owner.require_and_mark_archive_header_phase(); };
template<class T>concept PublicRequestIssuer = requires(const T& owner) { owner.primary_request_at_source_ordinal(0); };
}
int main() {try {
    static_assert(!std::is_default_constructible_v<BnwNativeColdFilesystemOwner> &&
                  !std::is_copy_constructible_v<BnwNativeColdFilesystemOwner> &&
                  !std::is_default_constructible_v<BnwNativeColdArchiveRequest> &&
                  !std::is_copy_constructible_v<BnwNativeColdArchiveRequest> &&
                  !std::is_aggregate_v<BnwNativeColdArchiveRequest>);
    static_assert(!PublicAcquire<BnwNativeColdFilesystemOwner> && !PublicRequestIssuer<BnwNativeColdFilesystemOwner>);
    static_assert(!PublicPrefixClaim<BnwNativeColdFilesystemOwner> &&
                  !PublicHeaderMark<BnwNativeColdFilesystemOwner>);
    const auto first=bnw_primary_archive_raw_ansi_literal(0);
    need(first=="Resource\\DX9\\AmericanWashingtonTextures.fpk","Source first primary token changed");
    need(bnw_primary_archive_raw_ansi_literal(18)=="Resource\\DX9\\JapaneseOdaNobunugaTextures.fpk" &&
         bnw_primary_archive_raw_ansi_literal(21)=="Resource\\DX9\\OttomanSuliemanTextures.fpk" &&
         bnw_primary_archive_raw_ansi_literal(31)=="Resource\\DX9\\WonderTextures.fpk",
         "Native spelling/order must not be normalized");
    refuses([]{bnw_primary_archive_raw_ansi_literal(32);});
    refuses([]{bnw_primary_archive_raw_ansi_literal(std::numeric_limits<std::size_t>::max());});
    need(bnw_primary_archive_ascii_stream_token("Resource/DX9/Stock.FPK")==u"Resource\\DX9\\Stock.FPK",
         "Source slash normalization must preserve case and relative identity");
    refuses([]{bnw_primary_archive_ascii_stream_token(std::string_view("name\0suffix",11));});
    refuses([]{bnw_primary_archive_ascii_stream_token(std::string_view("\x80",1));});
    const std::filesystem::path original=L"C:\\PinnedOriginal";
    const std::filesystem::path payload=L"C:\\CrazyProject\\civ5-assets";
    const std::filesystem::path relative=L"Resource\\DX9\\AmericanWashingtonTextures.fpk";
    const auto original_candidate=confined_original_stat_path(original,relative);
    const auto payload_candidate=confined_original_stat_path(payload,relative);
    need(original_candidate!=payload_candidate && original_candidate.is_absolute() &&
         payload_candidate.is_absolute() && !relative.is_absolute(),"Physical roots collapsed into raw relative name");
    for(const auto* path:{L"Assets\\..\\..\\outside",L"C:\\outside",L"\\outside",L"Resource\\name:stream"})
        refuses([&]{confined_original_stat_path(original,std::filesystem::path(path));});
    std::cout<<"PASS pure native primary name/ACP-ASCII scope and closed factories; genuine cold epoch/lease/lifetime acceptance requires root private bridge\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
