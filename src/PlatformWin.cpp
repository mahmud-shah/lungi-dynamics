#include "Platform.h"

#ifdef _WIN32

#include <windows.h>
#include <commdlg.h>

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

#else

bool Platform::chooseImageFile(std::string&)
{
    return false;
}

#endif
