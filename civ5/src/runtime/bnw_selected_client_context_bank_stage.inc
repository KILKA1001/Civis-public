// Inside the actual cold constructor, before any VM/native callback.
// Every declared Root child needs actual VFS winner XML/Lua, even when its
// script is excluded from the single-player execution lane (no ready grant).
{
 const auto actual=loaded("Assets/DLC/Expansion2/UI/InGame/InGame.xml");require_current();
 if(!actual.xml)throw std::runtime_error("Selected client Root document was not actually parsed");
 std::set<std::string> identities;
 const auto walk=[&](auto&& self,const content::XmlNode& node)->void{
  if(node.name=="Instance"||node.name=="InstanceData")return;
  if(node.name=="LuaContext"){
   const auto stem=node.attribute("FileName");auto id=node.attribute("ID");
   if(id.empty()){const auto slash=stem.find_last_of("/\\");id=stem.substr(slash==std::string::npos?0:slash+1);}
   if(stem.empty()||id.empty()||!identities.insert(id).second||identities.size()>4096)
    throw std::runtime_error("Selected client Root declarations missing/repeated/exhausted");
   require_current();bank_->load(assets::AssetKey(stem+".xml"),assets::CivisResourceParse::xml);require_current();
   bank_->load(assets::AssetKey(stem+".lua"),assets::CivisResourceParse::bytes);require_current();return;
  }
  for(const auto& child:node.children)self(self,child);
 };
 walk(walk,*actual.xml);require_current();
 std::set<std::string> documents;std::vector<std::string> active;
 const auto stage=[&](auto&& self,const std::string& stem)->void{
  const auto key=assets::normalize_asset_name(stem+".xml");
  if(std::find(active.begin(),active.end(),key)!=active.end())throw std::runtime_error("Selected nested WorldView source cycle");
  if(!documents.insert(key).second)return;
  if(documents.size()>4096||active.size()>=128)throw std::runtime_error("Selected nested source closure exhausted");
  active.push_back(key);require_current();
  bank_->load(assets::AssetKey(stem+".xml"),assets::CivisResourceParse::xml);require_current();
  bank_->load(assets::AssetKey(stem+".lua"),assets::CivisResourceParse::bytes);require_current();
  const auto doc=loaded(stem+".xml");require_current();
  if(!doc.xml||doc.xml->name!="Context")throw std::runtime_error("Selected nested WorldView has no actual Context document");
  const auto descend=[&](auto&& visit,const content::XmlNode& node)->void{
   if(node.name=="Instance"||node.name=="InstanceData")return;
   if(node.name=="LuaContext"){
    const auto child=node.attribute("FileName");if(child.empty())throw std::runtime_error("Selected nested Context source missing");
    self(self,child);return;
   }
   for(const auto& child:node.children)visit(visit,child);
  };
  descend(descend,*doc.xml);require_current();active.pop_back();
 };
 stage(stage,"Assets/UI/InGame/WorldView/WorldView");require_current();
}
