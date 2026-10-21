 // Source17: native7FE470 fields are retained by the SAME actual inner.
 // Constructor source values below are not post-Read or copied client DTOs.
 struct NativeMetadata final {
  float offset0c=0,offset10=0,offset14=0;
  std::uint32_t parent_id=0,descriptor=0,offset20=0,offset24=0,offset28=0,
   offset30=0,offset2c=0;
  bool offset1c=false,offset1e=false,offset34=false;
  struct Observer {std::uint32_t descriptor=0,value=0;};
  std::unique_ptr<Observer> observer;
  bool returned=false;
 };
 std::unique_ptr<NativeMetadata> native_metadata;
 std::uint32_t info_parent_id=0,info_expected_count=0,info_offset10=0;
 void metadata_constructor(){
  current();need(!native_metadata,"SP native metadata constructor replay");
  native_metadata=std::make_unique<NativeMetadata>();auto& m=*native_metadata;
  // Native7FE473..7FE4AF: keep initial fields and paid rate/enable stores
  // contiguous in their original order; later setters stay at their callers.
  store(m.offset0c,0.0f);store(m.offset10,0.0f);store(m.parent_id,registry_id);
  store(m.offset14,0.0f);store(metadata_rate,0.25f);
  store(m.descriptor,std::uint32_t{0xb74658});store(m.offset1c,false);
  store(metadata_enabled,true);store(m.offset1e,false);store(m.offset20,std::uint32_t{0});
  store(m.offset24,std::uint32_t{0});store(m.offset28,std::uint32_t{0});
  store(m.offset30,std::uint32_t{0});store(m.offset34,false);store(m.offset2c,std::uint32_t{1});
  // Allocation belongs to this partial constructor. Failure must not supply
  // a NULL observer and continue through the original required dereference.
  auto child=std::make_unique<NativeMetadata::Observer>();
  store(child->value,std::uint32_t{0});store(child->descriptor,std::uint32_t{0xb7506c});
  native_metadata->observer=std::move(child);increment(native_stores);
  store(native_metadata->observer->value,std::uint32_t{0});m.returned=true;current();
 }
 struct Control1208 final {
  std::int32_t offset18=0,offset14=0,offset1c=0;
  std::uint32_t parts=0,type=0,offset08=0,size=0,offset20=0,offset24=0,descriptor=0;
  bool offset04=false,offset05=false,offset2c=false;
  std::string text;
  bool constructed=false,submit_entered=false,submit_returned=false,retired=false;
 };
 std::shared_ptr<Control1208> registration_control;
 void construct_registration_control(){
  current();need(!registration_control,"SP genuine registration Control1208 constructor replay");
  registration_control=std::make_shared<Control1208>();auto& m=*registration_control;
  // Actual7E8DF0 implicit ESI constructor, including native equal stores.
  store(m.offset18,std::int32_t{-1});store(m.offset14,std::int32_t{-1});
  store(m.offset1c,std::int32_t{-1});store(m.parts,std::uint32_t{1});store(m.offset04,true);
  // lea ecx,[message+30] selects the SAME global-version copy constructor;
  // the constructor call itself follows the scalar stores below at7E8E30.
  store(m.type,std::uint32_t{1208});store(m.offset05,false);store(m.offset08,std::uint32_t{0});
  store(m.size,std::uint32_t{0});store(m.offset20,std::uint32_t{0});
  store(m.offset24,std::uint32_t{0});store(m.offset2c,false);
  store(m.descriptor,std::uint32_t{0xb72ce4});
  need(process->version_constructor_returned&&process->control_version_revision==0&&
   process->version_profile==Process::VersionProfile::selected_civis_crt_constructor,
   "SP Control1208 original retained version constructor/profile absent or changed");
  //7E8E09 passes actual5CF7EA0;7E8E30 is std::string COPY, not clear.
  m.text.assign(process->control_version);
  m.constructed=true;current();
 }
 void submit_registration_control_prefix(){
  current();need(registration_control&&registration_control->constructed&&
   !registration_control->submit_entered&&!registration_control->retired,
   "SP submit requires SAME returned Control1208 construction, no replay");
  auto& message=*registration_control;message.submit_entered=true;
  try{
   // Native+12C/804EB0 returns the retained inner+1C metadata, then the
   // original repeats that getter before its first irreversible message store.
   need(native_metadata&&native_metadata->returned&&native_metadata->observer,
    "SP actual inner metadata absent before Control1208 submit");current();
   auto& metadata=*native_metadata;
   store(message.offset20,metadata.offset2c+metadata.offset24);
   const auto previous_sequence=metadata.offset30;
   store(metadata.offset30,previous_sequence+std::uint32_t{1});
   store(message.offset24,previous_sequence);
   // Actual80612? tests inner+14 only AFTER the three stores above. A genuine
   // mode0 context exists here: its NULL-context early return is unavailable.
   need(context&&context->mode==0&&context->proxy==this,
    "SP Control1208 requires actual mode0 context after metadata partial writes");current();
   serialize_and_queue_registration_control();current();message.submit_returned=true;
  }catch(...){message.retired=true;retired=true;process->retired=true;throw;}
 }
