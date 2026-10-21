#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <map>
#include <sstream>
#include <iomanip>

namespace civ5::graphics {
// Single observer thread, immutable input assets. No resource override, copy,
// extraction, or replacement is performed; selected handles are retained for
// one digest recheck immediately before source-receipt publication.
class BnwUiSourceObservation {
    struct Candidate { assets::AssetHandle handle; std::string sha; };
    std::vector<Candidate> candidates_;
    std::map<std::string,std::size_t> cache_;
    std::vector<std::string> requests_;
    bool failed_=false;
    bool recording_=true;
    static std::string quote(std::string_view s) {
        std::ostringstream out;out<<'"';
        for(unsigned char c:s) {
            if(c=='"'||c=='\\')out<<'\\'<<c;
            else if(c<32)out<<"\\u00"<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(c)<<std::dec;
            else out<<c;
        }
        out<<'"';return out.str();
    }
    static std::string candidate_json(const assets::AssetCandidate &c) {
        const auto &p=c.provenance;
        return "\"original_name\":"+quote(c.original_name)+",\"source_kind\":"+quote(p.kind==assets::SourceKind::loose?"loose":"fpk")+
            ",\"source_id\":"+quote(p.source_id)+",\"source_relative_path\":"+quote(p.source_relative_path)+
            ",\"archive_relative_path\":"+(p.archive_relative_path?quote(*p.archive_relative_path):"null")+
            ",\"size\":"+std::to_string(c.payload_size);
    }
    std::size_t candidate(const assets::AssetHandle &handle) {
        const auto key=candidate_json(handle.candidate());
        if(auto found=cache_.find(key);found!=cache_.end())return found->second;
        const auto bytes=handle.read_bytes(256ull*1024*1024);
        if(bytes.size()!=handle.candidate().payload_size)throw std::runtime_error("UI source payload size changed");
        const auto index=candidates_.size();candidates_.push_back({handle,runtime::identify_bytes(bytes).sha256});cache_.emplace(key,index);return index;
    }
  public:
    // Stop one-shot discovery on the owning thread after receipt publication.
    // Retained handles/digests and failure state remain available for rechecks.
    void stop_recording() noexcept {recording_=false;}
    void mark_observer_failed() noexcept {failed_=true;}
    void observe(const assets::AssetKey &key,std::span<const assets::MountedResource> results,bool all) noexcept {
        if(!recording_)return;
        try {
            std::string request="{\"requested_key\":"+quote(key.original_name())+",\"operation\":"+quote(all?"resolve_all":"resolve")+",\"results\":[";
            for(std::size_t i=0;i<results.size();++i) {
                if(i)request+=',';
                request+="{\"mount_id\":"+(results[i].mount_id?std::to_string(*results[i].mount_id):"null");
                if(const auto *h=std::get_if<assets::AssetHandle>(&results[i].asset)) {
                    request+=",\"status\":\"selected\",\"candidate\":"+std::to_string(candidate(*h));
                } else if(const auto *a=std::get_if<assets::AmbiguousAsset>(&results[i].asset)) {
                    request+=",\"status\":\"ambiguous\",\"alternatives\":[";
                    for(std::size_t j=0;j<a->matches.size();++j) {if(j)request+=',';request+='{'+candidate_json(a->matches[j])+'}';}
                    request+=']';
                } else request+=",\"status\":\"missing\"";
                request+='}';
            }
            request+="]}";requests_.push_back(std::move(request));
        } catch(...) {failed_=true;}
    }
    // Does not clear requests or change resource selection. A changed file or
    // failed read is retained as pending evidence rather than a new baseline.
    std::string snapshot_json(bool observer_failed=false) const {
        bool unchanged=true;
        for(const auto &c:candidates_) {
            try {
                const auto bytes=c.handle.read_bytes(256ull*1024*1024);
                if(bytes.size()!=c.handle.candidate().payload_size || runtime::identify_bytes(bytes).sha256!=c.sha)unchanged=false;
            } catch(...) {unchanged=false;}
        }
        std::string out="{\"schema\":\"civis-resolved-ui-sources-v1\",\"scope\":\"actual-ui-resolve-requests\",\"observer_failed\":"+std::string(failed_||observer_failed?"true":"false")+
            ",\"immutable_recheck_passed\":"+(unchanged?"true":"false")+",\"request_count\":"+std::to_string(requests_.size())+",\"candidates\":[";
        for(std::size_t i=0;i<candidates_.size();++i) {
            if(i)out+=',';
            out+="{\"id\":"+std::to_string(i)+','+candidate_json(candidates_[i].handle.candidate())+",\"sha256\":"+quote(candidates_[i].sha)+'}';
        }
        out+="],\"requests\":[";
        for(std::size_t i=0;i<requests_.size();++i) {if(i)out+=',';out+=requests_[i];}
        return out+"]}";
    }
};
} // namespace civ5::graphics
