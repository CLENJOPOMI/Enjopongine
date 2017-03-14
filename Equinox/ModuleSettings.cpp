﻿#include "ModuleSettings.h"
#include "parson.h"

ModuleSettings::ModuleSettings(bool start_enabled) : Module(start_enabled)
{
}

ModuleSettings::~ModuleSettings()
{
}

bool ModuleSettings::Init()
{
	settings = json_value_get_object(json_parse_file(inputFile));

	if(settings != nullptr)
	{
		if (json_object_has_value(settings, "maxFps"))
			MaxFps = static_cast<int>(json_object_get_number(settings, "maxFps"));
		else
			MaxFps = 60;
		return true;
	}

	return false;
}

bool ModuleSettings::CleanUp()
{
	return true;
}