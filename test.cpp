#include "MSG_box.h"

#include "test_img.h"

int main()
{
  char * img = new char [width*height*3];
  const char * data = header_data;

  for (int y = 0; y < height; y++)
    for (int x = 0; x < width; x++)
      HEADER_PIXEL(data, (&img[(y*width+x)*3]));

  MSG_ShowSimpleImageBox("Title äöß", width, height, img);
}

