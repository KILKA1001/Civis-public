#include "civ5/saves/civis_save.hpp"
#include "civ5/runtime/bnw_unit_mission_continuation_checkpoint.hpp"
#include "civ5/runtime/bnw_military_request_checkpoint.hpp"
#include "civ5/runtime/bnw_unit_serial_checkpoint.hpp"
#include "civ5/runtime/bnw_population_totals_checkpoint.hpp"
#include "civ5/runtime/bnw_food_population_growth_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace civ5::saves {
namespace {
constexpr std::array<std::uint8_t, 8> magic{'C', 'I', 'V', 'I', 'S', 'S', 'A', 'V'};
void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
void write_bytes(std::FILE *out, const std::uint8_t *bytes, std::size_t count) {
    while (count) {
        const auto chunk = std::min<std::size_t>(count, 65536);
        require(std::fwrite(bytes, 1, chunk, out) == chunk, "Cannot write Civis save");
        bytes += chunk;
        count -= chunk;
    }
}
void write_u32(std::FILE *out, std::uint32_t value) {
    std::array<std::uint8_t, 4> bytes{};
    for (unsigned i = 0; i < bytes.size(); ++i)
        bytes[i] = static_cast<std::uint8_t>(value >> (8 * i));
    write_bytes(out, bytes.data(), bytes.size());
}
void write_u64(std::FILE *out, std::uint64_t value) {
    std::array<std::uint8_t, 8> bytes{};
    for (unsigned i = 0; i < bytes.size(); ++i)
        bytes[i] = static_cast<std::uint8_t>(value >> (8 * i));
    write_bytes(out, bytes.data(), bytes.size());
}
void read_bytes(std::istream &in, std::uint8_t *bytes, std::size_t count) {
    while (count) {
        const auto chunk = std::min<std::size_t>(count, 65536);
        in.read(reinterpret_cast<char *>(bytes), static_cast<std::streamsize>(chunk));
        require(static_cast<bool>(in), "Truncated Civis save");
        bytes += chunk;
        count -= chunk;
    }
}
std::uint32_t read_u32(std::istream &in) {
    std::array<std::uint8_t, 4> bytes{};
    read_bytes(in, bytes.data(), bytes.size());
    std::uint32_t result = 0;
    for (unsigned i = 0; i < bytes.size(); ++i)
        result |= std::uint32_t(bytes[i]) << (8 * i);
    return result;
}
std::uint64_t read_u64(std::istream &in) {
    std::array<std::uint8_t, 8> bytes{};
    read_bytes(in, bytes.data(), bytes.size());
    std::uint64_t result = 0;
    for (unsigned i = 0; i < bytes.size(); ++i)
        result |= std::uint64_t(bytes[i]) << (8 * i);
    return result;
}
void validate_document(const CivisSaveDocument &document, const CivisSaveBudget &budget,
                       std::uint64_t *total) {
    require(document.version == 1, "Unsupported Civis save version");
    require(document.sections.size() <= budget.maximum_sections, "Too many Civis save sections");
    std::set<std::uint32_t> ids;
    *total = 0;
    for (const auto &section : document.sections) {
        require(section.id != 0 && ids.insert(section.id).second, "Duplicate or empty Civis save section");
        if(section.id==static_cast<std::uint32_t>(CivisSaveSectionId::standalone_services))
            (void)read_standalone_services_section(section);
        if(section.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_unit_serial))
            runtime::validate_bnw_unit_serial_checkpoint_shape(section);
        if(section.id==47) runtime::validate_bnw_food_growth_journal_shape(section);
        if(section.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_population_totals))
            runtime::validate_bnw_population_totals_checkpoint_shape(section);
        if(section.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_military_requests))
            runtime::validate_bnw_military_request_checkpoint_shape(section);
        if(section.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_unit_mission_continuation))
            runtime::validate_bnw_unit_mission_continuation_checkpoint_shape(section);
        require(section.bytes.size() <= budget.maximum_section_bytes,
                "Civis save section exceeds budget");
        const auto bytes = static_cast<std::uint64_t>(section.bytes.size());
        require(bytes <= budget.maximum_total_section_bytes - *total,
                "Civis save sections exceed total budget");
        *total += bytes;
    }
}
struct TemporarySave {
    std::filesystem::path path;
    ~TemporarySave() {
        if (!path.empty()) {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }
    }
};
std::FILE *open_temporary_save(const std::filesystem::path &destination, TemporarySave &temporary) {
    static std::atomic<std::uint64_t> sequence{0};
    for (unsigned attempt = 0; attempt < 32; ++attempt) {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        auto candidate = destination;
        candidate += ".civis-tmp-" + std::to_string(nonce) + "-" +
                     std::to_string(sequence.fetch_add(1, std::memory_order_relaxed));
#ifdef _WIN32
        auto *out = _wfopen(candidate.c_str(), L"wbx");
#else
        auto *out = std::fopen(candidate.c_str(), "wbx");
#endif
        if (out) {
            temporary.path = std::move(candidate);
            return out;
        }
    }
    throw std::runtime_error("Cannot create temporary Civis save");
}
void publish_temporary_save(const std::filesystem::path &temporary,
                            const std::filesystem::path &destination) {
#ifdef _WIN32
    // MoveFileW does not replace an existing destination. The fully written
    // sibling becomes visible in one filesystem operation.
    require(MoveFileW(temporary.c_str(), destination.c_str()) != 0,
            "Cannot publish new Civis save (output may already exist)");
#else
    // Same-directory hard-link publication is atomic and refuses an existing
    // destination. Removing the temporary name leaves the published inode.
    require(::link(temporary.c_str(), destination.c_str()) == 0,
            "Cannot publish new Civis save (output may already exist)");
#endif
}
void flush_temporary_save(std::FILE *out) {
    require(std::fflush(out) == 0, "Cannot flush Civis save");
#ifdef _WIN32
    const auto descriptor = _fileno(out);
    require(descriptor >= 0, "Cannot access Civis save file descriptor");
    const auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(descriptor));
    require(handle != INVALID_HANDLE_VALUE && FlushFileBuffers(handle) != 0,
            "Cannot flush Civis save to storage");
#else
    require(::fsync(fileno(out)) == 0, "Cannot flush Civis save to storage");
#endif
}
} // namespace
void write_civis_save(const std::filesystem::path &path, const CivisSaveDocument &document,
                     CivisSaveBudget budget) {
    std::uint64_t total = 0;
    validate_document(document, budget, &total);
    // Flags=1 selects the import protocol. The SAME import parser must
    // accept every required section and companion before any output exists.
    // Other container flags keep their existing opaque-section contract.
    if(document.flags == 1U) validate_civis_import_document(document);
    const auto header_bytes = std::uint64_t(magic.size()) + 4 + 4 + 8;
    const auto per_section_bytes = std::uint64_t(4 + 4 + 8);
    require(document.sections.size() <=
                (std::numeric_limits<std::uint64_t>::max() - header_bytes) / per_section_bytes,
            "Civis save section table overflows");
    const auto table_bytes = header_bytes + per_section_bytes * document.sections.size();
    require(table_bytes <= budget.maximum_file_bytes &&
                total <= budget.maximum_file_bytes - table_bytes,
            "Civis save exceeds file budget");
    // Write a sibling first: interrupted or failed writes must never leave a
    // partial file under the requested save name.
    TemporarySave temporary;
    const auto out = open_temporary_save(path, temporary);
    std::unique_ptr<std::FILE, decltype(&std::fclose)> close(out, &std::fclose);
    write_bytes(out, magic.data(), magic.size());
    write_u32(out, document.version);
    write_u32(out, document.flags);
    write_u64(out, document.sections.size());
    for (const auto &section : document.sections) {
        write_u32(out, section.id);
        write_u32(out, section.flags);
        write_u64(out, section.bytes.size());
        if (!section.bytes.empty())
            write_bytes(out, section.bytes.data(), section.bytes.size());
    }
    flush_temporary_save(out);
    require(std::fclose(close.release()) == 0, "Cannot close Civis save");
    publish_temporary_save(temporary.path, path);
}
CivisSaveDocument read_civis_save(const std::filesystem::path &path, CivisSaveBudget budget) {
    const auto file_bytes = std::filesystem::file_size(path);
    require(file_bytes <= budget.maximum_file_bytes, "Civis save exceeds file budget");
    std::ifstream in(path, std::ios::binary);
    require(static_cast<bool>(in), "Cannot open Civis save");
    std::array<std::uint8_t, 8> actual{};
    read_bytes(in, actual.data(), actual.size());
    require(actual == magic, "Invalid Civis save magic");
    CivisSaveDocument result;
    result.version = read_u32(in);
    result.flags = read_u32(in);
    const auto count = read_u64(in);
    require(result.version == 1 && count <= budget.maximum_sections,
            "Unsupported or oversized Civis save");
    require(file_bytes >= 24 && count <= (file_bytes - 24) / 16,
            "Civis section count exceeds remaining file");
    require(count <= result.sections.max_size(), "Civis section count exceeds host size");
    result.sections.reserve(static_cast<std::size_t>(count));
    std::uint64_t remaining = file_bytes - 24;
    std::uint64_t total = 0;
    std::set<std::uint32_t> ids;
    for (std::uint64_t i = 0; i < count; ++i) {
        require(remaining >= 16, "Truncated Civis section header");
        CivisSaveSection section;
        section.id = read_u32(in);
        section.flags = read_u32(in);
        const auto bytes = read_u64(in);
        remaining -= 16;
        require(section.id != 0 && ids.insert(section.id).second,
                "Duplicate or empty Civis save section");
        require(bytes <= budget.maximum_section_bytes && bytes <= budget.maximum_total_section_bytes - total,
                "Civis save section exceeds budget");
        // Check the physical file before allocating memory from a wire length.
        require(bytes <= remaining && (count - i - 1) <= (remaining - bytes) / 16,
                "Civis section length exceeds remaining file");
        require(bytes <= section.bytes.max_size(), "Civis save section exceeds host size");
        section.bytes.resize(static_cast<std::size_t>(bytes));
        if (!section.bytes.empty())
            read_bytes(in, section.bytes.data(), section.bytes.size());
        if(section.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_unit_serial))
            runtime::validate_bnw_unit_serial_checkpoint_shape(section);
        if(section.id==47) runtime::validate_bnw_food_growth_journal_shape(section);
        if(section.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_population_totals))
            runtime::validate_bnw_population_totals_checkpoint_shape(section);
        if(section.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_military_requests))
            runtime::validate_bnw_military_request_checkpoint_shape(section);
        if(section.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_unit_mission_continuation))
            runtime::validate_bnw_unit_mission_continuation_checkpoint_shape(section);
        total += bytes;
        remaining -= bytes;
        result.sections.push_back(std::move(section));
    }
    require(in.peek() == std::char_traits<char>::eof(), "Trailing bytes after Civis save sections");
    return result;
}
} // namespace civ5::saves
