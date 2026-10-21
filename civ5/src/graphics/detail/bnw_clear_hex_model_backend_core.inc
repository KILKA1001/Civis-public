// Same actual UiScreenGlRenderer TU, after complete Highlight State. This is
// OWN decoded CPU construction from genuine SDK exports, not x86 object ABI.
#include "bnw_hex_model_sdk_json.hpp"
#include <fstream>
#include <set>
namespace civ5::graphics::detail {
struct BnwClearHexModelBackend::State final {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},failed{false};bool clearing=false;
 const std::weak_ptr<BnwClearHexHighlightOwner> highlight;
 const std::weak_ptr<const assets::CivisResourceBankOwner> bank;
 const std::size_t index;
 std::vector<assets::CivisLoadedResource> inputs;
 assets::CivisResourceBankSourceRecipe recipe;
 std::string authored_animation_alias;std::uint32_t authored_animation_event_code=0;
 std::array<std::shared_ptr<const assets::Bytes>,4> exports;
 struct Element final {std::string name;std::uint32_t type,count,offset,length;};
 struct Mesh final {
  std::string name,material_name;std::vector<float> vertices;
  std::vector<std::uint32_t> triangles;std::vector<std::uint8_t> raw;
  std::uint32_t stride;std::vector<Element> declaration;
  std::vector<std::string> bone_names;std::vector<std::size_t> bone_indices;
 };
 struct Bone final {
  std::string name;std::int32_t parent;float lod_error;std::uint32_t flags;
  std::vector<float> position,orientation,scale_shear,inverse_world;
 };
 struct Curve final {std::string type;std::uint32_t degree,dimension,knot_count;bool identity,constant,orientation_normalization_requested;std::vector<float> start;};
 struct Track final {std::string name;std::size_t bone;std::uint32_t flags;std::array<Curve,3> curves;};
 struct Clip final {std::string name,authored_alias;std::uint32_t authored_event_code;float duration,time_step,oversampling;std::vector<std::int32_t> events;std::vector<Track> tracks;};
 struct Model final {std::string name;std::uint32_t placement_flags;std::vector<float> placement_position,placement_orientation,placement_scale_shear;std::vector<Mesh> meshes;std::vector<Bone> bones;std::vector<Clip> clips;};
 std::shared_ptr<const Model> model;
 struct Pose final {std::uint32_t flags;std::vector<float> position,orientation,scale_shear,inverse_world;};
 struct Draw final {std::size_t mesh;std::vector<std::size_t> bone_bindings;};
 struct Runtime final {std::uint32_t filter=0xffffffff;double entering_clock=0;std::vector<std::size_t> clips;std::vector<std::vector<std::size_t>> track_bones;};
 struct Instance final {
  std::shared_ptr<const Model> model;std::uint32_t copy_flags=0x1c;
  std::uint32_t placement_flags;std::vector<float> placement_position,placement_orientation,placement_scale_shear;
  std::vector<Pose> poses;std::vector<Draw> draws;Runtime runtime;bool active=false;
 };
 std::array<std::unique_ptr<Instance>,8> instances;
 State(std::weak_ptr<BnwClearHexHighlightOwner> h,std::weak_ptr<const assets::CivisResourceBankOwner> b,std::size_t i):highlight(std::move(h)),bank(std::move(b)),index(i){}
 void first()const{if(creator!=std::this_thread::get_id()||!alive.load()||failed.load()||highlight.expired()||bank.expired())throw std::runtime_error("HexModel real model storage/owning Source receiver retired or foreign");}
 static std::uint32_t u32(const hex_sdk::Value& v){const auto x=v.integer();if(x<0||x>UINT32_MAX)hex_sdk::bad();return static_cast<std::uint32_t>(x);}
 static float finite(const hex_sdk::Value& v){const auto x=v.real();if(std::abs(x)>std::numeric_limits<float>::max())hex_sdk::bad();return static_cast<float>(x);}
 static Curve curve(const hex_sdk::Value& v,std::uint32_t dimension){
  // A missing curve has no invented identity sample. Initial local pose comes
  // from the actual skeleton; the null container remains preserved in exports.
  if(v.kind==hex_sdk::Value::Kind::null_value)return {"",0,0,0,false,false,false,{}};
  Curve c{v.at("type").text(),u32(v.at("degree")),u32(v.at("dimension")),u32(v.at("knotCount")),v.at("isIdentity").flag(),v.at("isConstant").flag(),v.at("orientationNormalizeRequested").flag(),{}};
  if(c.type.empty()||c.dimension!=dimension||c.orientation_normalization_requested!=(dimension==4))hex_sdk::bad();c.start=hex_sdk::floats(v.at("startSample"),dimension);return c;
 }
 // Pure allocation/assembly. The actual caller performs full PRE/POST around
 // this finite reader scope and after its parser/temporary targets retire.
 static std::shared_ptr<const Model> assemble(const std::array<std::shared_ptr<const assets::Bytes>,4>& blobs,const runtime::FileIdentity& mesh_identity,const std::string& authored_alias,std::uint32_t authored_ec){
  if(authored_alias!="SelectedAniOneTurn"||authored_ec!=1000)hex_sdk::bad();
  const auto read=[](const auto& b){if(!b)hex_sdk::bad();return hex_sdk::Reader(std::string_view(reinterpret_cast<const char*>(b->data()),b->size())).read();};
  const auto geometry=read(blobs[0]),metadata=read(blobs[1]),skeleton=read(blobs[2]),animation=read(blobs[3]);
  if(metadata.at("sourceGr2Sha256").text()!=mesh_identity.sha256||metadata.at("geometryCacheSha256").text()!=runtime::identify_bytes(*blobs[0]).sha256)hex_sdk::bad();
  if(metadata.at("source").text()!="SelectedAniOneTurn"||skeleton.at("source").text()!="SelectedAniOneTurn"||animation.at("source").text()!="SelectedAniOneTurn_SelectedAniOneTurn")hex_sdk::bad();
  const auto& meshes=geometry.at("meshes").elements();const auto& descriptions=metadata.at("meshes").elements();
  const auto& skeletons=skeleton.at("skeletons").elements();
  if(meshes.empty()||meshes.size()>256||meshes.size()!=descriptions.size()||skeletons.size()!=1)hex_sdk::bad();
  // Selected minimal one-model assembly. Actual SDK model association is
  // mandatory; no synthetic G3DJ root transform or global-mesh merge.
  const auto& record=skeletons[0];const auto model_ordinal=u32(record.at("modelOrdinal"));
  if(model_ordinal!=0)hex_sdk::bad();
  auto model=std::make_shared<Model>();model->name=record.at("modelName").text();
  const auto& placement=record.at("initialPlacement");model->placement_flags=u32(placement.at("flags"));
  model->placement_position=hex_sdk::floats(placement.at("position"),3);
  model->placement_orientation=hex_sdk::floats(placement.at("orientation"),4);
  model->placement_scale_shear=hex_sdk::floats(placement.at("scaleShear"),9);
  const auto& association=record.at("meshOrdinals").elements();if(association.size()!=meshes.size())hex_sdk::bad();
  std::set<std::uint32_t> associated;
  for(std::size_t i=0;i<association.size();++i){const auto ordinal=u32(association[i]);if(ordinal>=meshes.size()||!associated.insert(ordinal).second||u32(descriptions[ordinal].at("sourceModelOrdinal"))!=model_ordinal||u32(descriptions[ordinal].at("sourceBindingOrdinal"))!=i)hex_sdk::bad();}
  const auto& bones=record.at("bones").elements();if(bones.empty()||bones.size()>65535)hex_sdk::bad();
  std::map<std::string,std::size_t> bone_names;
  for(std::size_t i=0;i<bones.size();++i){const auto& b=bones[i];const auto& t=b.at("localTransform");const auto parent=b.at("parentIndex").integer();
   if(parent < -1||parent>=static_cast<std::int64_t>(bones.size())||parent==static_cast<std::int64_t>(i))hex_sdk::bad();
   Bone bone{b.at("name").text(),static_cast<std::int32_t>(parent),finite(b.at("lodError")),u32(t.at("flags")),hex_sdk::floats(t.at("position"),3),hex_sdk::floats(t.at("orientation"),4),hex_sdk::floats(t.at("scaleShear"),9),hex_sdk::floats(b.at("inverseWorldTransform"),16)};
   if(bone.name.empty()||!bone_names.emplace(bone.name,i).second)throw std::runtime_error("HexModel SDK bone names unresolved/ambiguous; no guessed index/name binding");
   model->bones.push_back(std::move(bone));
  }
  for(std::size_t i=0;i<bones.size();++i){auto p=model->bones[i].parent;for(std::size_t hops=0;p>=0;++hops){if(hops>=bones.size())hex_sdk::bad();p=model->bones[static_cast<std::size_t>(p)].parent;}}
  for(const auto& reference:association){const auto i=u32(reference);const auto& g=meshes[i];const auto& d=descriptions[i];const auto& attributes=g.at("attributes").elements();
   if(attributes.size()!=3||attributes[0].text()!="POSITION"||attributes[1].text()!="NORMAL"||attributes[2].text()!="TEXCOORD0")hex_sdk::bad();
   const auto count=u32(d.at("vertexCount")),stride=u32(d.at("vertexStride"));if(!count||count>1000000||!stride||stride>1024)hex_sdk::bad();
   Mesh mesh;mesh.name=d.at("name").text();mesh.material_name=d.at("materialName").text();mesh.stride=stride;
   mesh.vertices=hex_sdk::floats(g.at("vertices"),std::size_t(count)*8);mesh.raw=hex_sdk::base64(d.at("rawVertexBytesBase64").text());
   if(mesh.raw.size()!=std::size_t(count)*stride||runtime::identify_bytes(mesh.raw).sha256!=d.at("rawVertexBytesSha256").text())hex_sdk::bad();
   const auto& parts=g.at("parts").elements();if(parts.empty())hex_sdk::bad();
   for(const auto& part:parts){if(part.at("type").text()!="TRIANGLES")hex_sdk::bad();const auto& indices=part.at("indices").elements();if(indices.empty()||indices.size()%3)hex_sdk::bad();for(const auto& x:indices){const auto n=u32(x);if(n>=count)hex_sdk::bad();mesh.triangles.push_back(n);}}
   if(mesh.triangles.size()!=u32(d.at("triangleIndexCount")))hex_sdk::bad();
   for(const auto& v:d.at("vertexElements").elements()){Element e{v.at("name").text(),u32(v.at("type")),u32(v.at("count")),u32(v.at("offset")),u32(v.at("length"))};if(!e.count||!e.length||std::uint64_t(e.offset)+std::uint64_t(e.count)*e.length>stride)hex_sdk::bad();mesh.declaration.push_back(std::move(e));}
   if(mesh.declaration.empty())hex_sdk::bad();
   for(const auto& n:d.at("boneBindings").elements()){const auto& name=n.text();const auto b=bone_names.find(name);if(name.empty()||b==bone_names.end())throw std::runtime_error("HexModel mesh bone binding has no genuine named SDK skeleton target");mesh.bone_names.push_back(name);mesh.bone_indices.push_back(b->second);}
   model->meshes.push_back(std::move(mesh));
  }
  const auto& clips=animation.at("animations").elements();if(clips.empty()||clips.size()>256)hex_sdk::bad();
  for(const auto& a:clips){Clip clip{a.at("name").text(),authored_alias,authored_ec,finite(a.at("duration")),finite(a.at("timeStep")),finite(a.at("oversampling")),{}, {}};
   if(clip.name.empty()||clip.duration<0||clip.time_step<0||clip.oversampling<0)hex_sdk::bad();
   for(const auto& code:a.at("eventCodes").elements()){const auto n=code.integer();if(n<INT32_MIN||n>INT32_MAX)hex_sdk::bad();clip.events.push_back(static_cast<std::int32_t>(n));}
   std::set<std::size_t> bound;
   for(const auto& group:a.at("trackGroups").elements())for(const auto& t:group.at("transformTracks").elements()){
    const auto& name=t.at("name").text();const auto b=bone_names.find(name);if(name.empty()||b==bone_names.end()||!bound.insert(b->second).second)throw std::runtime_error("HexModel animation track has no unique genuine SDK skeleton target");
    clip.tracks.push_back({name,b->second,u32(t.at("flags")),{curve(t.at("positionCurve"),3),curve(t.at("orientationCurve"),4),curve(t.at("scaleShearCurve"),9)}});
   }
   if(clip.tracks.empty())hex_sdk::bad();model->clips.push_back(std::move(clip));
  }
  return model;
 }
 void construct_instance(std::size_t ordinal,double clock){first();if(!model||ordinal>=instances.size()||instances[ordinal])hex_sdk::bad();auto instance=std::make_unique<Instance>();instance->model=model;instance->runtime.entering_clock=clock;
  instance->placement_flags=model->placement_flags;instance->placement_position=model->placement_position;instance->placement_orientation=model->placement_orientation;instance->placement_scale_shear=model->placement_scale_shear;
  for(const auto& bone:model->bones)instance->poses.push_back({bone.flags,bone.position,bone.orientation,bone.scale_shear,bone.inverse_world});
  for(std::size_t i=0;i<model->meshes.size();++i)instance->draws.push_back({i,model->meshes[i].bone_indices});
  for(std::size_t i=0;i<model->clips.size();++i){instance->runtime.clips.push_back(i);std::vector<std::size_t> bindings;for(const auto& track:model->clips[i].tracks)bindings.push_back(track.bone);instance->runtime.track_bones.push_back(std::move(bindings));}
  // Actual initial skeleton pose is owned per instance. Curve containers and
  // their real entering samples remain in the model; no pretend full playback.
  if(instance->poses.empty()||instance->draws.empty()||instance->runtime.clips.empty())hex_sdk::bad();first();instances[ordinal]=std::move(instance);first();
 }
};
BnwClearHexModelBackend::BnwClearHexModelBackend(std::shared_ptr<State> s):state_(std::move(s)){if(!state_)hex_sdk::bad();state_->first();}
BnwClearHexModelBackend::~BnwClearHexModelBackend(){if(creator_!=std::this_thread::get_id())std::terminate();state_->alive.store(false);}
const SelectedHexModelSdkPins& BnwClearHexModelBackend::root_pins_owned(){return SelectedHexModelSdkPins::completed_root_export_owned();}
void BnwClearHexModelBackend::clear_instances_owned(const BnwClearHexHighlightOwner& actual,const runtime::BnwWorldViewSourceOperationLoan& op){
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("HexModel clear foreign creator");
 actual.first_owned();actual.require_current_without_callbacks_owned(op);const auto s=state_;s->first();
 const auto same=[](const auto&a,const auto&b){return !a.owner_before(b)&&!b.owner_before(a);};
 const auto current=[&]{s->first();actual.require_current_without_callbacks_owned(op);s->first();if(!same(s->highlight,actual.state_->self)||s->index>=actual.state_->models.size()||actual.state_->models[s->index].backend.get()!=this||!same(s->bank,actual.state_->bank))throw std::runtime_error("HexModel clear lost SAME actual style/Highlight/Bank instance receiver");};
 current();if(s->clearing){s->failed.store(true);throw std::runtime_error("HexModel clear reentry quarantined");}s->clearing=true;
 try{actual.require_current_owned(op);current();for(auto& instance:s->instances){current();if(!instance||instance->model!=s->model||instance->copy_flags!=0x1c||instance->runtime.filter!=0xffffffff)hex_sdk::bad();instance->active=false;current();}actual.require_current_owned(op);current();s->clearing=false;}
 catch(...){s->failed.store(true);s->clearing=false;throw;}
}
}
