#pragma once

#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <functional>
#include "civ5/runtime/bnw_stock_lua_client_extent.hpp"
#include <optional>
#include <vector>

namespace civ5::graphics { struct BnwRendererSceneReceipt; class BnwOwnedCameraRendererWindowLease; }
namespace civ5::runtime { class BnwSelectedStockLuaSourceOwner; class BnwCivisApplicationStateOwner; }

namespace civ5::assets { class CivisResourceBankOwner; }
namespace civ5::ui {

class BnwNativePackageDiscoveryOwner;
class CivisResourceBankUiSnapshot;
class CivisResourceBankFontColorSnapshot;
class BnwWorldViewFontSourceScope;
namespace detail {class CivisSelectedBnwRulesPreparation;}

struct BnwLivePreviewRequest {
    std::filesystem::path save_path;
    std::filesystem::path game_root;
    std::int64_t active_player_id = -1;
    // Explicit G0-only diagnostic. The receipt's native TGA and manifest,
    // original EXE, UI save and renderer save must all match their pinned
    // hashes before the one proven selected warrior is projected into U4.
    std::optional<std::filesystem::path> g0_selected_unit_receipt_root;
    // Observes the actual isolated activation before its host moves to the preview.
    std::function<void(const SelectedBnwBootstrapResult &)> observe_selected_profile;
    std::shared_ptr<runtime::BnwCivisApplicationStateOwner> application;
};

// Extract these three arguments before the graphics executable validates its
// legacy positional modes. An absent triple leaves argv unchanged. The
// caller must also supply --saved-fow-save and the corresponding saved grid,
// so the existing renderer's world is derived from the same save.
// --bnw-live-save FILE.Civ5Save|FILE.CivisSave --bnw-game-root DIRECTORY
// --bnw-active-player ID
// --bnw-g0-selected-unit-receipt-root DIRECTORY (optional diagnostic only)
std::optional<BnwLivePreviewRequest> consume_bnw_live_preview_options(
    int &argc, char **argv);

struct BnwLivePreviewLanguageChoice {
    std::string type, display_name;
};
struct BnwLivePreviewLanguageSelectionRequired {
    // Presentation choices copied from the actual local Languages source.
    // Choosing one must return through the genuine process/settings owner.
    std::vector<BnwLivePreviewLanguageChoice> available_languages;
};

struct BnwLivePreviewOutcome {
    bool rendered = false;
    std::optional<SelectedBnwPreviewBlocker> blocker;
    runtime::LiveSessionReadiness readiness;
    std::optional<BnwLivePreviewLanguageSelectionRequired> language_selection_required;
};

using BnwPrepareRendererScene=std::function<bool(
    const LiveBnwPreviewInput *,graphics::BnwRendererSceneReceipt &)>;

// The callback is the existing run_camera_preview(..., live_bnw_preview)
// call. Its borrowed input is valid only until the callback returns.
// A Civis import may pair with its exact original Civ5 scene only while it
// has no live overlays. Production supplies a post-decode and post-city-load
// scene receipt. An identical Civis UI/scene file may also carry the exact
// ID10/ID30 treasury pair: LiveSession restores it, the receipt binds its bytes,
// and scene reconciliation proves that map/fog/art stayed unchanged. All other
// live sections remain rejected by this independent renderer.
// Matching metadata, prefix, header and payload are checked before rendering.
// Production camera preview passes the renderer's post-decode scene receipt;
// omitting it is reserved for source-only launch tests without a renderer.
// Successful rendering remains read-only and does not imply turn readiness.
BnwLivePreviewOutcome launch_selected_bnw_camera_preview(
    const BnwLivePreviewRequest &request,
    const std::filesystem::path &scene_save_path,
    const std::function<void(const LiveBnwPreviewInput *)> &render,
    const graphics::BnwRendererSceneReceipt *scene_receipt = nullptr,
    const BnwPrepareRendererScene &prepare_scene = {});
// Explicit immutable rules scope for the owned read-only camera preview.
BnwLivePreviewOutcome launch_selected_bnw_camera_preview(
    const BnwLivePreviewRequest &request,
    const std::filesystem::path &scene_save_path,
    const std::function<void(const LiveBnwPreviewInput *)> &render,
    const graphics::BnwRendererSceneReceipt *scene_receipt,
    const BnwPrepareRendererScene &prepare_scene, bool immutable_preview_fingerprints);

// Production initial EARLY route. The caller supplies a fresh package created
// from the sole prepared window's real initial client lease, BEFORE any SQL
// host. This call executes primary/settings/texture/Common source stages and
// adopts the one bootstrap host into that SAME package; it cannot reuse a
// diagnostic or already registered/adopted package. Request layout is unchanged.
// A real independently decoded renderer receipt and actual load-hook projection
// reconciliation callback are mandatory. Unsupported overlays retain the same
// source/treasury gates as the legacy route, including legitimate Civis imports.
// The prepared window/ticket must stay current THROUGH callback return and final
// validation. An initial smoke may stop its loop without retiring that window;
// its parent closes the window only after this outcome and its receipt. Actual
// close/retirement fails publication permanently for this initial lease.
// rendered means this synchronous callback completed under these guards; it is
// not an issued GPU/native Label/App/UIManager/full-game readiness receipt.
BnwLivePreviewOutcome launch_selected_bnw_camera_preview_from_early_package(
    const BnwLivePreviewRequest &request,
    const std::filesystem::path &scene_save_path,
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> package,
    const std::function<void(const LiveBnwPreviewInput *)> &render,
    const graphics::BnwRendererSceneReceipt *scene_receipt = nullptr,
    const BnwPrepareRendererScene &prepare_scene = {});

using BnwOwnedHostCameraRender=std::function<void(
    std::shared_ptr<const SelectedBnwPreviewHost>,const LiveBnwPreviewInput*)>;
// Real shared parent already retained by SelectedBnwReadOnlyPreview. No
// observer unique_ptr extraction or borrowed-input/no-op-alias promotion.
BnwLivePreviewOutcome launch_selected_bnw_camera_preview_from_early_package_with_owned_host(
    const BnwLivePreviewRequest&,const std::filesystem::path&,
    std::shared_ptr<const BnwNativePackageDiscoveryOwner>,const BnwOwnedHostCameraRender&,
    const graphics::BnwRendererSceneReceipt* = nullptr,const BnwPrepareRendererScene& = {});
using BnwOwnedUiSourceCameraRender=std::function<void(std::shared_ptr<BnwOwnedCameraUiSourceInput>)>;
// Additive genuine source-owned route; named borrowed/owned-host APIs unchanged.
BnwLivePreviewOutcome launch_selected_bnw_camera_preview_from_early_package_with_owned_ui_source(
    const BnwLivePreviewRequest&,const std::filesystem::path&,
    std::shared_ptr<const BnwNativePackageDiscoveryOwner>,const BnwOwnedUiSourceCameraRender&,
    const graphics::BnwRendererSceneReceipt* = nullptr,const BnwPrepareRendererScene& = {});
using BnwOwnedStockHostCameraRender=std::function<void(
    std::shared_ptr<const SelectedBnwPreviewHost>,const LiveBnwPreviewInput*,
    std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner>)>;
// One actual SourceOwner on the selected shared host; initial Window binding
// precedes native advance. Owner survives the synchronous UI/render loop.
// This route grants no completed native UI/GPU/gameplay readiness.
BnwLivePreviewOutcome launch_selected_bnw_camera_preview_from_early_package_with_stock_renderer(
    const BnwLivePreviewRequest&,const std::filesystem::path&,
    std::shared_ptr<const BnwNativePackageDiscoveryOwner>,
    std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>,
    const runtime::BnwStockLuaClientExtentProvider&,const BnwOwnedStockHostCameraRender&,
    const graphics::BnwRendererSceneReceipt* = nullptr,const BnwPrepareRendererScene& = {});
using BnwObservedUiSourceCameraRender=std::function<void(
    std::shared_ptr<BnwOwnedCameraUiSourceInput>,const SelectedBnwReadOnlyPreview&)>;
using BnwObservedStockUiSourceCameraRender=std::function<void(
    std::shared_ptr<BnwOwnedCameraUiSourceInput>,const SelectedBnwReadOnlyPreview&,
    std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner>)>;
// The Preview reference is the genuine stack object inside Engine's active
// source borrower. It is valid only during this synchronous callback; neither
// it nor a pointer derived from it may be retained beyond render return.
// SAME actual stock owner/start/Window/Color route, with no wrapper from Host.
BnwLivePreviewOutcome launch_selected_bnw_camera_preview_from_early_package_with_stock_renderer_with_observed_ui_source(
    const BnwLivePreviewRequest&,const std::filesystem::path&,
    std::shared_ptr<const BnwNativePackageDiscoveryOwner>,
    std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>,
    const runtime::BnwStockLuaClientExtentProvider&,const BnwObservedStockUiSourceCameraRender&,
    const graphics::BnwRendererSceneReceipt* = nullptr,const BnwPrepareRendererScene& = {});
// Genuine standalone Civis sources, retaining the actual stack Preview and
// canonical UI source during one synchronous render call. The font scope must
// own the SAME bank memory table; it is data, not Original/GPU readiness.
using BnwObservedCivisStockUiSourceCameraRender=std::function<void(
    std::shared_ptr<BnwOwnedCameraUiSourceInput>,const SelectedBnwReadOnlyPreview&,
    std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner>,
    std::shared_ptr<BnwWorldViewFontSourceScope>)>;
BnwLivePreviewOutcome launch_selected_bnw_camera_preview_from_civis_resources_with_stock_renderer_with_observed_ui_source(
    const BnwLivePreviewRequest&,const std::filesystem::path&,
    std::shared_ptr<assets::CivisResourceBankOwner>,
    std::shared_ptr<const CivisResourceBankUiSnapshot>,
    std::shared_ptr<const CivisResourceBankFontColorSnapshot>,
    std::shared_ptr<BnwWorldViewFontSourceScope>,
    std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>,
    const runtime::BnwStockLuaClientExtentProvider&,
    const BnwObservedCivisStockUiSourceCameraRender&,
    const graphics::BnwRendererSceneReceipt*,const BnwPrepareRendererScene&,
    std::shared_ptr<const detail::CivisSelectedBnwRulesPreparation> prepared={});
} // namespace civ5::ui
