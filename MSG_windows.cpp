#include "MSG_box.h"

#define UNICODE

#include <windows.h>

#include <memory>

// taken from clang

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
 * left as-is for anyone who may want to do such conversion, which was
 * allowed in earlier algorithms.
 */
static const char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/*
 * Magic values subtracted from a buffer value during UTF8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence.
 */
static const uint32_t offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL,
                     0x03C82080UL, 0xFA082080UL, 0x82082080UL };

static const uint32_t halfBase = 0x0010000UL;
static const int halfShift  = 10; /* used for shifting by 10 bits */
static const uint32_t halfMask = 0x3FFUL;

#define UNI_MAX_BMP (uint32_t)0x0000FFFF
#define UNI_SUR_HIGH_START  (uint32_t)0xD800
#define UNI_SUR_HIGH_END    (uint32_t)0xDBFF
#define UNI_SUR_LOW_START   (uint32_t)0xDC00
#define UNI_SUR_LOW_END     (uint32_t)0xDFFF
#define UNI_MAX_UTF16 (uint32_t)0x0010FFFF
#define UNI_REPLACEMENT_CHAR (uint32_t)0x0000FFFD

/*
 * Utility routine to tell whether a sequence of bytes is legal UTF-8.
 * This must be called with the length pre-determined by the first byte.
 * If not calling this from ConvertUTF8to*, then the length can be set by:
 *  length = trailingBytesForUTF8[*source]+1;
 * and the sequence is illegal right away if there aren't that many bytes
 * available.
 * If presented with a length > 4, this returns false.  The Unicode
 * definition of UTF-8 goes up to 4-byte sequences.
 */

static bool isLegalUTF8(const uint8_t * source, int length)
{
  uint8_t a;
  const uint8_t *srcptr = source+length;

  switch (length)
  {
    default: return false;
      /* Everything else falls through when "true"... */
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 2: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;

      switch (*source)
      {
        /* no fall-through in this inner switch */
        case 0xE0: if (a < 0xA0) return false; break;
        case 0xED: if (a > 0x9F) return false; break;
        case 0xF0: if (a < 0x90) return false; break;
        case 0xF4: if (a > 0x8F) return false; break;
        default:   if (a < 0x80) return false;
      }

    case 1: if (*source >= 0x80 && *source < 0xC2) return false;
  }

  if (*source > 0xF4)
    return false;

  return true;
}

static void ConvertUTF8toUTF16(
    const uint8_t* sourceStart, const uint8_t* sourceEnd,
    wchar_t* targetStart, wchar_t* targetEnd)
{
     const uint8_t * source = sourceStart;
     wchar_t * target = targetStart;
     while (source < sourceEnd)
     {
         uint32_t ch = 0;
         unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
         if (extraBytesToRead >= sourceEnd - source)
         {
             break;
         }
         /* Do this check whether lenient or strict */
         if (!isLegalUTF8(source, extraBytesToRead+1))
         {
             break;
         }
         /*
          * The cases all fall through. See "Note A" below.
          */
         switch (extraBytesToRead)
         {
             case 5: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
             case 4: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
             case 3: ch += *source++; ch <<= 6;
             case 2: ch += *source++; ch <<= 6;
             case 1: ch += *source++; ch <<= 6;
             case 0: ch += *source++;
         }
         ch -= offsetsFromUTF8[extraBytesToRead];

         if (target >= targetEnd)
         {
           break;
         }

         if (ch <= UNI_MAX_BMP)
         { /* Target is a character <= 0xFFFF */
             /* UTF-16 surrogate values are illegal in UTF-32 */
           if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)
           {
             *target++ = UNI_REPLACEMENT_CHAR;
           }
           else
           {
             *target++ = (wchar_t)ch; /* normal case */
           }
         }
         else if (ch > UNI_MAX_UTF16)
         {
           *target++ = UNI_REPLACEMENT_CHAR;
         }
         else
         {
           /* target is a character in range 0xFFFF - 0x10FFFF. */
           if (target + 1 >= targetEnd)
           {
             break;
           }
           ch -= halfBase;
           *target++ = (wchar_t)((ch >> halfShift) + UNI_SUR_HIGH_START);
           *target++ = (wchar_t)((ch & halfMask) + UNI_SUR_LOW_START);
         }
     }
 }

static const char * image;
static int   image_w;
static int   image_h;
static WPARAM last_key_pressed;

// The function to draw our image to the display (the given DC is the screen DC)
static void drawImage(HDC screen)
{
    BITMAPINFO info;
    ZeroMemory(&info, sizeof(BITMAPINFO));
    info.bmiHeader.biBitCount = 24;
    info.bmiHeader.biWidth = image_w;
    info.bmiHeader.biHeight = -image_h;  // flip image
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biSizeImage = image_w*image_h*3;
    info.bmiHeader.biCompression = BI_RGB;

    // we need to swap red and blue, so make a copy
    std::unique_ptr<char[]> dat(new char[image_w*image_h*3]);

    for (int y = 0; y < image_h; y++)
      for (int x = 0; x < image_w; x++)
      {
        dat[(y*image_w+x)*3+0] = image[(y*image_w+x)*3+2];
        dat[(y*image_w+x)*3+1] = image[(y*image_w+x)*3+1];
        dat[(y*image_w+x)*3+2] = image[(y*image_w+x)*3+0];
      }

    StretchDIBits(screen, 0, 0, image_w, image_h, 0, 0, image_w, image_h, dat.get(), &info, DIB_RGB_COLORS, SRCCOPY);
}

// A callback to handle Windows messages as they happen
static LRESULT CALLBACK wndProc(HWND wnd,UINT msg,WPARAM w,LPARAM l)
{
    // what kind of message is this?
    switch(msg)
    {
        // we are interested in WM_PAINT, as that is how we draw
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC screen = BeginPaint(wnd,&ps);   // Get the screen DC
            drawImage(screen);                  // draw our image to our screen DC
            EndPaint(wnd,&ps);                  // clean up
        }
        break;

    case WM_KEYDOWN:
        last_key_pressed = w;
        break;

    case WM_KEYUP:
        if (w == last_key_pressed)
        {
            PostQuitMessage(0);
            return 0;
        }

        break;

    case WM_LBUTTONUP:
        PostQuitMessage(0);
        return 0;

    case WM_CLOSE:
        DestroyWindow(wnd);
        return 0;

        // we are also interested in the WM_DESTROY message, as that lets us know when to close the window
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // for everything else, let the default window message handler do its thing
    return DefWindowProc(wnd,msg,w,l);
}


// A function to create the window and get it set up
static HWND createWindow(const char * title)
{
    WNDCLASSEX wc = {0};        // create a WNDCLASSEX struct and zero it
    wc.cbSize =         sizeof(WNDCLASSEX);     // tell windows the size of this struct
    wc.hCursor =        LoadCursor(0, IDC_ARROW);        // tell it to use the normal arrow cursor for this window
    wc.lpfnWndProc =    wndProc;                // tell it to use our wndProc function to handle messages
    wc.lpszClassName =  TEXT("DisplayImage");   // give this window class a name.

    RegisterClassEx(&wc);           // register our window class with Windows

    // the style of the window we want... we want a normal window but do not want it resizable.
    int style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;    // normal overlapped window with a caption and a system menu (the X to close)

    // Figure out how big we need to make the window so that the CLIENT area (the part we will be drawing to) is
    // the desired size
    RECT rc = {0, 0, image_w, image_h};      // desired rect
    AdjustWindowRect(&rc,style,FALSE);              // adjust the rect with the given style, FALSE because there is no menu

    const uint8_t *t_start = (const uint8_t *)title;
    const uint8_t *t_end = t_start + strlen(title) + 1;

    wchar_t utf16title[strlen(title)*2];

    wchar_t *x_start = utf16title;
    wchar_t *x_end = utf16title + strlen(title)*2;

    ConvertUTF8toUTF16(t_start, t_end, x_start, x_end);

    return CreateWindow(            // create the window
        TEXT("DisplayImage"),  // the name of the window class to use for this window (the one we just registered)
        (LPCWSTR)utf16title, // the text to appear on the title of the window
        style | WS_VISIBLE,         // the style of this window (OR it with WS_VISIBLE so it actually becomes visible immediately)
        100,100,                    // create it at position 100,100
        rc.right - rc.left,         // width of the window we want
        rc.bottom - rc.top,         // height of the window
        NULL,NULL,                  // no parent window, no menu
        0,                          // our program instance
        NULL);                      // no extra parameter

}

int MSG_ShowSimpleImageBox(const char * title, int w, int h, const char * img)
{
    image = img;
    image_h = h;
    image_w = w;

    // create our window
    HWND wnd = createWindow(title);

    // Do the message pump!  keep polling for messages (and respond to them)
    //  until the user closes the window.
    MSG msg;

    while( GetMessage(&msg,wnd,0,0) ) // while we are getting non-WM_QUIT messages...
    {
        TranslateMessage(&msg);     // translate them
        DispatchMessage(&msg);      // and dispatch them (our wndProc will process them)
    }

    return 0;
}

