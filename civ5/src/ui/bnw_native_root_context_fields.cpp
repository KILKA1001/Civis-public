#include "civ5/ui/detail/bnw_native_root_context_fields.hpp"
#include <atomic>
#include "civ5/content/info_types.hpp"
#include <map>
#include <limits>
#include <stdexcept>
#include <thread>

namespace civ5::ui::detail {
namespace {
void need(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
}
struct BnwNativeRootContextFields::Cell {
    const std::thread::id thread = std::this_thread::get_id();
    BnwRootContextFieldBinding binding;
    const std::shared_ptr<const std::uint8_t> instance = std::make_shared<const std::uint8_t>(0);
    BnwRootContextStoredFields fields;
    std::map<std::uint32_t,BnwRootInstanceMetadataEntry> templates;
    std::uint64_t revision = 1;
    std::atomic<bool> closed{false}, failed{false}, busy{false};
    explicit Cell(BnwRootContextFieldBinding owned) : binding(std::move(owned)) {}
    struct Scope {
        std::shared_ptr<Cell> cell;
        explicit Scope(std::shared_ptr<Cell> retained) : cell(std::move(retained)) {
            need(std::this_thread::get_id() == cell->thread, "Root Context foreign thread");
            need(!cell->closed.load() && !cell->failed.load(), "Root Context retired");
            if (cell->busy.exchange(true)) {
                cell->failed.store(true);
                throw std::runtime_error("Root Context reentry");
            }
        }
        ~Scope() { cell->busy.store(false); }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
    };
    void source_current() {
        need(!closed.load() && !failed.load(), "Root Context retired during callback");
        // Copy callback before any external code; the retained Cell survives
        // deletion of the last public component owner inside this callback.
        try {
            const auto callback = binding.current;
            need(callback && callback(), "Root Context source/client changed");
            need(!closed.load() && !failed.load(), "Root Context callback lost ownership");
        } catch (...) {
            failed.store(true);
            throw;
        }
    }
    void advance() {
        need(revision != std::numeric_limits<std::uint64_t>::max(), "Root Context revision exhausted");
        ++revision;
    }
};
struct BnwRootContextFieldSnapshot::Data {
    BnwRootContextStoredFields fields;
    std::map<std::uint32_t,BnwRootInstanceMetadataEntry> templates;
    BnwRootContextFieldIdentity identity;
    std::uint64_t revision = 0;
    std::weak_ptr<BnwNativeRootContextFields::Cell> cell;
};
BnwRootContextFieldSnapshot::BnwRootContextFieldSnapshot(std::shared_ptr<const Data> data) : data_(std::move(data)) {}
const BnwRootContextStoredFields& BnwRootContextFieldSnapshot::fields() const { return data_->fields; }
const BnwRootContextFieldIdentity& BnwRootContextFieldSnapshot::identity() const { return data_->identity; }
std::uint64_t BnwRootContextFieldSnapshot::revision() const { return data_->revision; }
bool BnwRootContextFieldSnapshot::current() const noexcept {
    auto cell = data_->cell.lock();
    if (!cell || std::this_thread::get_id() != cell->thread) return false;
    try {
        BnwNativeRootContextFields::Cell::Scope scope(cell);
        if (data_->revision != cell->revision) return false;
        cell->source_current();
        return data_->revision == cell->revision && !cell->closed.load() && !cell->failed.load();
    } catch (...) { return false; }
}
BnwNativeRootContextFields::BnwNativeRootContextFields(BnwRootContextFieldBinding binding) {
    need(binding.document && binding.document->name == "Context" && binding.current,
         "Root Context requires actual selected XML/source guard");
    need(!binding.source_path.empty() && binding.xml_sha256.size() == 64,
         "Root Context source stamp missing");
    for (char c : binding.xml_sha256) need((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'), "Root Context malformed source stamp");
    cell_ = std::make_shared<Cell>(std::move(binding));
    auto cell = cell_;
    Cell::Scope scope(cell);
    cell->source_current();
}
BnwNativeRootContextFields::~BnwNativeRootContextFields() { close(); }
BnwRootContextFieldSnapshot BnwNativeRootContextFields::snapshot() const {
    auto cell = cell_;
    Cell::Scope scope(cell);
    cell->source_current();
    auto data = std::make_shared<BnwRootContextFieldSnapshot::Data>();
    data->fields = cell->fields;
    data->templates = cell->templates;
    data->identity = {cell->instance, cell->binding.document, cell->binding.source_path, cell->binding.xml_sha256};
    data->revision = cell->revision;
    data->cell = cell;
    cell->source_current();
    return BnwRootContextFieldSnapshot(std::move(data));
}
void BnwNativeRootContextFields::read_empty_declaration() {
    auto cell = cell_;
    Cell::Scope scope(cell);
    need(!cell->fields.empty_declaration_read_modeled, "Root Context empty declaration cannot replay");
    const auto document = cell->binding.document;
    need(document->attributes.empty(), "Root Context declaration needs actual registry/setter owner");
    for (const auto& child : document->children)
        need(child.name != "ColorSet", "Root Context direct ColorSet needs actual registry owner");
    cell->source_current();
    cell->advance();
    try {
        cell->fields.empty_declaration_read_modeled = true;
        cell->source_current();
    } catch (...) { cell->closed.store(true); throw; }
}
std::optional<BnwRootContextInstanceTemplateQuote>
BnwRootContextFieldSnapshot::instance_template(std::string_view input) const {
    // Retain immutable inputs before source callbacks can release this facade.
    const auto data = data_;
    const auto receipt = *this;
    const std::string name(input);
    const auto hash = content::saved_info_type_hash(name);
    need(receipt.current(), "Root Context template lookup stale");
    need(data->fields.instance_metadata_prefix_modeled, "Root Context template metadata absent");
    const auto found = data->templates.find(hash);
    std::optional<BnwRootContextInstanceTemplateQuote> result;
    if (found != data->templates.end()) result.emplace(BnwRootContextInstanceTemplateQuote{receipt,found->second});
    need(receipt.current(), "Root Context template lookup lost source/owner");
    return result;
}
void BnwNativeRootContextFields::register_instance_metadata_prefix() {
    auto cell = cell_;
    Cell::Scope scope(cell);
    need(cell->fields.empty_declaration_read_modeled, "Root Context declarations must precede metadata");
    need(!cell->fields.instance_metadata_prefix_modeled, "Root Context metadata cannot replay");
    const auto document = cell->binding.document;
    std::map<std::uint32_t,BnwRootInstanceMetadataEntry> planned;
    std::vector<BnwRootInstanceMetadataEntry> journal;
    std::size_t cursor = 0;
    for (const auto& node : document->children) {
        if (node.name != "Instance") break;
        const std::string* name = nullptr;
        // Canonical selected profile admission, not a replacement XML parser.
        // Duplicate/extra attributes, non-string/NUL input and mixed node text
        // are not an attested8355C0/8352A0 conversion and must not fallback.
        need(node.attributes.size()==1 && node.attributes[0].first=="Name" &&
             node.text.find_first_not_of(" \t\r\n")==std::string::npos,
             "Root Instance requires canonical Name source/conversion");
        name = &node.attributes[0].second;
        need(!name->empty(), "Root Instance empty Name conversion unavailable");
        for (const unsigned char c : *name)
            need(c >= 0x20 && c <= 0x7e, "Root Instance Name source conversion unavailable");
        BnwRootInstanceMetadataEntry entry{cursor,*name,content::saved_info_type_hash(*name),{},node.children.empty()};
        if (!node.children.empty()) {
            need(!node.children.front().name.empty(), "Root Instance malformed first-child source");
            entry.first_child = std::shared_ptr<const content::XmlNode>(document,&node.children.front());
            planned.insert_or_assign(entry.hash,entry); //7A9FF5: last same hash wins.
        }
        journal.push_back(std::move(entry));
        ++cursor;
    }
    need(cursor != 0, "Root Context metadata prefix absent");
    cell->source_current();
    cell->advance();
    try {
        // Allocation/validation precedes mutation; a later source failure
        // retires SAME cell, not an invented rollback/detached atomic receipt.
        cell->templates = std::move(planned);
        cell->fields.instance_metadata_journal = std::move(journal);
        cell->fields.next_source_cursor = cursor;
        cell->fields.instance_template_count = cell->templates.size();
        cell->fields.instance_metadata_prefix_modeled = true;
        cell->source_current();
    } catch (...) { cell->closed.store(true); throw; }
}
void BnwNativeRootContextFields::retire_for_graph_reset() {
    auto cell = cell_;
    Cell::Scope scope(cell);
    cell->source_current();
    cell->advance();
    cell->closed.store(true);
}
void BnwNativeRootContextFields::close() noexcept {
    if (cell_) cell_->closed.store(true);
}
}
