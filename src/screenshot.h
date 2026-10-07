#pragma once

#include <windows.h>
#include <objidl.h>

#ifdef __cplusplus
extern "C" {
#endif

bool SaveHBITMAPToPNGStream(HBITMAP hBitmap, IStream* pStream);
HBITMAP TakeScreenshot();

#ifdef __cplusplus
}
#endif