#include "civ5/runtime/bnw_creation_config.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <string_view>
#include <thread>
#include <utility>
namespace civ5::runtime {
namespace {
[[noreturn]] void refuse(BnwCreationConfigError code, const std::string &message) {
    throw BnwCreationConfigRefusal(code, message);
}
void choice_text(std::string_view value) {
    if (value.empty() || value.size() > 4096 || value.find('\0') != std::string_view::npos)
        refuse(BnwCreationConfigError::invalid_choice, "Creation choice requires an explicit nonempty Type without NUL");
}
std::int32_t native_id(std::int64_t value) {
    if (value < 0 || value > std::numeric_limits<std::int32_t>::max())
        refuse(BnwCreationConfigError::unsupported_native_metadata, "Creation Info ID exceeds native nonnegative int32");
    return static_cast<std::int32_t>(value);
}
char ascii_lower(unsigned char c) {
    if (c > 127)
        refuse(BnwCreationConfigError::unsupported_native_metadata, "Native case-insensitive choice requires ASCII Type");
    return static_cast<char>(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c);
}
bool ascii_equal(std::string_view a, std::string_view b) {
    // Validate the entire reached native string even when lengths differ.
    for (char c : a) (void)ascii_lower(static_cast<unsigned char>(c));
    for (char c : b) (void)ascii_lower(static_cast<unsigned char>(c));
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (ascii_lower(static_cast<unsigned char>(a[i])) != ascii_lower(static_cast<unsigned char>(b[i]))) return false;
    return true;
}
std::optional<std::int32_t> native_speed_timer_write(std::int32_t speed) {
    switch (speed) {
        case 1: return 1;
        case 2: return 3;
        case 3: return 4;
        default: return std::nullopt;
    }
}
bool same(const content::InfoTypeEntry &a, const content::InfoTypeEntry &b) {
    return a.type == b.type && a.table == b.table && a.id == b.id && a.hash == b.hash;
}
bool same_profile(const SavedContentProfile &a, const SavedContentProfile &b) {
    if (a.kind != b.kind || a.enabled_dlc != b.enabled_dlc || a.enabled_mods != b.enabled_mods ||
        a.all_requirements_located != b.all_requirements_located || a.files_complete != b.files_complete ||
        a.native_mods_require_port != b.native_mods_require_port || a.can_activate != b.can_activate ||
        a.requirements.size() != b.requirements.size()) return false;
    for (std::size_t i = 0; i < a.requirements.size(); ++i) {
        const auto &x = a.requirements[i]; const auto &y = b.requirements[i];
        if (x.kind != y.kind || x.id != y.id || x.name != y.name || x.wire_value != y.wire_value ||
            x.status != y.status || x.package != y.package || x.candidates != y.candidates) return false;
    }
    return true;
}
void bnw_profile(const BnwFreshGameContentResult &s) {
    if (!s.content_ready() || !s.profile.can_activate || s.profile.kind == ContentProfileKind::vanilla ||
        s.profile.native_mods_require_port || !s.activation || s.activation->profile_kind != s.profile.kind)
        refuse(BnwCreationConfigError::profile_not_bnw, "Creation metadata requires a selected compatible BNW rules profile");
    const bool selected_bnw = std::any_of(s.profile.requirements.begin(), s.profile.requirements.end(),
        [&](const ContentRequirement &r) {
            return r.kind == content::PackageKind::dlc && r.status == RequirementStatus::located && r.package &&
                std::find(s.profile.enabled_dlc.begin(), s.profile.enabled_dlc.end(), *r.package) != s.profile.enabled_dlc.end() &&
                content::canonical_guid(r.id) == "6da07636-4123-4018-b643-6575b4ec336b";
        });
    if (!selected_bnw)
        refuse(BnwCreationConfigError::profile_not_bnw, "Creation metadata requires explicitly selected Expansion2");
}
} // namespace
struct BnwCreationConfig::Impl {
    std::thread::id thread = std::this_thread::get_id();
    // Only the real factory publishes this weak facade control. Retained
    // Impl storage is never a substitute for an outside Config facade.
    std::weak_ptr<const BnwCreationConfig> factory_self;
    mutable std::size_t facade_pins=0;
    mutable bool facade_checking=false,facade_failed=false;
    class FacadePin final {
        const Impl& source_;
        const std::shared_ptr<const BnwCreationConfig> actual_;
      public:
        FacadePin(const Impl& source,const BnwCreationConfig& owner)
            :source_(source),actual_(source.factory_self.lock()) {
            if(!actual_||actual_.get()!=&owner||actual_->impl_.get()!=&source_||
               actual_.owner_before(source_.factory_self)||source_.factory_self.owner_before(actual_)||
               source_.facade_pins>=static_cast<std::size_t>(std::numeric_limits<long>::max()))
                refuse(BnwCreationConfigError::rules_retired,"Creation Config canonical factory/pin retired");
            ++source_.facade_pins;
        }
        ~FacadePin(){--source_.facade_pins;} // Before actual facade pin release.
        void require_outside_owner()const {
            if(source_.facade_failed||actual_.use_count()<=static_cast<long>(source_.facade_pins))
                refuse(BnwCreationConfigError::rules_retired,"Creation Config outside facade retired");
        }
        FacadePin(const FacadePin&)=delete;
        FacadePin& operator=(const FacadePin&)=delete;
    };
    void require_published_current(const BnwCreationConfig& owner)const {
        if(thread!=std::this_thread::get_id())
            refuse(BnwCreationConfigError::wrong_thread,"Creation metadata belongs to another thread");
        FacadePin retained(*this,owner);
        if(facade_checking){facade_failed=true;
            refuse(BnwCreationConfigError::rules_retired,"Creation Config currentness reentry");}
        facade_checking=true;
        struct Exit {const Impl& source;~Exit(){source.facade_checking=false;}} exit{*this};
        try {
            retained.require_outside_owner();
            require_current();
            retained.require_outside_owner();
        }catch(...){facade_failed=true;throw;}
    }
    std::shared_ptr<const BnwFreshGameContentResult> parent;
    std::shared_ptr<const BnwSelectedLanguageInputOwner> selected_language_input;
    std::function<bool(const content::RulesDatabase&)> actual_activation_identity;
    const content::RulesDatabase *db = nullptr;
    // Two-argument metadata remains diagnostic. Every route retains its SAME
    // facade; typed admission separately authenticates private activation.
    std::shared_ptr<const content::RulesDatabase> rules_owner;
    std::function<bool()> actual_impl_current;
    BnwCreationMetadata value;
    std::string selected_speed,selected_era;
    SavedContentProfile profile;
    void require_current() const {
        if (thread != std::this_thread::get_id())
            refuse(BnwCreationConfigError::wrong_thread, "Creation metadata belongs to another thread");
        // Compare actual facade identity before dereferencing a possibly retired one.
        if (!parent || !db || !rules_owner || parent->rules.get() != db || rules_owner.get()!=db ||
            parent->rules.owner_before(rules_owner)||rules_owner.owner_before(parent->rules)||
            !actual_impl_current || !actual_impl_current())
            refuse(BnwCreationConfigError::rules_retired, "Creation metadata rules owner retired");
        if (!same_profile(profile, parent->profile))
            refuse(BnwCreationConfigError::rules_retired, "Creation metadata selected profile changed");
        bnw_profile(*parent);
        if(selected_language_input) {
            // Strong authentic parent/facade operands cover actual callbacks.
            std::shared_ptr<const BnwFreshGameContentResult> actual_parent;
            try{actual_parent=BnwFreshGameContentResult::retain_factory_owner(parent);}
            catch(const std::exception& error){
                refuse(BnwCreationConfigError::rules_retired,error.what());
            }
            const auto actual_rules_owner=rules_owner;
            if(parent->activated_rules_!=db||!parent->activated_rules_owner_||
               parent->activated_rules_owner_.get()!=db||
               parent->activated_rules_owner_.owner_before(actual_rules_owner)||
               actual_rules_owner.owner_before(parent->activated_rules_owner_)||!parent->activated_game_root_||
               !actual_activation_identity||!actual_activation_identity(*parent->rules))
                refuse(BnwCreationConfigError::rules_retired,"Typed locale requires SAME actual rules activation occurrence");
            // Own the genuine activation path across actual Early callbacks.
            // A caller can move-assign the mutable result and retire its optional.
            const auto actual_root=*parent->activated_game_root_;
            selected_language_input->require_same_early_game_root(actual_root);
            try{(void)BnwFreshGameContentResult::retain_factory_owner(actual_parent);}
            catch(const std::exception& error){
                refuse(BnwCreationConfigError::rules_retired,error.what());
            }
            if(parent->rules.get()!=db||!parent->activated_rules_owner_||
               parent->rules.owner_before(actual_rules_owner)||actual_rules_owner.owner_before(parent->rules)||
               parent->activated_rules_owner_.get()!=db||
               parent->activated_rules_owner_.owner_before(actual_rules_owner)||
               actual_rules_owner.owner_before(parent->activated_rules_owner_)||parent->activated_rules_!=db||
               !parent->activated_game_root_||*parent->activated_game_root_!=actual_root||
               !actual_activation_identity(*parent->rules)||
               !actual_impl_current||!actual_impl_current()||!same_profile(profile,parent->profile))
                refuse(BnwCreationConfigError::rules_retired,"Typed creation activation retired during actual input callback");
            bnw_profile(*parent);
        }
        if (!db->immutable_preview_fingerprints_current() || db->gameplay_fingerprint_sha256() != value.rules_sha256)
            refuse(BnwCreationConfigError::rules_retired, "Creation metadata rules changed");
    }
    BnwCreationInfoBinding bind(std::string_view table, std::string_view requested,
                                bool native_string_scan, bool native_collection,
                                std::vector<BnwCreationSpeedAssignment> *speed_calls = nullptr) const {
        require_current(); choice_text(requested);
        // GC's global maps own distinct value copies. Compare all tuple fields.
        const auto types = db->cache_info_types(); require_current();
        if (!types.report().all_succeeded())
            refuse(BnwCreationConfigError::unsupported_native_metadata, "Creation Info catalog read failed");
        std::optional<BnwCreationInfoBinding> chosen;
        const auto accept = [&](std::int64_t id, std::string type) {
            choice_text(type); const auto id32 = native_id(id);
            const auto hash = content::saved_info_type_hash(type);
            const auto *named = types.find(type); const auto *hashed = types.find_hash(hash);
            if (!named || !hashed || !same(*named, *hashed) || named->table != table || named->type != type ||
                named->id != id || named->hash != hash)
                refuse(BnwCreationConfigError::unsupported_native_metadata, "Creation Info Type/catalog identity is ambiguous");
            chosen = BnwCreationInfoBinding{std::string(table), std::move(type), id32, hash};
            if (speed_calls) speed_calls->push_back({*chosen, native_speed_timer_write(id32)});
        };
        if (native_collection) {
            // Native Prefetch appends holes and overlapping rows. Never infer a
            // zero/default row, never index by authored MAX(ID), and skip holes.
            const auto report = db->prefetch_info(std::string(table),
                [&](std::uint64_t slot, std::int64_t authored, const content::RulesRowView &row) {
                    require_current();
                    if (slot > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) ||
                        !row.has_column("ID") || !row.has_column("Type") || row.integer("ID") != authored)
                        refuse(BnwCreationConfigError::unsupported_native_metadata, "Creation native Info span or schema unsupported");
                    (void)native_id(authored);
                    const auto type = row.text("Type");
                    if (!type)
                        refuse(BnwCreationConfigError::unsupported_native_metadata, "Creation native Info has no Type");
                    choice_text(*type);
                    if (native_string_scan && ascii_equal(requested, *type)) {
                        // CvPreGame loops all nonnull Info and assigns every
                        // match. Last qualifying native slot wins, including gaps.
                        if (slot != static_cast<std::uint64_t>(authored))
                            refuse(BnwCreationConfigError::unsupported_native_metadata, "Creation Type alias has different authored/native slot");
                        accept(authored, *type);
                    } else if (!native_string_scan && requested == *type) {
                        if (slot != static_cast<std::uint64_t>(authored))
                            refuse(BnwCreationConfigError::unsupported_native_metadata, "Creation Handicap has different authored/native slot");
                        accept(authored, *type);
                    }
                    return true;
                }, content::InfoCollectionBudget{65536});
            require_current();
            if (!report.all_succeeded() || report.slots > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
                refuse(BnwCreationConfigError::unsupported_native_metadata, "Creation native Info collection incomplete");
        } else {
            // Direct pregame cached World/Climate/SeaLevel Type lookup; no rowid
            // equality assumption (CacheResult stores the authored ID).
            const auto report = db->query_rows({"SELECT ID, Type FROM " + std::string(table) + " WHERE Type = ? LIMIT 1", {std::string(requested)}},
                [&](const content::RulesRowView &row) {
                    require_current();
                    if (!row.has_column("ID") || !row.has_column("Type"))
                        refuse(BnwCreationConfigError::unsupported_native_metadata, "Creation cached Info schema incomplete");
                    const auto type = row.text("Type");
                    if (!type || *type != requested)
                        refuse(BnwCreationConfigError::unsupported_native_metadata, "Creation Type lookup changed identity");
                    accept(row.integer("ID"), *type);
                }, 1);
            require_current();
            if (!report.all_succeeded())
                refuse(BnwCreationConfigError::unsupported_native_metadata, "Creation cached Info lookup failed");
        }
        if (!chosen)
            refuse(BnwCreationConfigError::invalid_choice, "Creation choice missing from " + std::string(table));
        return *chosen;
    }
};
BnwCreationConfigRefusal::BnwCreationConfigRefusal(BnwCreationConfigError c, const std::string &m)
    : std::runtime_error(m), code_(c) {}
BnwCreationConfig::BnwCreationConfig(std::shared_ptr<Impl> p) : impl_(std::move(p)) {}
std::shared_ptr<const BnwCreationConfig> BnwCreationConfig::from_selected_rules(
    std::shared_ptr<const BnwFreshGameContentResult> parent, BnwCreationChoices choices) {
    if (!parent || !parent->rules)
        refuse(BnwCreationConfigError::parent_unavailable, "Creation metadata needs actual selected rules");
    auto p = std::make_shared<Impl>(); p->parent = std::move(parent); p->rules_owner=p->parent->rules; p->db = p->rules_owner.get();
    if (!p->db->immutable_preview_fingerprints_current())
        refuse(BnwCreationConfigError::rules_not_immutable, "Creation metadata requires already sealed SAME immutable rules");
    bnw_profile(*p->parent); p->profile = p->parent->profile;
    p->selected_speed=choices.speed;p->selected_era=choices.era;
    p->actual_impl_current = p->db->immutable_preview_fingerprint_currentness();
    p->value.rules_sha256 = p->db->gameplay_fingerprint_sha256(); p->require_current();
    p->value.info[0] = p->bind("Worlds", choices.world, false, false);
    p->value.info[1] = p->bind("Climates", choices.climate, false, false);
    p->value.info[2] = p->bind("SeaLevels", choices.sea_level, false, false);
    p->value.info[3] = p->bind("GameSpeeds", choices.speed, true, true, &p->value.speed_assignments);
    p->value.info[4] = p->bind("Eras", choices.era, true, true);
    p->value.info[5] = p->bind("HandicapInfos", choices.handicap, false, true);
    // Every native string match invokes the numeric setter. A later call
    // with no timer write does not erase a preceding reached timer write.
    for (const auto &call : p->value.speed_assignments)
        if (call.timer_write) p->value.speed_timer_write = call.timer_write;
    p->require_current();
    auto result=std::shared_ptr<const BnwCreationConfig>(new BnwCreationConfig(std::move(p)));
    result->impl_->factory_self=result;return result;
}
std::shared_ptr<const BnwCreationConfig> BnwCreationConfig::from_selected_rules(
    std::shared_ptr<const BnwFreshGameContentResult> parent,BnwCreationChoices choices,
    std::shared_ptr<const BnwSelectedLanguageInputOwner> input) {
    if(!parent||!input)
        refuse(BnwCreationConfigError::parent_unavailable,"Typed creation needs genuine rules activation and early input");
    if(parent->owner_thread_!=std::this_thread::get_id())
        refuse(BnwCreationConfigError::wrong_thread,"Typed creation belongs to another content thread");
    // Only genuine factory Content admission can reach actual input callbacks.
    try{parent=BnwFreshGameContentResult::retain_factory_owner(parent);}
    catch(const std::exception& error){
        refuse(BnwCreationConfigError::parent_unavailable,error.what());
    }
    const auto actual_rules_owner=parent->activated_rules_owner_;
    if(!parent->activated_game_root_||!actual_rules_owner||
       parent->activated_rules_!=parent->rules.get()||actual_rules_owner.get()!=parent->rules.get()||
       parent->rules.owner_before(actual_rules_owner)||actual_rules_owner.owner_before(parent->rules))
        refuse(BnwCreationConfigError::parent_unavailable,"Typed creation needs genuine rules activation and early input");
    const auto actual_root=*parent->activated_game_root_;
    const auto* const actual_rules=parent->rules.get();
    if(!actual_rules)
        refuse(BnwCreationConfigError::parent_unavailable,"Typed creation actual activated rules absent");
    const auto actual_identity=parent->activated_rules_identity_;
    const auto actual_profile=parent->profile;
    if(!actual_identity||!actual_identity(*actual_rules))
        refuse(BnwCreationConfigError::rules_retired,"Typed creation actual rules identity retired");
    input->require_same_early_game_root(actual_root);
    try{(void)BnwFreshGameContentResult::retain_factory_owner(parent);}
    catch(const std::exception& error){
        refuse(BnwCreationConfigError::rules_retired,error.what());
    }
    // Pointer checks precede candidate facade access; the private actual Impl
    // brand rejects a replacement even if a new facade reuses an address.
    if(parent->rules.get()!=actual_rules||!parent->activated_rules_owner_||
       parent->rules.owner_before(actual_rules_owner)||actual_rules_owner.owner_before(parent->rules)||
       parent->activated_rules_owner_.get()!=actual_rules||
       parent->activated_rules_owner_.owner_before(actual_rules_owner)||
       actual_rules_owner.owner_before(parent->activated_rules_owner_)||parent->activated_rules_!=actual_rules||
       !parent->activated_game_root_||*parent->activated_game_root_!=actual_root||
       !actual_identity(*parent->rules)||!same_profile(actual_profile,parent->profile))
        refuse(BnwCreationConfigError::rules_retired,"Typed creation activation replaced during actual input callback");
    bnw_profile(*parent);
    auto result=from_selected_rules(std::move(parent),std::move(choices));
    result->impl_->actual_activation_identity=actual_identity;
    result->impl_->selected_language_input=std::move(input);result->require_current();return result;
}
std::shared_ptr<const BnwFreshGameContentResult> BnwCreationConfig::borrow_activated_content_for_game_setup()const {
    const auto storage=impl_;storage->require_published_current(*this);
    auto actual=BnwFreshGameContentResult::retain_factory_owner(storage->parent);
    const auto& rules=actual->activated_rules_owner_;
    if(!actual->activated_game_root_||!rules||rules.get()!=storage->db||actual->rules.get()!=storage->db||
       actual->rules.owner_before(rules)||rules.owner_before(actual->rules)||actual->activated_rules_!=storage->db||
       !actual->activated_rules_identity_||!actual->activated_rules_identity_(*rules))
        refuse(BnwCreationConfigError::rules_retired,"Gameplay setup requires SAME genuine activated Content/rules source");
    storage->require_published_current(*this);(void)BnwFreshGameContentResult::retain_factory_owner(actual);
    return actual;
}
std::shared_ptr<const BnwSelectedLanguageInputOwner> BnwCreationConfig::borrow_selected_language_input()const {
    require_current();const auto input=impl_->selected_language_input;require_current();return input;
}
void BnwCreationConfig::require_current() const {
    // Real factory facade pin and local Impl hold cover all actual callbacks;
    // no access to this/impl_ follows outside-facade retirement.
    const auto source=impl_;
    source->require_published_current(*this);
}
BnwCreationMetadata BnwCreationConfig::metadata() const { require_current(); return impl_->value; }
std::string BnwCreationConfig::selected_speed_input() const {require_current();return impl_->selected_speed;}
std::string BnwCreationConfig::selected_era_input() const {require_current();return impl_->selected_era;}
const content::RulesDatabase& BnwCreationConfig::game_rules() const {
    require_current();return *impl_->db;
}
} // namespace civ5::runtime
