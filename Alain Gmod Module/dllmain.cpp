// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>
#include "Internal.h"

#define GMOD_ALLOW_DEPRECATED 1
#include "../garrysmod_common/include/GarrysMod/Lua/Interface.h"
#include "../garrysmod_common/include/GarrysMod/Lua/LuaInterface.h"
#include "../garrysmod_common/include/GarrysMod/Lua/LuaShared.h"
using namespace GarrysMod::Lua;
using namespace GarrysMod::Lua::State;
using namespace GarrysMod::Lua::Type;

HMODULE myModule;

typedef int(*tluaL_loadstring)(lua_State* state, const char *s);
tluaL_loadstring luaL_loadstring;
typedef int(*tluaL_loadfile)(lua_State* state, const char *s);
tluaL_loadfile luaL_loadfile;

DWORD WINAPI Main(LPVOID lParam);
lua_State* GetClientState();
ILuaInterface* ClientLua;
void PrintMessage(const char* message, lua_State* state);

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// Save dll handle
		myModule = hModule;
		// Create thread
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Main, 0, 0, 0);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

// Run lua string
int runString(lua_State* state)
{
	if (LUA->IsType(4, String))
	{
		luaL_loadstring(state, LUA->GetString(4));
		LUA->PCall(0, 0, 0);
		return 0;
	}
	return 1;
}

// Load lua file
int loadLuaFile(lua_State* state)
{
	if (LUA->IsType(4, String))
	{
		string fileName = LUA->GetString(4);

		cout << "Loading file...\nName: " << fileName << "  Path: " << ("C:\\Lua\\" + fileName) << endl;

		ifstream file("C:\\Lua\\" + fileName, ios::out);
		string line;
		string fileContent;

		// Read file
		while (getline(file, line))
		{
			fileContent += line;
			fileContent += "\n";
		}

		// Check if text is not zero
		if (!fileContent.empty())
		{
			// run lua file
			//DOESNT WORK ANYMORE: LUA->RunString("cl_init.lua", "gamemodes/base/gamemode/cl_init.lua", fileContent.c_str());
			ClientLua->RunString("cl_init.lua", "gamemodes/base/gamemode/cl_init.lua", fileContent.c_str(), true, true);

			cout << "Loaded file!\n";
			// Print success
			LUA->PushSpecial(SPECIAL_GLOB);
			LUA->GetField(-1, "print");
			LUA->PushString(("Alain Module: Loaded file " + fileName).c_str());
			LUA->Call(1, 0);
			LUA->Pop();

			return 0;
		}
	}

	return 0;
}

DWORD WINAPI Main(LPVOID lParam)
{
	// Create console
	Internal::CreateConsole();
	cout << "Loaded!\n";
	// Erase pe header
	Internal::ErasePEHeader((DWORD)myModule);
	cout << "Erased PE header!\n";

	lua_State* state = GetClientState();
	cout << "Got lua state!\nCode begins here:\n";
	// Print loaded message ------------------------------

	PrintMessage("Alain module loaded!", state);

	// Push _G
	LUA->PushSpecial(SPECIAL_GLOB);

	// Get concommand library
	LUA->GetField(-1, "concommand");

	// Get add function
	LUA->GetField(-1, "Add");
	// Push name
	LUA->PushString("alain_load");
	// Push function
	LUA->PushCFunction(loadLuaFile);
	// Call concommand.add
	LUA->Call(2, 0); 

	// Get add field
	LUA->GetField(-1, "Add");
	// Push name
	LUA->PushString("alain_run");
	// Push function
	LUA->PushCFunction(runString);
	// Call concommand.add
	LUA->Call(2, 0);

	// Pop concommand
	LUA->Pop();

	LUA->Pop();


	PrintMessage("Usage: ", state);
	PrintMessage("alain_run <code> - Runs Lua code", state);
	PrintMessage("alain_load <filename.lua> - Runs a file from C:\\Lua\\", state);

	while (!GetAsyncKeyState(VK_END))
	{
		Sleep(5);
	}
	cout << "Unloading...\n";
	// Close console
	FreeConsole();
	// Unload module and exit
	FreeLibraryAndExitThread(myModule, 0);
}

lua_State* GetClientState()
{
	// Get lua module
	HMODULE LuaShared_modhandle = GetModuleHandle("lua_shared.dll");
	cout << "Got lua_shared module!\n";
	// Check for valid handle
	if (LuaShared_modhandle != NULL)
	{
		cout << "Valid!\n";
		// Function assignments
		typedef void* (*CreateInterfaceFn)(const char *name, int *returncode);
		CreateInterfaceFn LuaShared_createinter = (CreateInterfaceFn)GetProcAddress(LuaShared_modhandle, "CreateInterface");
		luaL_loadstring = (tluaL_loadstring)GetProcAddress(LuaShared_modhandle, "luaL_loadstring");
		luaL_loadfile = (tluaL_loadfile)GetProcAddress(LuaShared_modhandle, "luaL_loadfile");

		ILuaShared* LuaShared = (ILuaShared*)LuaShared_createinter("LUASHARED003", NULL);
		cout << "Created interface!\n";
		if (LuaShared != NULL)
		{
			cout << "Valid!\n";
			ClientLua = LuaShared->GetLuaInterface(CLIENT);
			cout << "Got lua interface!\n";
			if (ClientLua != NULL)
			{
				cout << "Valid!\n";
				lua_State* state = ClientLua->GetState();
				
				return state;
			}
		}
	}
	cout << "Error!\n";
	cout << "Unloaded!\n";
	// Close console
	FreeConsole();
	// Unload module and exit
	FreeLibraryAndExitThread(myModule, 0);
	return 0;
}

void PrintMessage(const char* message, lua_State* state)
{
	LUA->PushSpecial(SPECIAL_GLOB);
	LUA->GetField(-1, "print");
	LUA->PushString(message);
	LUA->Call(1,0);
	LUA->Pop();
}