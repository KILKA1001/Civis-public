// Root-owned attachment uses the sole original Atlas supplier's capsule.
// Complete Pair/Node/AcceptedPlanes storage bodies remain in AtlasCPP only.
graphics::GlBnwSystemFontAtlas::AcceptedFontPlanes::HistoricalGuard
detail::BnwNormalItemOccurrence::Guard::historical_font_planes_without_callbacks()const{
    const auto saved=impl_;saved->first();
    require(saved->planes_installed&&!saved->planes_installing&&saved->historical_planes&&saved->actual_atlas,
        "Item history genuine historical Atlas attachment absent/incomplete");
    const auto planes=*saved->historical_planes;
    planes.require_physical_current_without_callbacks();
    planes.require_actual_atlas_without_callbacks(*saved->actual_atlas);
    planes.require_actual_positioned_data_without_callbacks(saved->data);
    saved->first();return planes;
}
std::shared_ptr<graphics::GlBnwSystemFontAtlas>
detail::BnwNormalItemOccurrence::Guard::actual_font_atlas_owned_without_callbacks()const{
    const auto saved=impl_;saved->first();
    require(saved->planes_installed&&!saved->planes_installing&&saved->historical_planes&&saved->actual_atlas,
        "Item history actual Atlas has no completed historical attachment");
    const auto atlas=saved->actual_atlas;saved->first();return atlas;
}

void BnwWorldViewControlTree::attach_normal_item_historical_font_planes_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalItemOccurrence::Guard& actual_item,
    const detail::BnwLabelPaintPositionedData& original_data,
    std::shared_ptr<graphics::GlBnwSystemFontAtlas> actual_atlas)const{
    using Atlas=graphics::GlBnwSystemFontAtlas;using Item=detail::BnwNormalItemOccurrence;
    // Capture the genuine Item and original DATA controls before any FULL.
    const auto item=actual_item;item.require_physical_current_without_callbacks();
    const auto saved=item.impl_;const auto data=original_data.capture_guard_without_callbacks();
    const auto atlas=std::move(actual_atlas);
    require(saved&&saved->tree.get()==this&&atlas,"Historical Atlas attach has another Item/Tree or no actual Atlas");
    data.require_current_without_callbacks();item.require_actual_positioned_data_without_callbacks(data);
    require(!saved->planes_installed&&!saved->planes_installing&&!saved->historical_planes&&!saved->actual_atlas,
        "Historical Atlas original Item attachment is already installed/reentered");
    // This latch is metadata of the actual once-only effect; no fake Pair,
    // names/commit/readiness/caller stage argument is introduced.
    saved->planes_installing=true;
    struct End{std::shared_ptr<const Item::Impl> item;~End(){item->planes_installing=false;}}end{saved};
    try{
        // No allocator/FULL before this last raw original DATA facade operand.
        // Sole Atlas factory captures OWN DATA before its internal FULL/new.
        const Atlas::HistoricalPlanesIssuerKey key;
        const auto planes=Atlas::capture_historical_font_planes_owned(key,operation,original_data,atlas);
        const auto active_first=[&]{
            item.require_physical_current_without_callbacks();data.require_current_without_callbacks();
            item.require_actual_positioned_data_without_callbacks(data);
            planes.require_physical_current_without_callbacks();planes.require_actual_atlas_without_callbacks(*atlas);
            planes.require_actual_positioned_data_without_callbacks(data);
            require(saved->planes_installing&&!saved->planes_installed&&!saved->historical_planes&&!saved->actual_atlas,
                "Historical Atlas allocating factory POST changed one-time Item slot");
            data.require_current_without_callbacks();item.require_physical_current_without_callbacks();
        };
        const auto active_full=[&]{active_first();data.require_source_owned(operation);active_first();
            item.require_source_owned(operation);active_first();
            data.require_same_normal_draw_records_source_owned(operation,saved->binding);active_first();};
        active_first();active_full();active_first();
        // Inline VALUE copies only: no allocator, Source, Graph/current or GL
        // between actual original owner adoption and attachment seal.
        saved->actual_atlas=atlas;saved->historical_planes.emplace(planes);
        saved->planes_installed=true;saved->planes_installing=false;
        item.require_physical_current_without_callbacks();
        const auto accepted=item.historical_font_planes_without_callbacks();
        accepted.require_actual_positioned_data_without_callbacks(data);
        data.require_current_without_callbacks();item.require_source_owned(operation);
        data.require_current_without_callbacks();item.require_physical_current_without_callbacks();
        // Graph must separately require SAME original AcceptedFontPlanes
        // inside its genuine Binding before current-entry stage/consume. The
        // capsule compares Pair shared control/Node/Pin/frame/names there.
    }catch(...){const auto owner=core_;owner->failed.store(true);throw;}
}
