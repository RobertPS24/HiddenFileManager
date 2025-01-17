#include <windows.h>
#include <shlobj.h>
#include <string>
#include <sstream>
#include <set>
#include "resource.h"

#pragma comment(lib, "shell32.lib")

// Declaratii pentru controalele dialogului
HWND hDlg, hEditDir, hEditHidden, hButtonSelect;

// Variabile globale
std::wstring currentDirectory;
std::set<std::wstring> uniqueHiddenFiles;
bool moveConfirmed = false;
bool searchInProgress = false;

// ID-uri pentru controale si timer
#define IDT_TIMER 1004

// Functie pentru afisarea continutului directorului
void DisplayDirectoryContents(const std::wstring& directory, std::wostringstream& output, int level = 0) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile((directory + L"\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0) {
            for (int i = 0; i < level; i++) output << L"  ";
            output << findFileData.cFileName << L"\r\n";

            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                DisplayDirectoryContents(directory + L"\\" + findFileData.cFileName, output, level + 1);
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

// Functie pentru cautarea fisierelor ascunse
void FindHiddenFiles(const std::wstring& directory) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile((directory + L"\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0) {
            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
                std::wstring fullPath = directory + L"\\" + findFileData.cFileName;
                uniqueHiddenFiles.insert(fullPath);
            }

            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                FindHiddenFiles(directory + L"\\" + findFileData.cFileName);
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

void PerformFileMoveOperation() {
    std::wstring tempHiddenPath = currentDirectory + L"\\TempHidden";
    if (!CreateDirectory(tempHiddenPath.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        MessageBox(hDlg, L"Nu s-a putut crea folderul TempHidden.", L"Eroare", MB_OK | MB_ICONERROR);
        return;
    }

    for (const auto& file : uniqueHiddenFiles) {
        std::wstring fileName = file.substr(file.find_last_of(L"\\") + 1);
        std::wstring newPath = tempHiddenPath + L"\\" + fileName;
        if (MoveFile(file.c_str(), newPath.c_str())) {
            // Fisier mutat cu succes
        }
        else {
            DWORD error = GetLastError();
            wchar_t errorMsg[256];
            swprintf_s(errorMsg, L"Eroare la mutarea fisierului %s. Cod eroare: %d", file.c_str(), error);
            MessageBox(hDlg, errorMsg, L"Eroare", MB_OK | MB_ICONERROR);
        }
    }

    uniqueHiddenFiles.clear();
    SetDlgItemText(hDlg, IDC_EDIT_HIDDEN, L"");

    // Actualizam afisarea continutului directorului
    std::wostringstream output;
    DisplayDirectoryContents(currentDirectory, output);
    SetDlgItemText(hDlg, IDC_EDIT_DIR, output.str().c_str());
}

// Functie pentru mutarea fisierelor ascunse
void MoveHiddenFiles() {
    if (uniqueHiddenFiles.empty()) {
        SetDlgItemText(hDlg, IDC_EDIT_HIDDEN, L"Nu s-au gasit fisiere ascunse.");
        return;
    }

    std::wostringstream hiddenFilesList;
    for (const auto& file : uniqueHiddenFiles) {
        hiddenFilesList << file << L"\r\n";
    }
    SetDlgItemText(hDlg, IDC_EDIT_HIDDEN, hiddenFilesList.str().c_str());

    if (!moveConfirmed) {
        int result = MessageBox(hDlg, L"Doriti sa mutati fisierele ascunse?", L"Confirmare", MB_YESNO | MB_ICONQUESTION);
        if (result == IDYES) {
            moveConfirmed = true;
            PerformFileMoveOperation();
        }
    }
    else {
        PerformFileMoveOperation();
    }
}

void ResetDialogState() {
    moveConfirmed = false;
    uniqueHiddenFiles.clear();
    SetDlgItemText(hDlg, IDC_EDIT_HIDDEN, L"");
}

// Functie pentru selectarea directorului
void SelectDirectory() {
    BROWSEINFO bi = { 0 };
    bi.hwndOwner = hDlg;
    bi.lpszTitle = L"Selectati directorul";
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if (pidl != 0) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDList(pidl, path)) {
            currentDirectory = path;
            std::wostringstream output;
            DisplayDirectoryContents(currentDirectory, output);
            SetDlgItemText(hDlg, IDC_EDIT_DIR, output.str().c_str());

            ResetDialogState();
        }

        IMalloc* imalloc = 0;
        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->Free(pidl);
            imalloc->Release();
        }
    }
}

// Functie pentru cautarea si mutarea fisierelor (apelat de timer)
void SearchAndMoveFiles() {
    if (searchInProgress || currentDirectory.empty()) return;

    searchInProgress = true;
    uniqueHiddenFiles.clear();
    FindHiddenFiles(currentDirectory);
    MoveHiddenFiles();
    searchInProgress = false;
}

// Procedura dialogului
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        hDlg = hwndDlg;
        hEditDir = GetDlgItem(hwndDlg, IDC_EDIT_DIR);
        hEditHidden = GetDlgItem(hwndDlg, IDC_EDIT_HIDDEN);
        hButtonSelect = GetDlgItem(hwndDlg, IDC_BUTTON_SELECT);
        SetTimer(hwndDlg, IDT_TIMER, 5000, NULL);
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BUTTON_SELECT) {
            SelectDirectory();
        }
        return TRUE;

    case WM_TIMER:
        if (wParam == IDT_TIMER) {
            SearchAndMoveFiles();
        }
        return TRUE;

    case WM_CLOSE:
        EndDialog(hwndDlg, 0);
        return TRUE;
    }
    return FALSE;
}

// Functia WinMain
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    return DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DialogProc);
}