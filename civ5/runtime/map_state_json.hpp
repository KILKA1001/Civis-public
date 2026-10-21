#pragma once
void map_resource_json(const std::vector<civ5::saves::MapResourceCount> &rows) {
    std::cout << '[';
    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << "{\"type_hash\":";
        if (rows[i].type_hash)
            std::cout << *rows[i].type_hash;
        else
            std::cout << "null";
        std::cout << ",\"count\":";
        if (rows[i].count)
            std::cout << *rows[i].count;
        else
            std::cout << "null";
        std::cout << '}';
    }
    std::cout << ']';
}
void map_header_json(const civ5::saves::MapHeaderObservation &h) {
    std::cout << "{\"layout\":"
              << quote(h.layout == civ5::saves::GameStateLayout::vp542_corpus ? "vp542_corpus_comparison"
                                                                              : "expansion2_version1")
              << ",\"version\":";
    if (h.version)
        std::cout << *h.version;
    else
        std::cout << "null";
    std::cout << ",\"start_offset\":" << h.start_offset << ",\"resources_offset\":" << h.resources_offset
              << ",\"end_offset\":" << h.end_offset << ",\"width\":" << h.grid.width
              << ",\"height\":" << h.grid.height << ",\"plot_count\":" << h.grid.size()
              << ",\"land_plots\":" << h.land_plots << ",\"owned_plots\":" << h.owned_plots
              << ",\"natural_wonders\":" << h.natural_wonders << ",\"top_latitude\":" << h.top_latitude
              << ",\"bottom_latitude\":" << h.bottom_latitude
              << ",\"wrap_x\":" << (h.wrap_x ? "true" : "false")
              << ",\"wrap_y\":" << (h.wrap_y ? "true" : "false")
              << ",\"guid_hex\":" << quote(bytes_hex(h.guid)) << ",\"map_generated\":";
    if (h.map_generated)
        std::cout << (*h.map_generated ? "true" : "false");
    else
        std::cout << "null";
    std::cout << ",\"resources\":[";
    map_resource_json(h.resources[0]);
    std::cout << ',';
    map_resource_json(h.resources[1]);
    std::cout << "],\"next_field\":\"plots\",\"saved_content_profile_verified\":false,\"load_side_effects_"
                 "applied\":false}";
}
void map_plots_json(const civ5::saves::MapPlotsObservation &m) {
    const auto &d = m.dimensions;
    std::cout << "{\"start_offset\":" << m.start_offset << ",\"end_offset\":" << m.end_offset
              << ",\"dimensions\":{\"players\":" << d.players << ",\"teams\":" << d.teams
              << ",\"major_civilizations\":" << d.major_civilizations << ",\"yields\":" << d.yields
              << ",\"invisible_types\":" << d.invisible_types << "},\"plots\":[";
    for (std::size_t i = 0; i < m.plots.size(); ++i) {
        if (i)
            std::cout << ',';
        const auto &p = m.plots[i];
        std::cout << "{\"index\":" << p.index << ",\"x\":" << p.x << ",\"y\":" << p.y
                  << ",\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
                  << ",\"version\":";
        if (p.version)
            std::cout << *p.version;
        else
            std::cout << "null";
        std::cout << ",\"fields\":{";
        for (std::size_t j = 0; j < p.fields.size(); ++j) {
            if (j)
                std::cout << ',';
            std::cout << quote(p.fields[j].name) << ':' << p.fields[j].value;
        }
        std::cout << "},\"arrays\":[";
        for (std::size_t j = 0; j < p.arrays.size(); ++j) {
            if (j)
                std::cout << ',';
            const auto &a = p.arrays[j];
            std::cout << "{\"name\":" << quote(a.name) << ",\"columns\":" << a.columns << ",\"values\":[";
            for (std::size_t k = 0; k < a.values.size(); ++k) {
                if (k)
                    std::cout << ',';
                std::cout << a.values[k];
            }
            std::cout << "]}";
        }
        std::cout << "],\"script_data_hex\":";
        if (p.script_data)
            std::cout << quote(bytes_hex(*p.script_data));
        else
            std::cout << "null";
        std::cout << ",\"legacy_build_progress\":";
        map_resource_json(p.legacy_build_progress);
        std::cout << ",\"invisible_visibility\":[";
        for (std::size_t j = 0; j < p.invisible_visibility.size(); ++j) {
            if (j)
                std::cout << ',';
            const auto &v = p.invisible_visibility[j];
            std::cout << "{\"key\":" << v.key << ",\"wire_count\":" << v.wire_count << ",\"values\":[";
            for (std::size_t k = 0; k < v.values.size(); ++k) {
                if (k)
                    std::cout << ',';
                std::cout << v.values[k];
            }
            std::cout << "]}";
        }
        std::cout << "]}";
    }
    std::cout << "],\"next_field\":\"areas\",\"saved_content_profile_verified\":false,\"load_side_effects_"
                 "applied\":false}";
}
void saved_map_terrain_json(const civ5::saves::SavedMapTerrainSource &source) {
    std::cout << "{\"width\":" << source.grid.width << ",\"height\":" << source.grid.height
              << ",\"wrap_x\":" << (source.wrap_x ? "true" : "false")
              << ",\"wrap_y\":" << (source.wrap_y ? "true" : "false")
              << ",\"known_terrain_masks\":" << source.known_terrain_masks
              << ",\"native_markup_complete\":"
              << (source.native_markup_complete ? "true" : "false") << ",\"cells\":[";
    for (std::size_t i = 0; i < source.cells.size(); ++i) {
        if (i)
            std::cout << ',';
        const auto &cell = source.cells[i];
        std::cout << "{\"x\":" << i / source.grid.height << ",\"y\":" << i % source.grid.height
                  << ",\"native_terrain_mask\":" << cell.native_terrain_mask
                  << ",\"native_terrain_mask_known\":"
                  << (cell.native_terrain_mask_known ? "true" : "false")
                  << ",\"native_category\":";
        if (cell.native_category)
            std::cout << static_cast<unsigned>(*cell.native_category);
        else
            std::cout << "null";
        std::cout << '}';
    }
    std::cout << "]}";
}
