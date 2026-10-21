#pragma once
#include "bnw_native_completed_cold_source_loan.hpp"
#include <atomic>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace civ5::ui::detail {
class BnwNativeGlobalResourceWarmOwner;
class BnwNativePackageLoadActor;
class BnwNativePackageFStringField0;
class BnwNativePackageXmlParserOwner;

//8346A0/8348C0 byte producer, before834B30. No parsed manifest, physical
//path, caller-ready value or functional callback can issue a selected stream.
//Only the genuine Warm or initial pre-DX9 owner may construct the private
//Source subclass; each source authenticates its actual receiver and domain.
class BnwNativePackageDescriptorByteReader final {
    friend class BnwNativeInitialDescriptorSource;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativePreDx9DiscoveryOwner;
    friend class BnwNativePackageLoadActor;
    friend class BnwNativePackageXmlParserOwner;
    class Source {
        friend class BnwNativeGlobalResourceWarmOwner;
        friend class BnwNativePreDx9DiscoveryOwner;
        Source()=default;
        Source(const Source&)=delete;
        Source&operator=(const Source&)=delete;
        Source(Source&&)=delete;
        Source&operator=(Source&&)=delete;
      public:
        virtual void require_current()const=0;
        // Closed genuine Warm supplier must preserve selected stream/file/
        // receiver/membership checks against this explicit completed loan.
        virtual void require_current_completed(const BnwNativeCompletedColdSourceLoan&,
            const BnwNativeColdFilesystemOwner&)const=0;
        //The selected native stream must belong to THIS receiver after actual
        //498940 lookup and conditional839FE0/820040 canonical ACP rewrite.
        virtual void require_same_receiver(const BnwNativePackageFStringField0&)const=0;
        virtual std::uint32_t native_open_error_code()const=0; //virtual+8: zero succeeds.
        virtual std::uint32_t native_size_32()const=0;         //virtual+20 DWORD extent.
        virtual std::uint32_t native_read(std::span<std::uint8_t>)=0; //virtual+48 count.
        //Actual operation error only, never an ambient/stale GetLastError.
        virtual std::uint32_t native_read_error_code()const=0;
        virtual void close_native_stream()noexcept=0; //actual stream destructor.
        virtual void invalidate()noexcept=0;
        virtual ~Source()=default;
    };
    struct Buffer {
        const std::uint32_t native_size;
        const std::vector<std::uint8_t> nul_terminated;
        Buffer(std::uint32_t size,std::vector<std::uint8_t> bytes)
            :native_size(size),nul_terminated(std::move(bytes)){}
    };
    struct State {
        const std::thread::id thread=std::this_thread::get_id();
        const std::weak_ptr<Source> source;
        std::weak_ptr<const BnwNativePackageDescriptorByteReader> reader;
        std::atomic<bool> closed{false},failed{false},busy{false};
        std::atomic<bool> stream_closed{false}; //Actual resource-release occurrence only.
        void release_stream(const std::shared_ptr<Source>& actual)noexcept {
            if(actual&&!stream_closed.exchange(true))actual->close_native_stream();
        }
        bool read_attempted=false; //Port occurrence fence, NOT parsed/Loaded readiness.
        std::shared_ptr<const Buffer> buffer;
        explicit State(const std::shared_ptr<Source>& actual):source(actual){}
        static void need(bool value,const char* error){if(!value)throw std::runtime_error(error);}
        void live()const {
            need(thread==std::this_thread::get_id(),"Package descriptor reader foreign thread");
            need(!closed.load()&&!failed.load()&&!reader.expired(),"Package descriptor reader retired");
        }
        void check(const std::shared_ptr<Source>& actual)const {
            live();
            //Exactly one local Source pin is held by each operation; this
            //reference parameter creates no second pin to mask last-owner loss.
            need(actual&&actual.use_count()>1&&!source.owner_before(actual)&&
                 !actual.owner_before(source),"Package descriptor stream lost actual external source");
            actual->require_current();live();
            need(actual.use_count()>1&&!source.owner_before(actual)&&!actual.owner_before(source),
                 "Package descriptor stream retired/substituted in callback");
        }
        void check_completed(const std::shared_ptr<Source>& actual,const BnwNativeCompletedColdSourceLoan& loan,
            const BnwNativeColdFilesystemOwner& cold)const {
            live();loan.require_same_cold(cold);
            need(actual&&actual.use_count()>1&&!source.owner_before(actual)&&!actual.owner_before(source),
                 "Completed descriptor stream lost same external source/control");
            actual->require_current_completed(loan,cold);live();loan.require_same_cold(cold);
            need(actual.use_count()>1&&!source.owner_before(actual)&&!actual.owner_before(source),
                 "Completed descriptor stream source retired/substituted during check");
        }
        struct Scope {
            const std::shared_ptr<State> owned;
            explicit Scope(std::shared_ptr<State> value):owned(std::move(value)) {
                owned->live();
                if(owned->busy.exchange(true)) {
                    owned->failed.store(true);throw std::runtime_error("Package descriptor reader reentry");
                }
            }
            ~Scope(){owned->busy.store(false);}
        };
    };
    class HeldSourceGuard {
        friend class BnwNativePackageDescriptorByteReader;
        const std::shared_ptr<State>& state;
        const std::shared_ptr<Source>& source;
        const BnwNativeCompletedColdSourceLoan* const completed_loan=nullptr;
        const BnwNativeColdFilesystemOwner* const completed_cold=nullptr;
        const std::thread::id thread=std::this_thread::get_id();
        HeldSourceGuard(const std::shared_ptr<State>& actual,const std::shared_ptr<Source>& selected)
            :state(actual),source(selected){}
        HeldSourceGuard(const std::shared_ptr<State>& actual,const std::shared_ptr<Source>& selected,
            const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold)
            :state(actual),source(selected),completed_loan(&loan),completed_cold(&cold){}
      public:
        void require_current()const {
            if(completed_loan) {
                State::need(thread==std::this_thread::get_id(),"Completed source guard foreign creator thread");
                State::need(state->busy.load()&&completed_cold,"Completed source guard outside actual reader scope");
                state->check_completed(source,*completed_loan,*completed_cold);
            }else state->check(source); // Initial unbound guard retains its original route.
        } // No extra counted Source pin.
        HeldSourceGuard(const HeldSourceGuard&)=delete;
        HeldSourceGuard&operator=(const HeldSourceGuard&)=delete;
    };
    const std::shared_ptr<State> state_;
    explicit BnwNativePackageDescriptorByteReader(const std::shared_ptr<Source>& actual)
        :state_(std::make_shared<State>(actual)){}
    static std::shared_ptr<BnwNativePackageDescriptorByteReader> create(
            const std::shared_ptr<Source>& actual) {
        State::need(bool(actual),"Package descriptor reader lacks genuine selected stream Source");
        auto result=std::shared_ptr<BnwNativePackageDescriptorByteReader>(
            new BnwNativePackageDescriptorByteReader(actual));
        result->state_->reader=result;return result;
    }
    bool read_8348c0(const BnwNativePackageFStringField0& actual_receiver)const {
        const auto owned=state_;State::Scope operation(owned);
        const auto source=owned->source.lock();
        try {
            owned->check(source);
            State::need(!owned->read_attempted,"Package descriptor read occurrence replay");
            owned->read_attempted=true;
            source->require_same_receiver(actual_receiver);owned->check(source);
            const auto open_error=source->native_open_error_code();owned->check(source);
            if(open_error!=0) {
                owned->release_stream(source);owned->live();
                return false; //Computed actual failed open, never a query MISS.
            }
            const auto native_size=source->native_size_32();owned->check(source);
            //Existing metadata budget. Native UINT_MAX allocation wrap and
            //larger resources remain unsupported, never accepted as read success.
            constexpr std::uint32_t maximum_metadata_bytes=64U*1024U*1024U;
            State::need(native_size<=maximum_metadata_bytes,
                "Package descriptor native extent exceeds supported metadata budget");
            std::vector<std::uint8_t> bytes(std::size_t(native_size)+1U);
            std::uint32_t consumed=0;
            do { //Native8348C0 performs ONE virtual read even for a zero extent.
                owned->check(source);source->require_same_receiver(actual_receiver);owned->check(source);
                const auto remaining=native_size-consumed;
                const auto count=source->native_read(std::span(bytes).subspan(consumed,remaining));
                owned->check(source);
                const auto read_error=source->native_read_error_code();owned->check(source);
                State::need(read_error==0,"Package descriptor native stream read failed");
                State::need(count<=remaining,"Package descriptor native read count exceeds remaining extent");
                State::need(count!=0||remaining==0,
                    "Package descriptor zero-progress native read cannot prove8348C0 completion");
                consumed+=count;
            }while(consumed<native_size);
            bytes[native_size]=0; //83498F actual buffer terminator after the full read.
            source->require_same_receiver(actual_receiver);owned->check(source);
            auto completed=std::make_shared<const Buffer>(native_size,std::move(bytes));
            owned->check(source);owned->buffer=std::move(completed);
            return true; //Actual BYTE read only;834B30 parser has NOT succeeded.
        }catch(...) {
            owned->failed.store(true);
            if(source){owned->release_stream(source);source->invalidate();}
            throw;
        }
    }
    template<class Consumer> void with_current_bytes_before_834b30(Consumer&& consumer)const {
        const auto owned=state_;State::Scope operation(owned);
        const auto source=owned->source.lock();
        try {
            owned->check(source);
            State::need(bool(owned->buffer),"Package descriptor actual byte read is absent");
            const auto& value=*owned->buffer;
            State::need(value.nul_terminated.size()==std::size_t(value.native_size)+1U&&
                value.nul_terminated[value.native_size]==0,"Package descriptor byte extent/NUL changed");
            //Private consumer borrow, not callback-based Source admission.
            const HeldSourceGuard guard(owned,source);
            if constexpr(std::is_invocable_v<Consumer&,std::span<const std::uint8_t>,std::uint32_t,const HeldSourceGuard&>)
                consumer(std::span<const std::uint8_t>(value.nul_terminated),value.native_size,guard);
            else consumer(std::span<const std::uint8_t>(value.nul_terminated),value.native_size);
            guard.require_current();
        }catch(...) {
            owned->failed.store(true);
            if(source){owned->release_stream(source);source->invalidate();}
            throw;
        }
    }
    template<class Consumer>void with_current_bytes_completed(const BnwNativeCompletedColdSourceLoan& loan,
        const BnwNativeColdFilesystemOwner& cold,Consumer&& consumer)const {
        const auto owned=state_;State::Scope operation(owned);const auto source=owned->source.lock();
        try {
            owned->live();loan.require_same_cold(cold);
            State::need(source&&source.use_count()>1&&!owned->source.owner_before(source)&&!source.owner_before(owned->source),
                "Completed reader lacks actual external source/control before target copy");
            {
                const auto consume=std::forward<Consumer>(consumer);
                owned->check_completed(source,loan,cold);
                const auto buffer=owned->buffer;State::need(bool(buffer),"Completed actual byte read absent");
                State::need(buffer->nul_terminated.size()==std::size_t(buffer->native_size)+1U&&
                    buffer->nul_terminated[buffer->native_size]==0,"Completed actual byte extent/NUL changed");
                const HeldSourceGuard guard(owned,source,loan,cold);
                guard.require_current();
                if constexpr(std::is_invocable_v<decltype(consume)&,std::span<const std::uint8_t>,std::uint32_t,const HeldSourceGuard&>)
                    consume(std::span<const std::uint8_t>(buffer->nul_terminated),buffer->native_size,guard);
                else consume(std::span<const std::uint8_t>(buffer->nul_terminated),buffer->native_size);
                guard.require_current();owned->check_completed(source,loan,cold);
            } // Target destructor executes while original Reader Scope/Source/Cold are held.
            owned->check_completed(source,loan,cold);
        }catch(...){owned->failed.store(true);if(source){owned->release_stream(source);source->invalidate();}throw;}
    }
    //8349AA closes the SAME stream after the actual parser, including syntax
    //failure. Source currentness must continue checking receiver/epoch after
    //close; close is resource release, not authority to replace the byte source.
    void close_after_actual_parser_8349aa()const {
        const auto owned=state_;State::Scope operation(owned);const auto source=owned->source.lock();
        try {owned->check(source);owned->release_stream(source);owned->check(source);}
        catch(...){owned->failed.store(true);if(source){owned->release_stream(source);source->invalidate();}throw;}
    }
    void retire_after_parser_failure()const noexcept {
        state_->failed.store(true);
        if(const auto source=state_->source.lock()){state_->release_stream(source);source->invalidate();}
    }
public:
    ~BnwNativePackageDescriptorByteReader(){state_->closed.store(true);state_->release_stream(state_->source.lock());}
    BnwNativePackageDescriptorByteReader(const BnwNativePackageDescriptorByteReader&)=delete;
    BnwNativePackageDescriptorByteReader&operator=(const BnwNativePackageDescriptorByteReader&)=delete;
};
static_assert(!std::is_default_constructible_v<BnwNativePackageDescriptorByteReader>);
} //namespace civ5::ui::detail
