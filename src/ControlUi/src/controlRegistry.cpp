#include "controlRegistry.h"

using namespace controls;

std::shared_ptr<ControlRegistry> ControlRegistry::instance;

ControlRegistry& ControlRegistry::getInstance()
{
	if (!instance)
	{
		instance = std::shared_ptr<ControlRegistry>(new ControlRegistry());
	}
	return *instance;
}
