#include "civ5/content/mod_archive.hpp"
#include "civ5/platform/windows_path.hpp"
#include "7z.h"
#include "7zCrc.h"
#include "7zFile.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <map>
#include <mutex>
#include <stdexcept>
#include <type_traits>

namespace civ5::content {
namespace fs = std::filesystem;
namespace {
void require(bool value, const char *reason) {
    if (!value)
        throw std::runtime_error(reason);
}
void sz_require(SRes value, const char *stage) {
    if (value != SZ_OK)
        throw std::runtime_error(std::string("7z ") + stage + " decoder error " + std::to_string(value));
}
struct BudgetAllocator {
    ISzAlloc functions{allocate, release};
    std::uint64_t used = 0, peak = 0, limit;
    struct alignas(std::max_align_t) Header {
        std::size_t bytes;
    };
    explicit BudgetAllocator(std::uint64_t budget) : limit(budget) {}
    static BudgetAllocator &self(ISzAllocPtr p) noexcept {
        return *reinterpret_cast<BudgetAllocator *>(const_cast<ISzAlloc *>(p));
    }
    static void *allocate(ISzAllocPtr p, std::size_t bytes) noexcept {
        auto &state = self(p);
        if (bytes == 0 || bytes > state.limit - state.used || bytes > SIZE_MAX - sizeof(Header))
            return nullptr;
        auto *allocation = static_cast<Header *>(std::malloc(bytes + sizeof(Header)));
        if (!allocation)
            return nullptr;
        allocation->bytes = bytes;
        state.used += bytes;
        state.peak = std::max(state.peak, state.used);
        return allocation + 1;
    }
    static void release(ISzAllocPtr p, void *allocation) noexcept {
        if (!allocation)
            return;
        auto *header = static_cast<Header *>(allocation) - 1;
        self(p).used -= header->bytes;
        std::free(header);
    }
};
static_assert(std::is_standard_layout_v<BudgetAllocator> && offsetof(BudgetAllocator, functions) == 0);
struct Archive {
    BudgetAllocator allocator;
    CFileInStream file{};
    CLookToRead2 look{};
    CSzArEx database{};
    std::array<Byte, 65536> input{};
    Byte *block = nullptr;
    size_t block_size = 0;
    UInt32 block_index = UINT32_MAX;
    explicit Archive(std::uint64_t budget) : allocator(budget) {
        File_Construct(&file.file);
        FileInStream_CreateVTable(&file);
        LookToRead2_CreateVTable(&look, False);
        look.buf = input.data();
        look.bufSize = input.size();
        look.realStream = &file.vt;
        LookToRead2_INIT(&look) SzArEx_Init(&database);
    }
    ~Archive() {
        allocator.release(&allocator.functions, block);
        SzArEx_Free(&database, &allocator.functions);
        File_Close(&file.file);
    }
};
struct PathLess {
    bool operator()(const std::wstring &a, const std::wstring &b) const {
        return CompareStringOrdinal(a.data(), static_cast<int>(a.size()), b.data(),
                                    static_cast<int>(b.size()), TRUE) == CSTR_LESS_THAN;
    }
};
void write_new(const fs::path &file, const Byte *bytes, std::size_t size) {
    const auto handle =
        CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    require(handle != INVALID_HANDLE_VALUE, "Cannot create new extracted file");
    try {
        std::size_t done = 0;
        while (done < size) {
            const auto count = static_cast<DWORD>(std::min<std::size_t>(65536, size - done));
            DWORD wrote = 0;
            require(WriteFile(handle, bytes + done, count, &wrote, nullptr) && wrote == count,
                    "Extracted file write failed");
            done += wrote;
        }
        require(FlushFileBuffers(handle), "Extracted file flush failed");
    } catch (...) {
        CloseHandle(handle);
        throw;
    }
    CloseHandle(handle);
}
} // namespace
fs::path archive_relative_path(const std::wstring &name) {
    require(!name.empty() && name.size() <= 32760, "Invalid archive filename length");
    std::wstring normalized = name;
    for (std::size_t i = 0; i < normalized.size(); ++i) {
        auto &c = normalized[i];
        require(c >= 32 && std::wstring(L"<>:\"|?*").find(c) == std::wstring::npos,
                "Invalid Windows archive filename");
        if (c == L'/')
            c = L'\\';
        if (c >= 0xd800 && c <= 0xdbff) {
            require(i + 1 < normalized.size() && normalized[i + 1] >= 0xdc00 && normalized[i + 1] <= 0xdfff,
                    "Invalid UTF-16 archive name");
            ++i;
        } else
            require(!(c >= 0xdc00 && c <= 0xdfff), "Invalid UTF-16 archive name");
    }
    require(normalized.front() != L'\\' && normalized.back() != L'\\',
            "Absolute or trailing-separator archive path");
    for (std::size_t begin = 0; begin < normalized.size();) {
        const auto next = normalized.find(L'\\', begin);
        const auto part = normalized.substr(begin, next == std::wstring::npos ? next : next - begin);
        require(!part.empty() && part != L"." && part != L".." && part.back() != L'.' && part.back() != L' ',
                "Archive path traversal or Windows alias");
        auto stem = part.substr(0, part.find(L'.'));
        while (!stem.empty() && stem.back() == L' ')
            stem.pop_back();
        for (auto &c : stem)
            if (c >= L'a' && c <= L'z')
                c = c - L'a' + L'A';
        require(stem != L"CON" && stem != L"PRN" && stem != L"AUX" && stem != L"NUL" && stem != L"CONIN$" &&
                    stem != L"CONOUT$",
                "Device path in archive");
        if (stem.size() == 4 && (stem.starts_with(L"COM") || stem.starts_with(L"LPT")))
            require(std::wstring(L"123456789\u00b9\u00b2\u00b3").find(stem[3]) == std::wstring::npos,
                    "Device path in archive");
        if (next == std::wstring::npos)
            break;
        begin = next + 1;
    }
    return fs::path(normalized);
}
void require_plain_directory(const fs::path &directory) {
    const auto absolute = fs::absolute(directory).lexically_normal();
    fs::path current = absolute.root_path();
    for (const auto &part : absolute.relative_path()) {
        current /= part;
        const auto attributes = GetFileAttributesW(platform::extended_windows_path(current).c_str());
        require(attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    !(attributes & FILE_ATTRIBUTE_REPARSE_POINT),
                "Cache directory is missing or contains a reparse point");
    }
}
ArchiveExtraction extract_mod_7z(const fs::path &archive, const fs::path &destination, ArchiveBudget budget) {
    static_assert(sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(UInt64) == 8);
    static std::once_flag crc;
    std::call_once(crc, CrcGenerateTable);
    const auto output = fs::absolute(destination).lexically_normal();
    require_plain_directory(output.parent_path());
    require(!fs::exists(output), "Extraction destination already exists");
    Archive decoder(budget.maximum_decoder_memory);
    require(InFile_OpenW(&decoder.file.file, archive.c_str()) == 0, "Cannot open 7z input");
    sz_require(SzArEx_Open(&decoder.database, &decoder.look.vt, &decoder.allocator.functions,
                           &decoder.allocator.functions), "header");
    const auto &db = decoder.database;
    require(db.NumFiles <= budget.maximum_entries, "Archive entry count exceeds budget");
    ArchiveExtraction result;
    std::map<std::wstring, bool, PathLess> paths;
    for (UInt32 i = 0; i < db.NumFiles; ++i) {
        const auto length = SzArEx_GetFileNameUtf16(&db, i, nullptr);
        require(length > 1 && length <= 32761, "Invalid archive filename length");
        std::vector<UInt16> raw(length);
        require(SzArEx_GetFileNameUtf16(&db, i, raw.data()) == length && raw.back() == 0,
                "Invalid archive filename");
        std::wstring name(raw.begin(), raw.end() - 1);
        ArchiveEntry entry{archive_relative_path(name), SzArEx_GetFileSize(&db, i),
                           SzArEx_IsDir(&db, i) != 0};
        require(!entry.directory || entry.bytes == 0, "Directory has archive payload");
        if (SzBitWithVals_Check(&db.Attribs, i)) {
            const auto flags = db.Attribs.Vals[i];
            const auto unix_type = (flags >> 16) & 0170000;
            require(!(flags & FILE_ATTRIBUTE_REPARSE_POINT) &&
                        (unix_type == 0 || unix_type == 0100000 || unix_type == 0040000),
                    "Archive contains link or special file");
            require(unix_type != 0040000 || entry.directory, "Inconsistent directory attributes");
        }
        require(paths.emplace(entry.relative.native(), entry.directory).second,
                "Duplicate or case-alias archive path");
        require(entry.bytes <= budget.maximum_expanded_bytes - result.expanded_bytes,
                "Archive expanded size exceeds budget");
        result.expanded_bytes += entry.bytes;
        result.entries.push_back(std::move(entry));
    }
    // Validate the whole directory tree before creating any extracted content.
    for (const auto &entry : result.entries) {
        auto parent = entry.relative.parent_path();
        while (!parent.empty()) {
            const auto found = paths.find(parent.native());
            require(found == paths.end() || found->second, "File/directory collision in archive");
            parent = parent.parent_path();
        }
    }
    require(fs::create_directory(output), "Cannot create extraction directory");
    for (UInt32 i = 0; i < db.NumFiles; ++i) {
        const auto &entry = result.entries[i];
        const auto target = output / entry.relative;
        // All names above are relative and traversal-free; the fresh root has no links.
        fs::create_directories(entry.directory ? target : target.parent_path());
        require_plain_directory(entry.directory ? target : target.parent_path());
        if (entry.directory)
            continue;
        size_t offset = 0, bytes = 0;
        const auto status = SzArEx_Extract(&db, &decoder.look.vt, i, &decoder.block_index,
                                           &decoder.block, &decoder.block_size, &offset, &bytes,
                                           &decoder.allocator.functions, &decoder.allocator.functions);
        if (status != SZ_OK) {
            std::string message = "7z payload decoder error " + std::to_string(status);
            const auto folder_index = db.FileToFolder[i];
            if (folder_index < db.db.NumFolders) {
                const auto begin = db.db.FoCodersOffsets[folder_index];
                const auto end = db.db.FoCodersOffsets[folder_index + 1];
                CSzData data{db.db.CodersData + begin, end - begin};
                CSzFolder folder{};
                if (SzGetNextFolderItem(&folder, &data) == SZ_OK) {
                    message += " (methods";
                    for (UInt32 coder = 0; coder < folder.NumCoders; ++coder)
                        message += " " + std::to_string(folder.Coders[coder].MethodID);
                    message += ")";
                }
            }
            throw std::runtime_error(message);
        }
        require(bytes == entry.bytes && offset <= decoder.block_size && bytes <= decoder.block_size - offset,
                "7z extracted size mismatch");
        write_new(target, bytes ? decoder.block + offset : nullptr, bytes);
    }
    result.peak_decoder_memory = decoder.allocator.peak;
    return result;
}
} // namespace civ5::content
