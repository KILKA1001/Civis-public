// Inside the existing genuine UnitReference::read chain. SAME raw live unit,
// original maxHP getter then damage getter; SDK does not clamp this result.
else if(method=="GetCurrHitPoints"){
 const auto max_hit_points=owner->unit_max_hit_points();
 const auto damage=unit_scalar(actual->core,"m_iDamage");
 const auto current=static_cast<std::int64_t>(max_hit_points)-static_cast<std::int64_t>(damage);
 need(current>=INT32_MIN&&current<=INT32_MAX,"Unit current HP original signed arithmetic overflows");
 value=current;
}
