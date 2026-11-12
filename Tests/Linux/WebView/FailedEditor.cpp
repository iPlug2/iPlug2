#include "FailedEditor.h"
#include "IPlug_include_in_plug_src.h"

LinuxFailedEditor::LinuxFailedEditor(const iplug::InstanceInfo& info)
: iplug::Plugin(info, iplug::MakeConfig(0, 1))
{}
