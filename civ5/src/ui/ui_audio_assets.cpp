#include "civ5/ui/ui_audio_assets.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace civ5::ui {
namespace {

std::string required_text(const content::XmlNode &row, const char *key) {
    const auto *value = row.child(key);
    if (!value || value->text.empty())
        throw std::runtime_error(std::string("Audio XML row lacks ") + key);
    return value->text;
}

bool ambiguous_in_selected_source(const assets::ResourceMountTable &resources,
                                  const assets::AssetKey &key,
                                  const assets::MountedResource &selected) {
    const auto *chosen = std::get_if<assets::AssetHandle>(&selected.asset);
    if (!chosen) return false;
    std::set<std::pair<std::string, std::optional<std::string>>> paths;
    const auto &chosen_origin = chosen->candidate().provenance;
    for (const auto &candidate : resources.resolve_all(key)) {
        const auto *handle = std::get_if<assets::AssetHandle>(&candidate.asset);
        if (!handle || candidate.mount_id != selected.mount_id) continue;
        const auto &origin = handle->candidate().provenance;
        if (origin.kind != chosen_origin.kind || origin.source_id != chosen_origin.source_id)
            continue;
        paths.emplace(origin.source_relative_path, origin.archive_relative_path);
    }
    return paths.size() > 1;
}

UiAudioXmlLayer load_layer(const assets::ResourceMountTable &resources,
                          const std::string &key) {
    const assets::AssetKey requested(key);
    if (!requested.safe_relative_path())
        throw std::runtime_error("Unsafe selected audio XML path: " + key);
    const auto selected = resources.resolve(requested);
    const auto *handle = std::get_if<assets::AssetHandle>(&selected.asset);
    if (!handle || ambiguous_in_selected_source(resources, requested, selected))
        throw std::runtime_error("Audio XML missing or ambiguous: " + key);
    const auto bytes = handle->read_bytes(64ULL * 1024 * 1024);
    return {content::read_xml_metadata(std::string(bytes.begin(), bytes.end())),
            UiAudioSource{key, handle->candidate().original_name, selected.mount_id,
                          handle->candidate().provenance}};
}

void append_once(const assets::ResourceMountTable &resources, const std::string &key,
    std::vector<UiAudioXmlLayer> &layers,
    std::set<std::tuple<assets::SourceKind, std::string, std::string,
                        std::optional<std::string>>> &seen) {
    auto layer = load_layer(resources, key);
    const auto &provenance = *layer.source->provenance;
    if (seen.emplace(provenance.kind, provenance.source_id,
                     provenance.source_relative_path,
                     provenance.archive_relative_path).second)
        layers.push_back(std::move(layer));
}

std::uint16_t u16(std::span<const std::uint8_t> bytes, std::size_t at) {
    return static_cast<std::uint16_t>(bytes[at] | (bytes[at + 1] << 8));
}
std::uint32_t u32(std::span<const std::uint8_t> bytes, std::size_t at) {
    return static_cast<std::uint32_t>(bytes[at]) |
           (static_cast<std::uint32_t>(bytes[at + 1]) << 8) |
           (static_cast<std::uint32_t>(bytes[at + 2]) << 16) |
           (static_cast<std::uint32_t>(bytes[at + 3]) << 24);
}
bool tag(std::span<const std::uint8_t> bytes, std::size_t at, std::string_view name) {
    return at + name.size() <= bytes.size() &&
           std::equal(name.begin(), name.end(), bytes.begin() + at);
}

struct WaveInspection {
    UiAudioAssetStatus status;
    std::optional<UiPcmFormat> pcm;
    std::optional<UiImaAdpcmFormat> ima_adpcm;
    std::string reason;
};

WaveInspection inspect_wave(const assets::AssetHandle &asset) {
    auto stream = asset.open_stream();
    std::array<std::uint8_t, 4096> prefix{};
    const auto count = stream.read(prefix);
    const auto bytes = std::span<const std::uint8_t>(prefix.data(), count);
    if (count < 12 || !tag(bytes, 0, "RIFF") || !tag(bytes, 8, "WAVE"))
        return {UiAudioAssetStatus::malformed_wave, {}, {}, "Not a RIFF/WAVE file"};
    if (static_cast<std::uint64_t>(u32(bytes, 4)) + 8 != asset.candidate().payload_size)
        return {UiAudioAssetStatus::malformed_wave, {}, {}, "RIFF length differs from asset"};
    std::optional<std::uint16_t> codec, channels, bits, block_align, samples_per_block;
    std::optional<std::uint32_t> fact_samples;
    std::uint32_t sample_rate = 0;
    std::optional<std::uint64_t> data_offset, data_bytes;
    for (std::size_t at = 12; at + 8 <= count;) {
        const auto size = static_cast<std::uint64_t>(u32(bytes, at + 4));
        const auto start = static_cast<std::uint64_t>(at) + 8;
        const auto next = start + size + (size & 1);
        if (next > asset.candidate().payload_size)
            return {UiAudioAssetStatus::malformed_wave, {}, {}, "WAVE chunk exceeds asset"};
        if (tag(bytes, at, "fmt ")) {
            if (size < 16 || start + 16 > count)
                return {UiAudioAssetStatus::malformed_wave, {}, {}, "WAVE format chunk is incomplete"};
            codec = u16(bytes, start);
            channels = u16(bytes, start + 2);
            sample_rate = u32(bytes, start + 4);
            block_align = u16(bytes, start + 12);
            bits = u16(bytes, start + 14);
            if (*codec == 0x11) {
                if (size < 20 || start + 20 > count || u16(bytes, start + 16) < 2)
                    return {UiAudioAssetStatus::malformed_wave, {}, {},
                            "IMA ADPCM format extension is incomplete"};
                samples_per_block = u16(bytes, start + 18);
            }
        } else if (tag(bytes, at, "fact")) {
            if (size < 4 || start + 4 > count)
                return {UiAudioAssetStatus::malformed_wave, {}, {}, "WAVE fact chunk is incomplete"};
            fact_samples = u32(bytes, start);
        } else if (tag(bytes, at, "data")) {
            data_offset = start;
            data_bytes = size;
        }
        if (next > count) break;
        at = static_cast<std::size_t>(next);
    }
    if (!codec)
        return {UiAudioAssetStatus::malformed_wave, {}, {}, "WAVE format chunk was not found"};
    if (!data_offset || !data_bytes)
        return {UiAudioAssetStatus::malformed_wave, {}, {}, "WAVE data chunk was not found"};
    if (*codec == 0x11) {
        if (!channels || (*channels != 1 && *channels != 2) ||
            !sample_rate || !bits || *bits != 4)
            return {UiAudioAssetStatus::unsupported_codec, {}, {},
                    "Only mono/stereo IMA ADPCM 4-bit WAVE is supported"};
        const auto header_bytes = 4u * *channels;
        if (!block_align || *block_align <= header_bytes ||
            (*channels == 2 && (*block_align - header_bytes) % 8 != 0) ||
            !samples_per_block ||
            *samples_per_block != 1 + 2 * (*block_align - header_bytes) / *channels ||
            !fact_samples || !*fact_samples)
            return {UiAudioAssetStatus::malformed_wave, {}, {},
                    "IMA ADPCM blockAlign, samplesPerBlock or fact count is invalid"};
        const auto full_blocks = *data_bytes / *block_align;
        const auto partial_bytes = *data_bytes % *block_align;
        if ((!full_blocks && !partial_bytes) ||
            (partial_bytes && (partial_bytes < header_bytes ||
             (*channels == 2 && (partial_bytes - header_bytes) % 8 != 0))))
            return {UiAudioAssetStatus::malformed_wave, {}, {},
                    "IMA ADPCM data ends inside a block header"};
        const auto last_samples = partial_bytes
            ? 1 + 2 * (partial_bytes - header_bytes) / *channels
                                                : *samples_per_block;
        const auto total_samples = full_blocks * *samples_per_block +
                                   (partial_bytes ? last_samples : 0);
        const auto prior_samples = total_samples - last_samples;
        if (*fact_samples <= prior_samples || *fact_samples > total_samples)
            return {UiAudioAssetStatus::malformed_wave, {}, {},
                    "IMA ADPCM fact count exceeds or skips encoded blocks"};
        return {UiAudioAssetStatus::pcm_ready,
                UiPcmFormat{*channels, 16, sample_rate, 0,
                            static_cast<std::uint64_t>(*fact_samples) * *channels * 2},
                UiImaAdpcmFormat{*data_offset, *data_bytes, *block_align,
                                 *samples_per_block, *fact_samples,*channels}, {}};
    }
    if (*codec != 1 || !channels || (*channels != 1 && *channels != 2) ||
        !sample_rate || !bits || (*bits != 8 && *bits != 16))
        return {UiAudioAssetStatus::unsupported_codec, {}, {},
                "Only PCM 8/16-bit and mono/stereo IMA ADPCM WAVE are supported"};
    const auto pcm_align = static_cast<std::uint64_t>(*channels) * (*bits / 8);
    if (*data_bytes % pcm_align)
        return {UiAudioAssetStatus::malformed_wave, {}, {}, "PCM data is not frame aligned"};
    return {UiAudioAssetStatus::pcm_ready,
            UiPcmFormat{*channels, *bits, sample_rate, *data_offset, *data_bytes}, {}, {}};
}

std::vector<std::uint8_t> decode_ima_adpcm(std::span<const std::uint8_t> bytes,
                                            const UiImaAdpcmFormat &format) {
    static constexpr std::array<int, 89> steps = {
        7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,
        66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,
        337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,
        1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,
        4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,
        13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
    static constexpr std::array<int, 16> index_delta = {
        -1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};
    std::vector<std::uint8_t> output;
    output.reserve(static_cast<std::size_t>(format.sample_count) *
                   format.channels * 2);
    auto emit = [&](std::span<const int> frame) {
        if (output.size() / (2 * format.channels) >= format.sample_count) return;
        for (const auto sample : frame) {
            output.push_back(static_cast<std::uint8_t>(sample));
            output.push_back(static_cast<std::uint8_t>(sample >> 8));
        }
    };
    auto decode = [&](int &predictor,int &index,int nibble) {
        const int step = steps[index];
        int delta = step >> 3;
        if (nibble & 1) delta += step >> 2;
        if (nibble & 2) delta += step >> 1;
        if (nibble & 4) delta += step;
        predictor = std::clamp(predictor + ((nibble & 8) ? -delta : delta),
                               -32768, 32767);
        index = std::clamp(index + index_delta[nibble], 0, 88);
        return predictor;
    };
    const auto end = format.data_offset + format.data_bytes;
    for (std::uint64_t at = format.data_offset; at < end;) {
        const auto length = std::min<std::uint64_t>(format.block_align, end - at);
        if (length < 4u * format.channels ||
            (format.channels == 2 && (length - 8) % 8 != 0))
            throw std::runtime_error("Invalid IMA ADPCM block header");
        std::array<int,2> predictor{},index{};
        for (std::uint16_t channel=0;channel<format.channels;++channel) {
            const auto header=at+4*channel;
            if (bytes[header+2] > 88 || bytes[header+3] != 0)
                throw std::runtime_error("Invalid IMA ADPCM block header");
            predictor[channel]=static_cast<std::int16_t>(u16(bytes,header));
            index[channel]=bytes[header+2];
        }
        emit(std::span<const int>(predictor.data(),format.channels));
        if (format.channels == 1) {
            for (std::uint64_t i=at+4;i<at+length;++i)
                for (const int nibble : {bytes[i] & 0x0f,bytes[i] >> 4}) {
                    const int sample=decode(predictor[0],index[0],nibble);
                    emit(std::span<const int>(&sample,1));
                }
        } else {
            // Microsoft IMA stereo stores four bytes for the left channel,
            // then four for the right channel, repeating within each block.
            for (std::uint64_t group=at+8;group<at+length;group+=8) {
                std::array<std::array<int,8>,2> channel_samples{};
                for (std::uint16_t channel=0;channel<2;++channel)
                    for (std::uint64_t byte=0;byte<4;++byte) {
                        const auto packed=bytes[group+4*channel+byte];
                        channel_samples[channel][2*byte]=decode(
                            predictor[channel],index[channel],packed & 0x0f);
                        channel_samples[channel][2*byte+1]=decode(
                            predictor[channel],index[channel],packed >> 4);
                    }
                for (std::size_t sample=0;sample<8;++sample) {
                    const std::array<int,2> frame{
                        channel_samples[0][sample],channel_samples[1][sample]};
                    emit(frame);
                }
            }
        }
        at += length;
    }
    if (output.size() != static_cast<std::size_t>(format.sample_count) *
                         format.channels * 2)
        throw std::runtime_error("IMA ADPCM decoded sample count differs from fact");
    return output;
}

} // namespace

UiAudioAssetResolver UiAudioAssetResolver::from_documents(
    const assets::ResourceMountTable &resources,
    std::vector<UiAudioXmlLayer> scripts,
    std::vector<UiAudioXmlLayer> definitions) {
    UiAudioAssetResolver result(resources);
    for (const auto &layer : scripts) {
        if (layer.document.name != "Script2DFile")
            throw std::runtime_error("Expected Script2DFile audio XML");
        const auto *rows = layer.document.child("Script2DSounds");
        if (!rows) throw std::runtime_error("Script2DFile lacks Script2DSounds");
        for (const auto &row : rows->children)
            if (row.name == "Script2DSound")
                result.scripts_[required_text(row, "ScriptID")].push_back(
                    {required_text(row, "SoundID"), layer.source});
    }
    for (const auto &layer : definitions) {
        if (layer.document.name != "AudioDefinesFile")
            throw std::runtime_error("Expected AudioDefinesFile audio XML");
        const auto *rows = layer.document.child("SoundDatas");
        if (!rows) throw std::runtime_error("AudioDefinesFile lacks SoundDatas");
        for (const auto &row : rows->children)
            if (row.name == "SoundData")
                result.definitions_[required_text(row, "SoundID")].push_back(
                    {required_text(row, "Filename"), layer.source});
    }
    return result;
}

UiAudioAssetResolver UiAudioAssetResolver::from_resources(
    const assets::ResourceMountTable &resources,
    const std::vector<std::string> &script_keys,
    const std::vector<std::string> &definition_keys) {
    std::vector<UiAudioXmlLayer> scripts, definitions;
    for (const auto &key : script_keys) scripts.push_back(load_layer(resources, key));
    for (const auto &key : definition_keys) definitions.push_back(load_layer(resources, key));
    return from_documents(resources, std::move(scripts), std::move(definitions));
}

UiAudioAssetResolver UiAudioAssetResolver::from_selected_dlc(
    const assets::ResourceMountTable &resources, const content::Catalog &catalog,
    std::span<const UiSelectedDlcAudio> selected) {
    using Identity = std::tuple<assets::SourceKind, std::string, std::string,
                                std::optional<std::string>>;
    std::set<Identity> seen_scripts, seen_definitions;
    std::set<std::size_t> seen_packages;
    std::vector<UiAudioXmlLayer> scripts, definitions;
    append_once(resources, "Assets/Sounds/XML/Audio2DScripts.xml", scripts, seen_scripts);
    append_once(resources, "Assets/Sounds/XML/AudioDefines.xml", definitions,
                seen_definitions);
    for (const auto &entry : selected) {
        if (!seen_packages.insert(entry.package_index).second)
            throw std::runtime_error("DLC selected twice for UI audio");
        const auto &package = catalog.packages.at(entry.package_index);
        if (package.kind != content::PackageKind::dlc)
            throw std::runtime_error("Selected UI audio package is not DLC");
        if (!entry.enabled_categories[0] && !entry.enabled_categories[1] &&
            !entry.enabled_categories[2])
            throw std::runtime_error("Selected UI audio DLC has no active category");
        for (const auto &selection : {
                 std::pair{"AudioScripts2D", &scripts},
                 std::pair{"AudioDefines", &definitions}}) {
            content::PackageResourceQuery query;
            query.selector = selection.first;
            query.enabled_categories = entry.enabled_categories;
            for (const auto &declaration : content::package_resources(package, query)) {
                if (declaration.value.empty())
                    throw std::runtime_error("Selected DLC audio declaration is empty");
                append_once(resources, declaration.value, *selection.second,
                            selection.first == std::string_view("AudioScripts2D")
                                ? seen_scripts : seen_definitions);
            }
        }
    }
    return from_documents(resources, std::move(scripts), std::move(definitions));
}

UiAudioAssetResolution UiAudioAssetResolver::resolve(std::string_view script_id) const {
    UiAudioAssetResolution result;
    result.script_id = script_id;
    const auto script = scripts_.find(script_id);
    if (script == scripts_.end()) {
        result.reason = "Audio script ID is absent from selected XML";
        return result;
    }
    if (script->second.size() != 1) {
        result.status = UiAudioAssetStatus::ambiguous_script;
        result.reason = "Audio script ID has multiple selected definitions";
        for (const auto &entry : script->second) {
            result.conflicting_scripts.push_back({entry.sound_id, entry.source});
            if (entry.source) result.conflicting_sources.push_back(*entry.source);
        }
        return result;
    }
    result.sound_id = script->second.front().sound_id;
    result.script_source = script->second.front().source;
    const auto definition = definitions_.find(result.sound_id);
    if (definition == definitions_.end()) {
        result.status = UiAudioAssetStatus::missing_sound;
        result.reason = "Sound ID is absent from selected XML";
        return result;
    }
    if (definition->second.size() != 1) {
        result.status = UiAudioAssetStatus::ambiguous_sound;
        result.reason = "Sound ID has multiple selected definitions";
        for (const auto &entry : definition->second) {
            result.conflicting_definitions.push_back({entry.filename, entry.source});
            if (entry.source) result.conflicting_sources.push_back(*entry.source);
        }
        return result;
    }
    result.filename = definition->second.front().filename;
    result.definition_source = definition->second.front().source;
    std::vector<std::string> names;
    if (result.filename.ends_with(".wav") || result.filename.ends_with(".ogg") ||
        result.filename.ends_with(".mp3")) names.push_back(result.filename);
    else for (const auto suffix : {".wav", ".ogg", ".mp3"})
        names.push_back(result.filename + suffix);
    for (const auto &name : names) {
        const assets::AssetKey key(name);
        const auto selected = resources_->resolve(key);
        if (std::holds_alternative<assets::AmbiguousAsset>(selected.asset)) {
            result.status = UiAudioAssetStatus::ambiguous_asset;
            result.reason = "Resource mount contains ambiguous audio basename";
            result.asset.reset();
            return result;
        }
        if (const auto *handle = std::get_if<assets::AssetHandle>(&selected.asset)) {
            if (ambiguous_in_selected_source(*resources_, key, selected)) {
                result.status = UiAudioAssetStatus::ambiguous_asset;
                result.reason = "Selected audio source has multiple matching payloads";
                result.asset.reset();
                return result;
            }
            if (result.asset) {
                result.status = UiAudioAssetStatus::ambiguous_asset;
                result.reason = "Multiple audio extensions resolve";
                result.asset.reset();
                return result;
            }
            result.asset = *handle;
        }
    }
    if (!result.asset) {
        result.status = UiAudioAssetStatus::missing_asset;
        result.reason = "Selected sound filename has no mounted audio payload";
        return result;
    }
    const auto &selected_name = result.asset->candidate().original_name;
    if (!selected_name.ends_with(".wav") && !selected_name.ends_with(".WAV")) {
        result.status = UiAudioAssetStatus::unsupported_codec;
        result.reason = "No decoder is connected for this audio extension";
        return result;
    }
    const auto wave = inspect_wave(*result.asset);
    result.status = wave.status;
    result.pcm = wave.pcm;
    result.ima_adpcm = wave.ima_adpcm;
    result.reason = wave.reason;
    return result;
}

UiPcmSubmitStatus submit_ui_pcm(const UiAudioAssetResolver &resolver,
    std::string_view script_id, UiPcmSink &sink, std::uint64_t maximum_bytes) {
    const auto resolved = resolver.resolve(script_id);
    if (resolved.status != UiAudioAssetStatus::pcm_ready ||
        !resolved.asset || !resolved.pcm)
        return UiPcmSubmitStatus::resolution_blocked;
    if (resolved.asset->candidate().payload_size > maximum_bytes)
        return UiPcmSubmitStatus::size_blocked;
    const auto bytes = resolved.asset->read_bytes(maximum_bytes);
    if (resolved.pcm->data_bytes > maximum_bytes)
        return UiPcmSubmitStatus::size_blocked;
    std::vector<std::uint8_t> samples;
    if (resolved.ima_adpcm) {
        const auto &encoded = *resolved.ima_adpcm;
        if (encoded.data_offset > bytes.size() ||
            encoded.data_bytes > bytes.size() - encoded.data_offset)
            return UiPcmSubmitStatus::resolution_blocked;
        try {
            samples = decode_ima_adpcm(bytes, encoded);
        } catch (const std::runtime_error &) {
            return UiPcmSubmitStatus::resolution_blocked;
        }
    } else {
        const auto offset = resolved.pcm->data_offset;
        const auto count = resolved.pcm->data_bytes;
        if (offset > bytes.size() || count > bytes.size() - offset)
            return UiPcmSubmitStatus::resolution_blocked;
        samples.assign(bytes.begin() + offset, bytes.begin() + offset + count);
    }
    UiPcmBuffer buffer{*resolved.pcm, std::move(samples),
                       resolved.asset->candidate().provenance};
    return sink.submit(buffer) ? UiPcmSubmitStatus::submitted
                               : UiPcmSubmitStatus::sink_rejected;
}

std::vector<UiAudioPlaybackResult> submit_ui_audio_events(
    UiAudioQueue &queue, const UiAudioAssetResolver &resolver, UiPcmSink &sink,
    std::uint64_t maximum_bytes) {
    auto events = queue.drain();
    std::vector<UiAudioPlaybackResult> results;
    results.reserve(events.size());
    for (auto &event : events) {
        UiAudioPlaybackResult result;
        result.event = std::move(event);
        try {
            result.status = submit_ui_pcm(resolver, result.event.script_id, sink, maximum_bytes);
        } catch (const std::exception &error) {
            result.status = UiPcmSubmitStatus::failed_exception;
            result.error = error.what();
        } catch (...) {
            result.status = UiPcmSubmitStatus::failed_exception;
            result.error = "Unknown UI audio submission error";
        }
        results.push_back(std::move(result));
    }
    return results;
}

} // namespace civ5::ui
