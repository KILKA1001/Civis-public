#include "civ5/ui/detail/bnw_native_label_field_storage.hpp"
#include <atomic>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <thread>

namespace civ5::ui::detail {
namespace {
void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
}
struct BnwNativeLabelFieldStorage::Cell {
    BnwLabelFieldBinding binding;
    BnwLabelStoredFields fields;
    std::optional<BnwLabelColorFieldBinding> color_binding;
    const std::thread::id thread=std::this_thread::get_id();
    std::atomic<bool> closed{false}, failed{false};
    std::uint64_t revision=0;
    bool busy=false;
    explicit Cell(BnwLabelFieldBinding b):binding(std::move(b)){}
    void available(){
        need(std::this_thread::get_id()==thread,"Label field storage wrong thread");
        need(!closed&&!failed,"Label field storage retired");
    }
    void enter(){
        available();
        if(busy){failed=true;throw std::runtime_error("Label field storage reentry");}
        busy=true;
    }
    void validate(){
        available();
        need(binding.current&&binding.current(),"Label graph/source/parent changed");
        available();
        if(fields.font){need(fields.font->current&&fields.font->current(),"Label authored font changed");available();}
        if(fields.extended){need(fields.extended->current&&fields.extended->current(),"Label extended font changed");available();}
        if(color_binding){need(color_binding->current&&color_binding->current(),"Label Color quote changed");available();}
        // Font/Color current callbacks may change a base owner while returning
        // true. Independently retain and recheck the SAME original base guard.
        need(binding.current&&binding.current(),"Label base changed during dependency check");
        available();
    }
    struct Scope { Cell& c;~Scope(){c.busy=false;} };
    bool matches(std::uint64_t expected) noexcept {
        // No mutable source state is accessed from a foreign thread.
        if(std::this_thread::get_id()!=thread||closed||failed)return false;
        try{enter();Scope scope{*this};if(revision!=expected)return false;validate();return revision==expected&&!closed&&!failed;}
        catch(...){failed=true;return false;}
    }
    void quote_layers(){
        fields.layer_indices.reset();
        if(fields.font&&fields.font->layers){
            auto quote=fields.font->layers(fields.style);
            available();fields.layer_indices=std::move(quote);
        }
    }
};
struct BnwLabelFieldSnapshot::Data {
    std::weak_ptr<BnwNativeLabelFieldStorage::Cell> cell;
    BnwLabelStoredFields fields;
    BnwLabelFieldIdentity identity;
    std::uint64_t revision=0;
};
BnwLabelFieldSnapshot::BnwLabelFieldSnapshot(std::shared_ptr<const Data>d):data_(std::move(d)){}
const BnwLabelStoredFields&BnwLabelFieldSnapshot::fields()const{return data_->fields;}
const BnwLabelFieldIdentity&BnwLabelFieldSnapshot::identity()const{return data_->identity;}
std::uint64_t BnwLabelFieldSnapshot::revision()const{return data_->revision;}
bool BnwLabelFieldSnapshot::current()const noexcept{auto c=data_->cell.lock();return c&&c->matches(data_->revision);}
BnwNativeLabelFieldStorage::BnwNativeLabelFieldStorage(BnwLabelFieldBinding binding){
    need(binding.graph&&binding.control&&binding.parent&&!binding.source_path.empty()&&binding.current,"Label constructor SAME graph/control/parent binding absent");
    cell_=std::make_shared<Cell>(std::move(binding));
    (void)snapshot();
}
BnwNativeLabelFieldStorage::~BnwNativeLabelFieldStorage(){close();}
void BnwNativeLabelFieldStorage::close()noexcept{if(cell_)cell_->closed=true;}
BnwLabelFieldSnapshot BnwNativeLabelFieldStorage::snapshot()const{
    auto c=cell_;c->enter();Cell::Scope scope{*c};
    // Own every value BEFORE any callback. No exposed mutable/borrowed fields.
    auto d=std::make_shared<BnwLabelFieldSnapshot::Data>();d->cell=c;d->fields=c->fields;d->revision=c->revision;
    d->identity={c->binding.graph,c->binding.control,c->binding.parent,c->binding.control_index,c->binding.source_path};
    try{c->validate();need(c->revision==d->revision,"Label changed during field capture");}
    catch(...){c->failed=true;throw;}
    return BnwLabelFieldSnapshot(std::move(d));
}
BnwLabelFieldTransition BnwNativeLabelFieldStorage::mutate(BnwLabelFieldOperation op,bool calls_update,std::function<void(Cell&)> change){
    auto c=cell_;c->enter();Cell::Scope scope{*c};
    try{
        need(c->revision!=std::numeric_limits<std::uint64_t>::max(),"Label Civis revision exhausted");
        // Admission retires old requests before source/owner callbacks, even
        // identical text or a source no-op. This revision is a Civis fence.
        ++c->revision;c->validate();change(*c);c->validate();
        return {op,c->revision,calls_update,calls_update,false};
    }catch(...){c->failed=true;throw;}
}
BnwLabelFieldTransition BnwNativeLabelFieldStorage::set_text(std::optional<std::string> input){
    // Narrow safe complete-string input: original strnlen0x4000 +length+1 copy.
    std::string owned=input?std::move(*input):std::string{};
    need(owned.size()<=0x4000&&owned.find('\0')==std::string::npos,"Label SetText complete string outside bounded source input");
    return mutate(BnwLabelFieldOperation::set_text,true,[owned=std::move(owned)](Cell&c){c.fields.text=owned;});
}
BnwLabelFieldTransition BnwNativeLabelFieldStorage::set_font(std::optional<BnwLabelFontFieldBinding> input,ExtendedSelector selector){
    if(input)need(input->authored&&!input->source_sha256.empty()&&input->current,"Label authored font binding incomplete");
    return mutate(BnwLabelFieldOperation::set_font,true,[input=std::move(input),selector=std::move(selector)](Cell&c){
        const auto old=c.fields.font?c.fields.font->authored.get():nullptr;
        const auto incoming=input?input->authored.get():nullptr;
        if(old==incoming)return; // 6F3CDD ->6F3D42: no +170 reselection.
        if(input)c.fields.font=input; // NULL incoming retains current +CC.
        if(c.fields.font){
            need(static_cast<bool>(selector),"Label current extended-font policy supplier unavailable");
            c.fields.extended=selector(*c.fields.font,c.fields.force_ime,c.fields.force_non_ime);c.available();
            if(c.fields.extended)need(c.fields.extended->owner&&c.fields.extended->current,"Label extended font binding incomplete");
        }else c.fields.extended.reset();
        c.quote_layers();
    });
}
BnwLabelFieldTransition BnwNativeLabelFieldStorage::set_style(std::uint8_t style){
    auto c=cell_;c->available();
    const bool has_font=c->fields.font.has_value();
    return mutate(BnwLabelFieldOperation::set_style,has_font,[style](Cell&c){if(c.fields.font){c.fields.style=style;c.quote_layers();}});
}
BnwLabelFieldTransition BnwNativeLabelFieldStorage::set_colors(BnwLabelColorFieldBinding binding){
    need(binding.owner&&binding.current,"Label Color quote owner missing");
    // Preserve finite float fields and values outside0..1. No normalization,
    // packing, Color parser, or x87 profile is introduced here.
    for(const auto&color:binding.values)for(float f:color)need(std::isfinite(f),"Label Color quote nonfinite outside bounded storage scope");
    return mutate(BnwLabelFieldOperation::set_colors,true,[binding=std::move(binding)](Cell&c){c.fields.colors=binding.values;c.fields.color_override=true;c.color_binding=binding;});
}
BnwLabelFieldTransition BnwNativeLabelFieldStorage::xml_wrap(std::int16_t width){return mutate(BnwLabelFieldOperation::xml_wrap,true,[width](Cell&c){c.fields.truncate=-1;c.fields.wrap=width;});}
BnwLabelFieldTransition BnwNativeLabelFieldStorage::xml_truncate(std::int16_t width){return mutate(BnwLabelFieldOperation::xml_truncate,true,[width](Cell&c){c.fields.wrap=-1;c.fields.truncate=width;});}
BnwLabelFieldTransition BnwNativeLabelFieldStorage::xml_leading(std::int16_t leading){return mutate(BnwLabelFieldOperation::xml_leading,false,[leading](Cell&c){c.fields.leading=leading;});}
BnwLabelFieldTransition BnwNativeLabelFieldStorage::xml_force_ime(bool parsed_true){return mutate(BnwLabelFieldOperation::xml_force_ime,false,[parsed_true](Cell&c){c.fields.force_ime|=parsed_true;});}
BnwLabelFieldTransition BnwNativeLabelFieldStorage::xml_force_non_ime(bool parsed_true){return mutate(BnwLabelFieldOperation::xml_force_non_ime,false,[parsed_true](Cell&c){c.fields.force_non_ime|=parsed_true;});}
BnwLabelFieldTransition BnwNativeLabelFieldStorage::xml_read_entry(){return mutate(BnwLabelFieldOperation::xml_read_entry,false,[](Cell&c){c.fields.subtype_read_entered=true;});}
BnwLabelFieldTransition BnwNativeLabelFieldStorage::reset(){return mutate(BnwLabelFieldOperation::reset,false,[](Cell&c){
    //64C8C0 is NOT constructor replay: style/colors/override survive it.
    c.fields.text.reset();c.fields.font.reset();c.fields.extended.reset();c.fields.layer_indices.reset();c.fields.wrap=c.fields.truncate=-1;c.fields.leading=0;
});}
}
