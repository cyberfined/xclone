#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include "dsimple.h"

int create_window_copy(Display *display, Window src, Window *ret_window, GLXContext *ret_context) {
  Window root;
  int dummyi;
  unsigned int width, height, dummyu;
  
  if(!XGetGeometry(display, src, &root, &dummyi, &dummyi, &width, &height, &dummyu, &dummyu)) {
    fputs("failed to gen window geometry", stderr);
    return 0;
  }

  root = RootWindow(display, 0);
  
  int gl_attribs[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
  XVisualInfo *vinfo = glXChooseVisual(display, 0, gl_attribs);
  if(vinfo == NULL) {
    fputs("no appropriate visual found", stderr);
    return 0;
  }
  
  XSetWindowAttributes win_attribs = {
    .background_pixel = WhitePixel(display, 0),
    .border_pixel = BlackPixel(display, 0),
    .event_mask = ExposureMask,
    .colormap = XCreateColormap(display, root, vinfo->visual, AllocNone)
  };

  Window window = XCreateWindow(display,
				root,
				0, 0,
				width, height,
				1,
				vinfo->depth,
				InputOutput,
				vinfo->visual,
				CWBackPixel | CWBorderPixel | CWEventMask | CWColormap,
				&win_attribs);
  
  GLXContext glc = glXCreateContext(display, vinfo, NULL, GL_TRUE);
  glXMakeCurrent(display, window, glc);

  *ret_window = window;
  *ret_context = glc;
  
  return 1;
}

void exposeHandler(Display *display, Window window, Window src, int width, int height, int src_width, int src_height) {
  XImage *image = XGetImage(display, src, 0, 0, src_width, src_height, AllPlanes, ZPixmap);
  GLuint texture;
  
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D,
	       0,
	       GL_RGB,
	       src_width, src_height,
	       0,
	       GL_BGRA,
	       GL_UNSIGNED_INT_8_8_8_8_REV,
	       image->data);
  XDestroyImage(image);
  
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, width, height, 0, -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glBegin(GL_QUADS);

  glTexCoord2f(0.0, 0.0); glVertex2f(0.0, 0.0);
  glTexCoord2f(1.0, 0.0); glVertex2f(width, 0);
  glTexCoord2f(1.0, 1.0); glVertex2f(width, height);
  glTexCoord2f(0.0, 1.0); glVertex2f(0, height);
  
  glEnd();
  glDeleteTextures(1, &texture);
}

int main(int argc, char **argv) {
  Display *display = XOpenDisplay(NULL);
  if(display == NULL) {
    fputs("Failed to connect to X server", stderr);
    exit(EXIT_FAILURE);
  }

  puts("Choose window, which would be copied");
  
  Window src = Select_Window(display, 0);
  Window window;
  GLXContext context;
  if(!create_window_copy(display, src, &window, &context)) {
    exit(EXIT_FAILURE);
  }

  XMapWindow(display, window);
  XSelectInput(display, src, ExposureMask | KeyPressMask | KeyReleaseMask);
  XSelectInput(display, window, ExposureMask);

  glEnable(GL_TEXTURE_2D);

  XEvent event;
  XWindowAttributes wa, sa;
  for(;;) {
    XNextEvent(display, &event);

    if(event.type == Expose) {
      XGetWindowAttributes(display, window, &wa);
      XGetWindowAttributes(display, src, &sa);
      glViewport(0, 0, wa.width, wa.height);
    }
    
    exposeHandler(display, window, src, wa.width, wa.height, sa.width, sa.height);
    glXSwapBuffers(display, window);
  }
}
