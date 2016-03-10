#include "Common.h"
#include "lua.hpp"
#include "Script.h"
#include "Api.h"
#include "CallbackHandler.h"

#if LEGACY_MODE == 1

// Converts lower case first letter function name to upper case for compatibility with old code
static const char *__legacy = "\
	if (saitek ~= nil) then \n\
		passerelle = saitek \n\
		for key,value in pairs(passerelle) do \n\
			upper = string.upper(string.sub(key, 1, 1)) .. string.sub(key, 2) \n\
			saitek[upper] = passerelle[key] \n\
		end \n\
	end";

#endif

static void luaF_RegisterConst(lua_State* L) {
	LUA_START(list, luaI_const)
		// Errors
		LUA_CONST(ERR_NONE)
		LUA_CONST(ERR_HANDLE)
		LUA_CONST(ERR_NOTIMPL)
		LUA_CONST(ERR_INVALIDARG)
		LUA_CONST(ERR_PAGENOTACTIVE)
		LUA_CONST(ERR_OUTOFMEMORY)
		LUA_CONST(ERR_UNKNOWN)
		// X52 Pro
		LUA_CONST(BIT_SCROLLWHEEL_CLICK)
		LUA_CONST(BIT_SCROLLWHEEL_UP)
		LUA_CONST(BIT_SCROLLWHEEL_DOWN)
		// FIP
		LUA_CONST(BIT_RIGHTROTARY_CW)
		LUA_CONST(BIT_RIGHTROTARY_CCW)
		LUA_CONST(BIT_LEFTROTARY_CW)
		LUA_CONST(BIT_LEFTROTARY_CCW)
		LUA_CONST(BIT_S1BUTTON)
		LUA_CONST(BIT_S2BUTTON)
		LUA_CONST(BIT_S3BUTTON)
		LUA_CONST(BIT_S4BUTTON)
		LUA_CONST(BIT_S5BUTTON)
		LUA_CONST(BIT_S6BUTTON)
		// Callback mode
		LUA_CONST(CBM_DIRECT)
		LUA_CONST(CBM_EVENT)
		LUA_CONST(CBM_FSUIPC)
	LUA_END()

	for (int i = 0; list[i].name != NULL; i++) {
		lua_pushinteger(L, list[i].value);
		lua_setfield(L, -2, list[i].name);
	}
}

int luaF_Finalizer(lua_State* L) {
	LuaMan->ReleaseSlot(L);
	DevMan->Release();
	return 0;
}

extern "C" LUALIB_OPEN() {
	// Activate
	LUA_START(API, luaL_reg)
		LUA_ENTRY(getVersion)
		LUA_ENTRY(listen)
		LUA_ENTRY(getNumDevices)
		LUA_ENTRY(addPage)
		LUA_ENTRY(removePage)
		LUA_ENTRY(setLed)
		LUA_ENTRY(setImageFromFile)
		LUA_ENTRY(setImage)
		LUA_ENTRY(setString)
		LUA_ENTRY(setProfile)
		LUA_ENTRY(registerDeviceChangeCallback)
		LUA_ENTRY(registerPageCallback)
		LUA_ENTRY(registerSoftButtonCallback)
		LUA_ENTRY(registerSoftButtonUpCallback)
		LUA_ENTRY(registerSoftButtonDownCallback)
		// New in v0.7
		LUA_ENTRY(setMode)
		LUA_ENTRY(poll)
		LUA_ENTRY(sleep)
		// Obsolete
		LUA_ENTRY(Initialize)
		LUA_ENTRY(Release)
	LUA_END()

	// We already have a slot for this context
	if (LuaMan->HaveSlot(L)) {
		return 0;
	}

	// Find a free slot...
	ScriptInfo *slotP = LuaMan->GetFreeSlot(L);
	if (!slotP) {
		// Initialization failed
		return -1;
	}

	// Initialize
	DevMan->Initialize();

	// Register methods
	luaL_register(L, LUALIB_NAME, API);

#if LEGACY_MODE == 1
	// Add legacy function declarations
	luaL_dostring(L, __legacy);
#endif

	// Set constants
	luaF_RegisterConst(L);

	// Create a meta table for the script data to be collected upon script interruption
	luaL_newmetatable(L, LUALIB_TABLE);
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, luaF_Finalizer);
	lua_settable(L, -3);

	// Create the associated userdata
	ScriptInfo **dataP = (ScriptInfo **)lua_newuserdata(L, sizeof(ScriptInfo **));
	luaL_getmetatable(L, LUALIB_TABLE);
	lua_setmetatable(L, -2);

	// Save it so that it is collected only at the end of the execution
	slotP->luaRef = luaL_ref(L, LUA_REGISTRYINDEX);

	// Save the script data
	*dataP = slotP;

	return 1;
}
