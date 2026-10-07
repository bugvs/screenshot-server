#include "./screenshot.h"
#include <wincodec.h>
#include <gdiplus.h>
#include <memory>

using namespace Gdiplus;
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

bool SaveHBITMAPToPNGStream(HBITMAP hBitmap, IStream* pStream) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    bool result = false;
    IWICImagingFactory* pFactory = NULL;
    IWICBitmapEncoder* pEncoder = NULL;
    IWICBitmapFrameEncode* pFrame = NULL;
    IWICStream* pWicStream = NULL;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        GdiplusShutdown(gdiplusToken);
        return false;
    }

    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory,
        (LPVOID*)&pFactory
    );

    if (SUCCEEDED(hr)) {
        hr = pFactory->CreateStream(&pWicStream);
        if (SUCCEEDED(hr)) {
            hr = pWicStream->InitializeFromIStream(pStream);
            if (SUCCEEDED(hr)) {
                hr = pFactory->CreateEncoder(
                    GUID_ContainerFormatPng,
                    NULL,
                    &pEncoder
                );
                if (SUCCEEDED(hr)) {
                    hr = pEncoder->Initialize(pWicStream, WICBitmapEncoderNoCache);
                    if (SUCCEEDED(hr)) {
                        hr = pEncoder->CreateNewFrame(&pFrame, NULL);
                        if (SUCCEEDED(hr)) {
                            hr = pFrame->Initialize(NULL);
                            if (SUCCEEDED(hr)) {
                                Gdiplus::Bitmap bitmap(hBitmap, NULL);
                                UINT width = bitmap.GetWidth();
                                UINT height = bitmap.GetHeight();

                                WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
                                hr = pFrame->SetSize(width, height);
                                hr = pFrame->SetPixelFormat(&format);

                                Gdiplus::BitmapData bitmapData;
                                Gdiplus::Rect rect(0, 0, width, height);
                                if (bitmap.LockBits(
                                    &rect,
                                    Gdiplus::ImageLockModeRead,
                                    PixelFormat32bppARGB,
                                    &bitmapData
                                ) == Gdiplus::Ok) {
                                    hr = pFrame->WritePixels(
                                        height,
                                        bitmapData.Stride,
                                        bitmapData.Stride * height,
                                        static_cast<BYTE*>(bitmapData.Scan0)
                                    );
                                    bitmap.UnlockBits(&bitmapData);
                                    if (SUCCEEDED(hr)) {
                                        hr = pFrame->Commit();
                                        if (SUCCEEDED(hr)) {
                                            hr = pEncoder->Commit();
                                            if (SUCCEEDED(hr)) {
                                                result = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (pFrame) pFrame->Release();
    if (pEncoder) pEncoder->Release();
    if (pWicStream) pWicStream->Release();
    if (pFactory) pFactory->Release();
    
    CoUninitialize();
    GdiplusShutdown(gdiplusToken);
    
    return result;
}

HBITMAP TakeScreenshot() {
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    SelectObject(hdcMem, hBitmap);
    BitBlt(hdcMem, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY);

    ReleaseDC(NULL, hdcScreen);
    DeleteDC(hdcMem);
    return hBitmap;
}