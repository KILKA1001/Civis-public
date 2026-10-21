// Private fresh full traversal, no snapshot vector or cached verdict.
bool scan_matches(const std::filesystem::path& root,const std::vector<Entry>& expected) {
    std::size_t index=0;bool matched=true;
    const auto observe=[&](const std::filesystem::path& relative,DWORD attributes){
        if(index>=expected.size()||expected[index].relative!=relative||expected[index].attributes!=attributes)matched=false;
        ++index;
    };
    const auto visit = [&](auto&& self, const std::filesystem::path& relative) -> void {
        const auto dir = root / relative;
        need(std::filesystem::is_directory(dir), "Native package Assets directory missing");
        // 821FE0 emits matching ordinary files, then recurses directories in
        // FindFirst/Next order. No sorting or save-selected inventory substitution.
        struct Find {
            HANDLE h = INVALID_HANDLE_VALUE;
            ~Find() { if (h != INVALID_HANDLE_VALUE) FindClose(h); }
        } files, dirs;
        WIN32_FIND_DATAW data{};
        files.h = native_fresh_find_detail::first((dir / L"*.civ5pkg").c_str(), &data);
        if (files.h != INVALID_HANDLE_VALUE) {
            do {
                const auto attrs = data.dwFileAttributes;
                if ((attrs & 0x16) != 0) continue;
                const std::filesystem::path name = data.cFileName;
                need((attrs & FILE_ATTRIBUTE_REPARSE_POINT) == 0,
                     "Native package reparse file requires target owner");
                observe(relative / name,attrs);
            } while (FindNextFileW(files.h, &data));
            need(GetLastError() == ERROR_NO_MORE_FILES, "Native package file enumeration incomplete");
        } else {
            need(GetLastError() == ERROR_FILE_NOT_FOUND,
                 "Native package source file enumeration failed");
        }
        dirs.h = native_fresh_find_detail::first((dir / L"*").c_str(), &data);
        need(dirs.h != INVALID_HANDLE_VALUE, "Native package directory enumeration failed");
        do {
            if ((data.dwFileAttributes & 0x16) != 0x10) continue;
            const std::filesystem::path name = data.cFileName;
            if (name == L"." || name == L"..") continue;
            need((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0,
                 "Native package reparse traversal requires target owner");
            const auto path = relative / name;
            // The source callback can also receive matching package directories;
            // that descriptor semantics is not silently treated as a normal dir.
            need(runtime::ascii_lower(assets::path_utf8(name.extension())) != ".civ5pkg",
                 "Native matching package directory requires descriptor owner");
            observe(path,data.dwFileAttributes);
            self(self, path);
        } while (FindNextFileW(dirs.h, &data));
        need(GetLastError() == ERROR_NO_MORE_FILES, "Native package directory enumeration incomplete");
    };
    visit(visit, {});
    return matched&&index==expected.size();
}
