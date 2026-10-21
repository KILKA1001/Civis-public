// First original UnitFlag.new predicate prefix, inside the existing actual
// UnitReference read chain; no new unit, flag or interface reference is minted.
else if(method=="IsCombatUnit")value=unit_scalar(actual->core,"m_iBaseCombat")>0;
else if(method=="IsEmbarked"){
 const saves::UnitCoreField*embarked=nullptr;
 for(const auto&field:actual->core.fields)if(field.name&&std::string_view(field.name)=="m_bEmbarked"){
  need(!embarked,"Unit actual embarked member duplicated");embarked=&field;
 }
 need(embarked&&embarked->encoding==saves::UnitCoreEncoding::boolean&&embarked->values.size()==1&&
  (embarked->values.front()==0||embarked->values.front()==1),"Unit actual embarked member/boolean schema unavailable");
 value=embarked->values.front()!=0;
}
