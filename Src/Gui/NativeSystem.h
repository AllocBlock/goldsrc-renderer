#pragma once
#include <string>

enum EDialogAction
{
	CANCEL,
	CONFIRM,
};

struct EFileDialogResult
{
	EDialogAction Action = EDialogAction::CANCEL;
	std::string FilePath;

	operator bool() const
	{
		return Action == EDialogAction::CONFIRM;
	}
};

namespace Gui
{
	// Filter format: ";" for a new entry, "," to separate in same entry, e.g. "jpg,png,bmp;psd"
	EFileDialogResult createOpenFileDialog(std::string vFilter = "");
	EFileDialogResult createSaveFileDialog(std::string vFilter = "");
};

