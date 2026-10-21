#include "civ5/ui/civis_new_era_audio_completion_owner.hpp"
#include "civ5/ui/bnw_new_era_sound_archive.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

using namespace civ5;
using namespace civ5::ui;
namespace {
void check(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
template<class F> void rejects(F f, const char *why) {
    bool failed = false;
    try { f(); } catch (...) { failed = true; }
    check(failed, why);
}
struct Slot {
    std::atomic<bool> done = false;
    std::atomic<int> starts = 0, releases = 0, cancellations = 0;
    std::atomic<std::size_t> bytes = 0;
    bool fail_start = false, release_ok = true, release_throws = false;
};
struct Device final : IWaveOutDevice {
    explicit Device(std::shared_ptr<Slot> s) : slot(std::move(s)) {}
    std::shared_ptr<Slot> slot;
    UiPcmBuffer pcm;
    bool start(UiPcmBuffer b, std::string &error) override {
        pcm = std::move(b);
        slot->bytes = pcm.interleaved_samples.size();
        ++slot->starts;
        if (slot->fail_start) { error = "injected start failure"; return false; }
        return true;
    }
    bool wait_for_progress(std::chrono::milliseconds, std::string &) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return true;
    }
    bool done() const override { return slot->done.load(); }
    bool release(bool cancel, std::string &error) override {
        ++slot->releases;
        if (cancel) ++slot->cancellations;
        if (slot->release_throws) throw std::runtime_error("injected release throw");
        if (!slot->release_ok) { error = "injected DMA release uncertainty"; return false; }
        pcm.interleaved_samples.clear();
        return true;
    }
};
struct Devices {
    std::mutex mutex;
    std::vector<std::shared_ptr<Slot>> slots;
    bool fail_start = false, release_ok = true, release_throws = false;
    WinMmPcmSink::DeviceFactory factory() {
        return [this] {
            auto slot = std::make_shared<Slot>();
            slot->fail_start = fail_start;
            slot->release_ok = release_ok;
            slot->release_throws = release_throws;
            { std::lock_guard lock(mutex); slots.push_back(slot); }
            return std::make_unique<Device>(slot);
        };
    }
    std::shared_ptr<Slot> at(std::size_t index) {
        std::lock_guard lock(mutex);
        return index < slots.size() ? slots[index] : nullptr;
    }
};
template<class F> void wait(F f, const char *why) {
    const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!f()) {
        check(std::chrono::steady_clock::now() < limit, why);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
assets::ResourceMountTable mount(const std::filesystem::path &root, bool sound, bool bnw = true) {
    auto base = std::make_shared<assets::LooseResourceIndex>(root);
    for (const auto *path : {"Assets/Sounds/XML/ButtonPopupSounds.xml",
        "Assets/Sounds/XML/NotificationSounds.xml", "Assets/Sounds/XML/Audio2DScripts.xml",
        "Assets/Sounds/XML/AudioDefines.xml"}) base->add_file(root / path);
    assets::ResourceMountTable resources(assets::resource_lookup(base), {}, assets::resource_keys(base));
    if (bnw) {
        auto expansion = std::make_shared<assets::LooseResourceIndex>(root);
        for (const auto *path : {"Assets/DLC/Expansion2/UI/InGame/InGame.xml",
            "Assets/DLC/Expansion2/UI/InGame/Popups/NewEraPopup.lua",
            "Assets/DLC/Expansion2/UI/InGame/Popups/NewEraPopup.xml"}) expansion->add_file(root / path);
        resources.mount({2, 0, 0, assets::resource_lookup(expansion), true, true, {}, assets::resource_keys(expansion)});
    }
    if (sound) check(mount_bnw_new_era_sound_archive(resources, root, 30).status ==
        BnwNewEraSoundMountStatus::mounted, "installed unique NewEra FPK payload must mount");
    return resources;
}
const CivisAudioSessionIdentity session{std::string(64, 'a'), std::string(64, 'b'), 9};
const CivisNewEraAudioContextIdentity identity{session, 3, 5, 7, 12};
NewEraPopupShownReceipt receipt(std::uint64_t cue = 700, std::int64_t event = 91) {
    return {7, event, 12, 5, 20, cue};
}
UiFrameSnapshot frame(std::int64_t event = 91, bool fences = true) {
    PopupEvent popup;
    popup.context_id = "NewEraPopup";
    popup.event_id = event;
    auto snapshot = UiFrameSnapshot(20, 7, 12).with(std::make_shared<const PopupFrame>(PopupFrame{7, {popup}, 12, 5}));
    if (fences) snapshot = snapshot.with(std::make_shared<const UiOwnerFences>(UiOwnerFences{{[] { return true; }}}));
    return snapshot;
}
using Status = CivisNewEraCompletionStatus;
}
int main(int argc, char **argv) {
    try {
        check(argc == 2, "installed original game root required");
        const std::filesystem::path root(argv[1]);
        const auto resources = mount(root, true);
        auto token = std::make_shared<CivisAudioSessionRetirementToken>(session);
        Devices devices;
        bool current = true;
        int completions = 0;
        auto guard = [&](const auto &id) { return current && id.session == session && id.context_generation == 3; };
        auto observer = [&](const CivisNewEraCompletionNotice &n) {
            ++completions;
            check(n.owner.session == session && n.shown.owner_sequence == 5 &&
                n.released_device.completed == static_cast<std::uint64_t>(completions) &&
                !n.released_device.active && n.released_device.queued == 0, "completed notice requires exact released counter");
        };
        auto owner = CivisNewEraAudioCompletionOwner::open(token, identity, resources, guard, observer, devices.factory());
        rejects([&] { CivisNewEraAudioCompletionOwner::open(token, identity, resources, guard, observer, devices.factory()); },
            "session cannot own two private bridges");
        check(owner->shown(frame(91, false), receipt()).status == Status::rejected, "missing frame fence rejected");
        auto foreign = receipt(); foreign.owner_sequence = 6;
        check(owner->shown(frame(), foreign).status == Status::rejected && !devices.at(0), "foreign receipt causes no backend effect");
        check(owner->shown(frame(), receipt()).status == Status::submitted, "real selected PCM submitted once");
        wait([&] { return devices.at(0) && devices.at(0)->starts == 1; }, "first device started");
        check(devices.at(0)->bytes > 0, "installed original WAV decoded into retained PCM");
        check(owner->poll().status == Status::pending && completions == 0, "acceptance cannot become completion");
        check(owner->shown(frame(), receipt()).status == Status::duplicate, "same pending shown not replayed");
        check(owner->shown(frame(92), receipt(701, 92)).status == Status::busy, "second cue cannot queue behind outstanding cue");
        Status foreign_thread = Status::idle;
        std::thread other([&] { foreign_thread = owner->poll().status; }); other.join();
        check(foreign_thread == Status::wrong_thread, "poll rejects wrong thread without affecting pending cue");
        devices.at(0)->done = true;
        wait([&] { return owner->poll().status == Status::completed; }, "completion after successful device release");
        check(completions == 1 && devices.at(0)->releases == 1 && owner->poll().status == Status::idle,
            "completion notice is one-shot");
        check(owner->shown(frame(), receipt()).status == Status::duplicate, "completed receipt cannot replay");
        check(owner->shown(frame(92), receipt(701, 92)).status == Status::submitted, "next real cue accepted after release proof");
        wait([&] { return devices.at(1) && devices.at(1)->starts == 1; }, "second device started");
        devices.at(0)->done = true; // Late old progress cannot complete the new private worker item.
        check(owner->poll().status == Status::pending && completions == 1, "old completion cannot complete newer cue");
        devices.at(1)->done = true;
        wait([&] { return owner->poll().status == Status::completed; }, "second unique completion");
        check(completions == 2 && owner->close().status == Status::closed && !token->quarantined(), "idle close permits safe same-session owner replacement");
        auto replacement = CivisNewEraAudioCompletionOwner::open(token, identity, resources, guard, observer, devices.factory());
        check(replacement->shown(frame(), receipt()).status == Status::duplicate,
            "safe context replacement retains consumed session cue watermark");
        replacement->close();

        auto pending_token = std::make_shared<CivisAudioSessionRetirementToken>(session);
        Devices pending_device;
        auto pending_owner = CivisNewEraAudioCompletionOwner::open(pending_token, identity, resources, guard, observer, pending_device.factory());
        pending_owner->shown(frame(), receipt());
        wait([&] { return pending_device.at(0) && pending_device.at(0)->starts == 1; }, "cancel test device started");
        pending_owner->close();
        check(pending_token->quarantined() && pending_device.at(0)->cancellations == 1, "pending close retires session even if cancellation seems clean");
        rejects([&] { CivisNewEraAudioCompletionOwner::open(pending_token, identity, resources, guard, observer, pending_device.factory()); },
            "new context owner cannot bypass old cancellation uncertainty");

        for (int mode = 0; mode != 3; ++mode) {
            auto failed_token = std::make_shared<CivisAudioSessionRetirementToken>(session);
            Devices failed_device;
            failed_device.fail_start = mode == 0;
            failed_device.release_ok = mode != 1;
            failed_device.release_throws = mode == 2;
            int published = 0;
            auto failed_owner = CivisNewEraAudioCompletionOwner::open(failed_token, identity, resources, guard,
                [&](const auto &) { ++published; }, failed_device.factory());
            failed_owner->shown(frame(), receipt());
            wait([&] { return failed_device.at(0) && failed_device.at(0)->starts == 1; }, "failing device started");
            failed_device.at(0)->done = true;
            wait([&] { return failed_owner->poll().status == Status::retired; }, "failed release retires owner");
            check(published == 0 && failed_token->quarantined(), "failure cannot publish completion or allow fresh backend");
        }
        for (int mode = 0; mode != 4; ++mode) {
            auto callback_token = std::make_shared<CivisAudioSessionRetirementToken>(session);
            Devices callback_device;
            int published = 0;
            bool valid = true;
            std::shared_ptr<CivisNewEraAudioCompletionOwner> callback_owner;
            auto callback_guard = [&](const auto &) { return valid; };
            auto callback = [&](const auto &) {
                ++published;
                if (mode == 0) valid = false;
                if (mode == 1) throw std::runtime_error("observer throw");
                if (mode == 2) check(callback_owner->poll().status == Status::retired, "reentrant poll retires owner");
                if (mode == 3) callback_owner->close();
            };
            callback_owner = CivisNewEraAudioCompletionOwner::open(callback_token, identity, resources, callback_guard, callback, callback_device.factory());
            callback_owner->shown(frame(), receipt());
            wait([&] { return callback_device.at(0) && callback_device.at(0)->starts == 1; }, "callback test device started");
            callback_device.at(0)->done = true;
            Status terminal = Status::pending;
            wait([&] { terminal = callback_owner->poll().status; return terminal == Status::retired || terminal == Status::closed; }, "callback failure fails closed");
            check(published == 1 && !callback_token->quarantined(), "already released completion callback cannot replay or invent DMA uncertainty");
            callback_owner->poll();
            check(published == 1, "failed/closed observer never replayed");
        }
        auto stale_token = std::make_shared<CivisAudioSessionRetirementToken>(session);
        Devices stale_device;
        int stale_published = 0;
        current = true;
        auto stale_owner = CivisNewEraAudioCompletionOwner::open(stale_token, identity, resources, guard,
            [&](const auto &) { ++stale_published; }, stale_device.factory());
        stale_owner->shown(frame(), receipt());
        wait([&] { return stale_device.at(0) && stale_device.at(0)->starts == 1; }, "stale context device started");
        current = false;
        check(stale_owner->poll().status == Status::retired && stale_token->quarantined() && stale_published == 0,
            "session/context generation change rejects completion and retires uncertain backend");
        current = true;
        auto blocked_token = std::make_shared<CivisAudioSessionRetirementToken>(session);
        Devices blocked_device;
        auto blocked = CivisNewEraAudioCompletionOwner::open(blocked_token, identity, mount(root, false), guard, observer, blocked_device.factory());
        check(blocked->shown(frame(), receipt()).status == Status::rejected && !blocked_device.at(0) && !blocked_token->quarantined(),
            "missing actual payload blocks before backend without claiming uncertain submission");
        blocked->close();
        rejects([&] { CivisNewEraAudioCompletionOwner::open(blocked_token, identity, mount(root, false, false), guard, observer, blocked_device.factory()); },
            "missing installed context source refuses owner construction");
        auto ambiguous_resources = resources;
        ambiguous_resources.mount({99, 0, 0, [](const assets::AssetKey &key) -> assets::ResourceMountHit {
            if (assets::resource_basename(key.original_name()) != "newerapopup.lua") return {};
            return {assets::AmbiguousAsset{key, {assets::SourceKind::loose, "explicit ambiguity fixture", "NewEraPopup.lua", {}}, {}}, true};
        }, true, true, {}, {}});
        rejects([&] { CivisNewEraAudioCompletionOwner::open(blocked_token, identity, ambiguous_resources, guard, observer, blocked_device.factory()); },
            "ambiguous selected context source refuses completion owner");
        rejects([&] { CivisAudioSessionRetirementToken bad({"fake", "fake", 0}); }, "unpinned session refused");
        auto wrong = identity; ++wrong.session.generation;
        rejects([&] { CivisNewEraAudioCompletionOwner::open(blocked_token, wrong, resources, guard, observer, blocked_device.factory()); },
            "foreign session generation cannot claim token");
        for (int mode = 0; mode != 3; ++mode) {
            auto guard_token = std::make_shared<CivisAudioSessionRetirementToken>(session);
            Devices guard_device;
            std::shared_ptr<CivisNewEraAudioCompletionOwner> guard_owner;
            auto before_guard = [&](const auto &) {
                if (mode == 0) return false;
                if (mode == 1) throw std::runtime_error("current provider throw");
                guard_owner->close();
                return true;
            };
            guard_owner = CivisNewEraAudioCompletionOwner::open(guard_token, identity, resources, before_guard,
                [](const auto &) { throw std::runtime_error("stale callback must not run"); }, guard_device.factory());
            check(guard_owner->shown(frame(), receipt()).status == Status::retired && !guard_device.at(0) && !guard_token->quarantined(),
                "stale/throwing/closing current provider rejects before backend effect");
        }
        auto fence_token = std::make_shared<CivisAudioSessionRetirementToken>(session);
        Devices fence_device;
        bool fence_current = true;
        auto fence_owner = CivisNewEraAudioCompletionOwner::open(fence_token, identity, resources,
            [&](const auto &) { return fence_current; }, observer, fence_device.factory());
        auto changing_frame = frame().with(std::make_shared<const UiOwnerFences>(UiOwnerFences{{[&] { fence_current = false; return true; }}}));
        check(fence_owner->shown(changing_frame, receipt()).status == Status::retired && !fence_device.at(0),
            "current provider rechecked after borrowed frame fences");
        auto lifetime_token = std::make_shared<CivisAudioSessionRetirementToken>(session);
        Devices lifetime_device;
        int lifetime_calls = 0;
        std::shared_ptr<CivisNewEraAudioCompletionOwner> lifetime;
        lifetime = CivisNewEraAudioCompletionOwner::open(lifetime_token, identity, resources, guard,
            [&](const auto &) { ++lifetime_calls; lifetime.reset(); }, lifetime_device.factory());
        lifetime->shown(frame(), receipt());
        wait([&] { return lifetime_device.at(0) && lifetime_device.at(0)->starts == 1; }, "last-reference callback device started");
        lifetime_device.at(0)->done = true;
        wait([&] { return lifetime->poll().status == Status::completed; }, "owner retained through last-reference callback");
        check(!lifetime && lifetime_calls == 1 && !lifetime_token->quarantined(),
            "observer may release final public owner reference after backend release");
        auto resource_token = std::make_shared<CivisAudioSessionRetirementToken>(session);
        Devices resource_device;
        std::shared_ptr<CivisNewEraAudioCompletionOwner> resource_owner;
        bool armed = false;
        const auto retained_resources = std::make_shared<const assets::ResourceMountTable>(resources);
        assets::ResourceMountTable watched([&](const assets::AssetKey &key) -> assets::ResourceMountHit {
            if (armed) { armed = false; resource_owner->close(); }
            return {retained_resources->resolve(key).asset, true};
        });
        resource_owner = CivisNewEraAudioCompletionOwner::open(resource_token, identity, watched, guard,
            [](const auto &) { throw std::runtime_error("retired resource observer must not publish"); }, resource_device.factory());
        armed = true;
        check(resource_owner->shown(frame(), receipt()).status == Status::retired && resource_token->quarantined(),
            "resource callback close defers borrowed bridge teardown until in-flight delivery unwinds");

        // Storage replacement is not an event authority change. Keep the old
        // source-issued fixture occurrence alive; never read a dangling alias.
        auto alias_token = std::make_shared<CivisAudioSessionRetirementToken>(session);
        Devices alias_device;
        std::optional<UiFrameSnapshot> borrowed_frame = frame();
        std::vector<NewEraPopupShownReceipt> borrowed_events{receipt()};
        bool replace_arguments = true;
        int alias_published = 0;
        auto alias_owner = CivisNewEraAudioCompletionOwner::open(alias_token, identity, resources,
            [&](const auto &) {
                if (replace_arguments) {
                    replace_arguments = false;
                    std::vector<NewEraPopupShownReceipt> replacement(1024, receipt(900, 92));
                    borrowed_events.swap(replacement); // old element is destroyed
                    borrowed_frame.reset();
                    borrowed_frame.emplace(frame(92));
                }
                return true; // fixture authority still refers to the original occurrence
            }, [&](const auto &n) {
                ++alias_published;
                check(n.shown.event_id == 91 && n.shown.cue_sequence == 700 && n.shown.frame == 20,
                    "completion must retain original receipt, not the replacement event");
            }, alias_device.factory());
        check(alias_owner->shown(*borrowed_frame, borrowed_events.front()).status == Status::submitted,
            "arguments pinned before Current replaces optional and event vector");
        wait([&] { return alias_device.at(0) && alias_device.at(0)->starts == 1; }, "pinned old occurrence device started");
        alias_device.at(0)->done = true;
        wait([&] { return alias_owner->poll().status == Status::completed; }, "pinned old occurrence completed");
        check(alias_published == 1, "only the retained original occurrence notifies");
        alias_owner->close();

        auto replaced_fence_token = std::make_shared<CivisAudioSessionRetirementToken>(session);
        Devices replaced_fence_device;
        bool original_current = true;
        std::optional<UiFrameSnapshot> original_argument = frame().with(
            std::make_shared<const UiOwnerFences>(UiOwnerFences{{[&] { return original_current; }}}));
        auto replaced_fence_owner = CivisNewEraAudioCompletionOwner::open(replaced_fence_token, identity, resources,
            [&](const auto &) {
                original_current = false;
                original_argument.reset();
                original_argument.emplace(frame()); // new fences must not erase old ones
                return true;
            }, [](const auto &) { throw std::runtime_error("stale original must not notify"); },
            replaced_fence_device.factory());
        check(replaced_fence_owner->shown(*original_argument, receipt()).status == Status::retired &&
            !replaced_fence_device.at(0) && !replaced_fence_token->quarantined(),
            "replacement current fences cannot authorize invalid original publication");

        // Real delivery's mounted lookup is an external callback boundary. It
        // changes the original frame fence while keeping Current true. A private
        // accepted cue must retire/quarantine even if its source bytes still load.
        auto delivered_fence_token = std::make_shared<CivisAudioSessionRetirementToken>(session);
        Devices delivered_fence_device;
        bool delivery_armed = false, delivery_frame_current = true, delivery_callback_ran = false;
        int delivered_fence_published = 0;
        assets::ResourceMountTable delivery_watched([&](const assets::AssetKey &key) -> assets::ResourceMountHit {
            if (delivery_armed) {
                delivery_armed = false;
                delivery_callback_ran = true;
                delivery_frame_current = false;
            }
            return {retained_resources->resolve(key).asset, true};
        });
        auto delivered_fence_owner = CivisNewEraAudioCompletionOwner::open(delivered_fence_token, identity,
            delivery_watched, [](const auto &) { return true; }, [&](const auto &) { ++delivered_fence_published; },
            delivered_fence_device.factory());
        const auto delivered_frame = frame().with(std::make_shared<const UiOwnerFences>(
            UiOwnerFences{{[&] { return delivery_frame_current; }}}));
        delivery_armed = true;
        check(delivered_fence_owner->shown(delivered_frame, receipt()).status == Status::retired &&
            delivery_callback_ran && delivered_fence_token->quarantined() && delivered_fence_published == 0,
            "post-submission original frame check retires/quarantines stale accepted cue");
        check(delivered_fence_owner->poll().status == Status::retired && delivered_fence_published == 0,
            "stale accepted cue cannot later publish completion");
        check(delivered_fence_owner->shown(delivered_frame, receipt()).status == Status::retired &&
            !delivered_fence_device.at(1) && delivered_fence_published == 0,
            "duplicate stale shown cannot replay a retired accepted cue");
        rejects([&] { CivisNewEraAudioCompletionOwner::open(delivered_fence_token, identity, resources,
            [](const auto &) { return true; }, [](const auto &) {}, delivered_fence_device.factory()); },
            "source callback quarantine cannot be bypassed by replacement owner");

        auto blocked_fence_token = std::make_shared<CivisAudioSessionRetirementToken>(session);
        Devices blocked_fence_device;
        bool blocked_armed = false, blocked_frame_current = true, blocked_callback_ran = false;
        const auto no_payload_resources = std::make_shared<const assets::ResourceMountTable>(mount(root, false));
        assets::ResourceMountTable blocked_watched([&](const assets::AssetKey &key) -> assets::ResourceMountHit {
            if (blocked_armed) {
                blocked_armed = false;
                blocked_callback_ran = true;
                blocked_frame_current = false;
            }
            return {no_payload_resources->resolve(key).asset, true};
        });
        auto blocked_fence_owner = CivisNewEraAudioCompletionOwner::open(blocked_fence_token, identity,
            blocked_watched, [](const auto &) { return true; },
            [](const auto &) { throw std::runtime_error("blocked stale occurrence must not notify"); }, blocked_fence_device.factory());
        const auto blocked_frame = frame().with(std::make_shared<const UiOwnerFences>(
            UiOwnerFences{{[&] { return blocked_frame_current; }}}));
        blocked_armed = true;
        check(blocked_fence_owner->shown(blocked_frame, receipt()).status == Status::retired && blocked_callback_ran &&
            !blocked_fence_device.at(0) && !blocked_fence_token->quarantined(),
            "rejected preflight rechecks original frame but proves no backend effect/no quarantine");

        std::cout << "Civis private NewEra completion owner: PASS (injected device, original mounted PCM; no audible/native Wwise claim)\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
