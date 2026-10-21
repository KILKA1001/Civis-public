#include "civ5/ui/ui_audio_assets.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
using namespace civ5;
namespace fs = std::filesystem;

void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}

void write(const fs::path &root, std::string_view name,
           std::span<const std::uint8_t> bytes) {
    const auto path = root / fs::path(name);
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    if (!out) throw std::runtime_error("Could not write audio fixture");
}

void write(const fs::path &root, std::string_view name, std::string_view text) {
    write(root, name, std::span(reinterpret_cast<const std::uint8_t *>(text.data()),
                                text.size()));
}

std::vector<std::uint8_t> pcm_wave() {
    return { 'R','I','F','F', 40,0,0,0, 'W','A','V','E',
             'f','m','t',' ', 16,0,0,0, 1,0, 1,0,
             0x40,0x1f,0,0, 0x40,0x1f,0,0, 1,0, 8,0,
             'd','a','t','a', 4,0,0,0, 0,64,128,255 };
}

std::vector<std::uint8_t> ima_wave() {
    return { 'R','I','F','F', 66,0,0,0, 'W','A','V','E',
             'f','m','t',' ', 20,0,0,0,
             0x11,0, 1,0, 0x40,0x1f,0,0, 0x40,0x1f,0,0,
             8,0, 4,0, 2,0, 9,0,
             'f','a','c','t', 4,0,0,0, 11,0,0,0,
             'd','a','t','a', 13,0,0,0,
             0,0,0,0, 0x11,0x11,0x88,0x88,
             100,0,0,0, 0x11,0 };
}

std::vector<std::uint8_t> stereo_ima_wave() {
    return { 'R','I','F','F', 68,0,0,0, 'W','A','V','E',
             'f','m','t',' ', 20,0,0,0,
             0x11,0, 2,0, 0x40,0x1f,0,0, 0x40,0x1f,0,0,
             16,0, 4,0, 2,0, 9,0,
             'f','a','c','t', 4,0,0,0, 9,0,0,0,
             'd','a','t','a', 16,0,0,0,
             0,0,0,0, 100,0,0,0,
             0x11,0x11,0x11,0x11, 0,0,0,0 };
}

std::vector<std::uint8_t> pcm16(std::initializer_list<int> values) {
    std::vector<std::uint8_t> result;
    for (int value : values) {
        result.push_back(static_cast<std::uint8_t>(value));
        result.push_back(static_cast<std::uint8_t>(value >> 8));
    }
    return result;
}

struct Sink final : ui::UiPcmSink {
    int calls = 0;
    bool accept = true;
    bool throw_once = false;
    std::optional<ui::UiPcmBuffer> last;
    bool submit(const ui::UiPcmBuffer &buffer) override {
        ++calls;
        if (throw_once) {
            throw_once = false;
            throw std::runtime_error("audio device unavailable");
        }
        last = buffer;
        return accept;
    }
};

} // namespace

int main(int argc, char **argv) {
    try {
        const auto root = fs::temp_directory_path() / "civis-ui-audio-assets-test";
        constexpr auto scripts = R"(<Script2DFile><Script2DSounds>
          <Script2DSound><ScriptID>AS2D_TEST_PCM</ScriptID><SoundID>SND_TEST_PCM</SoundID></Script2DSound>
          <Script2DSound><ScriptID>AS2D_TEST_MISSING</ScriptID><SoundID>SND_TEST_MISSING</SoundID></Script2DSound>
          <Script2DSound><ScriptID>AS2D_TEST_NO_SOUND</ScriptID><SoundID>SND_ABSENT</SoundID></Script2DSound>
          <Script2DSound><ScriptID>AS2D_TEST_IMA</ScriptID><SoundID>SND_TEST_IMA</SoundID></Script2DSound>
        </Script2DSounds></Script2DFile>)";
        constexpr auto definitions = R"(<AudioDefinesFile><SoundDatas>
          <SoundData><SoundID>SND_TEST_PCM</SoundID><Filename>TestPcm</Filename></SoundData>
          <SoundData><SoundID>SND_TEST_MISSING</SoundID><Filename>NotMounted</Filename></SoundData>
          <SoundData><SoundID>SND_TEST_IMA</SoundID><Filename>TestIma</Filename></SoundData>
        </SoundDatas></AudioDefinesFile>)";
        write(root, "Assets/Sounds/XML/Audio2DScripts.xml", scripts);
        write(root, "Assets/Sounds/XML/AudioDefines.xml", definitions);
        const auto wav = pcm_wave();
        write(root, "Assets/Sounds/TestPcm.wav", wav);
        const auto adpcm = ima_wave();
        write(root, "Assets/Sounds/TestIma.wav", adpcm);
        auto source = std::make_shared<assets::LooseResourceIndex>(root);
        source->add_file("Assets/Sounds/XML/Audio2DScripts.xml");
        source->add_file("Assets/Sounds/XML/AudioDefines.xml");
        source->add_file("Assets/Sounds/TestPcm.wav");
        source->add_file("Assets/Sounds/TestIma.wav");
        assets::ResourceMountTable resources(assets::resource_lookup(source));
        const auto resolver = ui::UiAudioAssetResolver::from_resources(resources);
        const auto pcm = resolver.resolve("AS2D_TEST_PCM");
        require(pcm.status == ui::UiAudioAssetStatus::pcm_ready && pcm.asset && pcm.pcm &&
                    pcm.sound_id == "SND_TEST_PCM" && pcm.filename == "TestPcm" &&
                    pcm.pcm->sample_rate == 8000 && pcm.pcm->channels == 1 &&
                    pcm.pcm->bits_per_sample == 8 && pcm.pcm->data_bytes == 4 &&
                    pcm.script_source && pcm.definition_source &&
                    pcm.asset->candidate().provenance.kind == assets::SourceKind::loose,
                "script resolves through authored XML to provenance-bearing PCM WAV");
        require(resolver.resolve("AS2D_UNKNOWN").status ==
                    ui::UiAudioAssetStatus::missing_script &&
                    resolver.resolve("AS2D_TEST_NO_SOUND").status ==
                    ui::UiAudioAssetStatus::missing_sound &&
                    resolver.resolve("AS2D_TEST_MISSING").status ==
                    ui::UiAudioAssetStatus::missing_asset,
                "missing script, definition and payload remain distinct blockers");
        Sink sink;
        require(ui::submit_ui_pcm(resolver, "AS2D_TEST_PCM", sink) ==
                    ui::UiPcmSubmitStatus::submitted && sink.calls == 1 &&
                    sink.last && sink.last->interleaved_samples ==
                        std::vector<std::uint8_t>({0,64,128,255}),
                "PCM-only sink receives exact authored bytes once");
        sink.accept = false;
        require(ui::submit_ui_pcm(resolver, "AS2D_TEST_PCM", sink) ==
                    ui::UiPcmSubmitStatus::sink_rejected && sink.calls == 2,
                "sink rejection is distinct from playback claim");
        require(ui::submit_ui_pcm(resolver, "AS2D_TEST_PCM", sink, 16) ==
                    ui::UiPcmSubmitStatus::size_blocked && sink.calls == 2,
                "payload budget blocks before sink");
        const auto ima = resolver.resolve("AS2D_TEST_IMA");
        require(ima.status == ui::UiAudioAssetStatus::pcm_ready && ima.ima_adpcm &&
                    ima.pcm && ima.pcm->bits_per_sample == 16 &&
                    ima.pcm->data_bytes == 22 &&
                    ima.ima_adpcm->block_align == 8 &&
                    ima.ima_adpcm->samples_per_block == 9 &&
                    ima.ima_adpcm->sample_count == 11,
                "IMA ADPCM format and partial final block are inspected");
        sink.accept = true;
        require(ui::submit_ui_pcm(resolver, "AS2D_TEST_IMA", sink) ==
                    ui::UiPcmSubmitStatus::submitted && sink.last &&
                    sink.last->interleaved_samples ==
                        pcm16({0,1,2,3,4,4,4,4,4,100,101}),
                "IMA nibbles decode low-first, seed predictors and trim fact padding");
        require(ui::submit_ui_pcm(resolver, "AS2D_TEST_IMA", sink, 20) ==
                    ui::UiPcmSubmitStatus::size_blocked,
                "decoded PCM output respects caller memory budget");

        auto wrong_spb = adpcm;
        wrong_spb[38] = 8;
        write(root, "Assets/Sounds/TestIma.wav", wrong_spb);
        require(resolver.resolve("AS2D_TEST_IMA").status ==
                    ui::UiAudioAssetStatus::malformed_wave,
                "inconsistent samplesPerBlock is rejected");
        auto wrong_fact = adpcm;
        wrong_fact[48] = 13;
        write(root, "Assets/Sounds/TestIma.wav", wrong_fact);
        require(resolver.resolve("AS2D_TEST_IMA").status ==
                    ui::UiAudioAssetStatus::malformed_wave,
                "fact samples cannot exceed encoded blocks");
        auto wrong_align = adpcm;
        wrong_align[32] = 4;
        write(root, "Assets/Sounds/TestIma.wav", wrong_align);
        require(resolver.resolve("AS2D_TEST_IMA").status ==
                    ui::UiAudioAssetStatus::malformed_wave,
                "inconsistent blockAlign is rejected");
        auto stereo = adpcm;
        stereo[22] = 2;
        write(root, "Assets/Sounds/TestIma.wav", stereo);
        require(resolver.resolve("AS2D_TEST_IMA").status ==
                    ui::UiAudioAssetStatus::malformed_wave,
                "stereo header with mono block geometry is rejected");
        const auto stereo_valid=stereo_ima_wave();
        write(root, "Assets/Sounds/TestIma.wav", stereo_valid);
        auto stereo_source=std::make_shared<assets::LooseResourceIndex>(root);
        stereo_source->add_file("Assets/Sounds/XML/Audio2DScripts.xml");
        stereo_source->add_file("Assets/Sounds/XML/AudioDefines.xml");
        stereo_source->add_file("Assets/Sounds/TestIma.wav");
        const assets::ResourceMountTable stereo_resources(
            assets::resource_lookup(stereo_source));
        const auto stereo_resolver=ui::UiAudioAssetResolver::from_resources(
            stereo_resources);
        const auto stereo_resolved=stereo_resolver.resolve("AS2D_TEST_IMA");
        require(stereo_resolved.status == ui::UiAudioAssetStatus::pcm_ready &&
                    stereo_resolved.pcm && stereo_resolved.ima_adpcm &&
                    stereo_resolved.pcm->channels == 2 &&
                    stereo_resolved.ima_adpcm->channels == 2 &&
                    stereo_resolved.pcm->data_bytes == 36,
                "stereo IMA block and sample count are inspected");
        require(ui::submit_ui_pcm(stereo_resolver, "AS2D_TEST_IMA", sink) ==
                    ui::UiPcmSubmitStatus::submitted && sink.last &&
                    sink.last->interleaved_samples == pcm16({
                        0,100, 1,100, 2,100, 3,100, 4,100,
                        5,100, 6,100, 7,100, 8,100}),
                "stereo IMA groups decode into interleaved PCM frames");
        auto short_header = adpcm;
        short_header[56] = 11;
        write(root, "Assets/Sounds/TestIma.wav", short_header);
        require(resolver.resolve("AS2D_TEST_IMA").status ==
                    ui::UiAudioAssetStatus::malformed_wave,
                "partial final block cannot end inside IMA header");
        auto wrong_index = adpcm;
        wrong_index[62] = 89;
        write(root, "Assets/Sounds/TestIma.wav", wrong_index);
        require(ui::submit_ui_pcm(resolver, "AS2D_TEST_IMA", sink) ==
                    ui::UiPcmSubmitStatus::resolution_blocked,
                "invalid IMA step index blocks decode before sink");
        write(root, "Assets/Sounds/TestIma.wav", adpcm);

        const auto catalog = ui::UiAudioCatalog::from_documents(
            content::read_xml_metadata(
                "<ButtonPopupSounds><ButtonPopupSound><ID>POP_TEST</ID>"
                "<AudioScript>AS2D_TEST_PCM</AudioScript></ButtonPopupSound></ButtonPopupSounds>"),
            content::read_xml_metadata(
                "<NotificationSounds><NotificationSound><ID>NOTE_TEST</ID>"
                "<AudioScript>AS2D_TEST_MISSING</AudioScript></NotificationSound></NotificationSounds>"));
        ui::UiAudioQueue queue(catalog);
        require(queue.emit(ui::UiAudioKind::popup, "POP_TEST") == ui::UiAudioEmitStatus::queued &&
                    queue.emit_script("Lua", "AS2D_TEST_NO_SOUND") == ui::UiAudioEmitStatus::queued &&
                    queue.emit(ui::UiAudioKind::notification, "NOTE_TEST") == ui::UiAudioEmitStatus::queued,
                "authored UI transitions were not queued");
        sink.accept = true;
        const auto calls_before_events = sink.calls;
        const auto playback = ui::submit_ui_audio_events(queue, resolver, sink);
        require(playback.size() == 3 && queue.pending() == 0 &&
                    sink.calls == calls_before_events + 1 &&
                    playback[0].event.sequence == 1 &&
                    playback[0].status == ui::UiPcmSubmitStatus::submitted &&
                    playback[1].event.sequence == 2 &&
                    playback[1].status == ui::UiPcmSubmitStatus::resolution_blocked &&
                    playback[2].event.sequence == 3 &&
                    playback[2].status == ui::UiPcmSubmitStatus::resolution_blocked &&
                    ui::submit_ui_audio_events(queue, resolver, sink).empty() &&
                    sink.calls == calls_before_events + 1,
                "UI audio events lost their order, failure status or one-shot semantics");
        queue.emit(ui::UiAudioKind::popup, "POP_TEST");
        queue.emit(ui::UiAudioKind::popup, "POP_TEST");
        sink.throw_once = true;
        const auto recovered = ui::submit_ui_audio_events(queue, resolver, sink);
        require(recovered.size() == 2 &&
                    recovered[0].status == ui::UiPcmSubmitStatus::failed_exception &&
                    recovered[0].error == "audio device unavailable" &&
                    recovered[1].status == ui::UiPcmSubmitStatus::submitted &&
                    queue.pending() == 0 && sink.calls == calls_before_events + 3,
                "one failed audio event prevented later events or hid its error");

        const auto duplicate = ui::UiAudioAssetResolver::from_documents(resources,
            {{content::read_xml_metadata(scripts), {}},
             {content::read_xml_metadata(scripts), {}}},
            {{content::read_xml_metadata(definitions), {}}});
        require(duplicate.resolve("AS2D_TEST_PCM").status ==
                    ui::UiAudioAssetStatus::ambiguous_script,
                "duplicate selected script definition is not silently chosen");

        if (argc == 2) {
            const fs::path assets_root(argv[1]);
            auto retail_source = std::make_shared<assets::LooseResourceIndex>(assets_root);
            retail_source->add_file("Assets/Sounds/XML/Audio2DScripts.xml");
            retail_source->add_file("Assets/Sounds/XML/AudioDefines.xml");
            retail_source->add_file("Assets/Sounds/Streamed/NotStreamed/Ding.wav");
            assets::ResourceMountTable retail(assets::resource_lookup(retail_source));
            const auto real = ui::UiAudioAssetResolver::from_resources(retail);
            const auto ding = real.resolve("AS2D_IF_MP_CHAT_DING");
            require(ding.status == ui::UiAudioAssetStatus::pcm_ready &&
                        ding.asset && ding.ima_adpcm && ding.pcm &&
                        ding.ima_adpcm->block_align == 1024 &&
                        ding.ima_adpcm->samples_per_block == 2041 &&
                        ding.ima_adpcm->sample_count == 30353 &&
                        ding.pcm->data_bytes == 60706 &&
                        ding.sound_id == "SND_IF_MP_CHAT_DING" &&
                        ding.filename == "Ding" &&
                        ding.asset->candidate().provenance.kind == assets::SourceKind::loose,
                    "retail Ding format resolves with exact IMA block and fact metadata");
            const auto ding_prefix = pcm16({14,11,7,7,6,6,7});
            require(ui::submit_ui_pcm(real, "AS2D_IF_MP_CHAT_DING", sink) ==
                        ui::UiPcmSubmitStatus::submitted && sink.last &&
                        sink.last->interleaved_samples.size() == 60706 &&
                        std::equal(ding_prefix.begin(), ding_prefix.end(),
                                   sink.last->interleaved_samples.begin()),
                    "retail Ding IMA decodes to known initial PCM16 samples");
            require(real.resolve("AS2D_INTERFACE_TECH_WINDOW").status ==
                        ui::UiAudioAssetStatus::missing_asset,
                    "missing local TechWindow audio does not become a fake cue");
        }
        std::error_code ignored;
        fs::remove_all(root, ignored);
        std::cout << "UI audio asset tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
