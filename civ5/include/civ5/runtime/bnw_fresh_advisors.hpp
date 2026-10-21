#pragma once
#include "civ5/runtime/bnw_creation_config.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>

namespace civ5::runtime {
class BnwFreshGame;
enum class BnwFreshAdvisorError { wrong_thread, parent_unavailable, field_unwritten, index_unavailable };
class BnwFreshAdvisorRefusal : public std::runtime_error {
 public:
    BnwFreshAdvisorRefusal(BnwFreshAdvisorError,const char*);
    BnwFreshAdvisorError code() const noexcept { return code_; }
 private: BnwFreshAdvisorError code_;
};
struct BnwAdvisorCounselRow { std::int32_t advisor,value; };
struct BnwAdvisorBuildable { std::int32_t type,index,turns; };
enum class BnwAdvisorWriteField {
    counsel_advisor,counsel_value,member_type,member_index,member_turns,
    reset_type,reset_index,reset_turns,reset_tech
};
struct BnwAdvisorWrite { BnwAdvisorWriteField field; std::uint32_t slot; std::int32_t value; };
struct BnwAdvisorRecommenderSnapshot {
    std::array<BnwAdvisorBuildable,4> recommended_builds;
    std::array<std::int32_t,4> recommended_techs;
    // Native logical sizes; reserved slots are not readable elements.
    std::array<std::uint32_t,4> weighted_sizes;
};
namespace detail {
// Executable detached storage used by the genuine private owners. This leaf
// cannot issue a Game, player, recommendation, or initialization receipt.
class BnwAdvisorCounselStorage {
 public:
    BnwAdvisorCounselStorage();
    ~BnwAdvisorCounselStorage();
    BnwAdvisorCounselStorage(const BnwAdvisorCounselStorage&)=delete;
    BnwAdvisorCounselStorage& operator=(const BnwAdvisorCounselStorage&)=delete;
    std::size_t size() const;
    BnwAdvisorCounselRow row(std::size_t) const;
    char text_byte(std::size_t,std::size_t) const;
    std::span<const BnwAdvisorWrite> constructor_writes() const;
    void reset_detached();
    void uninit_detached();
 private: struct Impl; std::unique_ptr<Impl> impl_;
};
class BnwAdvisorRecommenderStorage {
 public:
    BnwAdvisorRecommenderStorage();
    ~BnwAdvisorRecommenderStorage();
    BnwAdvisorRecommenderStorage(const BnwAdvisorRecommenderStorage&)=delete;
    BnwAdvisorRecommenderStorage& operator=(const BnwAdvisorRecommenderStorage&)=delete;
    BnwAdvisorRecommenderSnapshot observe() const;
    std::span<const BnwAdvisorWrite> constructor_writes() const;
    void reset_detached();
    void uninit_detached(); // Native Uninit is empty.
 private: struct Impl; std::unique_ptr<Impl> impl_;
};
}
// There is no public issuance or continuation endpoint. Future Game alone
// allocates these after its actual TacticalMap allocation; observations are
// diagnostics, not evidence that the future callsite has executed.
class BnwFreshAdvisorCounsel {
 public:
    ~BnwFreshAdvisorCounsel();
    BnwFreshAdvisorCounsel(const BnwFreshAdvisorCounsel&)=delete;
    BnwFreshAdvisorCounsel& operator=(const BnwFreshAdvisorCounsel&)=delete;
    std::size_t size() const;
    BnwAdvisorCounselRow row(std::size_t) const;
    char text_byte(std::size_t,std::size_t) const;
 private:
    friend class BnwFreshGame;
    explicit BnwFreshAdvisorCounsel(std::shared_ptr<const BnwCreationConfig>);
    void require_current() const;
    struct Impl; std::unique_ptr<Impl> impl_;
};
class BnwFreshAdvisorRecommender {
 public:
    ~BnwFreshAdvisorRecommender();
    BnwFreshAdvisorRecommender(const BnwFreshAdvisorRecommender&)=delete;
    BnwFreshAdvisorRecommender& operator=(const BnwFreshAdvisorRecommender&)=delete;
    BnwAdvisorRecommenderSnapshot observe() const;
 private:
    friend class BnwFreshGame;
    explicit BnwFreshAdvisorRecommender(std::shared_ptr<const BnwCreationConfig>);
    void require_current() const;
    struct Impl; std::unique_ptr<Impl> impl_;
};
}
