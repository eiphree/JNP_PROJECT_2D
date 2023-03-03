#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

void InitDirect2D(HWND hwnd);

void DestroyDirect2D();

void OnPaint(HWND hwnd, INT mouse_clicked, FLOAT mouse_x, FLOAT mouse_y);