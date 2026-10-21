#pragma once
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace civ5::ui::detail {
// Source algorithm ONLY. This does not authenticate namespace enable state,
// global FIRST names, real registration order or a retained ResourceMountTable.
class NativeResourceBucketOrder {
 public:
    struct Node { std::uint32_t key,value; bool operator==(const Node&)const=default; };
    static constexpr std::uint32_t source_bucket_count=0x3fff;
    static constexpr std::uint32_t source_initial_pool=0x7000;
    static constexpr std::uint32_t source_pool_increment=0x1000;
    NativeResourceBucketOrder():NativeResourceBucketOrder(source_initial_pool){}
    // Explicit research fixture input. Never a production capability/authority.
    static NativeResourceBucketOrder diagnostic_pool(std::uint32_t initial){
        return NativeResourceBucketOrder(initial);
    }
    std::uint32_t insert(std::uint32_t key,std::uint32_t value){
        if(live_==pool_){
            if(pool_>UINT32_MAX-source_pool_increment)throw std::runtime_error("Native bucket pool growth overflow");
            pool_+=source_pool_increment;
            //849F20 copies ascending old bucket/head->next and tail-appends
            //into SAME bucket count. It increments +0C for each copied node
            //without resetting that field. Keep the observed field separately.
            source_count_+=live_;
        }
        auto& bucket=buckets_[bucket_index(key)];
        //84A0E0 checks duplicate CRCs before the final tail node only.
        //Selected-record insertion normally removes the old CRC first;
        //retain this generic table return quirk for bounded source parity.
        bool duplicate_before_tail=false;
        for(std::size_t i=0;i+1<bucket.size();++i)duplicate_before_tail|=bucket[i].key==key;
        bucket.push_back({key,value});++live_;++source_count_;
        return duplicate_before_tail?2:1;
    }
    std::uint32_t erase_first(std::uint32_t key){
        auto& bucket=buckets_[bucket_index(key)];
        const auto old=std::find_if(bucket.begin(),bucket.end(),[&](const auto& n){return n.key==key;});
        if(old==bucket.end())return 0;
        const auto value=old->value;bucket.erase(old);--live_;--source_count_;return value;
    }
    std::vector<Node> ordered_nodes()const{
        std::vector<Node> result;result.reserve(live_);
        for(const auto& bucket:buckets_)result.insert(result.end(),bucket.begin(),bucket.end());
        return result;
    }
    std::uint32_t pool_size()const{return pool_;}
    std::uint32_t source_count_field()const{return source_count_;}
    std::uint32_t live_nodes()const{return live_;}
    static constexpr std::uint32_t bucket_index(std::uint32_t key){
        return (key*std::uint32_t{127})%source_bucket_count;
    }
 private:
    explicit NativeResourceBucketOrder(std::uint32_t initial):pool_(std::max(initial,std::uint32_t{64})),buckets_(source_bucket_count){}
    std::uint32_t pool_,source_count_=0,live_=0;
    std::vector<std::vector<Node>> buckets_;
};
}
