#ifndef PRISM_FIXTURE_H
#define PRISM_FIXTURE_H
#include <windows.h>
#include <stddef.h>
/* Checked against include/prism/abi.hpp contract v1, not a private ABI copy. */
_Static_assert(sizeof(MSG)==48 && offsetof(MSG,wParam)==16 && offsetof(MSG,pt)==36, "MSG v1");
_Static_assert(sizeof(WNDCLASSEXW)==80 && offsetof(WNDCLASSEXW,lpszClassName)==64, "WNDCLASS v1");
_Static_assert(sizeof(CREATESTRUCTW)==80 && offsetof(CREATESTRUCTW,lpszName)==56, "CREATESTRUCT v1");
_Static_assert(sizeof(BITMAPINFOHEADER)==40 && offsetof(BITMAPINFOHEADER,biCompression)==16, "DIB v1");
_Static_assert(sizeof(RECT)==16 && sizeof(LARGE_INTEGER)==8 && sizeof(WCHAR)==2 && sizeof(LONG)==4, "widths v1");
#define WIDTH 320
#define HEIGHT 240
#endif
