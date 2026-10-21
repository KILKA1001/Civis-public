#pragma once
static constexpr char lua_locale_bootstrap[]=R"lua(
local current, select_language, convert_text_key = ...
return function(env)
    env.Locale={GetCurrentLanguage=current,SetCurrentLanguage=select_language,
        ConvertTextKey=convert_text_key}
end
)lua";
