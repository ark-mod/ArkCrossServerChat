#include <fstream>
#include <chrono>
#include "Utils.h"
#include "Plugin.h"
#include "Hooks.h"
#include "DatabaseCleanup.h"
#include "DebugTimer.h"
#include "MessageHandlers.h"

#pragma comment(lib, "ArkApi.lib")
#pragma comment(lib, "ArkPluginLibrary.lib")

void LoadConfig()
{
	auto& plugin = Plugin::Get();
	std::ifstream file(ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/ArkCrossServerChat/config.json");
	if (!file.is_open())
	{
		Log::GetLog()->error("Could not open file config.json");
		throw;
	}

	file >> plugin.json;
	file.close();
}

void Load()
{
	Log::Get().Init("ArkCrossServerChat");

	auto& plugin = Plugin::Get();
	auto& db = plugin.db;

	if (sqlite3_threadsafe() == 0)
	{
		Log::GetLog()->error("sqlite is not threadsafe");
		throw;
	}

	LoadConfig();
	plugin.hideServerTagOnLocal = plugin.json.value("HideServerTagOnLocal", true);
	plugin.namePattern = plugin.json.value("NamePattern", "{Name}({ServerTag})");
	plugin.serverTag = plugin.json.value("ServerTag", "");
	plugin.serverKey = plugin.json.value("ServerKey", "");
	if (plugin.serverKey.empty())
	{
		Log::GetLog()->error("ServerKey must be set in config.json");
		throw;
	}

	std::string clusterKey = plugin.json["ClusterKey"];
	if (clusterKey.empty())
	{
		Log::GetLog()->error("ClusterKey must be set in config.json");
		throw;
	}

	plugin.mre_cluster_name = std::wstring(L"ArkCrossServerChat_").append(ArkApi::Tools::ConvertToWideStr(clusterKey));

	std::string databasePath = plugin.json["DatabasePath"];
	if (databasePath.empty())
	{
		Log::GetLog()->error("DatabasePath must be set in config.json");
		throw;
	}

	try
	{
		db = new sqlite::database(databasePath);
	}
	catch (sqlite::sqlite_exception &e)
	{
		Log::GetLog()->error("({} {}) Failed to setup database {}", __FILE__, __FUNCTION__, e.what());
	}

	sqlite3_busy_timeout(db->connection().get(), 500);

	//setup and migrate database
	int hasTableMessages = false;
	*db << "PRAGMA table_info(Messages);" >> [&]() { hasTableMessages = true; };
	if (!hasTableMessages)
	{
		try
		{
			*db << "begin;";
			*db << u"create table if not exists Messages ("
				"Id integer primary key autoincrement not null,"
				"At integer default 0,"
				"ServerKey text default '',"
				"ServerTag text default '',"
				"SteamId integer default 0,"
				"PlayerName text default '',"
				"CharacterName text default '',"
				"TribeName text default '',"
				"Message text default '',"
				"Type integer default 0,"
				"Rcon integer default 0,"
				"Icon integer default 0"
				");";
			*db << "PRAGMA user_version = 1;";
			*db << "commit;";
		}
		catch(sqlite::sqlite_exception &e)
		{
			*db << "rollback;";
			Log::GetLog()->error("({} {}) Failed to setup database {}", __FILE__, __FUNCTION__, e.what());
			throw;
		}
	}
	else
	{
		int userVersion = 0;
		*db << "PRAGMA user_version;" >> userVersion;

		if (userVersion == 0)
		{
			try
			{
				userVersion = 1;
				*db << "begin;";
				*db << u"alter table Messages add column Icon integer default 0;";
				*db << u"alter table Messages add column ServerTag text default '';";
				*db << "PRAGMA user_version = " + std::to_string(userVersion) + ";";
				*db << "commit;";
			}
			catch (sqlite::sqlite_exception &e)
			{
				*db << "rollback;";
				Log::GetLog()->error("({} {}) Failed to migrate database to version 1 {}", __FILE__, __FUNCTION__, e.what());
				throw;
			}
		}
	}

	//todo: would be better to do this once the server is started and world is up and running
	*plugin.db << "SELECT Id FROM Messages ORDER BY Id DESC LIMIT 1;" >> [&](long long id) {
		plugin.lastRowId = id;
	};

	plugin.handle_mre_thread = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (plugin.handle_mre_thread == NULL)
	{
		Log::GetLog()->error("Failed to create sync event (thread).");
		throw;
	}

	plugin.handle_mre_cluster = CreateEvent(NULL, TRUE, FALSE, plugin.mre_cluster_name.c_str());
	if (plugin.handle_mre_cluster == NULL)
	{
		Log::GetLog()->error("Failed to create sync event (cluster).");
		throw;
	}

	DWORD dwThreadID;
	plugin.ghThread = CreateThread(NULL, 0, WaitForNewMessagesThreadProc, NULL, 0, &dwThreadID);
	if (plugin.ghThread == NULL)
	{
		Log::GetLog()->error("Failed to create update thread.");
		throw;
	}

	plugin.NextCleanupTime = std::chrono::system_clock::now() + std::chrono::seconds(300); // init with cleanup in 5 min
	plugin.last_test_time = std::chrono::system_clock::now();

	auto& hooks = ArkApi::GetHooks();
	auto& commands = ArkApi::GetCommands();

	hooks.SetHook("UShooterCheatManager.ServerChat", 
		&Hook_UShooterCheatManager_ServerChat, 
		&UShooterCheatManager_ServerChat_original);

	commands.AddOnChatMessageCallback("ChatMessageCallback", &ChatMessageCallback);
	commands.AddOnTimerCallback("CleanupTimer", &CleanupTimer);
	if (plugin.json.value("Debug", false) == true) commands.AddOnTimerCallback("DebugTimer", &DebugTimer);
	commands.AddOnTickCallback("MessageTimer", &MessageTimer);
}

void Unload()
{
	auto& plugin = Plugin::Get();
	auto& hooks = ArkApi::GetHooks();
	auto& commands = ArkApi::GetCommands();

	hooks.DisableHook("UShooterCheatManager.ServerChat", 
		&Hook_UShooterCheatManager_ServerChat);

	commands.RemoveOnChatMessageCallback("ChatMessageCallback");
	commands.RemoveOnTimerCallback("CleanupTimer");
	if (plugin.json.value("Debug", false) == true) commands.RemoveOnTimerCallback("DebugTimer");
	commands.RemoveOnTickCallback("MessageTimer");

	SetEvent(plugin.handle_mre_thread);

	CloseHandle(plugin.ghThread);

	if (plugin.db)
	{
		delete plugin.db;
		plugin.db = nullptr;
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		Unload();
		break;
	}
	return TRUE;
}
