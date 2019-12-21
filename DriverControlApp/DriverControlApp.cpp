#include "framework.h"
#include "DriverControlApp.h"
#include "CDriverControl.h"
#include "Utils.h"
#include <shellapi.h>
#include "Args.h"

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR lpCmdLine,
                     _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    int argc;
    LPTSTR* pszCmdLine = CommandLineToArgv(GetCommandLine(), &argc);

    ArgsHelper argsHelper;
    argsHelper.usage(_T("Automatically starts driver and performs either an alloc or a free for paged or non-paged pool memory."));
    argsHelper.arg(_T("h"), _T("help"), _T("Displays the help for available arguments."), false);
    argsHelper.arg(_T("?"), _T(""), _T("Displays the help for available arguments."), false);
    argsHelper.arg(_T("u"), _T("unload"), _T("Unloads the used driver (AppKernelDriver) if it has been previously loaded."), false);
    argsHelper.arg(_T("t"), _T("type"), _T("Pool type to allocate/free. Available values are \"pp\" for Paged Pool amd \"npp\" for NonPaged Pool."), true);
    argsHelper.arg(_T("o"), _T("operation"), _T("Operation to perform, allocate or free. Available values are \"alloc\" to allocate and \"free\" to free."), false);
    argsHelper.arg(_T("s"), _T("size"), _T("Size in MB to allocate"), true);
    
    if (!argsHelper.parseArgs(argc, pszCmdLine) || argsHelper.hasError())
    {
        utils::PrintInfo(argsHelper.error().c_str());
    }

    LocalFree(pszCmdLine);

    if (argsHelper.exists(_T("h")) || argsHelper.exists(_T("?")))
    {
        utils::PrintInfo(argsHelper.help().c_str());


        // WORK IN PROGRESS FOR DOING ACTIONS WITH THE CMDLINE
        return 0;
    }

    TCHAR szResourceFilePath[MAX_PATH];
    utils::ExtractAndGetDriverFullPathName(*szResourceFilePath);

    pDriver = new CDriverControl(
        szResourceFilePath,
        _T("AppKernelDriver"),
        _T("AppKernelDriver for Testing"),
        SERVICE_DEMAND_START
    );

    DWORD dwResult = pDriver->CreateDrv();
    if (dwResult != DRV_SUCCESS)
    {
        utils::PrintError(_T(__FUNCTION__), _T("CreateDrv"));
        pDriver->UnloadDrv();
        return FALSE;
    }

    dwResult = pDriver->StartDrv();
    if (dwResult != DRV_SUCCESS)
    {
        utils::PrintError(_T(__FUNCTION__), _T("StartDrv"));
        pDriver->UnloadDrv();
        return FALSE;
    }

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_DRIVERCONTROLAPP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DRIVERCONTROLAPP));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, NULL, NULL))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DRIVERCONTROLAPP));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DRIVERCONTROLAPP);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindow(
       szWindowClass,
       szTitle,
       WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,
       CW_USEDEFAULT,
       NULL,
       BUTTON_WIDTH * 7 + 10,
       BUTTON_HEIGHT + 80,
       nullptr,
       nullptr,
       hInstance,
       nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        hWndTextboxTag = CreateWindow(
            _T("edit"),
            _T("Leak"),
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            CONTROL_OFFSET,
            CONTROL_OFFSET,
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            hWnd,
            NULL,
            hInst,
            NULL);

        SendMessage(hWndTextboxTag, EM_LIMITTEXT, MAX_TEXTBOX_LENGTH, NULL);

        hWndTextboxSize = CreateWindow(
            _T("edit"),
            _T("102400"),
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            CONTROL_OFFSET * 2 + BUTTON_WIDTH,
            CONTROL_OFFSET,
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            hWnd,
            NULL,
            hInst,
            NULL);

        CreateWindow(
            _T("button"),
            _T("Leak PP"),
            WS_CHILD | WS_VISIBLE,
            CONTROL_OFFSET * 3 + BUTTON_WIDTH * 2,
            CONTROL_OFFSET,
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            hWnd,
            (HMENU)IDC_BUTTON_ALLOC_PP,
            hInst,
            NULL);

        CreateWindow(
            _T("button"),
            _T("Free PP"),
            WS_CHILD | WS_VISIBLE,
            CONTROL_OFFSET * 4 + BUTTON_WIDTH * 3,
            CONTROL_OFFSET,
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            hWnd,
            (HMENU)IDC_BUTTON_FREE_PP,
            hInst,
            NULL);

        CreateWindow(
            _T("button"),
            _T("Leak NPP"),
            WS_CHILD | WS_VISIBLE,
            CONTROL_OFFSET * 5 + BUTTON_WIDTH * 4,
            CONTROL_OFFSET,
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            hWnd,
            (HMENU)IDC_BUTTON_ALLOC_NPP,
            hInst,
            NULL);

        CreateWindow(
            _T("button"),
            _T("Free NPP"),
            WS_CHILD | WS_VISIBLE,
            CONTROL_OFFSET * 6 + BUTTON_WIDTH * 5,
            CONTROL_OFFSET,
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            hWnd,
            (HMENU)IDC_BUTTON_FREE_NPP,
            hInst,
            NULL);        

        SetFocus(hWndTextboxTag);
        break;

    case WM_COMMAND:
        {
            TCHAR pTagBuffer[MAX_TEXTBOX_LENGTH + 1];
            GetWindowText(hWndTextboxTag, pTagBuffer, sizeof(pTagBuffer));

            TCHAR pSizeBuffer[MAX_TEXTBOX_LENGTH + 1];
            GetWindowText(hWndTextboxTag, pSizeBuffer, sizeof(pSizeBuffer));

            int wmId = LOWORD(wParam);

            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;

            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;

            case IDC_BUTTON_ALLOC_PP:
                if (lstrlen(pTagBuffer) == MAX_TEXTBOX_LENGTH && lstrlen(pSizeBuffer) > 0)
                {
                    LONG size = _ttol(pSizeBuffer);
                    if (size != 0L)
                    {
                        pDriver->DoAllocatePool(1, (ULONG)size * 1000, pTagBuffer);
                    }
                    else
                    {
                        utils::PrintInfo(_T("Size (MB) field needs to contain only digits (at least 1)."));
                    }
                }
                else
                {
                    utils::PrintInfo(_T("Tag needs to be at 4 characters long and size (MB) needs to have at least 1 digit."));
                }
                break;

            case IDC_BUTTON_FREE_PP:
                if (lstrlen(pTagBuffer) == MAX_TEXTBOX_LENGTH)
                {
                    pDriver->DoFreePool(1, pTagBuffer);
                }
                else
                {
                    utils::PrintInfo(_T("Tag needs to be at 4 characters long and size needs to have at least 1 digit."));
                }
                break;

            case IDC_BUTTON_ALLOC_NPP:
                if (lstrlen(pTagBuffer) == MAX_TEXTBOX_LENGTH && lstrlen(pSizeBuffer) > 0)
                {
                    LONG size = _ttol(pSizeBuffer);
                    if (size != 0L)
                    {
                        pDriver->DoAllocatePool(0, (ULONG)size * 1000, pTagBuffer);
                    }
                    else
                    {
                        utils::PrintInfo(_T("Size (MB) field needs to contain only digits (at least 1)."));
                    }
                }
                else
                {
                    utils::PrintInfo(_T("Tag needs to be at 4 characters long and size needs to have at least 1 digit."));
                }
                break;

            case IDC_BUTTON_FREE_NPP:
                if (lstrlen(pTagBuffer) == MAX_TEXTBOX_LENGTH)
                {
                    pDriver->DoFreePool(0, pTagBuffer);
                }
                else
                {
                    utils::PrintInfo(_T("Tag needs to be at 4 characters long and size needs to have at least 1 digit."));
                }
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DESTROY:
        if (pDriver)
        {
            pDriver->UnloadDrv();
        }

        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }

    return (INT_PTR)FALSE;
}
