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

namespace NativeSystem
{
	EFileDialogResult createOpenFileDialog(std::string vFilter = "");
	EFileDialogResult createSaveFileDialog(std::string vFilter = "");
};

