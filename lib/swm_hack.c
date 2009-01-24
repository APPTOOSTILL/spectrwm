/* $scrotwm$ */
/*
 * Copyright (c) 2009 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2009 Ryan McBride <mcbride@countersiege.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Copyright (C) 2005-2007 Carsten Haitzler
 * Copyright (C) 2006-2007 Kim Woelders
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * Basic hack mechanism (dlopen etc.) taken from e_hack.c in e17.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xatom.h>

/* dlopened xlib so we can find the symbols in the real xlib to call them */
static void        *lib_xlib = NULL;

static Window       root = None;

/* Find our root window */
static              Window
MyRoot(Display * dpy)
{
	char               *s;

	if (root != None)
		return root;

	root = DefaultRootWindow(dpy);

	s = getenv("ENL_WM_ROOT");
	if (!s)
		return root;

	sscanf(s, "%lx", &root);
	return root;
}

#define SWM_PROPLEN	(16)
void
set_property(Display *dpy, Window id, char *name, char *val)
{
	Atom			atom = 0;
	char			prop[SWM_PROPLEN];

	/* Try to update the window's workspace property */
	atom = XInternAtom(dpy, name, False);
	if (atom) 
		if (snprintf(prop, SWM_PROPLEN, "%s", val) < SWM_PROPLEN)
			XChangeProperty(dpy, id, atom, XA_STRING,
			    8, PropModeReplace, prop, SWM_PROPLEN);
}

typedef             Window(CWF) (Display * _display, Window _parent, int _x,
				 int _y, unsigned int _width,
				 unsigned int _height,
				 unsigned int _border_width, int _depth,
				 unsigned int _class, Visual * _visual,
				 unsigned long _valuemask,
				 XSetWindowAttributes * _attributes);

/* XCreateWindow intercept hack */
Window
XCreateWindow(Display * display, Window parent, int x, int y,
   unsigned int width, unsigned int height,
   unsigned int border_width,
   int depth, unsigned int clss, Visual * visual,
   unsigned long valuemask, XSetWindowAttributes * attributes)
{
	static CWF	*func = NULL;
	char		*env;
	Window		id;

	/* find the real Xlib and the real X function */
	if (!lib_xlib)
		lib_xlib = dlopen("libX11.so", RTLD_GLOBAL | RTLD_LAZY);
	if (!func)
		func = (CWF *) dlsym(lib_xlib, "XCreateWindow");

	if (parent == DefaultRootWindow(display))
		parent = MyRoot(display);
	
	id = (*func) (display, parent, x, y, width, height, border_width,
	    depth, clss, visual, valuemask, attributes);

	if (id) {
		if ((env = getenv("_SWM_WS")) != NULL) 
			set_property(display, id, "_SWM_WS", env);
		if ((env = getenv("_SWM_PID")) != NULL)
			set_property(display, id, "_SWM_PID", env);
	}
	return (id);
}

typedef             Window(CSWF) (Display * _display, Window _parent, int _x,
				  int _y, unsigned int _width,
				  unsigned int _height,
				  unsigned int _border_width,
				  unsigned long _border,
				  unsigned long _background);

/* XCreateSimpleWindow intercept hack */
Window
XCreateSimpleWindow(Display * display, Window parent, int x, int y,
    unsigned int width, unsigned int height,
    unsigned int border_width,
    unsigned long border, unsigned long background)
{
	static CSWF	*func = NULL;
	char		*env;
	Window		id;

	/* find the real Xlib and the real X function */
	if (!lib_xlib)
		lib_xlib = dlopen("libX11.so", RTLD_GLOBAL | RTLD_LAZY);
	if (!func)
		func = (CSWF *) dlsym(lib_xlib, "XCreateSimpleWindow");

	if (parent == DefaultRootWindow(display))
		parent = MyRoot(display);

	id = (*func) (display, parent, x, y, width, height,
	    border_width, border, background);

	if (id) {
		if ((env = getenv("_SWM_WS")) != NULL) 
			set_property(display, id, "_SWM_WS", env);
		if ((env = getenv("_SWM_PID")) != NULL)
			set_property(display, id, "_SWM_PID", env);
	}
	return (id);
}

typedef int         (RWF) (Display * _display, Window _window, Window _parent,
			   int x, int y);

/* XReparentWindow intercept hack */
int
XReparentWindow(Display * display, Window window, Window parent, int x, int y)
{
	static RWF         *func = NULL;

	/* find the real Xlib and the real X function */
	if (!lib_xlib)
		lib_xlib = dlopen("libX11.so", RTLD_GLOBAL | RTLD_LAZY);
	if (!func)
		func = (RWF *) dlsym(lib_xlib, "XReparentWindow");

	if (parent == DefaultRootWindow(display))
		parent = MyRoot(display);

	return (*func) (display, window, parent, x, y);
}
