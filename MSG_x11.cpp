#include "MSG_box.h"

#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <string.h>

static Display *display;
static int screen;
static Window window;

static Atom wm_protocols;
static Atom wm_delete_message;

static const char * image;
static int image_w;
static int image_h;
static const char * title;

static XImage ximage;

/* Create and set up our X11 dialog box indow. */
static int X11_MessageBoxCreateWindow(void)
{
  int x, y;
  XSizeHints *sizehints;
  XSetWindowAttributes wnd_attr;
  Atom _NET_WM_NAME, UTF8_STRING;

  screen = DefaultScreen(display);

  wnd_attr.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask;

  window = XCreateWindow(
      display, RootWindow(display, screen),
      0, 0, image_w, image_h,
      0, CopyFromParent, InputOutput, CopyFromParent,
      CWEventMask, &wnd_attr );

  if (window == None)
  {
    return -1;
  }

  XStoreName(display, window, title);
  _NET_WM_NAME = XInternAtom(display, "_NET_WM_NAME", False);
  UTF8_STRING = XInternAtom(display, "UTF8_STRING", False);
  XChangeProperty(display, window, _NET_WM_NAME, UTF8_STRING, 8,
      PropModeReplace, (unsigned char *)title,
      strlen(title) + 1 );

  /* Allow the window to be deleted by the window manager */
  wm_protocols = XInternAtom( display, "WM_PROTOCOLS", False );
  wm_delete_message = XInternAtom( display, "WM_DELETE_WINDOW", False );
  XSetWMProtocols(display, window, &wm_delete_message, 1);

  x = ( DisplayWidth( display, screen ) -  image_w ) / 2;
  y = ( DisplayHeight( display, screen ) - image_h ) / 3 ;
  XMoveWindow( display, window, x, y );

  sizehints = XAllocSizeHints();
  if (sizehints)
  {
    sizehints->flags = PMaxSize | PMinSize;

    sizehints->min_width  = sizehints->max_width  = image_w;
    sizehints->min_height = sizehints->max_height = image_h;

    XSetWMNormalHints( display, window, sizehints );

    XFree( sizehints );
  }

  ximage.width =  image_w;
  ximage.height = image_h;
  ximage.xoffset = 0;
  ximage.format = ZPixmap;
  ximage.data = (char*)image;  /// hae do we need to copy TODO
  ximage.byte_order = MSBFirst;
  ximage.bitmap_pad = 32;   /* 8, 16, 32 either XY or ZPixmap */
  ximage.depth = 24;        /* depth of image */
  ximage.bytes_per_line = image_w*3;
  ximage.bits_per_pixel = 24;  /* bits per pixel (ZPixmap) */

  Status st = XInitImage(&ximage);

  if (st == 0)
  {
    // failed to create image
    XWithdrawWindow(display, window, screen);
    XDestroyWindow(display, window);
    window = None;

    return -1;
  }

  XMapRaised(display, window);

  return 0;
}

/* Loop and handle message box event messages until something kills it. */
static int X11_MessageBoxLoop(void)
{
  bool close_dialog = false;
  KeySym last_key_pressed = XK_VoidSymbol;

  GC ctx = XCreateGC(display, window, 0, 0);
  if (ctx == None)
  {
    return -1; // Couldn't create graphics context
  }

  while(!close_dialog)
  {
    XEvent e;
    bool draw = true;

    XNextEvent(display, &e);

    /* If X11_XFilterEvent returns True, then some input method has filtered the
       event, and the client should discard the event. */
    if ((e.type != Expose) && XFilterEvent(&e, None))
      continue;

    switch( e.type )
    {
      case Expose:
        if ( e.xexpose.count > 0 )
        {
          draw = false;
        }
        break;

      case ClientMessage:
        if (e.xclient.message_type == wm_protocols &&
            e.xclient.format == 32 &&
            (Atom)(e.xclient.data.l[ 0 ]) == wm_delete_message )
        {
          close_dialog = true;
        }
        break;

      case KeyPress:
        /* Store key press - we make sure in key release that we got both. */
        last_key_pressed = XLookupKeysym( &e.xkey, 0 );
        break;

      case KeyRelease:
        /* If this is a key release for something we didn't get the key down for, then bail. */
        if (XLookupKeysym(&e.xkey, 0) == last_key_pressed)
          close_dialog = true;
        break;

      case ButtonRelease:
        /* If button is released over the same button that was clicked down on, then return it. */
        if (e.xbutton.button == Button1)
          close_dialog = true;
        break;
    }

    /* Draw our dialog box. */
    if (draw)
      XPutImage(display, window, ctx, &ximage, 0, 0, 0, 0, ximage.width, ximage.height);
  }

  XFreeGC(display, ctx);
  return 0;
}

/* Display an x11 message box. */
int MSG_ShowSimpleImageBox(const char * tit, int w, int h, const char * img)
{
  int ret;

  /* This code could get called from multiple threads maybe? */
  XInitThreads();

  image_w = w;
  image_h = h;
  image = img;
  title = tit;

  display = XOpenDisplay( NULL );
  if (!display)
  {
    return -1;
  }

  ret = X11_MessageBoxCreateWindow();

  if ( ret != -1 ) {
    ret = X11_MessageBoxLoop();
  }

  if (window != None) {
    XWithdrawWindow(display, window, screen);
    XDestroyWindow(display, window);
    window = None;
  }

  XCloseDisplay(display);
  display = 0;

  return ret;
}

