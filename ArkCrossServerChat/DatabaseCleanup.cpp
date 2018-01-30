#include "DatabaseCleanup.h"

void CleanupTimer()
{
	auto& plugin = Plugin::Get();

	auto now = std::chrono::system_clock::now();
	auto diff = std::chrono::duration_cast<std::chrono::seconds>(plugin.NextCleanupTime - now);

	if (diff.count() <= 0)
	{
		plugin.NextCleanupTime = now + std::chrono::seconds(3600);

		try
		{
			*plugin.db << u"DELETE FROM Messages "
				"WHERE At <= ?;" << (std::chrono::system_clock::now() - std::chrono::seconds(3600 * 24 * 7)).time_since_epoch().count();
		}
		catch (sqlite::sqlite_exception& e)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, e.what());
		}
	}
}