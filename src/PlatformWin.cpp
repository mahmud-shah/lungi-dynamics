#include "Platform.h"

#ifdef _WIN32

#include <windows.h>
#include <commdlg.h>
#include <direct.h>
#include <cerrno>

// Native "Open" dialog from comdlg32. OFN_NOCHANGEDIR stops the dialog from
// changing the working directory, which would otherwise break the relative
// "assets/..." paths the rest of the program uses.
bool Platform::chooseImageFile(std::string& path)
{
    char buffer[MAX_PATH] = "";
    OPENFILENAMEA dialog;
    ZeroMemory(&dialog, sizeof(dialog));
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = GetActiveWindow();
    dialog.lpstrFilter = "Fabric photos (*.jpg;*.jpeg;*.png;*.bmp)\0*.jpg;*.jpeg;*.png;*.bmp\0All files\0*.*\0";
    dialog.lpstrFile = buffer;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrTitle = "Choose a photo of lungi fabric";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&dialog)) {
        path = buffer;
        return true;
    }
    return false;
}

bool Platform::makeDirectory(const std::string& path)
{
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
}

#else

#include <cerrno>
#include <sys/stat.h>

bool Platform::chooseImageFile(std::string&)
{
    return false;
}

bool Platform::makeDirectory(const std::string& path)
{
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
}

#endif
