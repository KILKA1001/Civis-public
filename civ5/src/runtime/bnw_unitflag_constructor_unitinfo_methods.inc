// Original UnitFlag constructor's current native reads. No fake flags.
   else if(method=="CargoSpace")value=std::int64_t{unit_scalar(actual->core,"m_iCargoCapacity")};
   else if(method=="GetUnitType")value=std::int64_t{owner->unit_info_type(*actual)};
   else if(method=="GetDomainType")value=std::int64_t{owner->unit_info_domain(*actual)};
   else if(method=="IsTrade")value=owner->unit_info_trade(*actual);
