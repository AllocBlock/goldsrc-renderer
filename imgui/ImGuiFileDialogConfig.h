#pragma once

// uncomment and modify defines under for customize ImGuiFileDialog

//#define MAX_FILE_DIALOG_NAME_BUFFER 1024
//#define MAX_PATH_BUFFER_SIZE 1024

#define USE_IMGUI_TABLES

//#define USE_EXPLORATION_BY_KEYS
// this mapping by default is for GLFW but you can use another
//#include <GLFW/glfw3.h> 
// Up key for explore to the top
//#define IGFD_KEY_UP GLFW_KEY_UP
// Down key for explore to the bottom
//#define IGFD_KEY_DOWN GLFW_KEY_DOWN
// Enter key for open directory
//#define IGFD_KEY_ENTER GLFW_KEY_ENTER
// BackSpace for comming back to the last directory
//#define IGFD_KEY_BACKSPACE GLFW_KEY_BACKSPACE

// widget
// filter combobox width
//#define FILTER_COMBO_WIDTH 120.0f
// button widget use for compose path
//#define IMGUI_PATH_BUTTON ImGui::Button
// standar button
//#define IMGUI_BUTTON ImGui::Button

// locales string
#define createDirButtonString u8"�½��ļ���"
#define okButtonString u8"ȷ��"
#define cancelButtonString u8"ȡ��"
#define resetButtonString u8"��ǰĿ¼"
#define drivesButtonString u8"Ӳ��"
#define searchString u8"����"
#define dirEntryString u8"[�ļ���] "
#define linkEntryString u8"[��ݷ�ʽ] "
#define fileEntryString " "
#define fileNameString u8"�ļ��� : "
#define buttonResetSearchString u8"�������"
#define buttonDriveString u8"��ת��Ӳ�̸�Ŀ¼"
#define buttonResetPathString u8"���õ���ǰĿ¼"
#define buttonCreateDirString u8"�½��ļ���"
#define OverWriteDialogTitleString u8"�ļ��Ѵ��ڣ�"
#define OverWriteDialogMessageString u8"�Ƿ񸲸��ļ���"
#define OverWriteDialogConfirmButtonString u8"ȷ��"
#define OverWriteDialogCancelButtonString u8"ȡ��"

// theses icons will appear in table headers
//#define USE_CUSTOM_SORTING_ICON
//#define tableHeaderAscendingIcon "A|"
//#define tableHeaderDescendingIcon "D|"
//#define tableHeaderFileNameString " File name"
//#define tableHeaderFileSizeString " Size"
//#define tableHeaderFileDateString " Date"

//#define USE_BOOKMARK
//#define bookmarkPaneWith 150.0f
//#define IMGUI_TOGGLE_BUTTON ToggleButton
//#define bookmarksButtonString "Bookmark"
//#define bookmarksButtonHelpString "Bookmark"
//#define addBookmarkButtonString "+"
//#define removeBookmarkButtonString "-"
