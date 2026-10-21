#pragma once
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
namespace civ5::runtime {
// Read-only observation of actual Bank registered m_iDamage and the SAME
// OriginalRead plain last-full-health storage. This is not a SetDamage call,
// a localized attrition entry, an effect receipt or continuation authority.
struct BnwNativeImportedDamageArchiveObservation {
 std::int32_t owner=-1,full_id=-1,value=0,loaded_value=0,last_full_health_turn=-1;
 std::size_t native_archive_index=0;
 std::string binding_name,binding_member;
 bool dirty=false,call_stack_tracking=true;
 std::uint64_t changes=0,last_full_health_stores=0;
 std::weak_ptr<const void>registered_lifetime,plain_runtime_lifetime;
};
}
