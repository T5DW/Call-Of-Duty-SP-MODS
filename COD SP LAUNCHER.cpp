#include <windows.h>
#include <commctrl.h>
#include <shobjidl.h>
#include <objbase.h>
#include <string>
#include <vector>
#include <io.h>
#include <shellapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

#define ID_TRAY_APP_ICON    1001
#define ID_TRAY_EXIT        1002
#define ID_TRAY_RESTORE     1003
#define ID_TRAY_ALWAYSONTOP 1004
#define WM_SYSICON          (WM_USER + 1)

HBITMAP hBackground = nullptr;
HWND hComboCat = nullptr;
HWND hComboGame = nullptr;
HWND hBtnPlay = nullptr;
HWND hBtnSetup = nullptr;

NOTIFYICONDATAW nid = { 0 };
bool alwaysOnTop = false;

std::wstring t7mpGamePath;

struct GameMod {
    std::wstring name;
    std::wstring exeName;
    std::wstring status;
    bool enabled;
    bool requiresSetup;
    std::wstring currentPath;
};

std::vector<GameMod> iwMods = {
    {L"IW4SP Mod",          L"iw4sp.exe",       L"",                     true,  false, L""},
    {L"IW5SP Mod",          L"iw5sp.exe",       L"(NOT RELEASED YET)",  false, false, L""},
    {L"IW3SP Mod",          L"iw3sp.exe",       L"(IN DEV)",            false, false, L""},
    {L"IW6SP Mod",          L"iw6x.exe",        L"(COMING SOON!)",      false, false, L""}
};

std::vector<GameMod> boMods = {
    {L"T5SP Mod",           L"t5sp.exe",        L"",                     true,  false, L""},
    {L"T7SP Mod",           L"t7sp.exe",        L"(IN DEV)",            false, false, L""},
    {L"T7MP Mod",           L"BlackOps3_fix.exe",L"",                   false, true,  L""},
    {L"T9SP Mod",           L"t9sp.exe",        L"(IN DEV)",            false, false, L""}
};

int currentCategory = 0;

bool FileExists(const std::wstring& path) {
    return _waccess_s(path.c_str(), 0) == 0;
}

void UpdateButtonsAndSetup() {
    const auto& currentMods = (currentCategory == 0) ? iwMods : boMods;
    int sel = (int)SendMessage(hComboGame, CB_GETCURSEL, 0, 0);
    bool ready = false;
    if (sel != CB_ERR) {
        const auto& mod = currentMods[sel];
        ready = mod.enabled && (!mod.requiresSetup || !mod.currentPath.empty()) &&
            (mod.name == L"T7MP Mod" || FileExists(mod.exeName));
    }
    EnableWindow(hBtnPlay, ready ? TRUE : FALSE);

    bool isT7MP = (currentCategory == 1) && (sel != CB_ERR) && (sel < (int)boMods.size()) && (boMods[sel].name == L"T7MP Mod");
    ShowWindow(hBtnSetup, isT7MP ? SW_SHOW : SW_HIDE);
    EnableWindow(hBtnSetup, (isT7MP && t7mpGamePath.empty()) ? TRUE : FALSE);
}

void UpdateGameCombo() {
    SendMessage(hComboGame, CB_RESETCONTENT, 0, 0);
    const auto& mods = (currentCategory == 0) ? iwMods : boMods;
    for (size_t i = 0; i < mods.size(); ++i) {
        const auto& mod = mods[i];
        std::wstring text = mod.name;
        if (mod.requiresSetup && mod.currentPath.empty()) {
            text += L" (SET UP)";
        }
        else {
            text += L" " + mod.status;
        }
        SendMessage(hComboGame, CB_ADDSTRING, 0, (LPARAM)text.c_str());
    }
    SendMessage(hComboGame, CB_SETCURSEL, 0, 0);
    UpdateButtonsAndSetup();
}

void AddTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_APP_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_SYSICON;
    nid.hIcon = LoadIcon(GetModuleHandle(NULL), IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"Wyatt's COD Launcher");
    Shell_NotifyIconW(NIM_ADD, &nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void ShowContextMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RESTORE, L"Restore");
    AppendMenuW(hMenu, MF_STRING | (alwaysOnTop ? MF_CHECKED : 0), ID_TRAY_ALWAYSONTOP, L"Always on Top");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

std::wstring BrowseForFolder(HWND hwnd) {
    std::wstring result;
    IFileDialog* pfd = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (SUCCEEDED(hr)) {
        DWORD dwOptions;
        pfd->GetOptions(&dwOptions);
        pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST);
        pfd->SetTitle(L"Select Game Folder (must contain BlackOps3_fix.exe)");
        if (SUCCEEDED(pfd->Show(hwnd))) {
            IShellItem* psi = nullptr;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR pszPath = nullptr;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                    result = pszPath;
                    CoTaskMemFree(pszPath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return result;
}

void SetupT7MP(HWND hwnd) {
    std::wstring folder = BrowseForFolder(hwnd);
    if (folder.empty()) return;

    std::wstring fullExe = folder + L"\\BlackOps3_fix.exe";
    if (_waccess_s(fullExe.c_str(), 0) == 0) {
        t7mpGamePath = folder;
        boMods[2].currentPath = folder;
        MessageBoxW(hwnd, L"Folder set successfully! You can now PLAY.", L"Setup Complete", MB_OK | MB_ICONINFORMATION);
        UpdateButtonsAndSetup();
    }
    else {
        MessageBoxW(hwnd, L"BlackOps3_fix.exe not found!\nSelect correct folder.", L"Invalid Folder", MB_OK | MB_ICONERROR);
    }
}

void LaunchSelected() {
    const auto& mods = (currentCategory == 0) ? iwMods : boMods;
    int sel = (int)SendMessage(hComboGame, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR) return;
    const auto& mod = mods[sel];

    if (!mod.enabled || (mod.requiresSetup && mod.currentPath.empty())) {
        MessageBoxW(NULL, L"This mod is not ready yet.", L"Not Available", MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::wstring fullPath = (mod.name == L"T7MP Mod") ? t7mpGamePath + L"\\BlackOps3_fix.exe" : mod.exeName;
    std::wstring workingDir = (mod.name == L"T7MP Mod") ? t7mpGamePath : L"";

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessW(fullPath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL,
        workingDir.empty() ? NULL : workingDir.c_str(), &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        MessageBoxW(NULL, (L"Failed to launch " + mod.name + L"\nCheck file location.").c_str(), L"Launch Error", MB_OK | MB_ICONERROR);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"Made By Wyatt Stark!", WS_VISIBLE | WS_CHILD | SS_CENTER,
            0, 20, 800, 40, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"STATIC", L"Category:", WS_VISIBLE | WS_CHILD,
            20, 80, 150, 20, hwnd, NULL, NULL, NULL);

        hComboCat = CreateWindowW(WC_COMBOBOX, NULL,
            CBS_DROPDOWNLIST | WS_VISIBLE | WS_CHILD | WS_VSCROLL,
            20, 105, 250, 300, hwnd, (HMENU)100, NULL, NULL);

        SendMessage(hComboCat, CB_ADDSTRING, 0, (LPARAM)L"IW Mods");
        SendMessage(hComboCat, CB_ADDSTRING, 0, (LPARAM)L"BO Mods");
        SendMessage(hComboCat, CB_SETCURSEL, 0, 0);

        CreateWindowW(L"STATIC", L"Select Game:", WS_VISIBLE | WS_CHILD,
            20, 160, 150, 20, hwnd, NULL, NULL, NULL);

        hComboGame = CreateWindowW(WC_COMBOBOX, NULL,
            CBS_DROPDOWNLIST | WS_VISIBLE | WS_CHILD | WS_VSCROLL,
            20, 185, 250, 400, hwnd, (HMENU)101, NULL, NULL);

        hBtnSetup = CreateWindowW(L"BUTTON", L"Setup Folder", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 240, 250, 40, hwnd, (HMENU)103, NULL, NULL);
        ShowWindow(hBtnSetup, SW_HIDE);

        hBtnPlay = CreateWindowW(L"BUTTON", L"PLAY", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_DISABLED,
            300, 480, 200, 80, hwnd, (HMENU)102, NULL, NULL);

        // Load background as JPG
        hBackground = (HBITMAP)LoadImageW(NULL, L"background.jpg", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        if (!hBackground) {
            MessageBoxW(NULL, L"background.jpg not found! Place your image as background.jpg next to the exe.", L"Missing Background", MB_OK);
        }

        AddTrayIcon(hwnd);
        UpdateGameCombo();
        break;
    }

    case WM_COMMAND: {
        if (HIWORD(wParam) == CBN_SELCHANGE) {
            if ((HWND)lParam == hComboCat) {
                currentCategory = (int)SendMessage(hComboCat, CB_GETCURSEL, 0, 0);
                UpdateGameCombo();
            }
            else if ((HWND)lParam == hComboGame) {
                UpdateButtonsAndSetup();
            }
        }
        switch (LOWORD(wParam)) {
        case 102: LaunchSelected(); break;
        case 103: SetupT7MP(hwnd); break;
        case ID_TRAY_RESTORE: ShowWindow(hwnd, SW_RESTORE); SetForegroundWindow(hwnd); break;
        case ID_TRAY_EXIT:
            if (MessageBoxW(hwnd, L"Are you sure you want to exit?", L"Exit", MB_YESNO | MB_ICONQUESTION) == IDYES)
                DestroyWindow(hwnd);
            break;
        case ID_TRAY_ALWAYSONTOP:
            alwaysOnTop = !alwaysOnTop;
            SetWindowPos(hwnd, alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            break;
        }
        break;
    }

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        break;

    case WM_SYSICON:
        if (lParam == WM_RBUTTONDOWN) {
            ShowContextMenu(hwnd);
        }
        else if (lParam == WM_LBUTTONDBLCLK) {
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
        }
        break;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (hBackground) {
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hBackground);
            BITMAP bm;
            GetObject(hBackground, sizeof(bm), &bm);
            StretchBlt(hdc, 0, 0, 800, 600, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
            SelectObject(hdcMem, hOld);
            DeleteDC(hdcMem);
        }
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        if (hBackground) DeleteObject(hBackground);
        RemoveTrayIcon();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    InitCommonControls();

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"WyattLauncher";
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"WyattLauncher", L"Wyatt Stark's COD Mod Launcher",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 820, 640, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();
    return 0;
}
