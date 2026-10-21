#include "civ5/ui/detail/notification_layout_runtime_dimensions.hpp"
#include <stdexcept>
#include <string>
namespace civ5::ui::detail {
std::array<float,4> NotificationLayoutRuntimeDimensions::image_color_owned(std::string_view input)const{
    const auto saved=state_;
    try{
        require_storage_without_callbacks(saved);const std::string request(input);
        const auto result=image_color_from_storage_owned(saved,request);
        require_storage_without_callbacks(saved);return result;
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}
std::string NotificationLayoutRuntimeDimensions::tooltip_text_owned(std::string_view input)const{
    const auto saved=state_;
    try{require_storage_without_callbacks(saved);const std::string key(input);
        auto result=tooltip_text_from_storage_owned(saved,key);
        require_storage_without_callbacks(saved);return result;
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}
void NotificationLayoutRuntimeDimensions::tooltip_changed_owned()const{
    const auto saved=state_;
    try{require_storage_without_callbacks(saved);tooltip_changed_from_storage_owned(saved);
        require_storage_without_callbacks(saved);
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}
float NotificationLayoutRuntimeDimensions::dimension_owned(std::size_t axis)const{
    const auto saved=state_; // No facade access after any supplier boundary.
    try{
        require_storage_without_callbacks(saved);
        if(axis>=2)throw std::runtime_error("Notification runtime Size axis invalid");
        const auto actual=dimensions_from_storage_without_callbacks(saved);
        require_storage_without_callbacks(saved);
        // Native CVTSI2SS consumes signed32, not unsigned or parent/viewport DTO.
        const float value=static_cast<float>(actual[axis]);
        require_storage_without_callbacks(saved);return value;
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}
void NotificationLayoutRuntimeDimensions::register_resize_owned(std::uint32_t flags)const{
    const auto saved=state_;
    try{
        require_storage_without_callbacks(saved);
        if(!(flags&0x300000u))throw std::runtime_error("Notification resize registration lacks Full axis");
        register_resize_from_storage_owned(saved,flags);
        require_storage_without_callbacks(saved);
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}
}
