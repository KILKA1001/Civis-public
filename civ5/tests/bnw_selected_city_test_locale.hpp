#pragma once
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/rules_database.hpp"
#include <thread>

namespace civ5::test_detail {
// Explicit EN selection for these two typed UI tests. Text comes only from
// the attached localization of the SAME actual selected Host. No key echoes.
inline ui::LiveSessionUiSources selected_city_frame_sources(
    std::shared_ptr<ui::SelectedBnwPreviewHost> host) {
    if(!host) throw std::invalid_argument("Selected city localization Host absent");
    (void)host->session().bnw_game_debug_mode(); // Actual constructor-thread guard before DB borrow.
    const auto thread=std::this_thread::get_id();
    const auto generation=host->session().state_generation();
    const auto rules_current=host->database().immutable_preview_fingerprint_currentness();
    if(!rules_current || !rules_current())
        throw std::runtime_error("Actual selected localization DB is not current immutable scope");
    std::size_t languages=0;
    host->database().query({"SELECT Type FROM Localization.Languages WHERE Type=?",
                           {std::string("en_US")}},[&](auto row){
        if(row.size()!=1 || !std::get_if<std::string>(&row[0]) ||
           std::get<std::string>(row[0])!="en_US")
            throw std::runtime_error("Actual selected EN language metadata malformed");
        ++languages;
    });
    if(languages!=1) throw std::runtime_error("Actual selected EN language absent or ambiguous");
    auto sources=host->preview_input().frame_sources;
    sources.localize_city=[host=std::move(host),thread,generation,rules_current](std::string_view key)
        ->std::optional<std::string> {
        const auto check=[&]{
            if(std::this_thread::get_id()!=thread)
                throw std::runtime_error("Selected city localization left owner thread");
            if(!rules_current()) throw std::runtime_error("Selected city localization DB retired");
            if(host->session().state_generation()!=generation)
                throw std::runtime_error("Selected city localization Session changed");
            (void)host->session().bnw_game_debug_mode();
        };
        check();
        if(key.empty() || key.find('\0')!=key.npos) return std::nullopt;
        std::optional<std::string> text;std::size_t rows=0;
        host->database().query({"SELECT Text FROM Localization.LocalizedText WHERE Language=? AND Tag=?",
                               {std::string("en_US"),std::string(key)}},[&](auto row){
            if(row.size()!=1) throw std::runtime_error("Actual selected city text shape malformed");
            ++rows;
            if(const auto* value=std::get_if<std::string>(&row[0]))text=*value;
        });
        check();
        if(rows>1) throw std::runtime_error("Actual selected city text ambiguous");
        if(rows!=1 || !text || text->empty() || *text==key) return std::nullopt;
        return text;
    };
    return sources;
}
} // namespace civ5::test_detail
