void BnwSpClientPregameStartup::set_actual_nickname(std::int32_t id,const std::string& native_name){
 current();need(id>=0&&id<64,"SP native nickname setter range absent");
 // SDK2775/native101A42A0 copies the source before stripping display suffix.
 std::string display(native_name);
 if(!native_name.empty()){
  const auto pos=native_name.rfind('@');
  if(pos!=native_name.npos&&pos>0&&pos<64&&native_name.size()-pos<32)
   display.erase(pos,native_name.size()-pos);
 }
 // s_displayNicknames is a plain std::map. Native [] insertion and its string
 // assignment happen unconditionally BEFORE archived nickname equality.
 auto& actual_display=display_nicknames_[id];actual_display=display;increment(display_stores_);current();
 auto& actual_name=actual_namespace_.archive.string_arrays.at("nicknames").at(id);
 if(actual_name==native_name){current();return;}
 const auto dirty=archive_dirty_.insert(Vector::nickname).second;if(dirty)increment(archive_insertions_);
 actual_name=native_name;increment(field_stores_);
 nickname_deltas_.push_back({id,native_name,2});increment(delta_appends_);current();
}
