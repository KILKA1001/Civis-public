#pragma once
static constexpr char lua_content_bootstrap[]=R"lua(
local ids, active, description, gameplay = ...
return function(env)
    env.ContentType={GAMEPLAY=gameplay}
    env.ContentManager={GetAllPackageIDs=ids,IsActive=active,GetPackageDescription=description}
end
)lua";
