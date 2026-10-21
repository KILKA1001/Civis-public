// Inside civ5::ui, AFTER genuine Borrower + Root Paint DATA definitions.
// The variant holds only closed lexical Guards; it issues no new authority.
struct detail::BnwLabelSystemFontPreparedRun::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 const std::shared_ptr<const detail::BnwLabelSystemFontBorrower::Impl> borrower;
 using Input=std::variant<detail::BnwLabelCurrentTextLoan::Guard,detail::BnwLabelPaintTextLoan::Guard>;
 const Input text;
 const std::u16string converted_units;
 const detail::BnwSystemFontNativeRunStage stage;
 const std::uint64_t serial,preparation;
 Impl(std::shared_ptr<const detail::BnwLabelSystemFontBorrower::Impl> b,
      Input input,std::u16string units,
      detail::BnwSystemFontNativeRunStage actual,std::uint64_t operation,std::uint64_t sequence)
  :borrower(std::move(b)),text(std::move(input)),converted_units(std::move(units)),
   stage(std::move(actual)),serial(operation),preparation(sequence){}
 void text_first_without_callbacks()const{
  std::visit([](const auto& input){input.require_current_without_callbacks();},text);
 }
 void text_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
  std::visit([&](const auto& input){input.require_source_owned(operation);},text);
 }
 void first()const{
  require(creator==std::this_thread::get_id(),"System font prepared run foreign creator");
  text_first_without_callbacks();(void)borrower->first();
  const auto entry=borrower->entry.lock();const auto cpu=borrower->cpu.lock();
  require(entry&&cpu&&entry->preparation_sequence==preparation,
   "System font prepared run superseded by actual repeated PrepareText");
  cpu->require_native_gcp_current_without_callbacks(stage.gcp);
  text_first_without_callbacks();(void)borrower->first();
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
  first();text_source_owned(operation);first();
  operation.require_operation(serial);first();
  borrower->full(operation,[&]{first();});first();
  text_source_owned(operation);first();operation.require_operation(serial);first();
 }
};
detail::BnwLabelSystemFontPreparedRun::BnwLabelSystemFontPreparedRun(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
detail::BnwLabelSystemFontPreparedRun::~BnwLabelSystemFontPreparedRun()=default;
void detail::BnwLabelSystemFontPreparedRun::require_current_without_callbacks()const{
 const auto saved=impl_;try{saved->first();}catch(...){saved->borrower->entry_life->failed.store(true);throw;}
}
void detail::BnwLabelSystemFontPreparedRun::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 const auto saved=impl_;try{saved->full(operation);saved->first();}
 catch(...){saved->borrower->entry_life->failed.store(true);throw;}
}
std::shared_ptr<const detail::BnwLabelSystemFontPreparedRun>
detail::BnwLabelSystemFontBorrower::prepare_label_text_run_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwLabelCurrentTextLoan& input)const{
 // Snapshot immutable physical input and weak canonical metadata before
 // callbacks or allocation. Never read raw input/this after this cut.
 const auto saved=impl_;const auto text=input.capture_guard_without_callbacks();
 text.require_system_font_borrower_without_callbacks(*this);
 const auto first=[&]{text.require_current_without_callbacks();(void)saved->first();};
 const auto full=[&]{first();text.require_source_owned(operation);first();
  auto current=saved->first();current.source->require_current_owned();first();
  current.atlas->require_cached_source_owned(operation,*current.source);first();
  text.require_source_owned(operation);first();};
 std::shared_ptr<const detail::BnwLabelSystemFontPreparedRun> result;
 try{
  {
   const Impl::TextOperation active(saved);first();full();first();
   const auto serial=operation.operation_serial();first();
   const auto entry=saved->entry.lock();first();
   require(entry&&entry->preparation_sequence<UINT64_MAX,"System font PrepareText sequence unavailable");
   const auto preparation=entry->preparation_sequence+1;
   // Native83B780 reads the real byte at the inclusive prefix end. The
   // retained immutable C4 Guard supplies the whole VALUE and true NUL.
   const auto utf8=text.actual_c4_bytes_without_callbacks();first();
   auto conversion=detail::convert_label_native_utf8_data(utf8);first();full();first();
   std::optional<detail::BnwSystemFontNativeRunStage> actual;
   {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired before PrepareText");
    actual.emplace(cache->prepare_native_gcp_owned(conversion.units));first();
   }
   first();full();first();
   auto prepared_input=detail::BnwLabelSystemFontPreparedRun::Impl::Input{text};first();
   auto data=std::make_shared<const detail::BnwLabelSystemFontPreparedRun::Impl>(
    saved,std::move(prepared_input),std::move(conversion.units),std::move(*actual),serial,preparation);first();full();first();
   result=std::shared_ptr<const detail::BnwLabelSystemFontPreparedRun>(
    new detail::BnwLabelSystemFontPreparedRun(std::move(data)));first();full();first();
   entry->preparation_sequence=preparation;first();
  }
  // All conversion/CPU/result-construction temporaries have unwound.
  first();full();first();result->require_source_owned(operation);first();full();first();
  result->require_current_without_callbacks();first();return result;
 }catch(...){saved->entry_life->failed.store(true);throw;}
}
bool detail::BnwLabelSystemFontBorrower::has_glyph_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwLabelCurrentTextLoan& input,
 std::uint32_t codepoint)const{
 const auto saved=impl_;const auto text=input.capture_guard_without_callbacks();
 text.require_system_font_borrower_without_callbacks(*this);
 const auto first=[&]{text.require_current_without_callbacks();(void)saved->first();};
 try{
  {
  const Impl::TextOperation active(saved);
  first();text.require_source_owned(operation);first();
  {auto current=saved->first();current.source->require_current_owned();first();
   current.atlas->require_cached_source_owned(operation,*current.source);first();}
  first();text.require_source_owned(operation);first();
  // Actual753CF0: unconditional TRUE. No GDI probe or missing-to-SPACE.
  (void)codepoint;
  }
  first();text.require_source_owned(operation);first();return true;
 }catch(...){saved->entry_life->failed.store(true);throw;}
}
std::array<std::uint32_t,9> detail::BnwLabelSystemFontBorrower::glyph_bits_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwLabelCurrentTextLoan& input,
 std::uint32_t codepoint)const{
 const auto saved=impl_;const auto text=input.capture_guard_without_callbacks();
 text.require_system_font_borrower_without_callbacks(*this);
 const auto first=[&]{text.require_current_without_callbacks();(void)saved->first();};
 const auto full=[&]{first();text.require_source_owned(operation);first();
  auto current=saved->first();current.source->require_current_owned();first();
  current.atlas->require_cached_source_owned(operation,*current.source);first();
  text.require_source_owned(operation);first();};
 std::array<std::uint32_t,9> result{};
 detail::BnwSystemFontNativeGlyphData glyph;
 try{
  {
   const Impl::TextOperation active(saved);first();full();first();
   {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired before GetGlyph");
    glyph=cache->get_native_glyph_owned(codepoint);first();
   }
   first();full();first();
   {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired after GetGlyph");
    cache->require_native_glyph_current_without_callbacks(glyph);first();result=glyph.words;}
  }
  first();full();first();
  {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired at final GetGlyph POST");
   cache->require_native_glyph_current_without_callbacks(glyph);first();}
  first();return result;
 }catch(...){saved->entry_life->failed.store(true);throw;}
}

// Closed actual591020 Paint overloads; repeated Prepare supersedes old metadata.
std::shared_ptr<const detail::BnwLabelSystemFontPreparedRun>
detail::BnwLabelSystemFontBorrower::prepare_label_text_run_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwLabelPaintTextLoan& input)const{
 // Capture the genuine Paint Guard and SAME actual170 identity BEFORE
 // callbacks/allocation. Never reread the Paint facade or raw this later.
 const auto saved=impl_;const auto text=input.capture_guard_without_callbacks();
 text.require_system_font_borrower_without_callbacks(*this);
 const auto first=[&]{text.require_current_without_callbacks();(void)saved->first();};
 const auto full=[&]{first();text.require_source_owned(operation);first();
  auto current=saved->first();current.source->require_current_owned();first();
  current.atlas->require_cached_source_owned(operation,*current.source);first();
  text.require_source_owned(operation);first();};
 std::shared_ptr<const detail::BnwLabelSystemFontPreparedRun> result;
 try{
  {
   const Impl::TextOperation active(saved);first();full();first();
   const auto serial=operation.operation_serial();first();
   const auto entry=saved->entry.lock();first();
   require(entry&&entry->preparation_sequence<UINT64_MAX,"System font PrepareText sequence unavailable");
   const auto preparation=entry->preparation_sequence+1;
   // Native83B780 reads the real byte at the inclusive prefix end. The
   // retained immutable C4 Guard supplies the whole VALUE and true NUL.
   const auto utf8=text.actual_c4_bytes_without_callbacks();first();
   auto conversion=detail::convert_label_native_utf8_data(utf8);first();full();first();
   std::optional<detail::BnwSystemFontNativeRunStage> actual;
   {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired before PrepareText");
    actual.emplace(cache->prepare_native_gcp_owned(conversion.units));first();
   }
   first();full();first();
   auto prepared_input=detail::BnwLabelSystemFontPreparedRun::Impl::Input{text};first();
   auto data=std::make_shared<const detail::BnwLabelSystemFontPreparedRun::Impl>(
    saved,std::move(prepared_input),std::move(conversion.units),std::move(*actual),serial,preparation);first();full();first();
   result=std::shared_ptr<const detail::BnwLabelSystemFontPreparedRun>(
    new detail::BnwLabelSystemFontPreparedRun(std::move(data)));first();full();first();
   entry->preparation_sequence=preparation;first();
  }
  // All conversion/CPU/result-construction temporaries have unwound.
  first();full();first();result->require_source_owned(operation);first();full();first();
  result->require_current_without_callbacks();first();return result;
 }catch(...){saved->entry_life->failed.store(true);throw;}
}
bool detail::BnwLabelSystemFontBorrower::has_glyph_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwLabelPaintTextLoan& input,
 std::uint32_t codepoint)const{
 const auto saved=impl_;const auto text=input.capture_guard_without_callbacks();
 text.require_system_font_borrower_without_callbacks(*this);
 const auto first=[&]{text.require_current_without_callbacks();(void)saved->first();};
 try{
  {
  const Impl::TextOperation active(saved);
  first();text.require_source_owned(operation);first();
  {auto current=saved->first();current.source->require_current_owned();first();
   current.atlas->require_cached_source_owned(operation,*current.source);first();}
  first();text.require_source_owned(operation);first();
  // Actual753CF0: unconditional TRUE. No GDI probe or missing-to-SPACE.
  (void)codepoint;
  }
  first();text.require_source_owned(operation);first();return true;
 }catch(...){saved->entry_life->failed.store(true);throw;}
}
std::array<std::uint32_t,9> detail::BnwLabelSystemFontBorrower::glyph_bits_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwLabelPaintTextLoan& input,
 std::uint32_t codepoint)const{
 const auto saved=impl_;const auto text=input.capture_guard_without_callbacks();
 text.require_system_font_borrower_without_callbacks(*this);
 const auto first=[&]{text.require_current_without_callbacks();(void)saved->first();};
 const auto full=[&]{first();text.require_source_owned(operation);first();
  auto current=saved->first();current.source->require_current_owned();first();
  current.atlas->require_cached_source_owned(operation,*current.source);first();
  text.require_source_owned(operation);first();};
 std::array<std::uint32_t,9> result{};
 detail::BnwSystemFontNativeGlyphData glyph;
 try{
  {
   const Impl::TextOperation active(saved);first();full();first();
   {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired before GetGlyph");
    glyph=cache->get_native_glyph_owned(codepoint);first();
   }
   first();full();first();
   {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired after GetGlyph");
    cache->require_native_glyph_current_without_callbacks(glyph);first();result=glyph.words;}
  }
  first();full();first();
  {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired at final GetGlyph POST");
   cache->require_native_glyph_current_without_callbacks(glyph);first();}
  first();return result;
 }catch(...){saved->entry_life->failed.store(true);throw;}
}

// Captured-Guard successors. Existing facade/Init bodies stay byte-exact.
std::shared_ptr<const detail::BnwLabelSystemFontPreparedRun>
detail::BnwLabelSystemFontBorrower::prepare_label_text_run_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwLabelPaintTextLoan::Guard& input)const{
 // Snapshot the already sealed genuine Paint Guard and SAME actual170 BEFORE
 // callbacks/allocation. Never reread the borrowed Guard reference or raw this later.
 const auto saved=impl_;const auto text=input; // VALUE snapshot before callbacks/allocation.
 text.require_system_font_borrower_without_callbacks(*this);
 const auto first=[&]{text.require_current_without_callbacks();(void)saved->first();};
 const auto full=[&]{first();text.require_source_owned(operation);first();
  auto current=saved->first();current.source->require_current_owned();first();
  current.atlas->require_cached_source_owned(operation,*current.source);first();
  text.require_source_owned(operation);first();};
 std::shared_ptr<const detail::BnwLabelSystemFontPreparedRun> result;
 try{
  {
   const Impl::TextOperation active(saved);first();full();first();
   const auto serial=operation.operation_serial();first();
   const auto entry=saved->entry.lock();first();
   require(entry&&entry->preparation_sequence<UINT64_MAX,"System font PrepareText sequence unavailable");
   const auto preparation=entry->preparation_sequence+1;
   // Native83B780 reads the real byte at the inclusive prefix end. The
   // retained immutable C4 Guard supplies the whole VALUE and true NUL.
   const auto utf8=text.actual_c4_bytes_without_callbacks();first();
   auto conversion=detail::convert_label_native_utf8_data(utf8);first();full();first();
   std::optional<detail::BnwSystemFontNativeRunStage> actual;
   {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired before PrepareText");
    actual.emplace(cache->prepare_native_gcp_owned(conversion.units));first();
   }
   first();full();first();
   auto prepared_input=detail::BnwLabelSystemFontPreparedRun::Impl::Input{text};first();
   auto data=std::make_shared<const detail::BnwLabelSystemFontPreparedRun::Impl>(
    saved,std::move(prepared_input),std::move(conversion.units),std::move(*actual),serial,preparation);first();full();first();
   result=std::shared_ptr<const detail::BnwLabelSystemFontPreparedRun>(
    new detail::BnwLabelSystemFontPreparedRun(std::move(data)));first();full();first();
   entry->preparation_sequence=preparation;first();
  }
  // All conversion/CPU/result-construction temporaries have unwound.
  first();full();first();result->require_source_owned(operation);first();full();first();
  result->require_current_without_callbacks();first();return result;
 }catch(...){saved->entry_life->failed.store(true);throw;}
}
// Closed bottom-fenced native410900 successor.
std::shared_ptr<const detail::BnwLabelSystemFontPreparedRun>
detail::BnwLabelSystemFontBorrower::prepare_label_text_run_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwLabelPaintTextLoan::Guard& input,
 const graphics::detail::BnwOwnedFontPaintEffectFence& fence)const{
 // Snapshot the already sealed genuine Paint Guard and SAME actual170 BEFORE
 // callbacks/allocation. Never reread the borrowed Guard reference or raw this later.
 const auto saved=impl_;const auto text=input; // VALUE snapshot before callbacks/allocation.
 text.require_system_font_borrower_without_callbacks(*this);
 // Both are already sealed VALUE Guards, compared before callbacks.
 fence.require_same_paint_guard_without_callbacks(text);
 fence.require_current_without_callbacks();
 const auto first=[&]{fence.require_current_without_callbacks();
  text.require_current_without_callbacks();(void)saved->first();
  fence.require_current_without_callbacks();};
 const auto full=[&]{first();text.require_source_owned(operation);first();
  auto current=saved->first();current.source->require_current_owned();first();
  current.atlas->require_cached_source_owned(operation,*current.source);first();
  text.require_source_owned(operation);first();};
 std::shared_ptr<const detail::BnwLabelSystemFontPreparedRun> result;
 try{
  {
   const Impl::TextOperation active(saved);first();full();first();
   const auto serial=operation.operation_serial();first();
   const auto entry=saved->entry.lock();first();
   require(entry&&entry->preparation_sequence<UINT64_MAX,"System font PrepareText sequence unavailable");
   const auto preparation=entry->preparation_sequence+1;
   // Native83B780 reads the real byte at the inclusive prefix end. The
   // retained immutable C4 Guard supplies the whole VALUE and true NUL.
   const auto utf8=text.actual_c4_bytes_without_callbacks();first();
   auto conversion=detail::convert_label_native_utf8_data(utf8);first();full();first();
   std::optional<detail::BnwSystemFontNativeRunStage> actual;
   {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired before PrepareText");
    actual.emplace(cache->prepare_native_gcp_owned(conversion.units,fence));first();
   }
   first();full();first();
   auto prepared_input=detail::BnwLabelSystemFontPreparedRun::Impl::Input{text};first();
   auto data=std::make_shared<const detail::BnwLabelSystemFontPreparedRun::Impl>(
    saved,std::move(prepared_input),std::move(conversion.units),std::move(*actual),serial,preparation);first();full();first();
   // Keep a genuine FIRST between facade and shared-control allocations.
   auto provisional=std::unique_ptr<detail::BnwLabelSystemFontPreparedRun>(
    new detail::BnwLabelSystemFontPreparedRun(std::move(data)));first();full();first();
   result=std::shared_ptr<const detail::BnwLabelSystemFontPreparedRun>(std::move(provisional));
   first();full();first();
   entry->preparation_sequence=preparation;first();
  }
  // All conversion/CPU/result-construction temporaries have unwound.
  first();full();first();result->require_source_owned(operation);first();full();first();
  result->require_current_without_callbacks();first();return result;
 }catch(...){saved->entry_life->failed.store(true);throw;}
}
bool detail::BnwLabelSystemFontBorrower::has_glyph_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwLabelPaintTextLoan::Guard& input,
 std::uint32_t codepoint)const{
 const auto saved=impl_;const auto text=input; // VALUE snapshot before callbacks/allocation.
 text.require_system_font_borrower_without_callbacks(*this);
 const auto first=[&]{text.require_current_without_callbacks();(void)saved->first();};
 try{
  {
  const Impl::TextOperation active(saved);
  first();text.require_source_owned(operation);first();
  {auto current=saved->first();current.source->require_current_owned();first();
   current.atlas->require_cached_source_owned(operation,*current.source);first();}
  first();text.require_source_owned(operation);first();
  // Actual753CF0: unconditional TRUE. No GDI probe or missing-to-SPACE.
  (void)codepoint;
  }
  first();text.require_source_owned(operation);first();return true;
 }catch(...){saved->entry_life->failed.store(true);throw;}
}
std::array<std::uint32_t,9> detail::BnwLabelSystemFontBorrower::glyph_bits_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwLabelPaintTextLoan::Guard& input,
 std::uint32_t codepoint)const{
 const auto saved=impl_;const auto text=input; // VALUE snapshot before callbacks/allocation.
 text.require_system_font_borrower_without_callbacks(*this);
 const auto first=[&]{text.require_current_without_callbacks();(void)saved->first();};
 const auto full=[&]{first();text.require_source_owned(operation);first();
  auto current=saved->first();current.source->require_current_owned();first();
  current.atlas->require_cached_source_owned(operation,*current.source);first();
  text.require_source_owned(operation);first();};
 std::array<std::uint32_t,9> result{};
 detail::BnwSystemFontNativeGlyphData glyph;
 try{
  {
   const Impl::TextOperation active(saved);first();full();first();
   {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired before GetGlyph");
    glyph=cache->get_native_glyph_owned(codepoint);first();
   }
   first();full();first();
   {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired after GetGlyph");
    cache->require_native_glyph_current_without_callbacks(glyph);first();result=glyph.words;}
  }
  first();full();first();
  {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired at final GetGlyph POST");
   cache->require_native_glyph_current_without_callbacks(glyph);first();}
  first();return result;
 }catch(...){saved->entry_life->failed.store(true);throw;}
}

// Closed native GetGlyph successor; no Prepare or upload prerequisite.
detail::BnwSystemFontNativeGlyphData detail::BnwLabelSystemFontBorrower::glyph_data_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwLabelPaintTextLoan::Guard& input,
 std::uint32_t codepoint,const graphics::detail::BnwOwnedFontPaintEffectFence& fence)const{
 const auto saved=impl_;const auto text=input; // VALUE snapshot before callbacks/allocation.
 text.require_system_font_borrower_without_callbacks(*this);
 fence.require_same_paint_guard_without_callbacks(text);
 fence.require_current_without_callbacks();
 const auto first=[&]{fence.require_current_without_callbacks();
  text.require_current_without_callbacks();(void)saved->first();
  fence.require_current_without_callbacks();};
 const auto full=[&]{first();text.require_source_owned(operation);first();
  auto current=saved->first();current.source->require_current_owned();first();
  current.atlas->require_cached_source_owned(operation,*current.source);first();
  text.require_source_owned(operation);first();};
 detail::BnwSystemFontNativeGlyphData glyph;
 try{
  {
   const Impl::TextOperation active(saved);first();full();first();
   {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired before GetGlyph");
    glyph=cache->get_native_glyph_owned(codepoint,fence);first();
   }
   first();full();first();
   {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired after GetGlyph");
    cache->require_native_glyph_current_without_callbacks(glyph);first();}
  }
  first();full();first();
  {auto cache=saved->cache.lock();first();require(bool(cache),"System font actual ring retired at final GetGlyph POST");
   cache->require_native_glyph_current_without_callbacks(glyph);first();}
  first();return glyph;
 }catch(...){saved->entry_life->failed.store(true);throw;}
}
