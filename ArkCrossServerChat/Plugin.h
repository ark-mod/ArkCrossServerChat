#pragma once
#include "Utils.h"

class Plugin
{
public:
	nlohmann::json json;
	sqlite::database* db;
	std::string serverKey;
	std::string serverTag;
	bool hideServerTagOnLocal;
	std::string namePattern;

	HANDLE handle_mre_thread;
	HANDLE handle_mre_cluster;
	HANDLE ghThread;
	std::wstring mre_cluster_name;
	std::chrono::time_point<std::chrono::system_clock> NextCleanupTime;
	long long lastRowId = 0;

	static Plugin& Get();

	Plugin(const Plugin&) = delete;
	Plugin(Plugin&&) = delete;
	Plugin& operator=(const Plugin&) = delete;
	Plugin& operator=(Plugin&&) = delete;

private:
	Plugin() = default;
	~Plugin() = default;
};