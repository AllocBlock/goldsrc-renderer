#include "NativeSystem.h"
#include <nfd.h>

std::string gLastLoadPath;

// Filter format: ";" for a new entry, "," to separate in same entry, e.g. "jpg,png,bmp;psd"
EFileDialogResult Gui::createOpenFileDialog(std::string vFilter)
{
	nfdchar_t* pOutPath = nullptr;
	nfdresult_t NfdResult = NFD_OpenDialog(vFilter.empty() ? nullptr : vFilter.data(), gLastLoadPath.empty() ? nullptr : gLastLoadPath.data(), &pOutPath);

	EFileDialogResult Result;
	Result.Action = (NfdResult == nfdresult_t::NFD_OKAY ? EDialogAction::CONFIRM : EDialogAction::CANCEL);
	Result.FilePath = pOutPath ? pOutPath : "";

	if (Result.Action == EDialogAction::CONFIRM)
		gLastLoadPath = Result.FilePath;
	return Result;
}

// Filter format: ";" for a new entry, "," to separate in same entry, e.g. "jpg,png,bmp;psd"
EFileDialogResult Gui::createSaveFileDialog(std::string vFilter)
{
	nfdchar_t* pOutPath = nullptr;
	nfdresult_t NfdResult = NFD_SaveDialog(vFilter.empty() ? nullptr : vFilter.data(), gLastLoadPath.empty() ? nullptr : gLastLoadPath.data(), &pOutPath);

	EFileDialogResult Result;
	Result.Action = (NfdResult == nfdresult_t::NFD_OKAY ? EDialogAction::CONFIRM : EDialogAction::CANCEL);
	Result.FilePath = pOutPath;

	if (Result.Action == EDialogAction::CONFIRM)
		gLastLoadPath = Result.FilePath;
	return Result;
}