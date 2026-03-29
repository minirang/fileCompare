#define UNICODE
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <shlobj.h>
#include <string>
#include <filesystem>
#include <fstream>

#pragma comment(lib, "Shell32.lib")

namespace fs = std::filesystem;

std::wstring dir1, dir2;
HWND hEdit;

bool compare_files(const fs::path& f1, const fs::path& f2) {
    if (fs::file_size(f1) != fs::file_size(f2))
        return false;

    std::ifstream a(f1, std::ios::binary);
    std::ifstream b(f2, std::ios::binary);

    char c1, c2;
    while (a.get(c1) && b.get(c2)) {
        if (c1 != c2)
            return false;
    }
    return true;
}

void append(const std::wstring& text) {
    int len = GetWindowTextLength(hEdit);
    SendMessage(hEdit, EM_SETSEL, len, len);
    SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)text.c_str());
}

void compare_dirs(const fs::path& d1, const fs::path& d2) {
    for (auto& entry : fs::recursive_directory_iterator(d1)) {
        if (fs::is_directory(entry)) continue;
        fs::path rel = fs::relative(entry.path(), d1);
        fs::path p2 = d2 / rel;

        if (!fs::exists(p2)) {
            append(L"[폴더1에만 있음] " + rel.wstring() + L"\r\n");
        }
        else if (fs::is_regular_file(entry) && fs::is_regular_file(p2)) {
            if (!compare_files(entry.path(), p2)) {
                append(L"[내용 다름] " + rel.wstring() + L"\r\n");
            }
        }
    }

    for (auto& entry : fs::recursive_directory_iterator(d2)) {
        if (fs::is_directory(entry)) continue;
        fs::path rel = fs::relative(entry.path(), d2);
        fs::path p1 = d1 / rel;

        if (!fs::exists(p1)) {
            append(L"[폴더2에만 있음] " + rel.wstring() + L"\r\n");
        }
    }
}

std::wstring pick_folder(HWND hwnd) {
    BROWSEINFO bi = { 0 };
    bi.hwndOwner = hwnd;
    bi.lpszTitle = L"폴더 선택";

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl) {
        wchar_t path[MAX_PATH];
        SHGetPathFromIDList(pidl, path);
        return path;
    }
    return L"";
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateWindow(L"BUTTON", L"폴더1", WS_VISIBLE | WS_CHILD, 10, 10, 100, 30, hwnd, (HMENU)1, NULL, NULL);
        CreateWindow(L"BUTTON", L"폴더2", WS_VISIBLE | WS_CHILD, 120, 10, 100, 30, hwnd, (HMENU)2, NULL, NULL);
        CreateWindow(L"BUTTON", L"비교", WS_VISIBLE | WS_CHILD, 230, 10, 80, 30, hwnd, (HMENU)3, NULL, NULL);
        hEdit = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL, 10, 50, 460, 300, hwnd, NULL, NULL, NULL);
        break;

    case WM_SIZE:
        if (hEdit) {
            MoveWindow(hEdit, 10, 50, LOWORD(lParam) - 20, HIWORD(lParam) - 60, TRUE);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            dir1 = pick_folder(hwnd);
            append(L"[폴더1] " + dir1 + L"\r\n");
        }
        else if (LOWORD(wParam) == 2) {
            dir2 = pick_folder(hwnd);
            append(L"[폴더2] " + dir2 + L"\r\n");
        }
        else if (LOWORD(wParam) == 3) {
            SetWindowText(hEdit, L"");
            if (!dir1.empty() && !dir2.empty()) {
                append(L"비교 시작...\r\n");
                compare_dirs(dir1, dir2);
                append(L"\r\n완료.\r\n");
            }
            else {
                append(L"폴더를 먼저 선택하세요.\r\n");
            }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"MyApp";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"폴더 비교기",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 420,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
