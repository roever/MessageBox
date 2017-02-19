#ifndef _MSG_box_h_
#define _MSG_box_h_

#include <stdint.h>

/** open a simple message box containing an RGB image you provide.
  * This is intended for applications that otherwise need no usual GUI, like games
  * that only try to open an OpenGL Window and have a problem when that fails.
  * In this circumstances it might be nice to display a simple window stating
  * the error.
  *
  * The function doesn't handle text font and buttons, all it does is display the
  * image. You have to provide the contents... which games usually can because
  * they need text handling anyway... just render into a RAM-buffer.
  *
  * The displayed image should contain something "press any key", because it
  * closes on any keypress or mouse-click inside the window. That is when the
  * function will return.
  *
  * Only one box may be open at a time.
  *
  * \param title utf8 encoded string with the title that the window should have
  * \param w width of the image to display must be divisible by 4
  * \param h height of the image to display
  * \param img RGB encoded image, each line of the image must contain w*3 bytes
  *    making all in all w*h*3 bytes, first byte is red value of first (top left)
  *    pixel, followed by green and blue.
  *
  * \return 0, when box was shown, -1 when not
  */
int MSG_ShowSimpleImageBox(const char * title, int w, int h, const char * img);

#endif
