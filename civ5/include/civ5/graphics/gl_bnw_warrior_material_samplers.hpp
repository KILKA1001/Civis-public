#pragma once
#include "civ5/graphics/bnw_warrior_sampler_policy.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include <memory>
namespace civ5::graphics {
// Owns six actual sampler objects. Textures must use LINEAR storage views:
// native SRGBTexture0 has no corresponding GL sampler bit. The verified DDS
// loader uses DdsSamplingColor::linear. This owner never substitutes a resource.
class GlBnwWarriorMaterialSamplers {
    std::shared_ptr<GlContext> context_;
    GlRenderApi gl_;
    BnwWarriorNativeSamplerPolicy policy_;
    std::array<GLuint,6> samplers_{};
    static const GlContext &checked(const std::shared_ptr<GlContext>& c){if(!c)throw std::invalid_argument("Missing Warrior sampler GL owner");(void)c->api();return *c;}
public:
    GlBnwWarriorMaterialSamplers(std::shared_ptr<GlContext> context,const BnwWarriorNativeSamplerPolicy &policy)
        :context_(std::move(context)),gl_(checked(context_)),policy_(policy) {
        gl_.CreateSamplers(6,samplers_.data());
        try{for(unsigned i=0;i<6;++i){
            const auto r=policy_.slot(i,policy_.member(),policy_.unit(),policy_.revision());
            if(r!=BnwWarriorSamplerRecord{1,1,1,2,2,2,1,false})throw std::invalid_argument("Foreign native Warrior sampler record");
            gl_.SamplerParameteri(samplers_[i],GL_TEXTURE_WRAP_S,GL_REPEAT);
            gl_.SamplerParameteri(samplers_[i],GL_TEXTURE_WRAP_T,GL_REPEAT);
            gl_.SamplerParameteri(samplers_[i],GL_TEXTURE_WRAP_R,GL_REPEAT);
            gl_.SamplerParameteri(samplers_[i],GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            gl_.SamplerParameteri(samplers_[i],GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
            gl_.SamplerParameterf(samplers_[i],0x84fe /* GL_TEXTURE_MAX_ANISOTROPY */,1.f);
        }context_->check("Exact source Warrior sampler0 objects");}
        catch(...){gl_.DeleteSamplers(6,samplers_.data());throw;}
    }
    GlBnwWarriorMaterialSamplers(const GlBnwWarriorMaterialSamplers&)=delete;
    GlBnwWarriorMaterialSamplers&operator=(const GlBnwWarriorMaterialSamplers&)=delete;
    ~GlBnwWarriorMaterialSamplers(){(void)context_->api();gl_.DeleteSamplers(6,samplers_.data());}
    GLuint sampler(const GlContext &context,unsigned slot,unsigned member,std::int32_t unit,std::uint64_t revision)const {
        if(&context!=context_.get())throw std::invalid_argument("Foreign Warrior sampler GL context");
        (void)policy_.slot(slot,member,unit,revision);return samplers_[slot];
    }
};
} // namespace civ5::graphics
