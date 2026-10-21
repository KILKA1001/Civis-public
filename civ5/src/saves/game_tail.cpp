#include "civ5/saves/game_tail.hpp"
#include <algorithm>
#include <array>
#include <sqlite3.h>
#include <stdexcept>
#include <string_view>
#include <limits>

namespace civ5::saves {
GameFinalSystemsObservation observe_game_final_systems(runtime::BinaryReader &input, GameStateLayout layout,
                                                       GameStateBudget budget) {
    if (layout != GameStateLayout::vp542_corpus && layout != GameStateLayout::expansion2_version1)
        throw std::runtime_error("Unknown game final systems layout");
    GameFinalSystemsObservation result{};
    result.layout = layout;
    result.start_offset = input.position();
    if (layout == GameStateLayout::vp542_corpus) {
        result.vp.emplace();
        auto &vp = *result.vp;
        const auto count = [&] {
            const std::uint64_t size = input.u32();
            if (size > budget.elements || size > input.remaining() / 20)
                throw std::runtime_error("Final systems collection exceeds input or element budget");
            budget.elements -= size;
            return size;
        };
        const auto corporations = count();
        vp.corporations.reserve(static_cast<std::size_t>(corporations));
        for (std::uint64_t i = 0; i < corporations; ++i) {
            CorporationObservation c;
            c.type = input.i32();
            c.founder = input.i32();
            c.headquarters_x = input.i32();
            c.headquarters_y = input.i32();
            c.turn_founded = input.i32();
            vp.corporations.push_back(c);
        }
        for (auto *group : {&vp.active_contracts, &vp.inactive_contracts}) {
            const auto contracts = count();
            group->reserve(static_cast<std::size_t>(contracts));
            for (std::uint64_t i = 0; i < contracts; ++i) {
                ContractObservation c;
                c.type = input.i32();
                c.holder = input.i32();
                c.turns = input.i32();
                c.maintenance = input.i32();
                c.turn_start = input.i32();
                group->push_back(c);
            }
        }
        const std::uint64_t bytes = input.u32();
        if (bytes > budget.string_bytes || bytes > budget.total_string_bytes || bytes > input.remaining())
            throw std::runtime_error("Game ID exceeds input or string budget");
        vp.game_id.resize(static_cast<std::size_t>(bytes));
        input.read(vp.game_id);
    }
    result.end_offset = input.position();
    return result;
}

void validate_saved_database_image(std::span<const std::uint8_t> image) {
    constexpr std::string_view magic("SQLite format 3\0",16);
    if(image.size()<100||image.size()>static_cast<std::uint64_t>(std::numeric_limits<sqlite3_int64>::max())||
       !std::equal(magic.begin(),magic.end(),image.begin()))
        throw std::runtime_error("Invalid saved SQLite database image");
    sqlite3 *connection=nullptr;
    const auto opened=sqlite3_open_v2(":memory:",&connection,
        SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_PRIVATECACHE,nullptr);
    struct Close {sqlite3 *db;~Close(){sqlite3_close(db);}} close{connection};
    if(opened!=SQLITE_OK)throw std::runtime_error("Cannot open isolated saved database validator");
    sqlite3_db_config(connection,SQLITE_DBCONFIG_DEFENSIVE,1,nullptr);
    sqlite3_db_config(connection,SQLITE_DBCONFIG_TRUSTED_SCHEMA,0,nullptr);
    const auto size=static_cast<sqlite3_int64>(image.size());
    if(sqlite3_deserialize(connection,"main",const_cast<unsigned char*>(image.data()),size,size,
                           SQLITE_DESERIALIZE_READONLY)!=SQLITE_OK)
        throw std::runtime_error("Cannot deserialize saved database image");
    std::uint64_t remaining_batches=100000;
    sqlite3_progress_handler(connection,1000,[](void *context){
        auto &remaining=*static_cast<std::uint64_t*>(context);
        if(!remaining)return 1;
        --remaining;return 0;
    },&remaining_batches);
    sqlite3_stmt *statement=nullptr;
    const auto prepared=sqlite3_prepare_v2(connection,"PRAGMA quick_check",-1,&statement,nullptr);
    struct Finalize {sqlite3_stmt *stmt;~Finalize(){sqlite3_finalize(stmt);}} finalize{statement};
    if(prepared!=SQLITE_OK||sqlite3_step(statement)!=SQLITE_ROW)
        throw std::runtime_error("Saved database integrity check failed");
    const auto *text=sqlite3_column_text(statement,0);
    if(!text||std::string_view(reinterpret_cast<const char*>(text))!="ok"||sqlite3_step(statement)!=SQLITE_DONE)
        throw std::runtime_error("Saved database integrity check failed");
}

SavedDatabaseObservation observe_saved_database(runtime::BinaryReader &input, std::uint64_t maximum_bytes,
                                                 bool skip_non_sqlite_image_for_map_diagnostic) {
    SavedDatabaseObservation result;
    result.start_offset = input.position();
    const std::uint64_t bytes = input.u32();
    result.declared_image_bytes = bytes;
    result.data_offset = input.position();
    if (bytes > maximum_bytes || bytes > input.remaining())
        throw std::runtime_error("Saved database exceeds input or byte budget");
    if (bytes) {
        constexpr std::array<std::uint8_t, 16> magic{'S', 'Q', 'L', 'i', 't', 'e', ' ', 'f',
                                                     'o', 'r', 'm', 'a', 't', ' ', '3', 0};
        if (bytes < 100)
            throw std::runtime_error("Saved database is too short for a SQLite header");
        std::array<std::uint8_t, 16> prefix{};
        input.read(prefix);
        if (prefix != magic) {
            if (!skip_non_sqlite_image_for_map_diagnostic)
                throw std::runtime_error("Missing or invalid saved SQLite database signature; refusing to "
                                         "consume following game data");
            input.seek(result.data_offset);
            result.non_sqlite_image_skipped = true;
            result.end_offset = input.position();
            return result;
        }
        result.image.resize(static_cast<std::size_t>(bytes));
        std::copy(prefix.begin(), prefix.end(), result.image.begin());
        input.read(std::span(result.image).subspan(prefix.size()));
        validate_saved_database_image(result.image);
        result.quick_check_passed = true;
    }
    result.end_offset = input.position();
    return result;
}
} // namespace civ5::saves
