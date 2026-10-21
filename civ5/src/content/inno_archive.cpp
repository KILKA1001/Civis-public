#include "civ5/content/inno_archive.hpp"
#include "civ5/assets/asset_source.hpp"
#include "civ5/content/mod_acquisition.hpp"
#include "civ5/runtime/binary_reader.hpp"
#include "civ5/runtime/file_identity.hpp"
#define NOMINMAX
#include "inno_decoder_pins.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <memory>
#include <objbase.h>
#include <set>
#include <stdexcept>
#include <windows.h>

namespace civ5::content {
namespace fs = std::filesystem;
namespace {
constexpr std::uint64_t installer_bytes = 110473850, installer_files = 2393, expanded_bytes = 278892422;
// SHA-256 of sorted UTF-8/LF records: "sha256 bytes relative/path\n".
// Independently measured with .NET after checksum-verified extraction; includes
// ALL variants.
constexpr auto inventory_sha256 = "f53e24aa3a120961aa9f46e98e04f8a25c1af95caea30606b51604323f9733db";
constexpr auto installer_url = "https://github.com/LoneGazebo/Community-Patch-DLL/releases/download/"
                               "Release-5.4.2/Vox.Populi.5.4.2.exe";
void require(bool value, const char *reason) {
    if (!value)
        throw std::runtime_error(reason);
}
struct Handle {
    HANDLE value = INVALID_HANDLE_VALUE;
    explicit Handle(HANDLE handle = INVALID_HANDLE_VALUE) : value(handle) {}
    Handle(const Handle &) = delete;
    Handle &operator=(const Handle &) = delete;
    ~Handle() {
        if (value && value != INVALID_HANDLE_VALUE)
            CloseHandle(value);
    }
};
using Handles = std::vector<std::unique_ptr<Handle>>;
Handles lock_decoder(const fs::path &directory) {
    require_plain_directory(directory);
    std::set<std::string> expected;
    Handles locks;
    for (const auto &pin : decoder_pins) {
        expected.insert(pin.name);
        const auto path = package_file(directory, pin.name);
        auto handle =
            std::make_unique<Handle>(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        require(handle->value != INVALID_HANDLE_VALUE, "Cannot lock decoder binary");
        require(runtime::identify_file(path).sha256 == pin.sha256, "Untrusted or changed decoder binary");
        std::ifstream stream(path, std::ios::binary);
        runtime::BinaryReader image(stream, fs::file_size(path));
        require(image.u8() == 'M' && image.u8() == 'Z', "Decoder is not a PE image");
        image.seek(0x3c);
        const auto pe_offset = image.u32();
        image.seek(pe_offset);
        require(image.u32() == 0x4550 && image.u8() == 0x64 && image.u8() == 0x86,
                "Decoder dependency is not AMD64");
        locks.push_back(std::move(handle));
    }
    for (const auto &item : fs::directory_iterator(directory)) {
        const auto extension = runtime::ascii_lower(assets::path_utf8(item.path().extension()));
        const auto name = runtime::ascii_lower(assets::path_utf8(item.path().filename()));
        require((extension != ".dll" && extension != ".exe" && name != "innoextract.exe.local") ||
                    expected.contains(name),
                "Unexpected binary or loader override in decoder directory");
    }
    return locks;
}
void run_decoder(const fs::path &directory, const fs::path &stage, ArchiveBudget budget) {
    auto locks = lock_decoder(directory); // Held until exit: no replacement of helper or DLLs.
    Handle job(CreateJobObjectW(nullptr, nullptr));
    require(job.value != nullptr, "Cannot create decoder job");
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
                                              JOB_OBJECT_LIMIT_ACTIVE_PROCESS |
                                              JOB_OBJECT_LIMIT_PROCESS_MEMORY;
    limits.BasicLimitInformation.ActiveProcessLimit = 1;
    limits.ProcessMemoryLimit = static_cast<SIZE_T>(budget.maximum_decoder_memory);
    require(SetInformationJobObject(job.value, JobObjectExtendedLimitInformation, &limits, sizeof(limits)),
            "Cannot set decoder resource limits");
    SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
    Handle log(CreateFileW((stage / "decoder.log").c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security,
                           CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
    Handle input(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security,
                             OPEN_EXISTING, 0, nullptr));
    require(log.value != INVALID_HANDLE_VALUE && input.value != INVALID_HANDLE_VALUE,
            "Cannot open decoder logs");
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = input.value;
    startup.StartupInfo.hStdOutput = startup.StartupInfo.hStdError = log.value;
    SIZE_T attribute_bytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attribute_bytes);
    std::vector<std::uint8_t> attributes(attribute_bytes);
    startup.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
    require(InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &attribute_bytes),
            "Cannot initialize decoder process attributes");
    struct AttributeCleanup {
        LPPROC_THREAD_ATTRIBUTE_LIST value;
        ~AttributeCleanup() { DeleteProcThreadAttributeList(value); }
    } cleanup{startup.lpAttributeList};
    HANDLE inherited[] = {log.value, input.value};
    require(UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                      inherited, sizeof(inherited), nullptr, nullptr),
            "Cannot limit inherited decoder handles");
    std::array<wchar_t, 32768> windows{};
    const auto length = GetWindowsDirectoryW(windows.data(), static_cast<UINT>(windows.size()));
    require(length && length < windows.size(), "Cannot locate Windows directory");
    const std::wstring system_root(windows.data(), length);
    std::wstring environment;
    for (const auto &entry :
         {L"PATH=" + system_root + L"\\System32", L"SystemRoot=" + system_root, L"TEMP=" + stage.native(),
          L"TMP=" + stage.native(), L"WINDIR=" + system_root}) {
        environment += entry;
        environment += L'\0';
    }
    environment += L'\0';
    const auto executable = directory / "innoextract.exe";
    std::wstring command = quote_windows_argument(executable.native());
    for (const auto &argument :
         std::vector<std::wstring>{L"--extract", L"--collisions", L"rename-all", L"--color=0",
                                   L"--progress=0", L"--no-extract-unknown", L"--output-dir",
                                   (stage / "payload").native(), (stage / "installer.bin").native()})
        command += L' ' + quote_windows_argument(argument);
    require(command.size() < 32767, "Decoder command exceeds Windows argument limit");
    PROCESS_INFORMATION process{};
    require(CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
                           CREATE_NO_WINDOW | CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT |
                               EXTENDED_STARTUPINFO_PRESENT,
                           environment.data(), directory.c_str(), &startup.StartupInfo, &process),
            "Cannot start the verified x64 decoder");
    Handle child(process.hProcess), thread(process.hThread);
    if (!AssignProcessToJobObject(job.value, child.value)) {
        TerminateProcess(child.value, 1);
        WaitForSingleObject(child.value, 5000);
        throw std::runtime_error("Cannot contain decoder process");
    }
    require(ResumeThread(thread.value) != static_cast<DWORD>(-1), "Cannot resume decoder");
    const auto started = GetTickCount64();
    for (;;) {
        const auto result = WaitForSingleObject(child.value, 250);
        if (result == WAIT_OBJECT_0)
            break;
        require(result == WAIT_TIMEOUT && GetTickCount64() - started < 240000, "Decoder failed or timed out");
    }
    DWORD exit_code = 0;
    require(GetExitCodeProcess(child.value, &exit_code) && exit_code == 0,
            "Installer decoding/checksum verification failed; see decoder.log");
}
std::string inventory(const fs::path &root, ArchiveBudget budget) {
    require_plain_directory(root);
    std::map<std::string, runtime::FileIdentity> files;
    std::uint64_t total = 0, records_bytes = 0, entries = 0;
    for (fs::recursive_directory_iterator it(root), end; it != end; ++it) {
        require(++entries <= budget.maximum_entries, "Installer entry count exceeds budget");
        const auto attributes = GetFileAttributesW(it->path().c_str());
        require(attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_REPARSE_POINT),
                "Reparse point in installer payload");
        const auto relative = it->path().lexically_relative(root);
        archive_relative_path(relative.native());
        if (it->is_directory())
            continue;
        require(it->is_regular_file(), "Non-regular installer payload file");
        const auto size = it->file_size();
        require(size <= budget.maximum_expanded_bytes - total, "Installer expanded size exceeds budget");
        total += size;
        auto name = assets::path_utf8(relative);
        std::replace(name.begin(), name.end(), '\\', '/');
        records_bytes += name.size() + 90;
        require(records_bytes <= 16 * 1024 * 1024, "Installer inventory exceeds metadata budget");
        require(files.emplace(std::move(name), runtime::identify_file(it->path())).second,
                "Duplicate installer payload path");
    }
    require(files.size() == installer_files && total == expanded_bytes,
            "Pinned installer payload size/count changed");
    std::string result;
    for (const auto &[path, identity] : files)
        result += identity.sha256 + ' ' + std::to_string(identity.bytes) + ' ' + path + '\n';
    return result;
}
void write_inventory(const fs::path &file, const std::string &text) {
    Handle output(
        CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
    require(output.value != INVALID_HANDLE_VALUE, "Cannot create bundle inventory");
    std::size_t offset = 0;
    while (offset < text.size()) {
        const auto count = static_cast<DWORD>(std::min<std::size_t>(65536, text.size() - offset));
        DWORD written = 0;
        require(WriteFile(output.value, text.data() + offset, count, &written, nullptr) && written == count,
                "Cannot write bundle inventory");
        offset += written;
    }
    require(FlushFileBuffers(output.value), "Cannot flush bundle inventory");
}
void verify_installer(const fs::path &file) {
    const auto identity = runtime::identify_file(file);
    require(identity.bytes == installer_bytes && identity.sha256 == vp_542_installer_sha256,
            "Not the pinned official VP 5.4.2 installer");
}
InstallerBundle verified_bundle(const fs::path &root, ArchiveBudget budget) {
    require_plain_directory(root);
    verify_installer(package_file(root, "installer.bin"));
    const auto record = package_file(root, "inventory.txt");
    require(runtime::identify_file(record).sha256 == inventory_sha256,
            "Installer inventory identity changed");
    const auto actual = inventory(root / "payload", budget);
    std::ifstream stream(record, std::ios::binary);
    const std::string expected{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    require(actual == expected, "Installer bundle payload changed");
    return {root, vp_542_installer_sha256, installer_bytes, installer_files, expanded_bytes, true};
}
} // namespace
std::wstring quote_windows_argument(const std::wstring &argument) {
    require(argument.find(L'\0') == std::wstring::npos, "NUL in process argument");
    std::wstring quoted = L"\"";
    std::size_t slashes = 0;
    for (const auto c : argument) {
        if (c == L'\\') {
            ++slashes;
            continue;
        }
        quoted.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
        quoted += c;
        slashes = 0;
    }
    quoted.append(slashes * 2, L'\\');
    return quoted + L'"';
}
void verify_inno_decoder(const fs::path &directory) { auto locks = lock_decoder(directory); }
InstallerBundle verify_vp_542_bundle(const fs::path &root, ArchiveBudget budget) {
    return verified_bundle(fs::absolute(root).lexically_normal(), budget);
}
InstallerBundle acquire_vp_542_bundle(const fs::path &cache, const fs::path &decoder,
                                      const fs::path &local_installer, platform::DownloadProgress progress,
                                      ArchiveBudget budget) {
    require(budget.maximum_expanded_bytes >= expanded_bytes && budget.maximum_entries >= installer_files &&
                budget.maximum_decoder_memory > 0,
            "Pinned installer exceeds caller budget");
    if (!local_installer.empty())
        verify_installer(local_installer);
    initialize_mod_cache(cache);
    const auto canonical_cache = fs::canonical(cache);
    const auto bundles = canonical_cache / "bundles";
    fs::create_directory(bundles);
    require_plain_directory(bundles);
    const auto destination = bundles / vp_542_installer_sha256;
    if (fs::exists(destination))
        return verified_bundle(destination, budget);
    const auto decoder_root = fs::absolute(decoder).lexically_normal();
    verify_inno_decoder(decoder_root); // Fail before downloading if bootstrap is unavailable.
    GUID guid{};
    require(SUCCEEDED(CoCreateGuid(&guid)), "Cannot allocate installer stage");
    wchar_t id[40]{};
    require(StringFromGUID2(guid, id, 40) != 0, "Cannot format installer stage ID");
    const auto stage = canonical_cache / "staging" / std::wstring(id + 1, 36);
    require_plain_directory(stage.parent_path());
    require(fs::create_directory(stage), "Cannot create installer stage");
    if (local_installer.empty())
        platform::download_https({installer_url, {}}, stage / "installer.bin", installer_bytes,
                                 std::move(progress));
    else
        require(fs::copy_file(local_installer, stage / "installer.bin", fs::copy_options::none),
                "Cannot stage local installer");
    verify_installer(stage / "installer.bin");
    run_decoder(decoder_root, stage, budget);
    write_inventory(stage / "inventory.txt", inventory(stage / "payload", budget));
    require(runtime::identify_file(stage / "inventory.txt").sha256 == inventory_sha256,
            "Decoded installer does not match the independently pinned file "
            "inventory");
    require_plain_directory(bundles);
    require(fs::canonical(stage.parent_path()) == canonical_cache / "staging" &&
                fs::canonical(bundles) == canonical_cache / "bundles" && !fs::exists(destination),
            "Installer publication destination changed");
    // Antivirus/indexing may briefly keep a newly decoded DLL open without
    // delete sharing. Retry only these Windows failures and never replace an
    // occupied destination. Wide APIs also avoid locale-dependent path errors.
    const auto rename_started = GetTickCount64();
    while (!MoveFileExW(stage.c_str(), destination.c_str(), MOVEFILE_WRITE_THROUGH)) {
        const auto error = GetLastError();
        if ((error != ERROR_ACCESS_DENIED && error != ERROR_SHARING_VIOLATION && error != ERROR_LOCK_VIOLATION) ||
            GetTickCount64() - rename_started >= 10000)
            throw std::runtime_error("Cannot publish installer bundle; Windows error " + std::to_string(error));
        Sleep(100);
        require_plain_directory(bundles);
        require(fs::canonical(stage.parent_path()) == canonical_cache / "staging" && !fs::exists(destination),
                "Installer publication destination changed during retry");
    }
    return {destination, vp_542_installer_sha256, installer_bytes, installer_files, expanded_bytes, false};
}
} // namespace civ5::content
