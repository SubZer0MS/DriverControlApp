#pragma once

#include "resource.h"
#include "CDriverControl.h"

#ifdef _UNICODE
#define CommandLineToArgv CommandLineToArgvW
#else
#define CommandLineToArgv CommandLineToArgvA
#endif

#define MAX_LOADSTRING 100

#define MAX_TEXTBOX_LENGTH 4
#define MAX_SIZEBOX_LENGTH 100
#define CONTROL_OFFSET 10
#define BUTTON_HEIGHT 25
#define BUTTON_WIDTH 80

// Global Variables:
HINSTANCE hInst;                                // current instance
TCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hWndTextboxTag;                            // the text input text box for the tag
HWND hWndTextboxSize;                           // the text input text box for the size
CDriverControl* pDriver;                        // the driver used

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance);

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE, int);

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Message handler for about box.
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
