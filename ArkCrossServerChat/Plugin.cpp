#include "Plugin.h"

Plugin& Plugin::Get()
{
	static Plugin instance;
	return instance;
}