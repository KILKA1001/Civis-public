#pragma once
#include "civ5/ui/client_window_input.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <utility>
#include <stdexcept>

namespace civ5::ui {
class Win32ClientWindowBinding;
// Issued only while the SAME bound WndProc synchronously delivers an actual
// WM_ACTIVATE. No initial or pre-attachment message history is inferred.
class BnwAudioWindowActivationLoan final {
    friend class Win32ClientWindowBinding;
    struct State {
        const HWND window;
        const DWORD thread;
        const ClientWindowInput* const input;
        const Win32ClientWindowBinding* const binding;
        std::atomic<bool> alive{true},failed{false};
        std::uint64_t sequence=0,active_sequence=0;
        std::uint16_t observed_minimized_word=0; //Unreadable until a real observed sequence.
        std::weak_ptr<const void> operation; //Actual Binding shared Impl, never a facade pin.
        State(HWND w,const ClientWindowInput& i,const Win32ClientWindowBinding& b)
            :window(w),thread(GetCurrentThreadId()),input(&i),binding(&b){}
    };
    const std::shared_ptr<State> state_;
    const std::uint64_t sequence_;
    const std::uint16_t minimized_word_;
    BnwAudioWindowActivationLoan(std::shared_ptr<State> state,std::uint64_t sequence,WPARAM w)
        :state_(std::move(state)),sequence_(sequence),minimized_word_(HIWORD(w)){}
  public:
    class SourceHandle final {
        friend class BnwAudioWindowActivationLoan;
        const std::shared_ptr<State> state_;
        // Retain the SAME real Binding Impl, not a caller-supplied facade or
        // no-op control block. Actual HWND/property/alive checks remain required;
        // keeping storage alive never authorizes a detached/destroyed window.
        const std::shared_ptr<const void> operation_;
        explicit SourceHandle(std::shared_ptr<State> state)
            :state_(std::move(state)),operation_(state_->operation.lock()){}
      public:
        void require_current()const {
            if(GetCurrentThreadId()!=state_->thread)
                throw std::runtime_error("Audio window source foreign thread");
            if(!state_->alive.load()||state_->failed.load())
                throw std::runtime_error("Audio window source retired");
            const auto operation=state_->operation.lock();
            if(!operation||!operation_||operation.get()!=operation_.get()||
               operation.owner_before(operation_)||operation_.owner_before(operation)||
               !IsWindow(state_->window)||
               GetWindowThreadProcessId(state_->window,nullptr)!=state_->thread||
               GetPropW(state_->window,L"Civis.ProductionClientWindow")!=state_->binding||
               !state_->alive.load()||state_->failed.load())
                throw std::runtime_error("Audio actual window operation source retired");
        }
        HWND window()const {require_current();return state_->window;}
        DWORD ui_thread()const {require_current();return state_->thread;}
        const ClientWindowInput* input_owner()const {require_current();return state_->input;}
        std::uint64_t observed_sequence()const {require_current();return state_->sequence;}
        std::uint16_t native_minimized_word()const {
            require_current();
            if(!state_->sequence)throw std::runtime_error("Audio window source has no actual observed activation");
            return state_->observed_minimized_word;
        }
        void require_same_source(const SourceHandle& expected)const {
            require_current();expected.require_current();
            if(state_.get()!=expected.state_.get())
                throw std::runtime_error("Audio window operation source substituted");
        }
        void require_same_window_owner(const Win32ClientWindowBinding& binding,
                                       const ClientWindowInput& input)const {
            require_current();
            if(state_->binding!=&binding||state_->input!=&input)
                throw std::runtime_error("Audio source substituted binding/input owner");
        }
    };
    SourceHandle source_handle()const {require_current();return SourceHandle(state_);}
    void require_current()const {
        SourceHandle(state_).require_current();
        if(!sequence_||state_->sequence!=sequence_||state_->active_sequence!=sequence_)
            throw std::runtime_error("Audio actual window activation retired");
    }
    HWND window()const {require_current();return state_->window;}
    DWORD ui_thread()const {require_current();return state_->thread;}
    std::uint64_t sequence()const {require_current();return sequence_;}
    std::uint16_t native_minimized_word()const {require_current();return minimized_word_;}
    const ClientWindowInput* input_owner()const {require_current();return state_->input;}
    void require_same_window_owner(const Win32ClientWindowBinding& binding,
                                   const ClientWindowInput& input)const {
        require_current();
        if(state_->binding!=&binding||state_->input!=&input)
            throw std::runtime_error("Audio activation substituted window/input owner");
    }
};
} // namespace civ5::ui
#endif
