#include "bnw_native_default_namespace_policy.hpp"
#include "bnw_native_callback_owner_guard.hpp"
#include <iostream>
#include <thread>
#include <type_traits>
using namespace civ5::ui::detail;
namespace {
void need(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template<class F> void refuses(F&& f) { bool rejected = false; try { f(); } catch (const std::exception&) { rejected = true; } need(rejected, "Unsupported policy/query accepted"); }
template<class T> concept PublicAcquire = requires { T::acquire({}); };
// Independent lifetime-guard fixture, never a manufactured native policy owner.
struct LifetimeFixture { BnwNativeCallbackOwnerState state; std::weak_ptr<const LifetimeFixture> self; };
}
int main() { try {
    static_assert(!std::is_default_constructible_v<BnwNativeDefaultNamespacePolicyOwner> &&
                  !std::is_copy_constructible_v<BnwNativeDefaultNamespacePolicyOwner> &&
                  !std::is_constructible_v<BnwNativeDefaultNamespacePolicyOwner, bool> &&
                  !PublicAcquire<BnwNativeDefaultNamespacePolicyOwner>);
    BnwDefaultNamespaceInitialPolicyDiagnostic initial;
    need(initial.constructor_archive_priority() == 1 && initial.constructor_enabled() == 1 &&
         initial.constructor_prefix_filters().empty(), "Source82A050 initial fields differ");
    const auto query = bnw_default_namespace_ascii_query_diagnostic(u"C:/folder/Config.INI");
    need(query.raw_basename == "Config.INI" && query.crc == 0x446aef61u, "Native basename/raw CRC differs");
    need(bnw_default_namespace_ascii_query_diagnostic(u"C:CONFIG.ini").crc == query.crc, "FIRST colon case path differs");
    need(bnw_default_namespace_ascii_query_diagnostic(u"a:b:c").raw_basename == "b:c", "Colon incorrectly takes LAST");
    need(bnw_default_namespace_ascii_query_diagnostic(u"a:b/file:name").raw_basename == "file:name", "Colon overrides LAST slash");
    need(bnw_default_namespace_ascii_query_diagnostic(u"folder\\sub/Config.ini").crc == query.crc, "Mixed slash conversion differs");
    const auto collision = bnw_default_namespace_ascii_query_diagnostic(u"sounds/explosion200poundwater-002.wav");
    const auto other = bnw_default_namespace_ascii_query_diagnostic(u"data/work_boat_death.dge");
    need(collision.crc == 0x6b70aef3u && collision.crc == other.crc && collision.raw_basename != other.raw_basename, "CRC collision fixture differs");
    const std::map<std::uint32_t, std::string> records{{collision.crc, "FIRST source record"}};
    const auto* first = bnw_default_namespace_crc_selection_diagnostic(records, collision);
    need(first && first == bnw_default_namespace_crc_selection_diagnostic(records, other), "CRC-only hit rejected differing filename");
    need(!bnw_default_namespace_crc_selection_diagnostic(records, query), "Absent CRC selected a record");
    refuses([] { bnw_default_namespace_ascii_query_diagnostic(u""); });
    refuses([] { bnw_default_namespace_ascii_query_diagnostic(u"folder/"); });
    refuses([] { bnw_default_namespace_ascii_query_diagnostic(std::u16string(260, u'a')); });
    refuses([] { bnw_default_namespace_ascii_query_diagnostic(std::u16string(128, u'a')); });
    refuses([] { bnw_default_namespace_ascii_query_diagnostic(std::u16string_view(u"a\0b", 3)); });
    refuses([] { bnw_default_namespace_ascii_query_diagnostic(u"\u0414/Config.ini"); });
    auto owner = std::make_shared<LifetimeFixture>(); owner->self = owner;
    {
        BnwNativeCallbackOwnerGuard<LifetimeFixture> outer(owner->state, owner->self);
        need(outer.valid(), "Live standalone guard refused");
        { BnwNativeCallbackOwnerGuard<LifetimeFixture> nested(owner->state, owner->self); need(!nested.valid(), "Reentry accepted"); }
        need(!outer.valid(), "Caught reentry did not poison outer guard");
    }
    { BnwNativeCallbackOwnerGuard<LifetimeFixture> again(owner->state, owner->self); need(!again.valid(), "Retired guard replay accepted"); }
    auto live = std::make_shared<LifetimeFixture>(); live->self = live;
    std::shared_ptr<const int> parent = std::make_shared<int>(7);
    {
        BnwNativeCallbackOwnerGuard<LifetimeFixture> guard(live->state, live->self);
        std::weak_ptr<const int> weak_parent = parent; auto temporary = weak_parent.lock();
        need(guard.valid_parent(temporary), "Genuine persistent parent refused");
        parent.reset(); need(!guard.valid_parent(temporary), "Last temporary parent pin masked facade loss");
    }
    auto retiring = std::make_shared<LifetimeFixture>(); retiring->self = retiring;
    std::weak_ptr<const LifetimeFixture> retired = retiring;
    {
        BnwNativeCallbackOwnerGuard<LifetimeFixture> guard(retiring->state, retiring->self);
        retiring.reset(); need(!guard.valid() && !retired.expired(), "Own internal guard pin issued last-facade authority/dangling Impl");
    }
    need(retired.expired(), "Internal guard retained dead facade after final state access");
    auto threaded = std::make_shared<LifetimeFixture>(); threaded->self = threaded;
    bool foreign_rejected = false;
    std::thread foreign([&] { BnwNativeCallbackOwnerGuard<LifetimeFixture> guard(threaded->state, threaded->self); foreign_rejected = !guard.valid(); });
    foreign.join(); need(foreign_rejected, "Foreign thread acquired guard");
    { BnwNativeCallbackOwnerGuard<LifetimeFixture> guard(threaded->state, threaded->self); need(guard.valid(), "Foreign refusal changed source thread state"); }
    std::cout << "PASS initial-source policy/query/CRC-only/collision/closed factories/pure lifetime guards; genuine Arena integration pending\n";
    return 0;
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; } }
