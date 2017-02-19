# MessageBox
Simple Message Boxes

This is intended for applications that usually open an OpenGL window or something else that
might fail and that want to display at least a little bit of information for the user so that
he knows why it program doesn't work

Is is robbed from SDL... and heavily modified. Instead of real dialogs, it only provides the 
possibility to display an rgb image. This allows us to get rid of all the font and utf8 handling.

Intended usage is that the application does contain a prepared image for that case that is then
displayed...

It will not properly handle multi-window on X11. The message box will be on wrong display.
But who cares this is just for emergency cases.

If we need another platform then steal it from SDL :) Thanks guys

