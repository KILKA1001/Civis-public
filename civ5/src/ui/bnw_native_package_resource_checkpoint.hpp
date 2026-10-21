#pragma once
#include "bnw_native_package_load_actor.hpp"
#include "bnw_native_standalone_package_loaded.hpp"
#include "bnw_native_selected_category_record_borrow.hpp"
#include <limits>
#include <algorithm>
#include <utility>
#include <vector>
#include <string_view>

namespace civ5::ui::detail {
//Only the genuine fresh receiver's held pre-metadata resource channel can
//construct this stack Source. It does not issue Loaded, Category completion,
//manager priority/cache promotion, or a query universe of its own.
class BnwNativePackageResourceCheckpoint final
    :private BnwNativeSkinResourceQueryOutputProducer::Source {
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    using Load=BnwNativePackageLoadActor;
    using Parser=BnwNativePackageXmlParserOwner;
    using Output=BnwNativeSkinResourceQueryOutputProducer;
    using Node=Parser::Node;
    using Doc=Parser::Document;
    using RecordConsumer=Output::Source::Consumer;
    enum class Query {paks,directory,map_directory,skin_directory,
                      gameplay_skin_directory,file,skin_paks};
    const Load::ResourceLoan& actual_loan;
    mutable bool failed=false;
    const std::uint32_t selected_category_;
    explicit BnwNativePackageResourceCheckpoint(const Load::ResourceLoan& loan,std::uint32_t selected=UINT32_MAX)
        :actual_loan(loan),selected_category_(selected){need(selected==UINT32_MAX||selected<3,"Output source named category outside actual slots");require_current();}
    static void need(bool value,const char* error) {
        if(!value)throw std::runtime_error(error);
    }
    void require_current()const override {
        need(!failed,"Resource output checkpoint failed");actual_loan.require_current();
    }
    void with_native_record(const std::string& raw,const RecordConsumer& consume)const override {
        require_current();
        if(selected_category_<3)BnwNativeSelectedCategoryRecordBorrower::with_selected_record(actual_loan,selected_category_,raw,consume);
        else actual_loan.with_native_record(raw,consume);
        require_current();
    }
    std::string copied_receiver_for_62ec20_fallback()const override {
        require_current();auto result=actual_loan.copied_receiver_for_62ec20();
        require_current();return result;
    }
    void invalidate()const noexcept override {failed=true;actual_loan.invalidate();}
    static std::string_view range(const Doc& doc,Parser::Range value) {
        need(value.begin<=doc.decoded.size()&&value.size<=doc.decoded.size()-value.begin,
             "Resource property range escaped SAME actual Parser Buffer");
        return {reinterpret_cast<const char*>(doc.decoded.data()+value.begin),value.size};
    }
    static std::string_view path(Query query,std::uint32_t category) {
        switch(query) {
        case Query::paks:return "Paks";
        case Query::directory:return "Directory";
        case Query::map_directory:return "MapDirectory";
        case Query::file:return "File";
        case Query::skin_directory:
            need(category==2,"Skin/Directory requires actual UISkin category");return "Skin/Directory";
        case Query::gameplay_skin_directory:
            need(category==2,"GameplaySkin/Directory requires actual UISkin category");return "GameplaySkin/Directory";
        case Query::skin_paks:
            need(category==2,"Skin/Paks requires actual UISkin category");return "Skin/Paks";
        }
        throw std::runtime_error("Unknown private native property query");
    }
    static const Node* first_child(const Doc& doc,const Node& node,std::string_view name) {
        for(const auto& child:node.children)if(range(doc,child->name)==name)return child.get();
        return nullptr;
    }
    //8352A0 leaves its destination unchanged on failed ASCII conversion.
    //5955F0 ignores that return; a later failed File repeats the previous raw
    //string. An absent value is a successful empty getter and clears it.
    static void assign_ascii_value(const Doc& doc,const Node& node,std::string& destination) {
        const auto value=node.value?range(doc,*node.value):std::string_view{};
        for(unsigned char byte:value)if(byte&128)return;
        destination.assign(value.data(),value.size());
    }
    void append_output(const std::string& raw,int& count,std::vector<std::string>& outputs)const {
        require_current();need(count<std::numeric_limits<int>::max(),"Resource property count overflow");
        auto value=Output::produce(raw,*this);require_current();
        if(value)outputs.push_back(std::move(*value));
        ++count; //Native counts5EB290 calls, including empty C-prefix/no-output.
    }
    void property_5955f0(const Doc& doc,const Node& scope,std::string_view query,
                        int& count,std::vector<std::string>& outputs)const {
        require_current();const Node* parent=&scope;
        //Only the first matching intermediate child is selected. The last
        //component enumerates matching siblings, using exact835A80 name bytes.
        for(auto slash=query.find('/');slash!=std::string_view::npos;slash=query.find('/')) {
            parent=first_child(doc,*parent,query.substr(0,slash));if(!parent)return;
            query.remove_prefix(slash+1);
        }
        for(const auto& property:parent->children) {
            if(range(doc,property->name)!=query)continue;
            std::string raw; //Fresh native local per property occurrence.
            if(property->children.empty()) {
                assign_ascii_value(doc,*property,raw);append_output(raw,count,outputs);
            }else for(const auto& value:property->children) {
                if(range(doc,value->name)!="File")continue;
                assign_ascii_value(doc,*value,raw);append_output(raw,count,outputs);
            }
        }
        require_current();
    }
    template<class Consumer> static void with_outputs_from_resource_loan(
            const Load::ResourceLoan& loan,std::uint32_t category,std::uint32_t occurrence,
            Query query,Consumer&& consume) {
        need(category<3,"Resource checkpoint category outside actual0..2 callers");
        const auto property=path(query,category);
        BnwNativePackageResourceCheckpoint source(loan);
            try {
                const auto& fields=loan.fields();const auto& doc=loan.document();
                int count=0;std::vector<std::string> outputs;
                //54B8D0 fixed flags0/param7=0. Actual reader/root/core admission
                //belongs to the Loan. 891960(0) has no field or source effect.
                if(!((fields.current_flags()&1)&&!(fields.current_flags()&4))&&!(fields.current_flags()&64)&&
                   (fields.current_category_mask()&(std::uint32_t{1}<<category))) {
                    need(!doc.roots.empty(),"Actual resource core lacks Parser root");
                    const auto& root=*doc.roots.front();
                    static constexpr std::string_view names[]={"Common","Gameplay","UISkin"};
                    auto remaining=occurrence;
                    for(const auto& node:root.children) {
                        if(range(doc,node->name)!=names[category])continue;
                        if(remaining){--remaining;continue;}
                        source.property_5955f0(doc,*node,property,count,outputs);break;
                    }
                    //Common root-level declarations are queried even if the
                    //requested Common occurrence was absent/exhausted.
                    if(category==0)source.property_5955f0(doc,root,property,count,outputs);
                }
                source.require_current();consume(count,outputs);source.require_current();
            }catch(...){source.invalidate();throw;}
    }
    template<class Consumer> static void with_outputs_before_category_label(
            const Load& receiver,std::uint32_t category,std::uint32_t occurrence,
            Query query,Consumer&& consume) {
        receiver.with_resource_checkpoint_before_metadata_738f00([&](const Load::ResourceLoan& loan){
            with_outputs_from_resource_loan(loan,category,occurrence,query,std::forward<Consumer>(consume));
        });
    }
    template<class Consumer> static void with_outputs_from_loaded(
            const BnwNativeStandalonePackageLoaded& package,std::uint32_t category,std::uint32_t occurrence,
            Query query,Consumer&& consume) {
        package.with_actual_descriptor([&](const auto& loan){
            loan.require_current();with_outputs_from_resource_loan(loan.actual_resource_loan(),category,
                occurrence,query,std::forward<Consumer>(consume));loan.require_current();
        });
    }
    //64C610/686D80 uses categoryFFFFFFFF, flags0, output nonnull. The
    //actual root -> first Gameplay -> first UISkin -> first Common order is
    //different from a category0 query. Only genuine held ResourceLoan enters.
    template<class Consumer> static void with_initial_all_outputs_54b8d0(
        const Load::ResourceLoan& loan,std::string_view selector,Consumer&& consume) {
        need(selector=="GameCore"||selector=="Paks","Initial admission selector outside actual686D80 callers");
        BnwNativePackageResourceCheckpoint source(loan);
        try {
            const auto& fields=loan.fields();const auto& doc=loan.document();
            int count=0;std::vector<std::string> outputs;
            if(!((fields.current_flags()&1)&&!(fields.current_flags()&4))&&!(fields.current_flags()&64)) {
                need(!doc.roots.empty(),"Initial all-category query lacks SAME actual Parser root");
                const auto& root=*doc.roots.front();
                source.property_5955f0(doc,root,selector,count,outputs);
                for(const auto category:{1u,2u,0u})if(fields.current_category_mask()&(std::uint32_t{1}<<category)) {
                    static constexpr std::string_view names[]={"Common","Gameplay","UISkin"};
                    if(const auto* selected=first_child(doc,root,names[category]))
                        source.property_5955f0(doc,*selected,selector,count,outputs);
                }
            }
            source.require_current();consume(count,outputs);source.require_current();
        }catch(...){source.invalidate();throw;}
    }
    //762400(category,0): actual fresh ParserDoc direct sibling count, including
    //the native Gameplay GameData fallback. No old Skin roster supplies it.
    static std::uint32_t count_from_loaded_762400(const BnwNativeStandalonePackageLoaded& package,
                                                std::uint32_t category) {
        need(category<3,"Owned category count outside native0..2");std::uint32_t result=0;
        package.with_actual_descriptor([&](const auto& loan){
            loan.require_current();const auto& fields=loan.fields();const auto& doc=loan.document();
            if(((fields.current_flags()&1)&&!(fields.current_flags()&4))||(fields.current_flags()&64)||
               !(fields.current_category_mask()&(std::uint32_t{1}<<category)))return;
            need(!doc.roots.empty(),"Owned category count lost actual Parser root");
            static constexpr std::string_view names[]={"Common","Gameplay","UISkin"};
            for(const auto& child:doc.roots.front()->children)if(range(doc,child->name)==names[category]){
                need(result<UINT32_MAX,"Actual owned category count overflow");++result;
            }
            if(!result&&category==1)for(const auto& child:doc.roots.front()->children)
                if(range(doc,child->name)=="GameData"){result=1;break;}
            loan.require_current();
        });return result;
    }
    //48C800 passes an ordinary output vector and a nonnull EXTRA list to its
    //preconstructor Skin/Directory54B8D0 call.891960 copies ordered attributes
    //from the SAME selected nthUISkin before5955F0 produces ordinary outputs.
    template<class Consumer> static void with_skin_directory_and_extra_attributes_from_loaded_54b8d0(
        const BnwNativeStandalonePackageLoaded& package,std::uint32_t occurrence,Consumer&& consume) {
        package.with_actual_descriptor([&](const auto& held){
            held.require_current();BnwNativePackageResourceCheckpoint source(held.actual_resource_loan());
            try {
                const auto& fields=held.fields();const auto& doc=held.document();int count=0;
                std::vector<std::string> ordinary;
                std::vector<std::pair<std::string,std::string>> extra;
                if(!((fields.current_flags()&1)&&!(fields.current_flags()&4))&&!(fields.current_flags()&64)&&(fields.current_category_mask()&4)) {
                    need(!doc.roots.empty(),"Actual48C800 extra-output query has no Parser root");
                    const Node* skin=nullptr;auto remaining=occurrence;
                    for(const auto& node:doc.roots.front()->children)if(range(doc,node->name)=="UISkin"){
                        if(remaining){--remaining;continue;}skin=node.get();break;
                    }
                    if(skin){
                        //891960 makes fresh empty FString key/value each item;
                        //835090/8352A0 ASCII failure leaves that fresh field empty.
                        const auto ascii=[&](Parser::Range value){
                            const auto text=range(doc,value);std::string result;
                            if(std::none_of(text.begin(),text.end(),[](unsigned char c){return bool(c&128);}))
                                result.assign(text.data(),text.size());
                            return result;
                        };
                        for(const auto& item:skin->attributes){
                            source.require_current();extra.emplace_back(ascii(item.name),ascii(item.value));source.require_current();
                        }
                        source.property_5955f0(doc,*skin,"Skin/Directory",count,ordinary);
                    }
                }
                source.require_current();held.require_current();consume(count,ordinary,extra);
                held.require_current();source.require_current();
            }catch(...){source.invalidate();throw;}
        });
    }

    //67FDD0 asks762400(category1,force1): this force lane ignores actual+48
    //category-mask bit1, while still applying the native+44 flags/root gate.
    //The caller compares this count separately from its current mask bit1.
    static std::uint32_t forced_gameplay_count_from_loaded_762400_1_1(
        const BnwNativeStandalonePackageLoaded& package) {
        std::uint32_t result=0;
        package.with_actual_descriptor([&](const auto& loan){
            loan.require_current();const auto& fields=loan.fields();const auto& doc=loan.document();
            if(((fields.current_flags()&1)&&!(fields.current_flags()&4))||(fields.current_flags()&64))return;
            need(!doc.roots.empty(),"Actual forcedGameplay count has no fresh Parser root");
            for(const auto& child:doc.roots.front()->children)if(range(doc,child->name)=="Gameplay"){
                need(result<UINT32_MAX,"ForcedGameplay native sibling count exceeds supported DWORD domain");++result;
            }
            if(!result)for(const auto& child:doc.roots.front()->children)
                if(range(doc,child->name)=="GameData"){result=1;break;}
            loan.require_current();
        });return result;
    }

    //798CE0 genuine54B8D0 NULL-output lanes. Unlike a resource output query,
    //5EB290 never reaches lookup/fallback when its output pointer is null.
    //5955F0 still counts every invocation, including empty/stale ASCII input.
    static std::uint32_t null_output_from_loaded_54b8d0(const BnwNativeStandalonePackageLoaded& package,
        std::uint32_t occurrence,unsigned slot,const char* selector,std::uint32_t filter) {
        need(slot<2&&selector,"Owned Skin NULL query slot/selector absent");
        const std::string property(selector);
        need((property=="Directory"||property=="Paks")?filter==0:(property=="File"&&filter==0x80),
             "Owned Skin constructor accepts only actual Directory0/Paks0/File80 calls");
        const auto query=std::string(slot==0?"GameplaySkin/":"Skin/")+property;
        std::uint32_t result=0;
        package.with_actual_descriptor([&](const auto& loan){
            loan.require_current();const auto& fields=loan.fields();const auto& doc=loan.document();
            if(((fields.current_flags()&1)&&!(fields.current_flags()&4))||(fields.current_flags()&64)||!(fields.current_category_mask()&4))return;
            need(!doc.roots.empty(),"Owned Skin NULL query lost actual Parser root");
            const Node* skin=nullptr;auto remaining=occurrence;
            for(const auto& node:doc.roots.front()->children)if(range(doc,node->name)=="UISkin"){
                if(remaining){--remaining;continue;}skin=node.get();break;
            }
            if(!skin)return;
            const Node* parent=skin;std::string_view path=query;
            for(auto slash=path.find('/');slash!=std::string_view::npos;slash=path.find('/')){
                parent=first_child(doc,*parent,path.substr(0,slash));if(!parent)return;path.remove_prefix(slash+1);
            }
            const auto invoke_null=[&](const Node& node,std::string& raw){
                loan.require_current();assign_ascii_value(doc,node,raw);
                //Actual5EB290 copies its C-string prefix into a fresh local.
                //Its Boolean result is ignored by5955F0; completed calls are
                //counted with the native DWORD wrap, independently of emptiness.
                const auto stop=raw.find('\0');const auto extent=raw.substr(0,stop);
                need(extent.size()<=std::size_t(INT32_MAX),"Owned NULL FString length exceeds signed native domain");
                const std::string local(extent);(void)local;
                ++result;loan.require_current();
            };
            for(const auto& node:parent->children){
                if(range(doc,node->name)!=path)continue;std::string raw;
                if(node->children.empty())invoke_null(*node,raw);
                else for(const auto& value:node->children)if(range(doc,value->name)=="File")invoke_null(*value,raw);
            }
            loan.require_current();
        });return result;
    }
    //Actual6D4BD0 nonNULL output lanes: Directory/Paks0 use global8211F0;
    //File/Style/Color80 inspect SAME retained Package Core category2 slot.
    template<class Consumer> static void with_owned_skin_outputs_from_loaded_54b8d0(
        const BnwNativeStandalonePackageLoaded& package,std::uint32_t occurrence,unsigned slot,
        const char* selector,std::uint32_t filter,Consumer&& consume) {
        need(slot<2&&selector,"Owned Skin output query slot/property absent");const std::string property(selector);
        need((property=="Directory"||property=="Paks")?filter==0:
             ((property=="File"||property=="Style"||property=="Color")&&filter==0x80),
             "Owned Skin output query differs from genuine6D4BD0 calls");
        const auto query=std::string(slot==0?"GameplaySkin/":"Skin/")+property;
        package.with_actual_descriptor([&](const auto& held){
            held.require_current();BnwNativePackageResourceCheckpoint source(held.actual_resource_loan(),filter==0x80?2:UINT32_MAX);
            try {
                const auto& fields=held.fields();const auto& doc=held.document();int count=0;std::vector<std::string> outputs;
                if(!((fields.current_flags()&1)&&!(fields.current_flags()&4))&&!(fields.current_flags()&64)&&(fields.current_category_mask()&4)) {
                    need(!doc.roots.empty(),"Owned Skin output query lost actual Parser root");auto remaining=occurrence;
                    for(const auto& node:doc.roots.front()->children)if(range(doc,node->name)=="UISkin"){
                        if(remaining){--remaining;continue;}source.property_5955f0(doc,*node,query,count,outputs);break;
                    }
                }
                source.require_current();held.require_current();consume(count,outputs);held.require_current();source.require_current();
            }catch(...){source.invalidate();throw;}
        });
    }

    BnwNativePackageResourceCheckpoint(const BnwNativePackageResourceCheckpoint&)=delete;
    BnwNativePackageResourceCheckpoint&operator=(const BnwNativePackageResourceCheckpoint&)=delete;
};
} //namespace civ5::ui::detail
