# MessageBox
Simple Message Boxes

This is intended for applications that usually open an OpenGL window or something else that
might fail and that want to display at least a little bit of information for the user so that
he knows why his program doesn't work

Is is robbed from SDL... and heavily modified. Instead of real dialogs, it only provides the
possibility to display an rgb image. This allows us to get rid of all the font and utf8 handling.

Intended usage is that the application does contain a prepared image for that case that is then
displayed...

It will not properly handle multi-screen on X11. The message box will be on wrong display.
But who cares this is just for emergency cases.

If we need another platform then steal it from SDL :) Thanks guys

##Usage

To use simply include the right source file into your project and include the header file where you
want to open the window...

You have to provide the image so all text processing or whatever you need to create the image is
up to you. You might consider having some pre-rendered images ready to display.


