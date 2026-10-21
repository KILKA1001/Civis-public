#include "../src/ui/bnw_audio_user_settings_input_detail.hpp"
#include <climits>
#include <codecvt>
#include <chrono>

namespace {
void require(bool ok) { if (!ok) throw std::runtime_error("Audio settings expectation failed"); }
template<class F> void rejects(F fn) {
    bool failed = false;
    try { fn(); } catch (const std::runtime_error&) { failed = true; }
    require(failed);
}
}
struct TransformingCodecvt final : std::codecvt<char, char, std::mbstate_t> {
    static inline int input_calls = 0;
    bool do_always_noconv() const noexcept override { return false; }
    int do_encoding() const noexcept override { return 1; }
    int do_max_length() const noexcept override { return 1; }
    result do_in(std::mbstate_t&, const char* first, const char* last, const char*& next,
                 char* out, char* end, char*& outnext) const override {
        ++input_calls; next = first; outnext = out;
        while (next != last && outnext != end) { const char c = *next++; *outnext++ = c == '5' ? '9' : c; }
        return next == last ? ok : partial;
    }
};
struct RawFixture {
    std::filesystem::path path;
    std::locale old = std::locale();
    RawFixture() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() / ("civis_audio_raw_" + std::to_string(stamp));
        std::filesystem::create_directory(path); path /= "settings.ini";
    }
    ~RawFixture() { std::locale::global(old); std::error_code ec; std::filesystem::remove(path, ec); std::filesystem::remove(path.parent_path(), ec); }
};
int main() {
    using namespace civ5::ui::audio_settings_detail;
    {
        RawFixture fixture;
        const std::string authored = "[User Settings]\nMusic.Volume=53\n";
        std::ofstream output; output.imbue(std::locale::classic()); output.open(fixture.path, std::ios::binary);
        output.write(authored.data(), static_cast<std::streamsize>(authored.size())); output.close();
        require(static_cast<bool>(output));
        std::locale::global(std::locale(std::locale::classic(), new TransformingCodecvt));
        const auto raw = read_settings_raw(fixture.path);
        require(std::string(raw.begin(), raw.end()) == authored);
        require(TransformingCodecvt::input_calls == 0);
    }
    require(saved_settings_atoi_subset(" +75tail") == 75);
    require(saved_settings_atoi_subset("-0") == 0);
    require(saved_settings_atoi_subset("abc") == 0);
    require(saved_settings_atoi_subset("+") == 0);
    require(saved_settings_atoi_subset("-2147483648") == INT_MIN);
    require(saved_settings_atoi_subset("2147483647") == INT_MAX);
    rejects([] { saved_settings_atoi_subset("2147483648"); });
    rejects([] { saved_settings_atoi_subset("-2147483649"); });
#if defined(_WIN32)
    const std::string input = "[User Settings]\nMusic.Volume=53\nEffects.Volume=75\nAmbience.Volume=0\nSpeech.Volume=100\n";
    require(parse_audio_settings_existing_subset(input) == std::array<std::int32_t, 4>{53, 75, 0, 100});
    rejects([&] { auto padded = input; padded.replace(0, 15, "[ User Settings ]"); parse_audio_settings_existing_subset(padded); });
    rejects([&] { auto broken = input; broken.insert(broken.find("53") + 1, "\r"); parse_audio_settings_existing_subset(broken); });
    const auto zero = parse_audio_settings_existing_subset(
        "[user settings]\r\nmusic.volume=0\r\neffects.volume=0\r\nambience.volume=0\r\nspeech.volume=0\r\n");
    require(zero == std::array<std::int32_t, 4>{0, 0, 0, 0});
    rejects([&] { parse_audio_settings_existing_subset(input + "effects.volume=1\n"); });
    rejects([&] { parse_audio_settings_existing_subset(input + "[USER SETTINGS]\n"); });
    rejects([] { parse_audio_settings_existing_subset("[User Settings]\nMusic.Volume=53\n"); });
    rejects([] { parse_audio_settings_existing_subset("[Other]\nMusic.Volume=53\n"); });
    rejects([&] { parse_audio_settings_existing_subset(input + std::string(1, '\0')); });
    rejects([&] { parse_audio_settings_existing_subset(input + std::string(1024, 'x')); });
#endif
}
