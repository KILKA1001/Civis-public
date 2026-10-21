#pragma once
#include <functional>
#include <memory>
#include <span>
#include <vector>
#include <cstdint>
#include <thread>
#include <stdexcept>
#include <utility>
namespace civ5::ui {class BnwNativePackageDiscoveryOwner;}
namespace civ5::ui::detail {
class BnwNativeGlobalResourceWarmOwner;
class BnwNativeOwnedSkinOccurrence;
//Closed actual selected Style/Color mode1 stream read. Only a genuine
//Parent/Warm issuer can retain this input; arbitrary span/path constructors
//are absent. A parser copies these real bytes under SAME source bookends.
class BnwNativeOwnedSkinSelectedInput final:public std::enable_shared_from_this<BnwNativeOwnedSkinSelectedInput> {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::function<void()> check_;
    const std::vector<std::uint8_t> bytes_;
    const std::function<void(const BnwNativeOwnedSkinOccurrence&,unsigned,const char*)> affinity_;
    BnwNativeOwnedSkinSelectedInput(std::function<void()> check,std::vector<std::uint8_t> bytes,
        std::function<void(const BnwNativeOwnedSkinOccurrence&,unsigned,const char*)> affinity)
        :check_(std::move(check)),bytes_(std::move(bytes)),affinity_(std::move(affinity)){}
  public:
    void require_current()const {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned selected Skin input foreign creator thread");
        const auto retained=shared_from_this();check_();
    }
    void require_same_occurrence(const BnwNativeOwnedSkinOccurrence& occurrence,unsigned slot,const char* property)const {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned selected Skin affinity foreign creator thread");
        const auto retained=shared_from_this();require_current();affinity_(occurrence,slot,property);require_current();
    }
    void with_actual_bytes(const std::function<void(std::span<const std::uint8_t>)>& input)const {
        //Pins only this actual immutable byte storage, never Parent authority.
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned selected Skin bytes foreign creator thread");
        const auto retained=shared_from_this();require_current();
        {
            const auto consume=input;
            require_current(); //Target copy can synchronously retire its actual source.
            if(!consume)throw std::runtime_error("Owned selected Skin bytes consumer absent");
            consume(std::span<const std::uint8_t>(retained->bytes_));require_current();
        }
        //Target destruction is a callback boundary too. Immutable storage is
        //still retained here, while genuine Parent/source authority stays weak.
        require_current();
    }
    BnwNativeOwnedSkinSelectedInput(const BnwNativeOwnedSkinSelectedInput&)=delete;
};
}
