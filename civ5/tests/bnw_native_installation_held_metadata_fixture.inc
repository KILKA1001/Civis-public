// Included inside the prefix test's anonymous namespace, after its real helpers.
// No production friend, minted receipt or alternate admission route.
namespace held_metadata_fixture {
BY_HANDLE_FILE_INFORMATION information(const std::filesystem::path& path){
 const HANDLE file=CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
  nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 need(file!=INVALID_HANDLE_VALUE,"Held metadata fixture cannot inspect an actual file");
 BY_HANDLE_FILE_INFORMATION out{};const bool read=GetFileInformationByHandle(file,&out)!=FALSE;CloseHandle(file);
 need(read&&!(out.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)),"Held metadata fixture needs a direct ordinary file");return out;
}
bool same_object(const BY_HANDLE_FILE_INFORMATION& a,const BY_HANDLE_FILE_INFORMATION& b){
 return a.dwVolumeSerialNumber==b.dwVolumeSerialNumber&&a.nFileIndexHigh==b.nFileIndexHigh&&a.nFileIndexLow==b.nFileIndexLow;
}
bool same_metadata(const BY_HANDLE_FILE_INFORMATION& a,const BY_HANDLE_FILE_INFORMATION& b){
 return same_object(a,b)&&a.nFileSizeHigh==b.nFileSizeHigh&&a.nFileSizeLow==b.nFileSizeLow&&
  a.dwFileAttributes==b.dwFileAttributes&&CompareFileTime(&a.ftLastWriteTime,&b.ftLastWriteTime)==0;
}
struct SparseInstallation final {
 std::filesystem::path root,descriptor,archive;
 const std::filesystem::path base=std::filesystem::canonical(std::filesystem::current_path());
 const std::filesystem::path original;
 const std::array<std::filesystem::path,4> relatives{
  L"CivilizationV.exe",L"Assets/DLC/Expansion2/CvGameCore_Expansion2.dll",
  L"Assets/DLC/DLC_Deluxe/Babylon.Civ5Pkg",L"Resource/DX9/AmericanWashingtonTextures.fpk"};
 std::array<BY_HANDLE_FILE_INFORMATION,4> original_metadata{};
 explicit SparseInstallation(const char* source,bool copy_selected):original(std::filesystem::canonical(source)){
  for(const auto ch:base.native())need(ch>0&&ch<128,"Held metadata fixture requires an ASCII working directory for the genuine Stat service");
  for(unsigned attempt=0;attempt<32&&root.empty();++attempt){
   const auto trial=base/("bnw-held-metadata-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64())+"-"+std::to_string(attempt));
   if(std::filesystem::create_directory(trial))root=trial;
  }
  need(!root.empty(),"Held metadata fixture could not create its isolated ordinary directory");
  try{
   for(std::size_t n=0;n<relatives.size();++n){
    const auto from=original/relatives[n],to=root/relatives[n];original_metadata[n]=information(from);
    std::filesystem::create_directories(to.parent_path());
    const bool isolated=n==2||(copy_selected&&n==3);
    if(isolated){
     need(CopyFileW(from.c_str(),to.c_str(),TRUE)!=FALSE,"Held metadata isolated real-source copy failed");
     const auto copied=information(to);need(!same_object(copied,original_metadata[n])&&copied.nNumberOfLinks==1,
      "Timestamp mutation target is not a separate single-link copy");
     const auto a=runtime::identify_file(from),b=runtime::identify_file(to);
     need(a.bytes==b.bytes&&a.sha256==b.sha256,"Held metadata isolated copy changed actual source bytes");
    }else{
     need(CreateHardLinkW(to.c_str(),from.c_str(),nullptr)!=FALSE,"Held metadata read-only reference could not be created");
     need(same_object(information(to),original_metadata[n]),"Held metadata read-only reference changed actual file identity");
    }
   }
   descriptor=root/relatives[2];archive=root/relatives[3];
   const auto manifest=content::read_manifest(descriptor);
   need(manifest.kind==content::PackageKind::dlc&&std::none_of(manifest.document.children.begin(),manifest.document.children.end(),
    [](const auto& node){return node.name=="UISkin";}),"Sparse fixture must use the genuine Babylon descriptor without invented UISkin seeds");
  }catch(...){cleanup();throw;}
 }
 void cleanup()noexcept{
  if(root.empty()||!root.is_absolute()||root.parent_path()!=base||!root.filename().native().starts_with(L"bnw-held-metadata-"))return;
  const auto attributes=GetFileAttributesW(root.c_str());
  if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&FILE_ATTRIBUTE_REPARSE_POINT))return;
  std::error_code error;const auto resolved=std::filesystem::canonical(root,error);
  if(!error&&resolved==root)std::filesystem::remove_all(root,error);
 }
 ~SparseInstallation(){cleanup();}
 void require_real_sources_unchanged()const{
  for(std::size_t n=0;n<relatives.size();++n)need(same_metadata(information(original/relatives[n]),original_metadata[n]),
   "Metadata fixture changed a real assets object instead of its isolated copy");
 }
 SparseInstallation(const SparseInstallation&)=delete;SparseInstallation&operator=(const SparseInstallation&)=delete;
};
class CopiedTimestamp final {
 HANDLE file_=INVALID_HANDLE_VALUE;BY_HANDLE_FILE_INFORMATION a_{};FILETIME b_{};bool captured_=false;
public:
 ~CopiedTimestamp(){if(file_!=INVALID_HANDLE_VALUE){if(captured_)SetFileTime(file_,nullptr,nullptr,&a_.ftLastWriteTime);CloseHandle(file_);}}
 void open(const std::filesystem::path& copy,const std::filesystem::path& original){
  need(file_==INVALID_HANDLE_VALUE,"Copied timestamp writer opened twice");
  const auto actual=information(copy);need(!same_object(actual,information(original))&&actual.nNumberOfLinks==1,
   "WRITE_ATTRIBUTES is restricted to a separate real-source copy");
  file_=CreateFileW(copy.c_str(),FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
   nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  need(file_!=INVALID_HANDLE_VALUE,"WRITE_ATTRIBUTES open of isolated copy failed under genuine held locks");
  need(GetFileInformationByHandle(file_,&a_)!=FALSE&&same_metadata(actual,a_),"Copied timestamp writer lost its actual source identity");
  captured_=true;
  ULARGE_INTEGER ticks{};ticks.HighPart=a_.ftLastWriteTime.dwHighDateTime;ticks.LowPart=a_.ftLastWriteTime.dwLowDateTime;
  constexpr ULONGLONG delta=120ULL*10000000ULL;need(ticks.QuadPart<=~0ULL-delta,"Copied timestamp cannot encode the bounded B epoch");
  ticks.QuadPart+=delta;b_.dwHighDateTime=ticks.HighPart;b_.dwLowDateTime=ticks.LowPart;
 }
 bool at(const FILETIME& expected)const{
  BY_HANDLE_FILE_INFORMATION now{};need(file_!=INVALID_HANDLE_VALUE&&GetFileInformationByHandle(file_,&now)!=FALSE,
   "Copied timestamp writer lost its live held copy");
  need(same_object(now,a_)&&now.nFileSizeHigh==a_.nFileSizeHigh&&now.nFileSizeLow==a_.nFileSizeLow&&now.dwFileAttributes==a_.dwFileAttributes,
   "Metadata-only mutation changed a copy's identity, bytes length or attributes");
  return CompareFileTime(&now.ftLastWriteTime,&expected)==0;
 }
 bool at_a()const{return at(a_.ftLastWriteTime);}bool at_b()const{return at(b_);}
 void set_b(){need(SetFileTime(file_,nullptr,nullptr,&b_)!=FALSE&&at_b(),"Actual copied-file SetFileTime B failed");}
 void restore_a(){need(SetFileTime(file_,nullptr,nullptr,&a_.ftLastWriteTime)!=FALSE&&at_a(),"Actual copied-file SetFileTime A restore failed");}
 CopiedTimestamp()=default;CopiedTimestamp(const CopiedTimestamp&)=delete;CopiedTimestamp&operator=(const CopiedTimestamp&)=delete;
};
void run(const char* original_install,const char* canonical_assets,std::string_view which){
 need(which=="transient"||which=="persistent"||which=="unwind-selected","Expected held metadata transient|persistent|unwind-selected");
 const bool transient=which=="transient",selected_unwind=which=="unwind-selected";
 Phase total("held_metadata.total");SparseInstallation fixture(original_install,selected_unwind);
 Window window;need(window.hwnd&&!IsWindowVisible(window.hwnd),"Held metadata fixture needs a genuine hidden HWND");
 std::uint64_t revision=1;std::size_t callbacks=0,scoped_samples=0,mutation_ordinal=0,restore_ordinal=0;
 bool armed=false,second_sample_saw_b=false;const ui::BnwNativeInstallationInputOwner* callback_input=nullptr;
 CopiedTimestamp descriptor_time,selected_time;
 auto installation=ui::BnwNativeInstallationInputOwner::acquire(fixture.root,canonical_assets,reinterpret_cast<std::uintptr_t>(window.hwnd),
  runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(window.hwnd),"held-metadata-real-sparse-installation",[&]{
   const DWORD previous_error=GetLastError();struct RestoreError{DWORD value;~RestoreError(){SetLastError(value);}}restore_error{previous_error};
   ++callbacks;
   if(armed&&callback_input&&bnw_native_retained_primary32_borrower_active_for_test(*callback_input)){
    ++scoped_samples;
    if(scoped_samples==1){descriptor_time.set_b();if(selected_unwind)selected_time.set_b();mutation_ordinal=callbacks;}
    else if(scoped_samples==2){need(descriptor_time.at_b(),"Descriptor B did not survive through the second actual revision sample");second_sample_saw_b=true;}
    else if(transient&&scoped_samples==3){
     need(second_sample_saw_b&&descriptor_time.at_b(),"Transient descriptor did not cross the first retained Input POST as B");
     descriptor_time.restore_a();restore_ordinal=callbacks;armed=false;
    }
   }
   return revision;
  }));
 callback_input=installation.get();const auto admitted_input=installation->observe();
 need(admitted_input.installation==fixture.root&&admitted_input.pinned_files.size()==3&&admitted_input.client.revision==1,
  "Sparse fixture did not acquire actual qualified EXE/core/copied-descriptor pins");
 auto parent=ui::BnwNativePackageDiscoveryOwner::create(installation);
 exact_refusal([&]{parent->advance_installation_category2_first_prefix();},"Mandatory installation-aware retained UISkin occurrences/Cold/Stat before Boot/Skin");
 exact_refusal([&]{parent->advance_installation_retained_ui_prefix();},"Mandatory actual installation first-header Cold/Stat constructors before Boot/Skin");
 exact_refusal([&]{parent->advance_installation_cold_stat_prefix();},"Mandatory installation-aware initial PrefixRegistry/Arena/cache/first archive constructor before Boot/Skin");
 exact_refusal([&]{parent->advance_installation_prefix_arena_cache();},"Mandatory installation-aware SAME CurrentDefaultSource assignment before first archive request/constructor/Header/Boot");
 exact_refusal([&]{parent->advance_installation_current_first_archive_header();},"Mandatory actual installed Prefix transaction brand before shared prefix attempt/store/entry metadata/Boot");
 const auto prepared=parent->current();need(prepared.packages==1&&prepared.skin_contexts==0&&!prepared.primary_dx9_registration_complete,
  "Sparse fixture failed genuine Category2/empty retained UI/first Header prerequisites");
 descriptor_time.open(fixture.descriptor,fixture.original/fixture.relatives[2]);
 if(selected_unwind)selected_time.open(fixture.archive,fixture.original/fixture.relatives[3]);
 need(!bnw_native_retained_primary32_borrower_active_for_test(*installation)&&descriptor_time.at_a(),"Held metadata fixture armed before its actual operation boundary");
 const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> parent_witness=parent;
 const auto before_operation=callbacks;armed=true;
 const auto expected=transient?"Mandatory actual installed primary32 journal/capture bindings before remaining archive constructors/Boot":
  (selected_unwind?"Original stat read source metadata changed":"Installation original locked source retired/changed");
 exact_refusal([&]{parent->advance_installation_first_archive_publication();},expected);armed=false;
 need(mutation_ordinal>before_operation&&second_sample_saw_b&&scoped_samples>=3,
  "Held metadata mutation did not run inside the genuine admitted firstarchive borrower");
 need(!bnw_native_retained_primary32_borrower_active_for_test(*installation)&&!parent_witness.expired()&&IsWindow(window.hwnd)!=FALSE&&revision==1,
  "Held metadata unwind lost live owner/window or retained an active borrower");
 const auto before_control=callbacks;std::uint32_t positive_admitted=0;
 if(transient){
  need(scoped_samples==3&&restore_ordinal==mutation_ordinal+2&&descriptor_time.at_a(),"Transient A-B-A was not restored at the next actual Input current");
  installation->require_current();need(callbacks==before_control+2,"Restored transient descriptor did not leave the actual Input current");
  const auto archive=parent->inspect_installation_first_archive_admission();
  need(archive.admitted_entry_count>0&&archive.admitted_entry_count<=archive.declared_entry_count,
   "Transient metadata case fabricated firstarchive positive admission");positive_admitted=archive.admitted_entry_count;
 }else{
  need(descriptor_time.at_b()&&scoped_samples%2==0,"Persistent descriptor B did not survive the complete actual provider samples");
  if(selected_unwind){
   need(selected_time.at_b(),"Selected-source copy lost its actual changed Stat epoch");
   exact_refusal([&]{installation->require_current();},"Installation original locked source retired/changed");
   need(callbacks==before_control,"Selected guard unwind did not leave persistent metadata for the next strict entry");
   selected_time.restore_a();
  }
  descriptor_time.restore_a();
  exact_refusal([&]{installation->require_current();},"Installation input retired");
  need(callbacks==before_control,"Sticky strict-boundary retirement invoked another provider callback");
 }
 need(descriptor_time.at_a()&&(!selected_unwind||selected_time.at_a()),"Isolated copy timestamps were not restored exactly");
 fixture.require_real_sources_unchanged();total.end();
 std::cout<<"SUBSET held_metadata_case="<<which<<" actual_pinned_files="<<admitted_input.pinned_files.size()
  <<" actual_scoped_samples="<<scoped_samples<<" second_sample_B="<<second_sample_saw_b;
 if(transient)std::cout<<" restore_sample_delta="<<(restore_ordinal-mutation_ordinal)<<" actual_positive_admitted="<<positive_admitted;
 else std::cout<<" positive_admission_observation=unissued";
 std::cout<<" outer_refusal=exact borrower_cleanup=clear actual_owner_HWND=live"
  <<" Input_after="<<(transient?"current":"strict_retired")<<" copied_timestamps=restored real_assets=unchanged"
  <<" whole32=unissued Default=unissued full_game=0 can_continue=0\n";
}
}
