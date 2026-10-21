// SAME imported pre-Read namespace PackageIDList constructors, then native
// GetDLCAllowed→QueryInterface→MoveNext→SetDLCAvailable→Destroy→Destroy.
struct BnwSpClientPregameStartup::AllowedDlcEnumerator final {
 BnwSpClientPregameStartup* const owner;
 std::vector<PackageId> values;
 std::uint32_t references=1;
 std::int32_t index=-1;
 std::uint64_t reference_stores=0,index_stores=0;
 bool queried=false,returned_copy=false,destroyed=false;
 AllowedDlcEnumerator(BnwSpClientPregameStartup& p,const std::vector<PackageId>& actual)
  :owner(&p),values(actual){current();}
 void current()const{
  need(!destroyed&&owner&&references>0,"SP allowed-DLC enumerator destroyed/retired");owner->current();
  need(owner->dlc_namespace_born_,"SP allowed-DLC actual namespace constructor absent");
 }
 void query_same_enumerator(){
  current();need(!queried&&references==1,"SP allowed-DLC query cannot replay");
  // Source ICvUnknown/ICvEnumerator matched GUID returns SAME allocation and
  // calls IncrementReference. Unlike CvDllPreGame this interface does AddRef.
  increment(reference_stores);++references;queried=true;current();
 }
 bool move_next(){
  current();need(queried&&references==2,"SP allowed-DLC iterator requires actual queried lease");
  if(index==-1){
   if(values.empty())return false; // original empty branch has NO index store
   increment(index_stores);index=0;return true;
  }
  if(static_cast<std::size_t>(index)<values.size()){
   need(index<INT32_MAX,"SP allowed-DLC native index exhausted");increment(index_stores);++index;
  }
  return static_cast<std::size_t>(index)<values.size();
 }
 void destroy_query(){
  current();need(queried&&references==2&&returned_copy,"SP allowed-DLC queried destructor before actual copy return");
  increment(reference_stores);--references;current();
 }
 void destroy_caller(){
  current();need(references==1&&returned_copy,"SP allowed-DLC caller destructor before queried return");
  // Native DecrementReference at1 deletes the allocation, returning0 WITHOUT
  // a refcount0 field store. Retired model record preserves earned history.
  destroyed=true;
 }
};
void BnwSpClientPregameStartup::copy_actual_allowed_dlc(std::int32_t id){
 current();need(dlc_namespace_born_&&phase_==Phase::registration_prefix&&
  local_parent_&&id>=0&&id<64&&id==local_id_&&!allowed_dlc_enumerator_,
  "SP allowed-DLC copy requires original same local717F00 parent once");
 local_parent_->require_before_registration();
 // Original+330 new CvDllDlcPackageInfoList copies THIS actual namespace,
 // never selected content, active save packages, a fresh game or public DTO.
 allowed_dlc_enumerator_=std::make_shared<AllowedDlcEnumerator>(*this,allowed_dlc_);
 auto& enumerator=*allowed_dlc_enumerator_;
 enumerator.query_same_enumerator();
 std::vector<PackageId> copied;
 if(enumerator.move_next()){
  // Before actual GetCurrent/GUID package interface query/destructor. A later
  // nonempty producer requires that genuine suffix, not copying public IDs.
  throw std::runtime_error("SP allowed-DLC nonempty native GetCurrent/package-info suffix unjoined");
 }
 // Empty→empty native list assignment is still called, with no element or
 // archive/delta stores. Keep the distinct method-return observation; do not
 // fabricate an equal scalar store or UI/Game readiness from it.
 current();available_dlc_.at(id)=copied;++dlc_copy_calls_;current();
 enumerator.returned_copy=true;
 enumerator.destroy_query(); // CvEnumerator destructor at102EB47B
 enumerator.destroy_caller(); // outer717F00 original71800E after setter return
 current();
}
