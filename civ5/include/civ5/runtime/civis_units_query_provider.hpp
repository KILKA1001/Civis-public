#pragma once
#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>
namespace civ5::ui {class CivisCivilopediaUnitsQuery;}
namespace civ5::runtime {
// Opt-in diagnostic selected first Units statement, not native CreateQuery.
// The callback must retain its clone-backed, source/current/thread owner.
struct ScriptCivisUnitsQueryProvider {
    std::function<std::shared_ptr<ui::CivisCivilopediaUnitsQuery>(std::string_view)> prepare;
};
struct ScriptCivisUnitsQueryObservations {
    // Successful backend operations, also readable after a failed chunk.
    // They do not certify an active context or delivery after Lua OOM.
    std::size_t prepared=0,invocations=0,rows=0,eof=0;
};
}
