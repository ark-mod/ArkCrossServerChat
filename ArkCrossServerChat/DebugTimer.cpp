#include "DatabaseCleanup.h"

std::string get_current_time()
{
	auto now = time(NULL);
	struct tm tstruct;
	char buffer[40];
	tstruct = *localtime(&now);
	strftime(buffer, sizeof(buffer), "%X", &tstruct);

	return buffer;
}

void DebugTimer()
{
	auto& plugin = Plugin::Get();

	auto now = std::chrono::system_clock::now();
	auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - plugin.last_test_time);

	// send a test message every two seconds
	if (diff.count() >= 2000)
	{
		plugin.last_test_time = now;

		auto world = ArkApi::GetApiUtils().GetWorld();
		if (!world) return;

		auto playerController = world->GetFirstPlayerController();
		if (!playerController) return;

		auto aShooterPC = static_cast<AShooterPlayerController*>(playerController);
		auto when = get_current_time();

		Log::GetLog()->info("Sending test message ({})", when);
		aShooterPC->ServerSendChatMessage(&FString::Format("{}", when), EChatSendMode::GlobalChat);
	}
}