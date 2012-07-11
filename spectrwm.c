/*
 * Copyright (c) 2009-2012 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2009-2011 Ryan McBride <mcbride@countersiege.com>
 * Copyright (c) 2009 Darrin Chandler <dwchandler@stilyagin.com>
 * Copyright (c) 2009 Pierre-Yves Ritschard <pyr@spootnik.org>
 * Copyright (c) 2010 Tuukka Kataja <stuge@xor.fi>
 * Copyright (c) 2011 Jason L. Wright <jason@thought.net>
 * Copyright (c) 2011-2012 Reginald Kennedy <rk@rejii.com>
 * Copyright (c) 2011-2012 Lawrence Teo <lteo@lteo.net>
 * Copyright (c) 2011-2012 Tiago Cunha <tcunha@gmx.com>
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
 * Much code and ideas taken from dwm under the following license:
 * MIT/X Consortium License
 *
 * 2006-2008 Anselm R Garbe <garbeam at gmail dot com>
 * 2006-2007 Sander van Dijk <a dot h dot vandijk at gmail dot com>
 * 2006-2007 Jukka Salmi <jukka at salmi dot ch>
 * 2007 Premysl Hruby <dfenze at gmail dot com>
 * 2007 Szabolcs Nagy <nszabolcs at gmail dot com>
 * 2007 Christof Musik <christof at sendfax dot de>
 * 2007-2008 Enno Gottox Boland <gottox at s01 dot de>
 * 2007-2008 Peter Hartlich <sgkkr at hartlich dot com>
 * 2008 Martin Hurton <martin dot hurton at gmail dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <util.h>
#include <pwd.h>
#include <paths.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/queue.h>
#include <sys/param.h>
#include <sys/select.h>
#if defined(__linux__)
#include "tree.h"
#elif defined(__OpenBSD__)
#include <sys/tree.h>
#elif defined(__FreeBSD__)
#include <sys/tree.h>
#else
#include "tree.h"
#endif

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib-xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/XTest.h>

#ifdef __OSX__
#include <osx.h>
#endif

#include "version.h"

#ifdef SPECTRWM_BUILDSTR
static const char	*buildstr = SPECTRWM_BUILDSTR;
#else
static const char	*buildstr = SPECTRWM_VERSION;
#endif

#if RANDR_MAJOR < 1
#  error XRandR versions less than 1.0 are not supported
#endif

#if RANDR_MAJOR >= 1
#if RANDR_MINOR >= 2
#define SWM_XRR_HAS_CRTC
#endif
#endif

#if defined(__OpenBSD__)
#define xcb_icccm_wm_hints_t			xcb_wm_hints_t
#define xcb_icccm_get_wm_hints			xcb_get_wm_hints
#define xcb_icccm_get_wm_hints_reply		xcb_get_wm_hints_reply
#define XCB_ICCCM_WM_HINT_X_URGENCY		XCB_WM_HINT_X_URGENCY
#define XCB_ICCCM_WM_STATE_ICONIC		XCB_WM_STATE_ICONIC
#define XCB_ICCCM_WM_STATE_WITHDRAWN		XCB_WM_STATE_WITHDRAWN
#define XCB_ICCCM_WM_STATE_NORMAL		XCB_WM_STATE_NORMAL
#define	xcb_icccm_get_wm_name			xcb_get_wm_name
#define xcb_icccm_get_wm_name_reply		xcb_get_wm_name_reply
#define xcb_icccm_get_wm_transient_for		xcb_get_wm_transient_for
#define xcb_icccm_get_wm_transient_for_reply	xcb_get_wm_transient_for_reply
#endif

/*#define SWM_DEBUG*/
#ifdef SWM_DEBUG
#define DPRINTF(x...)		do { if (swm_debug) fprintf(stderr, x); } while (0)
#define DNPRINTF(n,x...)	do { if (swm_debug & n) fprintf(stderr, x); } while (0)
#define SWM_D_MISC		0x0001
#define SWM_D_EVENT		0x0002
#define SWM_D_WS		0x0004
#define SWM_D_FOCUS		0x0008
#define SWM_D_MOVE		0x0010
#define SWM_D_STACK		0x0020
#define SWM_D_MOUSE		0x0040
#define SWM_D_PROP		0x0080
#define SWM_D_CLASS		0x0100
#define SWM_D_KEY		0x0200
#define SWM_D_QUIRK		0x0400
#define SWM_D_SPAWN		0x0800
#define SWM_D_EVENTQ		0x1000
#define SWM_D_CONF		0x2000
#define SWM_D_BAR		0x4000

u_int32_t		swm_debug = 0
			    | SWM_D_MISC
			    | SWM_D_EVENT
			    | SWM_D_WS
			    | SWM_D_FOCUS
			    | SWM_D_MOVE
			    | SWM_D_STACK
			    | SWM_D_MOUSE
			    | SWM_D_PROP
			    | SWM_D_CLASS
			    | SWM_D_KEY
			    | SWM_D_QUIRK
			    | SWM_D_SPAWN
			    | SWM_D_EVENTQ
			    | SWM_D_CONF
			    | SWM_D_BAR
			    ;
#else
#define DPRINTF(x...)
#define DNPRINTF(n,x...)
#endif

#define LENGTH(x)		(sizeof x / sizeof x[0])
#define MODKEY			Mod1Mask
#define CLEANMASK(mask)		(mask & ~(numlockmask | LockMask))
#define BUTTONMASK		(ButtonPressMask|ButtonReleaseMask)
#define MOUSEMASK		(BUTTONMASK|PointerMotionMask)
#define SWM_PROPLEN		(16)
#define SWM_FUNCNAME_LEN	(32)
#define SWM_KEYS_LEN		(255)
#define SWM_QUIRK_LEN		(64)
#define X(r)			(r)->g.x
#define Y(r)			(r)->g.y
#define WIDTH(r)		(r)->g.w
#define HEIGHT(r)		(r)->g.h
#define BORDER(w)		(w->bordered ? border_width : 0)
#define MAX_X(r)		((r)->g.x + (r)->g.w)
#define MAX_Y(r)		((r)->g.y + (r)->g.h)
#define SH_MIN(w)		(w)->sh_mask & PMinSize
#define SH_MIN_W(w)		(w)->sh.min_width
#define SH_MIN_H(w)		(w)->sh.min_height
#define SH_MAX(w)		(w)->sh_mask & PMaxSize
#define SH_MAX_W(w)		(w)->sh.max_width
#define SH_MAX_H(w)		(w)->sh.max_height
#define SH_INC(w)		(w)->sh_mask & PResizeInc
#define SH_INC_W(w)		(w)->sh.width_inc
#define SH_INC_H(w)		(w)->sh.height_inc
#define SWM_MAX_FONT_STEPS	(3)
#define WINID(w)		((w) ? (w)->id : 0)
#define YESNO(x)		((x) ? "yes" : "no")

#define SWM_FOCUS_DEFAULT	(0)
#define SWM_FOCUS_SYNERGY	(1)
#define SWM_FOCUS_FOLLOW	(2)

#define SWM_CONF_DEFAULT	(0)
#define SWM_CONF_KEYMAPPING	(1)

#ifndef SWM_LIB
#define SWM_LIB			"/usr/local/lib/libswmhack.so"
#endif

char			**start_argv;
xcb_atom_t		astate;
xcb_atom_t		aprot;
xcb_atom_t		adelete;
xcb_atom_t		takefocus;
xcb_atom_t		a_wmname;
xcb_atom_t		a_netwmname;
xcb_atom_t		a_utf8_string;
xcb_atom_t		a_string;
xcb_atom_t		a_swm_iconic;
volatile sig_atomic_t   running = 1;
volatile sig_atomic_t   restart_wm = 0;
int			outputs = 0;
int			last_focus_event = FocusOut;
int			(*xerrorxlib)(Display *, XErrorEvent *);
int			other_wm;
int			ss_enabled = 0;
int			xrandr_support;
int			xrandr_eventbase;
unsigned int		numlockmask = 0;
Display			*display;
xcb_connection_t	*conn;

int			cycle_empty = 0;
int			cycle_visible = 0;
int			term_width = 0;
int			font_adjusted = 0;
unsigned int		mod_key = MODKEY;

/* dmenu search */
struct swm_region	*search_r;
int			select_list_pipe[2];
int			select_resp_pipe[2];
pid_t			searchpid;
volatile sig_atomic_t	search_resp;
int			search_resp_action;

struct search_window {
	TAILQ_ENTRY(search_window)	entry;
	int				idx;
	struct ws_win			*win;
	xcb_gcontext_t			gc;
	xcb_window_t			indicator;
};
TAILQ_HEAD(search_winlist, search_window);
struct search_winlist			search_wl;

/* search actions */
enum {
	SWM_SEARCH_NONE,
	SWM_SEARCH_UNICONIFY,
	SWM_SEARCH_NAME_WORKSPACE,
	SWM_SEARCH_SEARCH_WORKSPACE,
	SWM_SEARCH_SEARCH_WINDOW
};

#define SWM_STACK_TOP		(0)
#define SWM_STACK_BOTTOM	(1)
#define	SWM_STACK_ABOVE		(2)
#define	SWM_STACK_BELOW		(3)

/* dialog windows */
double			dialog_ratio = 0.6;
/* status bar */
#define SWM_BAR_MAX		(256)
#define SWM_BAR_JUSTIFY_LEFT	(0)
#define SWM_BAR_JUSTIFY_CENTER	(1)
#define SWM_BAR_JUSTIFY_RIGHT	(2)
#define SWM_BAR_OFFSET		(4)
#define SWM_BAR_FONTS		"-*-terminus-medium-*-*-*-*-*-*-*-*-*-*-*," \
				"-*-profont-*-*-*-*-*-*-*-*-*-*-*-*,"	    \
				"-*-times-medium-r-*-*-*-*-*-*-*-*-*-*,"    \
				"-misc-fixed-medium-r-*-*-*-*-*-*-*-*-*-*,"  \
				"-*-*-*-r-*--*-*-*-*-*-*-*-*"

char			*bar_argv[] = { NULL, NULL };
int			bar_pipe[2];
unsigned char		bar_ext[SWM_BAR_MAX];
char			bar_vertext[SWM_BAR_MAX];
int			bar_version = 0;
sig_atomic_t		bar_alarm = 0;
int			bar_delay = 30;
int			bar_enabled = 1;
int			bar_border_width = 1;
int			bar_at_bottom = 0;
int			bar_extra = 1;
int			bar_extra_running = 0;
int			bar_verbose = 1;
int			bar_height = 0;
int			bar_justify = SWM_BAR_JUSTIFY_LEFT;
char			*bar_format = NULL;
int			stack_enabled = 1;
int			clock_enabled = 1;
int			urgent_enabled = 0;
char			*clock_format = NULL;
int			title_name_enabled = 0;
int			title_class_enabled = 0;
int			window_name_enabled = 0;
int			focus_mode = SWM_FOCUS_DEFAULT;
int			focus_close = SWM_STACK_BELOW;
int			focus_close_wrap = 1;
int			focus_default = SWM_STACK_TOP;
int			spawn_position = SWM_STACK_TOP;
int			disable_border = 0;
int			border_width = 1;
int			verbose_layout = 0;
pid_t			bar_pid;
XFontSet		bar_fs;
XFontSetExtents		*bar_fs_extents;
char			*bar_fonts;
struct passwd		*pwd;

#define SWM_MENU_FN	(2)
#define SWM_MENU_NB	(4)
#define SWM_MENU_NF	(6)
#define SWM_MENU_SB	(8)
#define SWM_MENU_SF	(10)

/* layout manager data */
struct swm_geometry {
	int			x;
	int			y;
	int			w;
	int			h;
};

struct swm_screen;
struct workspace;

struct swm_bar {
	xcb_window_t		id;
	xcb_pixmap_t		buffer;
	struct swm_geometry	g;
};

/* virtual "screens" */
struct swm_region {
	TAILQ_ENTRY(swm_region)	entry;
	struct swm_geometry	g;
	struct workspace	*ws;	/* current workspace on this region */
	struct workspace	*ws_prior; /* prior workspace on this region */
	struct swm_screen	*s;	/* screen idx */
	struct swm_bar		*bar;
};
TAILQ_HEAD(swm_region_list, swm_region);

struct ws_win {
	TAILQ_ENTRY(ws_win)	entry;
	xcb_window_t		id;
	xcb_window_t		transient;
	struct ws_win		*child_trans;	/* transient child window */
	struct swm_geometry	g;		/* current geometry */
	struct swm_geometry	g_float;	/* region coordinates */
	int			g_floatvalid;	/* g_float geometry validity */
	int			floatmaxed;	/* whether maxed by max_stack */
	int			floating;
	int			manual;
	int			iconic;
	int			bordered;
	unsigned int		ewmh_flags;
	int			font_size_boundary[SWM_MAX_FONT_STEPS];
	int			font_steps;
	int			last_inc;
	int			can_delete;
	int			take_focus;
	int			java;
	unsigned long		quirks;
	struct workspace	*ws;	/* always valid */
	struct swm_screen	*s;	/* always valid, never changes */
	XWindowAttributes	wa;
	XSizeHints		sh;
	long			sh_mask;
	XClassHint		ch;
	XWMHints		*hints;
};
TAILQ_HEAD(ws_win_list, ws_win);

/* pid goo */
struct pid_e {
	TAILQ_ENTRY(pid_e)	entry;
	long			pid;
	int			ws;
};
TAILQ_HEAD(pid_list, pid_e);
struct pid_list			pidlist = TAILQ_HEAD_INITIALIZER(pidlist);

/* layout handlers */
void	stack(void);
void	vertical_config(struct workspace *, int);
void	vertical_stack(struct workspace *, struct swm_geometry *);
void	horizontal_config(struct workspace *, int);
void	horizontal_stack(struct workspace *, struct swm_geometry *);
void	max_stack(struct workspace *, struct swm_geometry *);
void	plain_stacker(struct workspace *);
void	fancy_stacker(struct workspace *);

struct ws_win *find_window(xcb_window_t);

void		grabbuttons(struct ws_win *, int);
void		new_region(struct swm_screen *, int, int, int, int);
void		unmanage_window(struct ws_win *);
uint16_t	getstate(xcb_window_t);

int		conf_load(char *, int);

struct layout {
	void		(*l_stack)(struct workspace *, struct swm_geometry *);
	void		(*l_config)(struct workspace *, int);
	u_int32_t	flags;
#define SWM_L_FOCUSPREV		(1<<0)
#define SWM_L_MAPONFOCUS	(1<<1)
	void		(*l_string)(struct workspace *);
} layouts[] =  {
	/* stack,		configure */
	{ vertical_stack,	vertical_config,	0,	plain_stacker },
	{ horizontal_stack,	horizontal_config,	0,	plain_stacker },
	{ max_stack,		NULL,
	  SWM_L_MAPONFOCUS | SWM_L_FOCUSPREV,			plain_stacker },
	{ NULL,			NULL,			0,	NULL  },
};

/* position of max_stack mode in the layouts array, index into layouts! */
#define SWM_V_STACK		(0)
#define SWM_H_STACK		(1)
#define SWM_MAX_STACK		(2)

#define SWM_H_SLICE		(32)
#define SWM_V_SLICE		(32)

/* define work spaces */
struct workspace {
	int			idx;		/* workspace index */
	char			*name;		/* workspace name */
	int			always_raise;	/* raise windows on focus */
	struct layout		*cur_layout;	/* current layout handlers */
	struct ws_win		*focus;		/* may be NULL */
	struct ws_win		*focus_prev;	/* may be NULL */
	struct swm_region	*r;		/* may be NULL */
	struct swm_region	*old_r;		/* may be NULL */
	struct ws_win_list	winlist;	/* list of windows in ws */
	struct ws_win_list	unmanagedlist;	/* list of dead windows in ws */
	char			stacker[10];	/* display stacker and layout */

	/* stacker state */
	struct {
				int horizontal_msize;
				int horizontal_mwin;
				int horizontal_stacks;
				int horizontal_flip;
				int vertical_msize;
				int vertical_mwin;
				int vertical_stacks;
				int vertical_flip;
	} l_state;
};

enum {
	SWM_S_COLOR_BAR,
	SWM_S_COLOR_BAR_BORDER,
	SWM_S_COLOR_BAR_FONT,
	SWM_S_COLOR_FOCUS,
	SWM_S_COLOR_UNFOCUS,
	SWM_S_COLOR_MAX
};

/* physical screen mapping */
#define SWM_WS_MAX		(22)	/* hard limit */
int		workspace_limit = 10;	/* soft limit */

struct swm_screen {
	int			idx;	/* screen index */
	struct swm_region_list	rl;	/* list of regions on this screen */
	struct swm_region_list	orl;	/* list of old regions */
	xcb_window_t		root;
	struct workspace	ws[SWM_WS_MAX];

	/* colors */
	struct {
		uint32_t	color;
		char		*name;
	} c[SWM_S_COLOR_MAX];

	xcb_gcontext_t		bar_gc;
};
struct swm_screen	*screens;

/* args to functions */
union arg {
	int			id;
#define SWM_ARG_ID_FOCUSNEXT	(0)
#define SWM_ARG_ID_FOCUSPREV	(1)
#define SWM_ARG_ID_FOCUSMAIN	(2)
#define SWM_ARG_ID_FOCUSCUR	(4)
#define SWM_ARG_ID_SWAPNEXT	(10)
#define SWM_ARG_ID_SWAPPREV	(11)
#define SWM_ARG_ID_SWAPMAIN	(12)
#define SWM_ARG_ID_MOVELAST	(13)
#define SWM_ARG_ID_MASTERSHRINK (20)
#define SWM_ARG_ID_MASTERGROW	(21)
#define SWM_ARG_ID_MASTERADD	(22)
#define SWM_ARG_ID_MASTERDEL	(23)
#define SWM_ARG_ID_FLIPLAYOUT	(24)
#define SWM_ARG_ID_STACKRESET	(30)
#define SWM_ARG_ID_STACKINIT	(31)
#define SWM_ARG_ID_CYCLEWS_UP	(40)
#define SWM_ARG_ID_CYCLEWS_DOWN	(41)
#define SWM_ARG_ID_CYCLESC_UP	(42)
#define SWM_ARG_ID_CYCLESC_DOWN	(43)
#define SWM_ARG_ID_CYCLEWS_UP_ALL	(44)
#define SWM_ARG_ID_CYCLEWS_DOWN_ALL	(45)
#define SWM_ARG_ID_STACKINC	(50)
#define SWM_ARG_ID_STACKDEC	(51)
#define SWM_ARG_ID_SS_ALL	(60)
#define SWM_ARG_ID_SS_WINDOW	(61)
#define SWM_ARG_ID_DONTCENTER	(70)
#define SWM_ARG_ID_CENTER	(71)
#define SWM_ARG_ID_KILLWINDOW	(80)
#define SWM_ARG_ID_DELETEWINDOW	(81)
#define SWM_ARG_ID_WIDTHGROW	(90)
#define SWM_ARG_ID_WIDTHSHRINK	(91)
#define SWM_ARG_ID_HEIGHTGROW	(92)
#define SWM_ARG_ID_HEIGHTSHRINK	(93)
#define SWM_ARG_ID_MOVEUP	(100)
#define SWM_ARG_ID_MOVEDOWN	(101)
#define SWM_ARG_ID_MOVELEFT	(102)
#define SWM_ARG_ID_MOVERIGHT	(103)
	char			**argv;
};

void	focus(struct swm_region *, union arg *);
void	focus_magic(struct ws_win *);

/* quirks */
struct quirk {
	TAILQ_ENTRY(quirk)	entry;
	char			*class;
	char			*name;
	unsigned long		quirk;
#define SWM_Q_FLOAT		(1<<0)	/* float this window */
#define SWM_Q_TRANSSZ		(1<<1)	/* transiend window size too small */
#define SWM_Q_ANYWHERE		(1<<2)	/* don't position this window */
#define SWM_Q_XTERM_FONTADJ	(1<<3)	/* adjust xterm fonts when resizing */
#define SWM_Q_FULLSCREEN	(1<<4)	/* remove border */
#define SWM_Q_FOCUSPREV		(1<<5)	/* focus on caller */
};
TAILQ_HEAD(quirk_list, quirk);
struct quirk_list		quirks = TAILQ_HEAD_INITIALIZER(quirks);

/*
 * Supported EWMH hints should be added to
 * both the enum and the ewmh array
 */
enum {
	_NET_ACTIVE_WINDOW,
	_NET_CLOSE_WINDOW,
	_NET_MOVERESIZE_WINDOW,
	_NET_WM_ACTION_CLOSE,
	_NET_WM_ACTION_FULLSCREEN,
	_NET_WM_ACTION_MOVE,
	_NET_WM_ACTION_RESIZE,
	_NET_WM_ALLOWED_ACTIONS,
	_NET_WM_STATE,
	_NET_WM_STATE_ABOVE,
	_NET_WM_STATE_FULLSCREEN,
	_NET_WM_STATE_HIDDEN,
	_NET_WM_STATE_MAXIMIZED_HORZ,
	_NET_WM_STATE_MAXIMIZED_VERT,
	_NET_WM_STATE_SKIP_PAGER,
	_NET_WM_STATE_SKIP_TASKBAR,
	_NET_WM_WINDOW_TYPE,
	_NET_WM_WINDOW_TYPE_DIALOG,
	_NET_WM_WINDOW_TYPE_DOCK,
	_NET_WM_WINDOW_TYPE_NORMAL,
	_NET_WM_WINDOW_TYPE_SPLASH,
	_NET_WM_WINDOW_TYPE_TOOLBAR,
	_NET_WM_WINDOW_TYPE_UTILITY,
	_SWM_WM_STATE_MANUAL,
	SWM_EWMH_HINT_MAX
};

struct ewmh_hint {
	char		*name;
	xcb_atom_t	atom;
} ewmh[SWM_EWMH_HINT_MAX] =	{
    /* must be in same order as in the enum */
    {"_NET_ACTIVE_WINDOW", XCB_ATOM_NONE},
    {"_NET_CLOSE_WINDOW", XCB_ATOM_NONE},
    {"_NET_MOVERESIZE_WINDOW", XCB_ATOM_NONE},
    {"_NET_WM_ACTION_CLOSE", XCB_ATOM_NONE},
    {"_NET_WM_ACTION_FULLSCREEN", XCB_ATOM_NONE},
    {"_NET_WM_ACTION_MOVE", XCB_ATOM_NONE},
    {"_NET_WM_ACTION_RESIZE", XCB_ATOM_NONE},
    {"_NET_WM_ALLOWED_ACTIONS", XCB_ATOM_NONE},
    {"_NET_WM_STATE", XCB_ATOM_NONE},
    {"_NET_WM_STATE_ABOVE", XCB_ATOM_NONE},
    {"_NET_WM_STATE_FULLSCREEN", XCB_ATOM_NONE},
    {"_NET_WM_STATE_HIDDEN", XCB_ATOM_NONE},
    {"_NET_WM_STATE_MAXIMIZED_HORZ", XCB_ATOM_NONE},
    {"_NET_WM_STATE_MAXIMIZED_VERT", XCB_ATOM_NONE},
    {"_NET_WM_STATE_SKIP_PAGER", XCB_ATOM_NONE},
    {"_NET_WM_STATE_SKIP_TASKBAR", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_DIALOG", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_DOCK", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_NORMAL", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_SPLASH", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_TOOLBAR", XCB_ATOM_NONE},
    {"_NET_WM_WINDOW_TYPE_UTILITY", XCB_ATOM_NONE},
    {"_SWM_WM_STATE_MANUAL", XCB_ATOM_NONE},
};

void		 store_float_geom(struct ws_win *, struct swm_region *);
int		 floating_toggle_win(struct ws_win *);
void		 constrain_window(struct ws_win *, struct swm_region *, int);
void		 update_window(struct ws_win *);
void		 spawn_select(struct swm_region *, union arg *, char *, int *);
char		*get_win_name(xcb_window_t);
xcb_atom_t	 get_atom_from_string(const char *);
void		map_window_raised(xcb_window_t);
void		do_sync(void);
xcb_screen_t	*get_screen(int);

xcb_screen_t *
get_screen(int screen)
{
	xcb_screen_iterator_t i;

	i = xcb_setup_roots_iterator(xcb_get_setup(conn));
	for (; i.rem; --screen, xcb_screen_next(&i))
		if (screen == 0)
			return (i.data);

	return (NULL);
}

void
do_sync(void)
{
	xcb_get_input_focus_cookie_t	c;
	xcb_get_input_focus_reply_t	*r;

	/* mimic XSync() */
	c = xcb_get_input_focus(conn);
	xcb_flush(conn);
	r = xcb_get_input_focus_reply(conn, c, NULL);
	if (r)
		free(r);
}

void
map_window_raised(xcb_window_t win)
{
	uint32_t	val = XCB_STACK_MODE_ABOVE;

	xcb_configure_window(conn, win,
		XCB_CONFIG_WINDOW_STACK_MODE, &val);

	xcb_map_window(conn, win);
	xcb_flush(conn);
}

xcb_atom_t
get_atom_from_string(const char *str)
{
	xcb_intern_atom_cookie_t	c;
	xcb_intern_atom_reply_t		*r;
	xcb_atom_t			atom;

	c = xcb_intern_atom(conn, False, strlen(str), str);
	r = xcb_intern_atom_reply(conn, c, NULL);
	if (r) {
		atom = r->atom;
		free(r);

		return (atom);
	}

	return (XCB_ATOM_NONE);
}

void
update_iconic(struct ws_win *win, int newv)
{
	int32_t				v = newv;
	xcb_atom_t			iprop;

	win->iconic = newv;

	iprop = get_atom_from_string("_SWM_ICONIC");
	if (iprop == XCB_ATOM_NONE)
		return;

	if (newv)
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->id,
			iprop, XCB_ATOM_INTEGER, 32, 1, &v);
	else
		xcb_delete_property(conn, win->id, iprop);
}

int
get_iconic(struct ws_win *win)
{
	int32_t v = 0, *vtmp;
	xcb_atom_t			iprop;
	xcb_get_property_cookie_t	pc;
	xcb_get_property_reply_t	*pr = NULL;

	iprop = get_atom_from_string("_SWM_ICONIC");
	if (iprop == XCB_ATOM_NONE)
		goto out;

	pc = xcb_get_property(conn, False, win->id, iprop, XCB_ATOM_INTEGER,
			0, 1);
	pr = xcb_get_property_reply(conn, pc, NULL);
	if (!pr)
		goto out;
	if (pr->type != XCB_ATOM_INTEGER || pr->format != 32)
		goto out;
	vtmp = xcb_get_property_value(pr);
	v = *vtmp;
out:
	if (pr != NULL)
		free(pr);
	return (v);
}

void
setup_ewmh(void)
{
	xcb_atom_t			sup_list;
	int				i, j, num_screens;

	sup_list = get_atom_from_string("_NET_SUPPORTED");

	for (i = 0; i < LENGTH(ewmh); i++)
		ewmh[i].atom = get_atom_from_string(ewmh[i].name);

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++) {
		/* Support check window will be created by workaround(). */

		/* Report supported atoms */
		xcb_delete_property(conn, screens[i].root, sup_list);
		for (j = 0; j < LENGTH(ewmh); j++)
			xcb_change_property(conn, XCB_PROP_MODE_APPEND,
				screens[i].root, sup_list, XCB_ATOM_ATOM, 32, 1,
				&ewmh[j].atom);
	}
}

void
teardown_ewmh(void)
{
	int				i, num_screens;
	xcb_atom_t			sup_check, sup_list;
	xcb_window_t			id;
	xcb_get_property_cookie_t	pc;
	xcb_get_property_reply_t	*pr;

	sup_check = get_atom_from_string("_NET_SUPPORTING_WM_CHECK");
	sup_list = get_atom_from_string("_NET_SUPPORTED");
	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));

	for (i = 0; i < num_screens; i++) {
		/* Get the support check window and destroy it */
		pc = xcb_get_property(conn, False, screens[i].root, sup_check,
			XCB_ATOM_WINDOW, 0, 1);
		pr = xcb_get_property_reply(conn, pc, NULL);
		if (pr) {
			id = *((xcb_window_t *)xcb_get_property_value(pr));

			xcb_destroy_window(conn, id);
			xcb_delete_property(conn, screens[i].root, sup_check);
			xcb_delete_property(conn, screens[i].root, sup_list);

			free(pr);
		}
	}
}

void
ewmh_autoquirk(struct ws_win *win)
{
	int			i;
	unsigned long		n;
	xcb_atom_t		type;

	xcb_get_property_cookie_t	c;
	xcb_get_property_reply_t	*r;

	c = xcb_get_property(conn, False, win->id,
		ewmh[_NET_WM_WINDOW_TYPE].atom, XCB_ATOM_ATOM, 0, (~0L));
	r = xcb_get_property_reply(conn, c, NULL);
	if (!r)
		return;
	n = xcb_get_property_value_length(r);

	for (i = 0; i < n; i++) {
		type = *((xcb_atom_t *)xcb_get_property_value(r));
		if (type == ewmh[_NET_WM_WINDOW_TYPE_NORMAL].atom)
			break;
		if (type == ewmh[_NET_WM_WINDOW_TYPE_DOCK].atom ||
		    type == ewmh[_NET_WM_WINDOW_TYPE_TOOLBAR].atom ||
		    type == ewmh[_NET_WM_WINDOW_TYPE_UTILITY].atom) {
			win->floating = 1;
			win->quirks = SWM_Q_FLOAT | SWM_Q_ANYWHERE;
			break;
		}
		if (type == ewmh[_NET_WM_WINDOW_TYPE_SPLASH].atom ||
		    type == ewmh[_NET_WM_WINDOW_TYPE_DIALOG].atom) {
			win->floating = 1;
			win->quirks = SWM_Q_FLOAT;
			break;
		}
	}
	free(r);
}

#define SWM_EWMH_ACTION_COUNT_MAX	(6)
#define EWMH_F_FULLSCREEN		(1<<0)
#define EWMH_F_ABOVE			(1<<1)
#define EWMH_F_HIDDEN			(1<<2)
#define EWMH_F_SKIP_PAGER		(1<<3)
#define EWMH_F_SKIP_TASKBAR		(1<<4)
#define SWM_F_MANUAL			(1<<5)

int
ewmh_set_win_fullscreen(struct ws_win *win, int fs)
{
	if (!win->ws->r)
		return (0);

	if (!win->floating)
		return (0);

	DNPRINTF(SWM_D_MISC, "ewmh_set_win_fullscreen: window: 0x%x, "
	    "fullscreen %s\n", win->id, YESNO(fs));

	if (fs) {
		if (!win->g_floatvalid)
			store_float_geom(win, win->ws->r);

		win->g = win->ws->r->g;
		win->bordered = 0;
	} else {
		if (win->g_floatvalid) {
			/* refloat at last floating relative position */
			win->g = win->g_float;
			X(win) += X(win->ws->r);
			Y(win) += Y(win->ws->r);
		}
	}

	return (1);
}

void
ewmh_update_actions(struct ws_win *win)
{
	xcb_atom_t		actions[SWM_EWMH_ACTION_COUNT_MAX];
	int			n = 0;

	if (win == NULL)
		return;

	actions[n++] = ewmh[_NET_WM_ACTION_CLOSE].atom;

	if (win->floating) {
		actions[n++] = ewmh[_NET_WM_ACTION_MOVE].atom;
		actions[n++] = ewmh[_NET_WM_ACTION_RESIZE].atom;
	}

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->id,
		ewmh[_NET_WM_ALLOWED_ACTIONS].atom, XCB_ATOM_ATOM, 32, 1,
		actions);
}

#define _NET_WM_STATE_REMOVE	0    /* remove/unset property */
#define _NET_WM_STATE_ADD	1    /* add/set property */
#define _NET_WM_STATE_TOGGLE	2    /* toggle property */

void
ewmh_update_win_state(struct ws_win *win, long state, long action)
{
	unsigned int		mask = 0;
	unsigned int		changed = 0;
	unsigned int		orig_flags;

	if (win == NULL)
		return;

	if (state == ewmh[_NET_WM_STATE_FULLSCREEN].atom)
		mask = EWMH_F_FULLSCREEN;
	if (state == ewmh[_NET_WM_STATE_ABOVE].atom)
		mask = EWMH_F_ABOVE;
	if (state == ewmh[_SWM_WM_STATE_MANUAL].atom)
		mask = SWM_F_MANUAL;
	if (state == ewmh[_NET_WM_STATE_SKIP_PAGER].atom)
		mask = EWMH_F_SKIP_PAGER;
	if (state == ewmh[_NET_WM_STATE_SKIP_TASKBAR].atom)
		mask = EWMH_F_SKIP_TASKBAR;


	orig_flags = win->ewmh_flags;

	switch (action) {
	case _NET_WM_STATE_REMOVE:
		win->ewmh_flags &= ~mask;
		break;
	case _NET_WM_STATE_ADD:
		win->ewmh_flags |= mask;
		break;
	case _NET_WM_STATE_TOGGLE:
		win->ewmh_flags ^= mask;
		break;
	}

	changed = (win->ewmh_flags & mask) ^ (orig_flags & mask) ? 1 : 0;

	if (state == ewmh[_NET_WM_STATE_ABOVE].atom)
		if (changed)
			if (!floating_toggle_win(win))
				win->ewmh_flags = orig_flags; /* revert */
	if (state == ewmh[_SWM_WM_STATE_MANUAL].atom)
		if (changed)
			win->manual = (win->ewmh_flags & SWM_F_MANUAL) != 0;
	if (state == ewmh[_NET_WM_STATE_FULLSCREEN].atom)
		if (changed)
			if (!ewmh_set_win_fullscreen(win,
			    win->ewmh_flags & EWMH_F_FULLSCREEN))
				win->ewmh_flags = orig_flags; /* revert */

	xcb_delete_property(conn, win->id, ewmh[_NET_WM_STATE].atom);

	if (win->ewmh_flags & EWMH_F_FULLSCREEN)
		xcb_change_property(conn, XCB_PROP_MODE_APPEND, win->id,
			ewmh[_NET_WM_STATE].atom, XCB_ATOM_ATOM, 32, 1,
			&ewmh[_NET_WM_STATE_FULLSCREEN].atom);
	if (win->ewmh_flags & EWMH_F_SKIP_PAGER)
		xcb_change_property(conn, XCB_PROP_MODE_APPEND, win->id,
			ewmh[_NET_WM_STATE].atom, XCB_ATOM_ATOM, 32, 1,
			&ewmh[_NET_WM_STATE_SKIP_PAGER].atom);
	if (win->ewmh_flags & EWMH_F_SKIP_TASKBAR)
		xcb_change_property(conn, XCB_PROP_MODE_APPEND, win->id,
			ewmh[_NET_WM_STATE].atom, XCB_ATOM_ATOM, 32, 1,
			&ewmh[_NET_WM_STATE_SKIP_TASKBAR].atom);
	if (win->ewmh_flags & EWMH_F_ABOVE)
		xcb_change_property(conn, XCB_PROP_MODE_APPEND, win->id,
			ewmh[_NET_WM_STATE].atom, XCB_ATOM_ATOM, 32, 1,
			&ewmh[_NET_WM_STATE_ABOVE].atom);
	if (win->ewmh_flags & SWM_F_MANUAL)
		xcb_change_property(conn, XCB_PROP_MODE_APPEND, win->id,
			ewmh[_NET_WM_STATE].atom, XCB_ATOM_ATOM, 32, 1,
			&ewmh[_SWM_WM_STATE_MANUAL].atom);
}

void
ewmh_get_win_state(struct ws_win *win)
{
	xcb_atom_t			*states;
	xcb_get_property_cookie_t	c;
	xcb_get_property_reply_t	*r;
	int				i, n;

	if (win == NULL)
		return;

	win->ewmh_flags = 0;
	if (win->floating)
		win->ewmh_flags |= EWMH_F_ABOVE;
	if (win->manual)
		win->ewmh_flags |= SWM_F_MANUAL;

	c = xcb_get_property(conn, False, win->id, ewmh[_NET_WM_STATE].atom,
		XCB_ATOM_ATOM, 0, (~0L));
	r = xcb_get_property_reply(conn, c, NULL);
	if (!r)
		return;

	states = xcb_get_property_value(r);
	n = xcb_get_property_value_length(r);

	free(r);

	for (i = 0; i < n; i++)
		ewmh_update_win_state(win, states[i], _NET_WM_STATE_ADD);
}

/* events */
#ifdef SWM_DEBUG
char *
geteventname(XEvent *e)
{
	char			*name = NULL;

	switch (e->type) {
	case KeyPress:
		name = "KeyPress";
		break;
	case KeyRelease:
		name = "KeyRelease";
		break;
	case ButtonPress:
		name = "ButtonPress";
		break;
	case ButtonRelease:
		name = "ButtonRelease";
		break;
	case MotionNotify:
		name = "MotionNotify";
		break;
	case EnterNotify:
		name = "EnterNotify";
		break;
	case LeaveNotify:
		name = "LeaveNotify";
		break;
	case FocusIn:
		name = "FocusIn";
		break;
	case FocusOut:
		name = "FocusOut";
		break;
	case KeymapNotify:
		name = "KeymapNotify";
		break;
	case Expose:
		name = "Expose";
		break;
	case GraphicsExpose:
		name = "GraphicsExpose";
		break;
	case NoExpose:
		name = "NoExpose";
		break;
	case VisibilityNotify:
		name = "VisibilityNotify";
		break;
	case CreateNotify:
		name = "CreateNotify";
		break;
	case DestroyNotify:
		name = "DestroyNotify";
		break;
	case UnmapNotify:
		name = "UnmapNotify";
		break;
	case MapNotify:
		name = "MapNotify";
		break;
	case MapRequest:
		name = "MapRequest";
		break;
	case ReparentNotify:
		name = "ReparentNotify";
		break;
	case ConfigureNotify:
		name = "ConfigureNotify";
		break;
	case ConfigureRequest:
		name = "ConfigureRequest";
		break;
	case GravityNotify:
		name = "GravityNotify";
		break;
	case ResizeRequest:
		name = "ResizeRequest";
		break;
	case CirculateNotify:
		name = "CirculateNotify";
		break;
	case CirculateRequest:
		name = "CirculateRequest";
		break;
	case PropertyNotify:
		name = "PropertyNotify";
		break;
	case SelectionClear:
		name = "SelectionClear";
		break;
	case SelectionRequest:
		name = "SelectionRequest";
		break;
	case SelectionNotify:
		name = "SelectionNotify";
		break;
	case ColormapNotify:
		name = "ColormapNotify";
		break;
	case ClientMessage:
		name = "ClientMessage";
		break;
	case MappingNotify:
		name = "MappingNotify";
		break;
	default:
		name = "Unknown";
	}

	return (name);
}

char *
xrandr_geteventname(XEvent *e)
{
	char			*name = NULL;

	switch(e->type - xrandr_eventbase) {
	case RRScreenChangeNotify:
		name = "RRScreenChangeNotify";
		break;
	default:
		name = "Unknown";
	}

	return (name);
}

void
dumpwins(struct swm_region *r, union arg *args)
{
	struct ws_win				*win;
	uint16_t				state;
	xcb_get_window_attributes_cookie_t	c;
	xcb_get_window_attributes_reply_t	*wa;

	if (r->ws == NULL) {
		warnx("dumpwins: invalid workspace");
		return;
	}

	warnx("=== managed window list ws %02d ===", r->ws->idx);
	TAILQ_FOREACH(win, &r->ws->winlist, entry) {
		state = getstate(win->id);
		c = xcb_get_window_attributes(conn, win->id);
		wa = xcb_get_window_attributes_reply(conn, c, NULL);
		if (wa) {
			warnx("window: 0x%x, map_state: %d, state: %u, "
				"transient: 0x%x", win->id, wa->map_state,
				state, win->transient);
			free(wa);
		} else
			warnx("window: 0x%x, failed xcb_get_window_attributes",
			    win->id);
	}

	warnx("===== unmanaged window list =====");
	TAILQ_FOREACH(win, &r->ws->unmanagedlist, entry) {
		state = getstate(win->id);
		c = xcb_get_window_attributes(conn, win->id);
		wa = xcb_get_window_attributes_reply(conn, c, NULL);
		if (wa) {
			warnx("window: 0x%x, map_state: %d, state: %u, "
				"transient: 0x%x", win->id, wa->map_state,
				state, win->transient);
			free(wa);
		} else
			warnx("window: 0x%x, failed xcb_get_window_attributes",
			    win->id);
	}

	warnx("=================================");
}
#else
void
dumpwins(struct swm_region *r, union arg *args)
{
}
#endif /* SWM_DEBUG */

void			expose(XEvent *);
void			keypress(XEvent *);
void			buttonpress(XEvent *);
void			configurerequest(XEvent *);
void			configurenotify(XEvent *);
void			destroynotify(XEvent *);
void			enternotify(XEvent *);
void			focusevent(XEvent *);
void			mapnotify(XEvent *);
void			mappingnotify(XEvent *);
void			maprequest(XEvent *);
void			propertynotify(XEvent *);
void			unmapnotify(XEvent *);
void			visibilitynotify(XEvent *);
void			clientmessage(XEvent *);

void			(*handler[LASTEvent])(XEvent *) = {
				[Expose] = expose,
				[KeyPress] = keypress,
				[ButtonPress] = buttonpress,
				[ConfigureRequest] = configurerequest,
				[ConfigureNotify] = configurenotify,
				[DestroyNotify] = destroynotify,
				[EnterNotify] = enternotify,
				[FocusIn] = focusevent,
				[FocusOut] = focusevent,
				[MapNotify] = mapnotify,
				[MappingNotify] = mappingnotify,
				[MapRequest] = maprequest,
				[PropertyNotify] = propertynotify,
				[UnmapNotify] = unmapnotify,
				[VisibilityNotify] = visibilitynotify,
				[ClientMessage] = clientmessage,
};

void
sighdlr(int sig)
{
	int			saved_errno, status;
	pid_t			pid;

	saved_errno = errno;

	switch (sig) {
	case SIGCHLD:
		while ((pid = waitpid(WAIT_ANY, &status, WNOHANG)) != 0) {
			if (pid == -1) {
				if (errno == EINTR)
					continue;
#ifdef SWM_DEBUG
				if (errno != ECHILD)
					warn("sighdlr: waitpid");
#endif /* SWM_DEBUG */
				break;
			}
			if (pid == searchpid)
				search_resp = 1;

#ifdef SWM_DEBUG
			if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) != 0)
					warnx("sighdlr: child exit status: %d",
					    WEXITSTATUS(status));
			} else
				warnx("sighdlr: child is terminated "
				    "abnormally");
#endif /* SWM_DEBUG */
		}
		break;

	case SIGHUP:
		restart_wm = 1;
		break;
	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
		running = 0;
		break;
	}

	errno = saved_errno;
}

struct pid_e *
find_pid(long pid)
{
	struct pid_e		*p = NULL;

	DNPRINTF(SWM_D_MISC, "find_pid: %lu\n", pid);

	if (pid == 0)
		return (NULL);

	TAILQ_FOREACH(p, &pidlist, entry) {
		if (p->pid == pid)
			return (p);
	}

	return (NULL);
}

uint32_t
name_to_color(const char *colorname)
{
	uint32_t			result = 0;
	char				cname[32] = "#";
	xcb_screen_t			*screen;
	xcb_colormap_t			cmap;
	xcb_alloc_named_color_cookie_t	c;
	xcb_alloc_named_color_reply_t	*r;

	/* XXX - does not support rgb:/RR/GG/BB
	 *       will need to use xcb_alloc_color
	 */
	screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
	cmap = screen->default_colormap;

	c = xcb_alloc_named_color(conn, cmap, strlen(colorname), colorname);
	r = xcb_alloc_named_color_reply(conn, c, NULL);
	if (!r) {
		strlcat(cname, colorname + 2, sizeof cname - 1);
		c = xcb_alloc_named_color(conn, cmap, strlen(cname),
			cname);
		r = xcb_alloc_named_color_reply(conn, c, NULL);
	}
	if (r) {
		result = r->pixel;
		free(r);
	} else
		warnx("color '%s' not found", colorname);

	return (result);
}

void
setscreencolor(char *val, int i, int c)
{
	int	num_screens;

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	if (i > 0 && i <= num_screens) {
		screens[i - 1].c[c].color = name_to_color(val);
		free(screens[i - 1].c[c].name);
		if ((screens[i - 1].c[c].name = strdup(val)) == NULL)
			err(1, "strdup");
	} else if (i == -1) {
		for (i = 0; i < num_screens; i++) {
			screens[i].c[c].color = name_to_color(val);
			free(screens[i].c[c].name);
			if ((screens[i].c[c].name = strdup(val)) == NULL)
				err(1, "strdup");
		}
	} else
		errx(1, "invalid screen index: %d out of bounds (maximum %d)",
		    i, num_screens);
}

void
fancy_stacker(struct workspace *ws)
{
	strlcpy(ws->stacker, "[   ]", sizeof ws->stacker);
	if (ws->cur_layout->l_stack == vertical_stack)
		snprintf(ws->stacker, sizeof ws->stacker,
		    ws->l_state.vertical_flip ? "[%d>%d]" : "[%d|%d]",
		    ws->l_state.vertical_mwin, ws->l_state.vertical_stacks);
	if (ws->cur_layout->l_stack == horizontal_stack)
		snprintf(ws->stacker, sizeof ws->stacker,
		    ws->l_state.horizontal_flip ? "[%dv%d]" : "[%d-%d]",
		    ws->l_state.horizontal_mwin, ws->l_state.horizontal_stacks);
}

void
plain_stacker(struct workspace *ws)
{
	strlcpy(ws->stacker, "[ ]", sizeof ws->stacker);
	if (ws->cur_layout->l_stack == vertical_stack)
		strlcpy(ws->stacker, ws->l_state.vertical_flip ? "[>]" : "[|]",
		    sizeof ws->stacker);
	if (ws->cur_layout->l_stack == horizontal_stack)
		strlcpy(ws->stacker, ws->l_state.horizontal_flip ? "[v]" : "[-]",
		    sizeof ws->stacker);
}

void
custom_region(char *val)
{
	unsigned int			sidx, x, y, w, h;
	int				num_screens;
	xcb_screen_t			*screen;

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	if (sscanf(val, "screen[%u]:%ux%u+%u+%u", &sidx, &w, &h, &x, &y) != 5)
		errx(1, "invalid custom region, "
		    "should be 'screen[<n>]:<n>x<n>+<n>+<n>");
	if (sidx < 1 || sidx > num_screens)
		errx(1, "invalid screen index: %d out of bounds (maximum %d)",
		    sidx, num_screens);
	sidx--;

	screen = get_screen(sidx);
	if (w < 1 || h < 1)
		errx(1, "region %ux%u+%u+%u too small", w, h, x, y);

	if (x > screen->width_in_pixels ||
	    y > screen->height_in_pixels ||
	    w + x > screen->width_in_pixels ||
	    h + y > screen->height_in_pixels) {
		warnx("ignoring region %ux%u+%u+%u - not within screen "
		    "boundaries (%ux%u)", w, h, x, y,
		    screen->width_in_pixels, screen->height_in_pixels);
		return;
	}

	new_region(&screens[sidx], x, y, w, h);
}

void
socket_setnonblock(int fd)
{
	int			flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
		err(1, "fcntl F_GETFL");
	flags |= O_NONBLOCK;
	if ((flags = fcntl(fd, F_SETFL, flags)) == -1)
		err(1, "fcntl F_SETFL");
}

void
bar_print(struct swm_region *r, const char *s)
{
	int			x = 0;
	size_t			len;
	xcb_rectangle_t		rect;
	uint32_t		gcv[1];
	XRectangle		ibox, lbox;

	len = strlen(s);
	XmbTextExtents(bar_fs, s, len, &ibox, &lbox);

	switch (bar_justify) {
	case SWM_BAR_JUSTIFY_LEFT:
		x = SWM_BAR_OFFSET;
		break;
	case SWM_BAR_JUSTIFY_CENTER:
		x = (WIDTH(r) - lbox.width) / 2;
		break;
	case SWM_BAR_JUSTIFY_RIGHT:
		x = WIDTH(r) - lbox.width - SWM_BAR_OFFSET;
		break;
	}

	if (x < SWM_BAR_OFFSET)
		x = SWM_BAR_OFFSET;

	rect.x = 0;
	rect.y = 0;
	rect.width = WIDTH(r->bar);
	rect.height = HEIGHT(r->bar);

	/* clear back buffer */
	gcv[0] = r->s->c[SWM_S_COLOR_BAR].color;
	xcb_change_gc(conn, r->s->bar_gc, XCB_GC_FOREGROUND, gcv);
	xcb_poly_fill_rectangle(conn, r->bar->buffer, r->s->bar_gc,
		sizeof(rect), &rect);

	/* draw back buffer */
	gcv[0] = r->s->c[SWM_S_COLOR_BAR_FONT].color;
	xcb_change_gc(conn, r->s->bar_gc, XCB_GC_FOREGROUND, gcv);
	xcb_image_text_8(conn, len, r->bar->buffer, r->s->bar_gc, x,
		(bar_fs_extents->max_logical_extent.height - lbox.height) / 2 -
		lbox.y, s);

	/* blt */
	xcb_copy_area(conn, r->bar->buffer, r->bar->id, r->s->bar_gc, 0, 0,
		0, 0, WIDTH(r->bar), HEIGHT(r->bar));
}

void
bar_extra_stop(void)
{
	if (bar_pipe[0]) {
		close(bar_pipe[0]);
		bzero(bar_pipe, sizeof bar_pipe);
	}
	if (bar_pid) {
		kill(bar_pid, SIGTERM);
		bar_pid = 0;
	}
	strlcpy((char *)bar_ext, "", sizeof bar_ext);
	bar_extra = 0;
}

void
bar_class_name(char *s, size_t sz, struct swm_region *r)
{
	if (r == NULL || r->ws == NULL || r->ws->focus == NULL)
		return;
	if (r->ws->focus->ch.res_class != NULL)
		strlcat(s, r->ws->focus->ch.res_class, sz);
}

void
bar_title_name(char *s, size_t sz, struct swm_region *r)
{
	if (r == NULL || r->ws == NULL || r->ws->focus == NULL)
		return;
	if (r->ws->focus->ch.res_name != NULL)
		strlcat(s, r->ws->focus->ch.res_name, sz);
}

void
bar_class_title_name(char *s, size_t sz, struct swm_region *r)
{
	if (r == NULL || r->ws == NULL || r->ws->focus == NULL)
		return;

	bar_class_name(s, sz, r);
	strlcat(s, ":", sz);
	bar_title_name(s, sz, r);
}

void
bar_window_float(char *s, size_t sz, struct swm_region *r)
{
	if (r == NULL || r ->ws == NULL || r->ws->focus == NULL)
		return;
	if (r->ws->focus->floating)
		strlcat(s, "(f)", sz);
}

void
bar_window_name(char *s, size_t sz, struct swm_region *r)
{
	char		*title;

	if (r == NULL || r->ws == NULL || r->ws->focus == NULL)
		return;
	if ((title = get_win_name(r->ws->focus->id)) == NULL)
		return;

	strlcat(s, title, sz);
	free(title);
}

int		urgent[SWM_WS_MAX];
void
bar_urgent(char *s, size_t sz)
{
	struct ws_win		*win;
	int			i, j, num_screens;
	char			b[8];
	xcb_get_property_cookie_t	c;
	xcb_icccm_wm_hints_t	hints;

	for (i = 0; i < workspace_limit; i++)
		urgent[i] = 0;

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++)
		for (j = 0; j < workspace_limit; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry) {
				c = xcb_icccm_get_wm_hints(conn, win->id);
				if (xcb_icccm_get_wm_hints_reply(conn, c,
						&hints, NULL) == 0)
					continue;
				if (hints.flags & XCB_ICCCM_WM_HINT_X_URGENCY)
					urgent[j] = 1;
			}

	for (i = 0; i < workspace_limit; i++) {
		if (urgent[i])
			snprintf(b, sizeof b, "%d ", i + 1);
		else
			snprintf(b, sizeof b, "- ");
		strlcat(s, b, sz);
	}
}

void
bar_workspace_name(char *s, size_t sz, struct swm_region *r)
{
	if (r == NULL || r->ws == NULL)
		return;
	if (r->ws->name != NULL)
		strlcat(s, r->ws->name, sz);
}

/* build the default bar format according to the defined enabled options */
void
bar_fmt(const char *fmtexp, char *fmtnew, struct swm_region *r, size_t sz)
{
	/* if format provided, just copy the buffers */
	if (bar_format != NULL) {
		strlcpy(fmtnew, fmtexp, sz);
		return;
	}

	/* reset the output buffer */
	*fmtnew = '\0';

	strlcat(fmtnew, "+N:+I ", sz);
	if (stack_enabled)
		strlcat(fmtnew, "+S", sz);
	strlcat(fmtnew, " ", sz);

	/* only show the workspace name if there's actually one */
	if (r != NULL && r->ws != NULL && r->ws->name != NULL)
		strlcat(fmtnew, "<+D>", sz);
	strlcat(fmtnew, "+3<", sz);

	if (clock_enabled) {
		strlcat(fmtnew, fmtexp, sz);
		strlcat(fmtnew, "+4<", sz);
	}

	/* bar_urgent already adds the space before the last asterisk */
	if (urgent_enabled)
		strlcat(fmtnew, "* +U*+4<", sz);

	if (title_class_enabled) {
		strlcat(fmtnew, "+C", sz);
		if (title_name_enabled == 0)
			strlcat(fmtnew, "+4<", sz);
	}

	/* checks needed by the colon and floating strlcat(3) calls below */
	if (r != NULL && r->ws != NULL && r->ws->focus != NULL) {
		if (title_name_enabled) {
			if (title_class_enabled)
				strlcat(fmtnew, ":", sz);
			strlcat(fmtnew, "+T+4<", sz);
		}
		if (window_name_enabled) {
			if (r->ws->focus->floating)
				strlcat(fmtnew, "+F ", sz);
			strlcat(fmtnew, "+64W ", sz);
		}
	}

	/* finally add the action script output and the version */
	strlcat(fmtnew, "+4<+A+4<+V", sz);
}

void
bar_replace_pad(char *tmp, int *limit, size_t sz)
{
	/* special case; no limit given, pad one space, instead */
	if (*limit == sz - 1)
		*limit = 1;
	snprintf(tmp, sz, "%*s", *limit, " ");
}

/* replaces the bar format character sequences (like in tmux(1)) */
char *
bar_replace_seq(char *fmt, char *fmtrep, struct swm_region *r, size_t *offrep,
    size_t sz)
{
	char			*ptr;
	char			tmp[SWM_BAR_MAX];
	int			limit, size;
	size_t			len;

	/* reset strlcat(3) buffer */
	*tmp = '\0';

	/* get number, if any */
	fmt++;
	size = 0;
	if (sscanf(fmt, "%d%n", &limit, &size) != 1)
		limit = sizeof tmp - 1;
	if (limit <= 0 || limit >= sizeof tmp)
		limit = sizeof tmp - 1;

	/* there is nothing to replace (ie EOL) */
	fmt += size;
	if (*fmt == '\0')
		return (fmt);

	switch (*fmt) {
	case '<':
		bar_replace_pad(tmp, &limit, sizeof tmp);
		break;
	case 'A':
		snprintf(tmp, sizeof tmp, "%s", bar_ext);
		break;
	case 'C':
		bar_class_name(tmp, sizeof tmp, r);
		break;
	case 'D':
		bar_workspace_name(tmp, sizeof tmp, r);
		break;
	case 'F':
		bar_window_float(tmp, sizeof tmp, r);
		break;
	case 'I':
		snprintf(tmp, sizeof tmp, "%d", r->ws->idx + 1);
		break;
	case 'N':
		snprintf(tmp, sizeof tmp, "%d", r->s->idx + 1);
		break;
	case 'P':
		bar_class_title_name(tmp, sizeof tmp, r);
		break;
	case 'S':
		snprintf(tmp, sizeof tmp, "%s", r->ws->stacker);
		break;
	case 'T':
		bar_title_name(tmp, sizeof tmp, r);
		break;
	case 'U':
		bar_urgent(tmp, sizeof tmp);
		break;
	case 'V':
		snprintf(tmp, sizeof tmp, "%s", bar_vertext);
		break;
	case 'W':
		bar_window_name(tmp, sizeof tmp, r);
		break;
	default:
		/* unknown character sequence; copy as-is */
		snprintf(tmp, sizeof tmp, "+%c", *fmt);
		break;
	}

	len = strlen(tmp);
	ptr = tmp;
	if (len < limit)
		limit = len;
	while (limit-- > 0) {
		if (*offrep >= sz - 1)
			break;
		fmtrep[(*offrep)++] = *ptr++;
	}

	fmt++;
	return (fmt);
}

void
bar_replace(char *fmt, char *fmtrep, struct swm_region *r, size_t sz)
{
	size_t			off;

	off = 0;
	while (*fmt != '\0') {
		if (*fmt != '+') {
			/* skip ordinary characters */
			if (off >= sz - 1)
				break;
			fmtrep[off++] = *fmt++;
			continue;
		}

		/* character sequence found; replace it */
		fmt = bar_replace_seq(fmt, fmtrep, r, &off, sz);
		if (off >= sz - 1)
			break;
	}

	fmtrep[off] = '\0';
}

void
bar_fmt_expand(char *fmtexp, size_t sz)
{
	char			*fmt = NULL;
	size_t			len;
	struct tm		tm;
	time_t			tmt;

	/* start by grabbing the current time and date */
	time(&tmt);
	localtime_r(&tmt, &tm);

	/* figure out what to expand */
	if (bar_format != NULL)
		fmt = bar_format;
	else if (bar_format == NULL && clock_enabled)
		fmt = clock_format;
	/* if nothing to expand bail out */
	if (fmt == NULL) {
		*fmtexp = '\0';
		return;
	}

	/* copy as-is, just in case the format shouldn't be expanded below */
	strlcpy(fmtexp, fmt, sz);
	/* finally pass the string through strftime(3) */
#ifndef SWM_DENY_CLOCK_FORMAT
	if ((len = strftime(fmtexp, sz, fmt, &tm)) == 0)
		warnx("format too long");
	fmtexp[len] = '\0';
#endif
}

void
bar_fmt_print(void)
{
	char			fmtexp[SWM_BAR_MAX], fmtnew[SWM_BAR_MAX];
	char			fmtrep[SWM_BAR_MAX];
	int			i, num_screens;
	struct swm_region	*r;

	/* expand the format by first passing it through strftime(3) */
	bar_fmt_expand(fmtexp, sizeof fmtexp);

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++) {
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			if (r->bar == NULL)
				continue;
			bar_fmt(fmtexp, fmtnew, r, sizeof fmtnew);
			bar_replace(fmtnew, fmtrep, r, sizeof fmtrep);
			bar_print(r, fmtrep);
		}
	}
}

void
bar_update(void)
{
	size_t			len;
	char			*b;

	if (bar_enabled == 0)
		return;
	if (bar_extra && bar_extra_running) {
		/* ignore short reads; it'll correct itself */
		while ((b = fgetln(stdin, &len)) != NULL)
			if (b && b[len - 1] == '\n') {
				b[len - 1] = '\0';
				strlcpy((char *)bar_ext, b, sizeof bar_ext);
			}
		if (b == NULL && errno != EAGAIN) {
			warn("bar_update: bar_extra failed");
			bar_extra_stop();
		}
	} else
		strlcpy((char *)bar_ext, "", sizeof bar_ext);

	bar_fmt_print();
	alarm(bar_delay);
}

void
bar_signal(int sig)
{
	bar_alarm = 1;
}

void
bar_toggle(struct swm_region *r, union arg *args)
{
	struct swm_region	*tmpr;
	int			i, num_screens;

	DNPRINTF(SWM_D_BAR, "bar_toggle\n");

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	if (bar_enabled) {
		for (i = 0; i < num_screens; i++)
			TAILQ_FOREACH(tmpr, &screens[i].rl, entry)
				if (tmpr->bar)
					xcb_unmap_window(conn, tmpr->bar->id);
	} else {
		for (i = 0; i < num_screens; i++)
			TAILQ_FOREACH(tmpr, &screens[i].rl, entry)
				if (tmpr->bar)
					map_window_raised(tmpr->bar->id);
	}

	bar_enabled = !bar_enabled;

	stack();
	/* must be after stack */
	bar_update();
}

void
bar_refresh(void)
{
	struct swm_region	*r;
	uint32_t		wa[2];
	int			i, num_screens;

	/* do this here because the conf file is in memory */
	if (bar_extra && bar_extra_running == 0 && bar_argv[0]) {
		/* launch external status app */
		bar_extra_running = 1;
		if (pipe(bar_pipe) == -1)
			err(1, "pipe error");
		socket_setnonblock(bar_pipe[0]);
		socket_setnonblock(bar_pipe[1]); /* XXX hmmm, really? */
		if (dup2(bar_pipe[0], 0) == -1)
			err(1, "dup2");
		if (dup2(bar_pipe[1], 1) == -1)
			err(1, "dup2");
		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			err(1, "could not disable SIGPIPE");
		switch (bar_pid = fork()) {
		case -1:
			err(1, "cannot fork");
			break;
		case 0: /* child */
			close(bar_pipe[0]);
			execvp(bar_argv[0], bar_argv);
			err(1, "%s external app failed", bar_argv[0]);
			break;
		default: /* parent */
			close(bar_pipe[1]);
			break;
		}
	}

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			if (r->bar == NULL)
				continue;
			wa[0] = screens[i].c[SWM_S_COLOR_BAR].color;
			wa[1] = screens[i].c[SWM_S_COLOR_BAR_BORDER].color;
			xcb_change_window_attributes(conn, r->bar->id,
				XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL, wa);
		}
	bar_update();
}

void
bar_setup(struct swm_region *r)
{
	char			*default_string;
	char			**missing_charsets;
	int			num_missing_charsets = 0;
	int			i;
	xcb_screen_t		*screen = get_screen(r->s->idx);
	uint32_t		wa[2];
	
	if (bar_fs) {
		XFreeFontSet(display, bar_fs);
		bar_fs = NULL;
	}

	if ((r->bar = calloc(1, sizeof(struct swm_bar))) == NULL)
		err(1, "bar_setup: calloc: failed to allocate memory.");

	DNPRINTF(SWM_D_BAR, "bar_setup: loading bar_fonts: %s\n", bar_fonts);

	bar_fs = XCreateFontSet(display, bar_fonts, &missing_charsets,
	    &num_missing_charsets, &default_string);

	if (num_missing_charsets > 0) {
		warnx("Unable to load charset(s):");

		for (i = 0; i < num_missing_charsets; ++i)
			warnx("%s", missing_charsets[i]);

		XFreeStringList(missing_charsets);

		if (strcmp(default_string, ""))
			warnx("Glyphs from those sets will be replaced "
			    "by '%s'.", default_string);
		else
			warnx("Glyphs from those sets won't be drawn.");
	}

	if (bar_fs == NULL)
		errx(1, "Error creating font set structure.");

	bar_fs_extents = XExtentsOfFontSet(bar_fs);

	bar_height = bar_fs_extents->max_logical_extent.height +
	    2 * bar_border_width;

	if (bar_height < 1)
		bar_height = 1;

	X(r->bar) = X(r);
	Y(r->bar) = bar_at_bottom ? (Y(r) + HEIGHT(r) - bar_height) : Y(r);
	WIDTH(r->bar) = WIDTH(r) - 2 * bar_border_width;
	HEIGHT(r->bar) = bar_height - 2 * bar_border_width;

	r->bar->id = xcb_generate_id(conn);
	wa[0] = r->s->c[SWM_S_COLOR_BAR].color;
	wa[1] = r->s->c[SWM_S_COLOR_BAR_BORDER].color;
	xcb_create_window(conn, screen->root_depth, r->bar->id, r->s->root,
		X(r->bar), Y(r->bar), WIDTH(r->bar), HEIGHT(r->bar),
		bar_border_width, XCB_WINDOW_CLASS_INPUT_OUTPUT,
		screen->root_visual, XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL,
		wa);

	r->bar->buffer = xcb_generate_id(conn);
	xcb_create_pixmap(conn, screen->root_depth, r->bar->buffer, r->bar->id,
		WIDTH(r->bar), HEIGHT(r->bar)); 

	xcb_randr_select_input(conn, r->bar->id,
		XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE);

	if (bar_enabled)
		map_window_raised(r->bar->id);

	DNPRINTF(SWM_D_BAR, "bar_setup: window: 0x%x, (x,y) w x h: (%d,%d) "
	    "%d x %d\n", WINID(r->bar), X(r->bar), Y(r->bar), WIDTH(r->bar),
	    HEIGHT(r->bar));

	if (signal(SIGALRM, bar_signal) == SIG_ERR)
		err(1, "could not install bar_signal");
	bar_refresh();
}

void
bar_cleanup(struct swm_region *r)
{
	if (r->bar == NULL)
		return;
	xcb_destroy_window(conn, r->bar->id);
	xcb_free_pixmap(conn, r->bar->buffer);
	free(r->bar);
	r->bar = NULL;
}

void
drain_enter_notify(void)
{
	int			i = 0;

	while (xcb_poll_for_event(conn))
		i++;

	DNPRINTF(SWM_D_EVENT, "drain_enter_notify: drained: %d\n", i);
}

void
set_win_state(struct ws_win *win, uint16_t state)
{
	uint16_t		data[2] = { state, XCB_ATOM_NONE };

	DNPRINTF(SWM_D_EVENT, "set_win_state: window: 0x%x\n", win->id);

	if (win == NULL)
		return;

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->id, astate,
		astate, 32, 2, data);
}

uint16_t
getstate(xcb_window_t w)
{
	uint16_t			result = 0;
	xcb_get_property_cookie_t	c;
	xcb_get_property_reply_t	*r;

	c = xcb_get_property(conn, False, w, astate, astate, 0L, 2L);
	r = xcb_get_property_reply(conn, c, NULL);

	if (r) {
		result = *((uint16_t *)xcb_get_property_value(r));
		free(r);
	}

	return (result);
}

void
version(struct swm_region *r, union arg *args)
{
	bar_version = !bar_version;
	if (bar_version)
		snprintf(bar_vertext, sizeof bar_vertext,
		    "Version: %s Build: %s", SPECTRWM_VERSION, buildstr);
	else
		strlcpy(bar_vertext, "", sizeof bar_vertext);
	bar_update();
}

void
client_msg(struct ws_win *win, xcb_atom_t a)
{
	xcb_client_message_event_t	ev;

	if (win == NULL)
		return;

	bzero(&ev, sizeof ev);
	ev.response_type = XCB_CLIENT_MESSAGE;
	ev.window = win->id;
	ev.type = aprot;
	ev.format = 32;
	ev.data.data32[0] = a;
	ev.data.data32[1] = XCB_CURRENT_TIME;

	xcb_send_event(conn, False, win->id,
		XCB_EVENT_MASK_NO_EVENT, (const char *)&ev);
	xcb_flush(conn);
}

/* synthetic response to a ConfigureRequest when not making a change */
void
config_win(struct ws_win *win, XConfigureRequestEvent  *ev)
{
	xcb_configure_notify_event_t ce;

	if (win == NULL)
		return;

	/* send notification of unchanged state. */
	bzero(&ce, sizeof(ce));
	ce.response_type = XCB_CONFIGURE_NOTIFY;
	ce.x = X(win);
	ce.y = Y(win);
	ce.width = WIDTH(win);
	ce.height = HEIGHT(win);
	ce.override_redirect = False;

	if (ev == NULL) {
		/* EWMH */
		ce.event = win->id;
		ce.window = win->id;
		ce.border_width = BORDER(win);
		ce.above_sibling = XCB_WINDOW_NONE;
	} else {
		/* normal */
		ce.event = ev->window;
		ce.window = ev->window;

		/* make response appear more WM_SIZE_HINTS-compliant */
		if (win->sh_mask)
			DNPRINTF(SWM_D_MISC, "config_win: hints: window: 0x%x,"
			    " sh_mask: %ld, min: %d x %d, max: %d x %d, inc: "
			    "%d x %d\n", win->id, win->sh_mask, SH_MIN_W(win),
			    SH_MIN_H(win), SH_MAX_W(win), SH_MAX_H(win),
			    SH_INC_W(win), SH_INC_H(win));

		/* min size */
		if (SH_MIN(win)) {
			/* the hint may be set... to 0! */
			if (SH_MIN_W(win) > 0 && ce.width < SH_MIN_W(win))
				ce.width = SH_MIN_W(win);
			if (SH_MIN_H(win) > 0 && ce.height < SH_MIN_H(win))
				ce.height = SH_MIN_H(win);
		}

		/* max size */
		if (SH_MAX(win)) {
			/* may also be advertized as 0 */
			if (SH_MAX_W(win) > 0 && ce.width > SH_MAX_W(win))
				ce.width = SH_MAX_W(win);
			if (SH_MAX_H(win) > 0 && ce.height > SH_MAX_H(win))
				ce.height = SH_MAX_H(win);
		}

		/* resize increment. */
		if (SH_INC(win)) {
			if (SH_INC_W(win) > 1 && ce.width > SH_INC_W(win))
				ce.width -= (ce.width - SH_MIN_W(win)) %
				    SH_INC_W(win);
			if (SH_INC_H(win) > 1 && ce.height > SH_INC_H(win))
				ce.height -= (ce.height - SH_MIN_H(win)) %
				    SH_INC_H(win);
		}

		/* adjust x and y for requested border_width. */
		ce.x += BORDER(win) - ev->border_width;
		ce.y += BORDER(win) - ev->border_width;
		ce.border_width = ev->border_width;
		ce.above_sibling = ev->above;
	}

	DNPRINTF(SWM_D_MISC, "config_win: ewmh: %s, window: 0x%x, (x,y) w x h: "
	    "(%d,%d) %d x %d, border: %d\n", YESNO(ev == NULL), win->id, ce.x,
	    ce.y, ce.width, ce.height, ce.border_width);

	xcb_send_event(conn, False, win->id, XCB_EVENT_MASK_STRUCTURE_NOTIFY,
		(char *)&ce);
	xcb_flush(conn);
}

int
count_win(struct workspace *ws, int count_transient)
{
	struct ws_win		*win;
	int			count = 0;

	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (count_transient == 0 && win->floating)
			continue;
		if (count_transient == 0 && win->transient)
			continue;
		if (win->iconic)
			continue;
		count++;
	}
	DNPRINTF(SWM_D_MISC, "count_win: %d\n", count);

	return (count);
}

void
quit(struct swm_region *r, union arg *args)
{
	DNPRINTF(SWM_D_MISC, "quit\n");
	running = 0;
}

void
unmap_window(struct ws_win *win)
{
	if (win == NULL)
		return;

	/* don't unmap again */
	if (getstate(win->id) == XCB_ICCCM_WM_STATE_ICONIC)
		return;

	set_win_state(win, XCB_ICCCM_WM_STATE_ICONIC);

	xcb_unmap_window(conn, win->id);
	xcb_change_window_attributes(conn, win->id,
		XCB_CW_BORDER_PIXEL, &win->s->c[SWM_S_COLOR_UNFOCUS].color);
}

void
unmap_all(void)
{
	struct ws_win		*win;
	int			i, j, num_screens;

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++)
		for (j = 0; j < workspace_limit; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				unmap_window(win);
}

void
fake_keypress(struct ws_win *win, xcb_keysym_t keysym, uint16_t modifiers)
{
	xcb_key_press_event_t	event;
	xcb_key_symbols_t	*syms;
	xcb_keycode_t		*keycode;

	if (win == NULL)
		return;

	syms = xcb_key_symbols_alloc(conn);
	keycode = xcb_key_symbols_get_keycode(syms, keysym);

	event.event = win->id;
	event.root = win->s->root;
	event.child = XCB_WINDOW_NONE;
	event.time = XCB_CURRENT_TIME;
	event.event_x = X(win);
	event.event_y = Y(win);
	event.root_x = 1;
	event.root_y = 1;
	event.same_screen = True;
	event.detail = *keycode;
	event.state = modifiers;

	event.response_type = XCB_KEY_PRESS;
	xcb_send_event(conn, True, win->id,
		 XCB_EVENT_MASK_KEY_PRESS, (char *)&event);

	event.response_type = XCB_KEY_RELEASE;
	xcb_send_event(conn, True, win->id,
		XCB_EVENT_MASK_KEY_RELEASE, (char *)&event);
	xcb_flush(conn);

	xcb_key_symbols_free(syms);
}

void
restart(struct swm_region *r, union arg *args)
{
	DNPRINTF(SWM_D_MISC, "restart: %s\n", start_argv[0]);

	/* disable alarm because the following code may not be interrupted */
	alarm(0);
	if (signal(SIGALRM, SIG_IGN) == SIG_ERR)
		err(1, "can't disable alarm");

	bar_extra_stop();
	bar_extra = 1;
	unmap_all();
	XCloseDisplay(display);
	execvp(start_argv[0], start_argv);
	warn("execvp failed");
	quit(NULL, NULL);
}

struct swm_region *
root_to_region(xcb_window_t root)
{
	struct swm_region	*r = NULL;
	int			i, num_screens;
	xcb_query_pointer_reply_t	*qpr;

	DNPRINTF(SWM_D_MISC, "root_to_region: window: 0x%x\n", root);

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++)
		if (screens[i].root == root)
			break;

	qpr = xcb_query_pointer_reply(conn, xcb_query_pointer(conn,
		screens[i].root), NULL);

	if (qpr) {
		DNPRINTF(SWM_D_MISC, "root_to_region: pointer: (%d,%d)\n",
		    qpr->root_x, qpr->root_y);
		/* choose a region based on pointer location */
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			if (X(r) <= qpr->root_x && qpr->root_x < MAX_X(r) &&
			    Y(r) <= qpr->root_y && qpr->root_y < MAX_Y(r))
				break;
		free(qpr);
	}

	if (r == NULL)
		r = TAILQ_FIRST(&screens[i].rl);

	return (r);
}

struct ws_win *
find_unmanaged_window(xcb_window_t id)
{
	struct ws_win		*win;
	int			i, j, num_screens;

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++)
		for (j = 0; j < workspace_limit; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].unmanagedlist,
			    entry)
				if (id == win->id)
					return (win);
	return (NULL);
}

struct ws_win *
find_window(xcb_window_t id)
{
	struct ws_win		*win;
	int			i, j, num_screens;
	xcb_query_tree_cookie_t	c;
	xcb_query_tree_reply_t	*r;

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++)
		for (j = 0; j < workspace_limit; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				if (id == win->id)
					return (win);

	c = xcb_query_tree(conn, id);
	r = xcb_query_tree_reply(conn, c, NULL);
	if (!r)
		return (NULL);

	/* if we were looking for the parent return that window instead */
	if (r->parent == 0 || r->root == r->parent)
		return (NULL);

	/* look for parent */
	for (i = 0; i < num_screens; i++)
		for (j = 0; j < workspace_limit; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				if (r->parent == win->id) {
					free(r);
					return (win);
				}

	free(r);
	return (NULL);
}

void
spawn(int ws_idx, union arg *args, int close_fd)
{
	int			fd;
	char			*ret = NULL;

	DNPRINTF(SWM_D_MISC, "spawn: %s\n", args->argv[0]);

	if (display)
		close(xcb_get_file_descriptor(conn));

	setenv("LD_PRELOAD", SWM_LIB, 1);

	if (asprintf(&ret, "%d", ws_idx) == -1) {
		warn("spawn: asprintf SWM_WS");
		_exit(1);
	}
	setenv("_SWM_WS", ret, 1);
	free(ret);
	ret = NULL;

	if (asprintf(&ret, "%d", getpid()) == -1) {
		warn("spawn: asprintf _SWM_PID");
		_exit(1);
	}
	setenv("_SWM_PID", ret, 1);
	free(ret);
	ret = NULL;

	if (setsid() == -1) {
		warn("spawn: setsid");
		_exit(1);
	}

	if (close_fd) {
		/*
		 * close stdin and stdout to prevent interaction between apps
		 * and the baraction script
		 * leave stderr open to record errors
		*/
		if ((fd = open(_PATH_DEVNULL, O_RDWR, 0)) == -1) {
			warn("spawn: open");
			_exit(1);
		}
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		if (fd > 2)
			close(fd);
	}

	execvp(args->argv[0], args->argv);

	warn("spawn: execvp");
	_exit(1);
}

void
kill_refs(struct ws_win *win)
{
	int			i, x, num_screens;
	struct swm_region	*r;
	struct workspace	*ws;

	if (win == NULL)
		return;

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			for (x = 0; x < workspace_limit; x++) {
				ws = &r->s->ws[x];
				if (win == ws->focus)
					ws->focus = NULL;
				if (win == ws->focus_prev)
					ws->focus_prev = NULL;
			}
}

int
validate_win(struct ws_win *testwin)
{
	struct ws_win		*win;
	struct workspace	*ws;
	struct swm_region	*r;
	int			i, x, num_screens;

	if (testwin == NULL)
		return (0);

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			for (x = 0; x < workspace_limit; x++) {
				ws = &r->s->ws[x];
				TAILQ_FOREACH(win, &ws->winlist, entry)
					if (win == testwin)
						return (0);
			}
	return (1);
}

int
validate_ws(struct workspace *testws)
{
	struct swm_region	*r;
	struct workspace	*ws;
	int			i, x, num_screens;

	/* validate all ws */
	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			for (x = 0; x < workspace_limit; x++) {
				ws = &r->s->ws[x];
				if (ws == testws)
					return (0);
			}
	return (1);
}

void
unfocus_win(struct ws_win *win)
{
	XEvent			cne;
	xcb_window_t		none = XCB_WINDOW_NONE;

	DNPRINTF(SWM_D_FOCUS, "unfocus_win: window: 0x%x\n", WINID(win));

	if (win == NULL)
		return;
	if (win->ws == NULL)
		return;

	if (validate_ws(win->ws))
		return; /* XXX this gets hit with thunderbird, needs fixing */

	if (win->ws->r == NULL)
		return;

	if (validate_win(win)) {
		kill_refs(win);
		return;
	}

	if (win->ws->focus == win) {
		win->ws->focus = NULL;
		win->ws->focus_prev = win;
	}

	if (validate_win(win->ws->focus)) {
		kill_refs(win->ws->focus);
		win->ws->focus = NULL;
	}
	if (validate_win(win->ws->focus_prev)) {
		kill_refs(win->ws->focus_prev);
		win->ws->focus_prev = NULL;
	}

	/* drain all previous unfocus events */
	while (XCheckTypedEvent(display, FocusOut, &cne) == True)
		;

	grabbuttons(win, 0);
	xcb_change_window_attributes(conn, win->id, XCB_CW_BORDER_PIXEL,
		&win->ws->r->s->c[SWM_S_COLOR_UNFOCUS].color);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->s->root,
		ewmh[_NET_ACTIVE_WINDOW].atom, XCB_ATOM_WINDOW, 32, 1,
		&none);
}

void
unfocus_all(void)
{
	struct ws_win		*win;
	int			i, j, num_screens;

	DNPRINTF(SWM_D_FOCUS, "unfocus_all\n");

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++)
		for (j = 0; j < workspace_limit; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				unfocus_win(win);
}

void
focus_win(struct ws_win *win)
{
	XEvent			cne;
	struct ws_win		*cfw = NULL;
	xcb_get_input_focus_cookie_t	c;
	xcb_get_input_focus_reply_t	*r;
	xcb_window_t			cur_focus = XCB_WINDOW_NONE;

	DNPRINTF(SWM_D_FOCUS, "focus_win: window: 0x%x\n", WINID(win));

	if (win == NULL)
		return;
	if (win->ws == NULL)
		return;

	if (validate_ws(win->ws))
		return; /* XXX this gets hit with thunderbird, needs fixing */

	if (validate_win(win)) {
		kill_refs(win);
		return;
	}

	if (validate_win(win)) {
		kill_refs(win);
		return;
	}

	c = xcb_get_input_focus(conn);
	r = xcb_get_input_focus_reply(conn, c, NULL);
	if (r) {
		cur_focus = r->focus;
		free(r);
	}
	if ((cfw = find_window(cur_focus)) != NULL)
		unfocus_win(cfw);
	else {
		/* use larger hammer since the window was killed somehow */
		TAILQ_FOREACH(cfw, &win->ws->winlist, entry)
			if (cfw->ws && cfw->ws->r && cfw->ws->r->s)
				xcb_change_window_attributes(conn, cfw->id,
					XCB_CW_BORDER_PIXEL,
					&cfw->ws->r->s->c[SWM_S_COLOR_UNFOCUS].color);
	}

	win->ws->focus = win;

	if (win->ws->r != NULL) {
		/* drain all previous focus events */
		while (XCheckTypedEvent(display, FocusIn, &cne) == True)
			;

		if (win->java == 0)
			xcb_set_input_focus(conn, XCB_INPUT_FOCUS_PARENT,
				win->id, XCB_CURRENT_TIME);
		grabbuttons(win, 1);
		xcb_change_window_attributes(conn, win->id,
			XCB_CW_BORDER_PIXEL,
			&win->ws->r->s->c[SWM_S_COLOR_FOCUS].color);
		if (win->ws->cur_layout->flags & SWM_L_MAPONFOCUS ||
		    win->ws->always_raise)
			map_window_raised(win->id);

		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->s->root,
			ewmh[_NET_ACTIVE_WINDOW].atom, XCB_ATOM_WINDOW, 32, 1,
			&win->id);
	}

	bar_update();
}

void
switchws(struct swm_region *r, union arg *args)
{
	int			wsid = args->id, unmap_old = 0;
	struct swm_region	*this_r, *other_r;
	struct ws_win		*win;
	struct workspace	*new_ws, *old_ws;
	union arg		a;

	if (!(r && r->s))
		return;

	if (wsid >= workspace_limit)
		return;

	this_r = r;
	old_ws = this_r->ws;
	new_ws = &this_r->s->ws[wsid];

	DNPRINTF(SWM_D_WS, "switchws: screen[%d]:%dx%d+%d+%d: %d -> %d\n",
	    r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), old_ws->idx, wsid);

	if (new_ws == NULL || old_ws == NULL)
		return;
	if (new_ws == old_ws)
		return;

	other_r = new_ws->r;
	if (other_r == NULL) {
		/* the other workspace is hidden, hide this one */
		old_ws->r = NULL;
		unmap_old = 1;
	} else {
		/* the other ws is visible in another region, exchange them */
		other_r->ws_prior = new_ws;
		other_r->ws = old_ws;
		old_ws->r = other_r;
	}
	this_r->ws_prior = old_ws;
	this_r->ws = new_ws;
	new_ws->r = this_r;

	/* this is needed so that we can click on a window after a restart */
	unfocus_all();

	stack();
	a.id = SWM_ARG_ID_FOCUSCUR;
	focus(new_ws->r, &a);

	/* unmap old windows */
	if (unmap_old)
		TAILQ_FOREACH(win, &old_ws->winlist, entry)
			unmap_window(win);

	if (focus_mode == SWM_FOCUS_DEFAULT)
		drain_enter_notify();
}

void
cyclews(struct swm_region *r, union arg *args)
{
	union			arg a;
	struct swm_screen	*s = r->s;
	int			cycle_all = 0;

	DNPRINTF(SWM_D_WS, "cyclews: id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n",
	    args->id, r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	a.id = r->ws->idx;
	do {
		switch (args->id) {
		case SWM_ARG_ID_CYCLEWS_UP_ALL:
			cycle_all = 1;
			/* FALLTHROUGH */
		case SWM_ARG_ID_CYCLEWS_UP:
			if (a.id < workspace_limit - 1)
				a.id++;
			else
				a.id = 0;
			break;
		case SWM_ARG_ID_CYCLEWS_DOWN_ALL:
			cycle_all = 1;
			/* FALLTHROUGH */
		case SWM_ARG_ID_CYCLEWS_DOWN:
			if (a.id > 0)
				a.id--;
			else
				a.id = workspace_limit - 1;
			break;
		default:
			return;
		};

		if (!cycle_all &&
		    (cycle_empty == 0 && TAILQ_EMPTY(&s->ws[a.id].winlist)))
			continue;
		if (cycle_visible == 0 && s->ws[a.id].r != NULL)
			continue;

		switchws(r, &a);
	} while (a.id != r->ws->idx);
}

void
priorws(struct swm_region *r, union arg *args)
{
	union arg		a;

	DNPRINTF(SWM_D_WS, "priorws: id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n",
	    args->id, r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	if (r->ws_prior == NULL)
		return;

	a.id = r->ws_prior->idx;
	switchws(r, &a);
}

void
cyclescr(struct swm_region *r, union arg *args)
{
	struct swm_region	*rr = NULL;
	union arg		a;
	int			i, x, y, num_screens;

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	/* do nothing if we don't have more than one screen */
	if (!(num_screens > 1 || outputs > 1))
		return;

	i = r->s->idx;
	switch (args->id) {
	case SWM_ARG_ID_CYCLESC_UP:
		rr = TAILQ_NEXT(r, entry);
		if (rr == NULL)
			rr = TAILQ_FIRST(&screens[i].rl);
		break;
	case SWM_ARG_ID_CYCLESC_DOWN:
		rr = TAILQ_PREV(r, swm_region_list, entry);
		if (rr == NULL)
			rr = TAILQ_LAST(&screens[i].rl, swm_region_list);
		break;
	default:
		return;
	};
	if (rr == NULL)
		return;

	/* move mouse to region */
	x = X(rr) + 1;
	y = Y(rr) + 1 + (bar_enabled ? bar_height : 0);
	xcb_warp_pointer(conn, XCB_WINDOW_NONE, rr->s[i].root, 0, 0, 0, 0,
		x, y);

	a.id = SWM_ARG_ID_FOCUSCUR;
	focus(rr, &a);

	if (rr->ws->focus) {
		/* move to focus window */
		x = X(rr->ws->focus) + 1;
		y = Y(rr->ws->focus) + 1;
		xcb_warp_pointer(conn, XCB_WINDOW_NONE, rr->s[i].root, 0, 0, 0,
			0, x, y);
	}
}

void
sort_windows(struct ws_win_list *wl)
{
	struct ws_win		*win, *parent, *nxt;

	if (wl == NULL)
		return;

	for (win = TAILQ_FIRST(wl); win != TAILQ_END(wl); win = nxt) {
		nxt = TAILQ_NEXT(win, entry);
		if (win->transient) {
			parent = find_window(win->transient);
			if (parent == NULL) {
				warnx("not possible bug");
				continue;
			}
			TAILQ_REMOVE(wl, win, entry);
			TAILQ_INSERT_AFTER(wl, parent, win, entry);
		}
	}

}

void
swapwin(struct swm_region *r, union arg *args)
{
	struct ws_win		*target, *source;
	struct ws_win		*cur_focus;
	struct ws_win_list	*wl;


	DNPRINTF(SWM_D_WS, "swapwin: id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n",
	    args->id, r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	cur_focus = r->ws->focus;
	if (cur_focus == NULL)
		return;

	source = cur_focus;
	wl = &source->ws->winlist;

	switch (args->id) {
	case SWM_ARG_ID_SWAPPREV:
		if (source->transient)
			source = find_window(source->transient);
		target = TAILQ_PREV(source, ws_win_list, entry);
		if (target && target->transient)
			target = find_window(target->transient);
		TAILQ_REMOVE(wl, source, entry);
		if (target == NULL)
			TAILQ_INSERT_TAIL(wl, source, entry);
		else
			TAILQ_INSERT_BEFORE(target, source, entry);
		break;
	case SWM_ARG_ID_SWAPNEXT:
		target = TAILQ_NEXT(source, entry);
		/* move the parent and let the sort handle the move */
		if (source->transient)
			source = find_window(source->transient);
		TAILQ_REMOVE(wl, source, entry);
		if (target == NULL)
			TAILQ_INSERT_HEAD(wl, source, entry);
		else
			TAILQ_INSERT_AFTER(wl, target, source, entry);
		break;
	case SWM_ARG_ID_SWAPMAIN:
		target = TAILQ_FIRST(wl);
		if (target == source) {
			if (source->ws->focus_prev != NULL &&
			    source->ws->focus_prev != target)
				source = source->ws->focus_prev;
			else
				return;
		}
		if (target == NULL || source == NULL)
			return;
		source->ws->focus_prev = target;
		TAILQ_REMOVE(wl, target, entry);
		TAILQ_INSERT_BEFORE(source, target, entry);
		TAILQ_REMOVE(wl, source, entry);
		TAILQ_INSERT_HEAD(wl, source, entry);
		break;
	case SWM_ARG_ID_MOVELAST:
		TAILQ_REMOVE(wl, source, entry);
		TAILQ_INSERT_TAIL(wl, source, entry);
		break;
	default:
		DNPRINTF(SWM_D_MOVE, "swapwin: invalid id: %d\n", args->id);
		return;
	}

	sort_windows(wl);

	stack();
}

void
focus_prev(struct ws_win *win)
{
	struct ws_win		*winfocus = NULL;
	struct ws_win		*cur_focus = NULL;
	struct ws_win_list	*wl = NULL;
	struct workspace	*ws = NULL;

	if (!(win && win->ws))
		return;

	ws = win->ws;
	wl = &ws->winlist;
	cur_focus = ws->focus;

	DNPRINTF(SWM_D_FOCUS, "focus_prev: window: 0x%x, cur_focus: 0x%x\n",
	    WINID(win), WINID(cur_focus));

	/* pickle, just focus on whatever */
	if (cur_focus == NULL) {
		/* use prev_focus if valid */
		if (ws->focus_prev && ws->focus_prev != cur_focus &&
		    find_window(WINID(ws->focus_prev)))
			winfocus = ws->focus_prev;
		goto done;
	}

	/* if transient focus on parent */
	if (cur_focus->transient) {
		winfocus = find_window(cur_focus->transient);
		goto done;
	}

	/* if in max_stack try harder */
	if ((win->quirks & SWM_Q_FOCUSPREV) ||
	    (ws->cur_layout->flags & SWM_L_FOCUSPREV)) {
		if (cur_focus != ws->focus_prev)
			winfocus = ws->focus_prev;
		else
			winfocus = TAILQ_PREV(win, ws_win_list, entry);
		if (winfocus)
			goto done;
	}

	DNPRINTF(SWM_D_FOCUS, "focus_prev: focus_close: %d\n", focus_close);

	if (winfocus == NULL || winfocus == win) {
		switch (focus_close) {
		case SWM_STACK_BOTTOM:
			winfocus = TAILQ_FIRST(wl);
			break;
		case SWM_STACK_TOP:
			winfocus = TAILQ_LAST(wl, ws_win_list);
			break;
		case SWM_STACK_ABOVE:
			if ((winfocus = TAILQ_NEXT(cur_focus, entry)) == NULL) {
				if (focus_close_wrap)
					winfocus = TAILQ_FIRST(wl);
				else
					winfocus = TAILQ_PREV(cur_focus,
					    ws_win_list, entry);
			}
			break;
		case SWM_STACK_BELOW:
			if ((winfocus = TAILQ_PREV(cur_focus, ws_win_list,
			    entry)) == NULL) {
				if (focus_close_wrap)
					winfocus = TAILQ_LAST(wl, ws_win_list);
				else
					winfocus = TAILQ_NEXT(cur_focus, entry);
			}
			break;
		}
	}
done:
	if (winfocus == NULL) {
		if (focus_default == SWM_STACK_TOP)
			winfocus = TAILQ_LAST(wl, ws_win_list);
		else
			winfocus = TAILQ_FIRST(wl);
	}

	kill_refs(win);
	focus_magic(winfocus);
}

void
focus(struct swm_region *r, union arg *args)
{
	struct ws_win		*winfocus = NULL, *head;
	struct ws_win		*cur_focus = NULL;
	struct ws_win_list	*wl = NULL;
	struct workspace	*ws = NULL;
	int			all_iconics;

	if (!(r && r->ws))
		return;

	DNPRINTF(SWM_D_FOCUS, "focus: id: %d\n", args->id);

	/* treat FOCUS_CUR special */
	if (args->id == SWM_ARG_ID_FOCUSCUR) {
		if (r->ws->focus && r->ws->focus->iconic == 0)
			winfocus = r->ws->focus;
		else if (r->ws->focus_prev && r->ws->focus_prev->iconic == 0)
			winfocus = r->ws->focus_prev;
		else
			TAILQ_FOREACH(winfocus, &r->ws->winlist, entry)
				if (winfocus->iconic == 0)
					break;

		focus_magic(winfocus);
		return;
	}

	if ((cur_focus = r->ws->focus) == NULL)
		return;
	ws = r->ws;
	wl = &ws->winlist;
	if (TAILQ_EMPTY(wl))
		return;
	/* make sure there is at least one uniconified window */
	all_iconics = 1;
	TAILQ_FOREACH(winfocus, wl, entry)
		if (winfocus->iconic == 0) {
			all_iconics = 0;
			break;
		}
	if (all_iconics)
		return;

	switch (args->id) {
	case SWM_ARG_ID_FOCUSPREV:
		head = TAILQ_PREV(cur_focus, ws_win_list, entry);
		if (head == NULL)
			head = TAILQ_LAST(wl, ws_win_list);
		winfocus = head;
		if (WINID(winfocus) == cur_focus->transient) {
			head = TAILQ_PREV(winfocus, ws_win_list, entry);
			if (head == NULL)
				head = TAILQ_LAST(wl, ws_win_list);
			winfocus = head;
		}

		/* skip iconics */
		if (winfocus && winfocus->iconic) {
			while (winfocus != cur_focus) {
				if (winfocus == NULL)
					winfocus = TAILQ_LAST(wl, ws_win_list);
				if (winfocus->iconic == 0)
					break;
				winfocus = TAILQ_PREV(winfocus, ws_win_list,
				    entry);
			}
		}
		break;

	case SWM_ARG_ID_FOCUSNEXT:
		head = TAILQ_NEXT(cur_focus, entry);
		if (head == NULL)
			head = TAILQ_FIRST(wl);
		winfocus = head;

		/* skip iconics */
		if (winfocus && winfocus->iconic) {
			while (winfocus != cur_focus) {
				if (winfocus == NULL)
					winfocus = TAILQ_FIRST(wl);
				if (winfocus->iconic == 0)
					break;
				winfocus = TAILQ_NEXT(winfocus, entry);
			}
		}
		break;

	case SWM_ARG_ID_FOCUSMAIN:
		winfocus = TAILQ_FIRST(wl);
		if (winfocus == cur_focus)
			winfocus = cur_focus->ws->focus_prev;
		break;

	default:
		return;
	}

	focus_magic(winfocus);
}

void
cycle_layout(struct swm_region *r, union arg *args)
{
	struct workspace	*ws = r->ws;
	union arg		a;

	DNPRINTF(SWM_D_EVENT, "cycle_layout: workspace: %d\n", ws->idx);

	ws->cur_layout++;
	if (ws->cur_layout->l_stack == NULL)
		ws->cur_layout = &layouts[0];

	stack();
	if (focus_mode == SWM_FOCUS_DEFAULT)
		drain_enter_notify();
	a.id = SWM_ARG_ID_FOCUSCUR;
	focus(r, &a);
}

void
stack_config(struct swm_region *r, union arg *args)
{
	struct workspace	*ws = r->ws;

	DNPRINTF(SWM_D_STACK, "stack_config: id: %d workspace: %d\n",
	    args->id, ws->idx);

	if (ws->cur_layout->l_config != NULL)
		ws->cur_layout->l_config(ws, args->id);

	if (args->id != SWM_ARG_ID_STACKINIT)
		stack();
	bar_update();
}

void
stack(void) {
	struct swm_geometry	g;
	struct swm_region	*r;
	int			i, num_screens;
#ifdef SWM_DEBUG
	int j;
#endif

	DNPRINTF(SWM_D_STACK, "stack: begin\n");

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++) {
#ifdef SWM_DEBUG
		j = 0;
#endif
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			DNPRINTF(SWM_D_STACK, "stack: workspace: %d "
			    "(screen: %d, region: %d)\n", r->ws->idx, i, j++);

			/* start with screen geometry, adjust for bar */
			g = r->g;
			g.w -= 2 * border_width;
			g.h -= 2 * border_width;
			if (bar_enabled) {
				if (!bar_at_bottom)
					g.y += bar_height;
				g.h -= bar_height;
			}
			r->ws->cur_layout->l_stack(r->ws, &g);
			r->ws->cur_layout->l_string(r->ws);
			/* save r so we can track region changes */
			r->ws->old_r = r;
		}
	}
	if (font_adjusted)
		font_adjusted--;

	if (focus_mode == SWM_FOCUS_DEFAULT)
		drain_enter_notify();

	DNPRINTF(SWM_D_STACK, "stack: end\n");
}

void
store_float_geom(struct ws_win *win, struct swm_region *r)
{
	/* retain window geom and region geom */
	win->g_float = win->g;
	win->g_float.x -= X(r);
	win->g_float.y -= Y(r);
	win->g_floatvalid = 1;
	DNPRINTF(SWM_D_MISC, "store_float_geom: window: 0x%x, g: (%d,%d)"
	    " %d x %d, g_float: (%d,%d) %d x %d\n", win->id, X(win), Y(win),
	    WIDTH(win), HEIGHT(win), win->g_float.x, win->g_float.y,
	    win->g_float.w, win->g_float.h);
}

void
stack_floater(struct ws_win *win, struct swm_region *r)
{
	if (win == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "stack_floater: window: 0x%x\n", win->id);

	/*
	 * to allow windows to change their size (e.g. mplayer fs) only retrieve
	 * geom on ws switches or return from max mode
	 */
	if (win->g_floatvalid && (win->floatmaxed || (r != r->ws->old_r &&
	    !(win->ewmh_flags & EWMH_F_FULLSCREEN)))) {
		/* refloat at last floating relative position */
		win->g = win->g_float;
		X(win) += X(r);
		Y(win) += Y(r);
	}

	win->floatmaxed = 0;

	/*
	 * if set to fullscreen mode, configure window to maximum size.
	 */
	if (win->ewmh_flags & EWMH_F_FULLSCREEN) {
		if (!win->g_floatvalid)
			store_float_geom(win, win->ws->r);

		win->g = win->ws->r->g;
	}

	/*
	 * remove border on fullscreen floater when in fullscreen mode or when
	 * the quirk is present.
	 */
	if ((win->ewmh_flags & EWMH_F_FULLSCREEN) ||
	    ((win->quirks & SWM_Q_FULLSCREEN) &&
	     (WIDTH(win) >= WIDTH(r)) && (HEIGHT(win) >= HEIGHT(r)))) {
		if (win->bordered) {
			win->bordered = 0;
			X(win) += border_width;
			Y(win) += border_width;
		}
	} else if (!win->bordered) {
		win->bordered = 1;
		X(win) -= border_width;
		Y(win) -= border_width;
	}

	if (win->transient && (win->quirks & SWM_Q_TRANSSZ)) {
		WIDTH(win) = (double)WIDTH(r) * dialog_ratio;
		HEIGHT(win) = (double)HEIGHT(r) * dialog_ratio;
	}

	if (!win->manual) {
		/*
		 * floaters and transients are auto-centred unless moved
		 * or resized
		 */
		X(win) = X(r) + (WIDTH(r) - WIDTH(win)) /  2 - BORDER(win);
		Y(win) = Y(r) + (HEIGHT(r) - HEIGHT(win)) / 2 - BORDER(win);
	}

	/* keep window within region bounds */
	constrain_window(win, r, 0);

	update_window(win);
}

/*
 * Send keystrokes to terminal to decrease/increase the font size as the
 * window size changes.
 */
void
adjust_font(struct ws_win *win)
{
	if (!(win->quirks & SWM_Q_XTERM_FONTADJ) ||
	    win->floating || win->transient)
		return;

	if (win->sh.width_inc && win->last_inc != win->sh.width_inc &&
	    WIDTH(win) / win->sh.width_inc < term_width &&
	    win->font_steps < SWM_MAX_FONT_STEPS) {
		win->font_size_boundary[win->font_steps] =
		    (win->sh.width_inc * term_width) + win->sh.base_width;
		win->font_steps++;
		font_adjusted++;
		win->last_inc = win->sh.width_inc;
		fake_keypress(win, XK_KP_Subtract, ShiftMask);
	} else if (win->font_steps && win->last_inc != win->sh.width_inc &&
	    WIDTH(win) > win->font_size_boundary[win->font_steps - 1]) {
		win->font_steps--;
		font_adjusted++;
		win->last_inc = win->sh.width_inc;
		fake_keypress(win, XK_KP_Add, ShiftMask);
	}
}

#define SWAPXY(g)	do {				\
	int tmp;					\
	tmp = (g)->y; (g)->y = (g)->x; (g)->x = tmp;	\
	tmp = (g)->h; (g)->h = (g)->w; (g)->w = tmp;	\
} while (0)
void
stack_master(struct workspace *ws, struct swm_geometry *g, int rot, int flip)
{
	XWindowAttributes	wa;
	struct swm_geometry	win_g, r_g = *g;
	struct ws_win		*win, *fs_win = NULL;
	int			i, j, s, stacks;
	int			w_inc = 1, h_inc, w_base = 1, h_base;
	int			hrh, extra = 0, h_slice, last_h = 0;
	int			split, colno, winno, mwin, msize, mscale;
	int			remain, missing, v_slice, reconfigure;
	int			bordered = 1;

	DNPRINTF(SWM_D_STACK, "stack_master: workspace: %d, rot: %s, "
	    "flip: %s\n", ws->idx, YESNO(rot), YESNO(flip));

	winno = count_win(ws, 0);
	if (winno == 0 && count_win(ws, 1) == 0)
		return;

	TAILQ_FOREACH(win, &ws->winlist, entry)
		if (win->transient == 0 && win->floating == 0
		    && win->iconic == 0)
			break;

	if (win == NULL)
		goto notiles;

	if (rot) {
		w_inc = win->sh.width_inc;
		w_base = win->sh.base_width;
		mwin = ws->l_state.horizontal_mwin;
		mscale = ws->l_state.horizontal_msize;
		stacks = ws->l_state.horizontal_stacks;
		SWAPXY(&r_g);
	} else {
		w_inc = win->sh.height_inc;
		w_base = win->sh.base_height;
		mwin = ws->l_state.vertical_mwin;
		mscale = ws->l_state.vertical_msize;
		stacks = ws->l_state.vertical_stacks;
	}
	win_g = r_g;

	if (stacks > winno - mwin)
		stacks = winno - mwin;
	if (stacks < 1)
		stacks = 1;

	h_slice = r_g.h / SWM_H_SLICE;
	if (mwin && winno > mwin) {
		v_slice = r_g.w / SWM_V_SLICE;

		split = mwin;
		colno = split;
		win_g.w = v_slice * mscale;

		if (w_inc > 1 && w_inc < v_slice) {
			/* adjust for window's requested size increment */
			remain = (win_g.w - w_base) % w_inc;
			win_g.w -= remain;
		}

		msize = win_g.w;
		if (flip)
			win_g.x += r_g.w - msize;
	} else {
		msize = -2;
		colno = split = winno / stacks;
		win_g.w = ((r_g.w - (stacks * 2 * border_width) +
		    2 * border_width) / stacks);
	}
	hrh = r_g.h / colno;
	extra = r_g.h - (colno * hrh);
	win_g.h = hrh - 2 * border_width;

	/*  stack all the tiled windows */
	i = j = 0, s = stacks;
	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (win->transient != 0 || win->floating != 0)
			continue;
		if (win->iconic != 0)
			continue;

		if (win->ewmh_flags & EWMH_F_FULLSCREEN) {
			fs_win = win;
			continue;
		}

		if (split && i == split) {
			colno = (winno - mwin) / stacks;
			if (s <= (winno - mwin) % stacks)
				colno++;
			split = split + colno;
			hrh = (r_g.h / colno);
			extra = r_g.h - (colno * hrh);
			if (flip)
				win_g.x = r_g.x;
			else
				win_g.x += win_g.w + 2 * border_width;
			win_g.w = (r_g.w - msize -
			    (stacks * 2 * border_width)) / stacks;
			if (s == 1)
				win_g.w += (r_g.w - msize -
				    (stacks * 2 * border_width)) % stacks;
			s--;
			j = 0;
		}
		win_g.h = hrh - 2 * border_width;
		if (rot) {
			h_inc = win->sh.width_inc;
			h_base = win->sh.base_width;
		} else {
			h_inc =	win->sh.height_inc;
			h_base = win->sh.base_height;
		}
		if (j == colno - 1) {
			win_g.h = hrh + extra;
		} else if (h_inc > 1 && h_inc < h_slice) {
			/* adjust for window's requested size increment */
			remain = (win_g.h - h_base) % h_inc;
			missing = h_inc - remain;

			if (missing <= extra || j == 0) {
				extra -= missing;
				win_g.h += missing;
			} else {
				win_g.h -= remain;
				extra += remain;
			}
		}

		if (j == 0)
			win_g.y = r_g.y;
		else
			win_g.y += last_h + 2 * border_width;

		if (disable_border && bar_enabled == 0 && winno == 1){
			bordered = 0;
			win_g.w += 2 * border_width;
			win_g.h += 2 * border_width;
		} else {
			bordered = 1;
		}
		if (rot) {
			if (X(win) != win_g.y || Y(win) != win_g.x ||
			    WIDTH(win) != win_g.h || HEIGHT(win) != win_g.w) {
				reconfigure = 1;
				X(win) = win_g.y;
				Y(win) = win_g.x;
				WIDTH(win) = win_g.h;
				HEIGHT(win) = win_g.w;
			}
		} else {
			if (X(win) != win_g.x || Y(win) != win_g.y ||
			    WIDTH(win) != win_g.w || HEIGHT(win) != win_g.h) {
				reconfigure = 1;
				X(win) = win_g.x;
				Y(win) = win_g.y;
				WIDTH(win) = win_g.w;
				HEIGHT(win) = win_g.h;
			}
		}

		if (bordered != win->bordered) {
			reconfigure = 1;
			win->bordered = bordered;
		}

		if (reconfigure) {
			adjust_font(win);
			update_window(win);
		}

		if (XGetWindowAttributes(display, win->id, &wa))
			if (wa.map_state == IsUnmapped)
				map_window_raised(win->id);

		last_h = win_g.h;
		i++;
		j++;
	}

notiles:
	/* now, stack all the floaters and transients */
	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (win->transient == 0 && win->floating == 0)
			continue;
		if (win->iconic == 1)
			continue;
		if (win->ewmh_flags & EWMH_F_FULLSCREEN) {
			fs_win = win;
			continue;
		}

		stack_floater(win, ws->r);
		map_window_raised(win->id);
	}

	if (fs_win) {
		stack_floater(fs_win, ws->r);
		map_window_raised(fs_win->id);
	}
}

void
vertical_config(struct workspace *ws, int id)
{
	DNPRINTF(SWM_D_STACK, "vertical_config: id: %d, workspace: %d\n",
	    id, ws->idx);

	switch (id) {
	case SWM_ARG_ID_STACKRESET:
	case SWM_ARG_ID_STACKINIT:
		ws->l_state.vertical_msize = SWM_V_SLICE / 2;
		ws->l_state.vertical_mwin = 1;
		ws->l_state.vertical_stacks = 1;
		break;
	case SWM_ARG_ID_MASTERSHRINK:
		if (ws->l_state.vertical_msize > 1)
			ws->l_state.vertical_msize--;
		break;
	case SWM_ARG_ID_MASTERGROW:
		if (ws->l_state.vertical_msize < SWM_V_SLICE - 1)
			ws->l_state.vertical_msize++;
		break;
	case SWM_ARG_ID_MASTERADD:
		ws->l_state.vertical_mwin++;
		break;
	case SWM_ARG_ID_MASTERDEL:
		if (ws->l_state.vertical_mwin > 0)
			ws->l_state.vertical_mwin--;
		break;
	case SWM_ARG_ID_STACKINC:
		ws->l_state.vertical_stacks++;
		break;
	case SWM_ARG_ID_STACKDEC:
		if (ws->l_state.vertical_stacks > 1)
			ws->l_state.vertical_stacks--;
		break;
	case SWM_ARG_ID_FLIPLAYOUT:
		ws->l_state.vertical_flip = !ws->l_state.vertical_flip;
		break;
	default:
		return;
	}
}

void
vertical_stack(struct workspace *ws, struct swm_geometry *g)
{
	DNPRINTF(SWM_D_STACK, "vertical_stack: workspace: %d\n", ws->idx);

	stack_master(ws, g, 0, ws->l_state.vertical_flip);
}

void
horizontal_config(struct workspace *ws, int id)
{
	DNPRINTF(SWM_D_STACK, "horizontal_config: workspace: %d\n", ws->idx);

	switch (id) {
	case SWM_ARG_ID_STACKRESET:
	case SWM_ARG_ID_STACKINIT:
		ws->l_state.horizontal_mwin = 1;
		ws->l_state.horizontal_msize = SWM_H_SLICE / 2;
		ws->l_state.horizontal_stacks = 1;
		break;
	case SWM_ARG_ID_MASTERSHRINK:
		if (ws->l_state.horizontal_msize > 1)
			ws->l_state.horizontal_msize--;
		break;
	case SWM_ARG_ID_MASTERGROW:
		if (ws->l_state.horizontal_msize < SWM_H_SLICE - 1)
			ws->l_state.horizontal_msize++;
		break;
	case SWM_ARG_ID_MASTERADD:
		ws->l_state.horizontal_mwin++;
		break;
	case SWM_ARG_ID_MASTERDEL:
		if (ws->l_state.horizontal_mwin > 0)
			ws->l_state.horizontal_mwin--;
		break;
	case SWM_ARG_ID_STACKINC:
		ws->l_state.horizontal_stacks++;
		break;
	case SWM_ARG_ID_STACKDEC:
		if (ws->l_state.horizontal_stacks > 1)
			ws->l_state.horizontal_stacks--;
		break;
	case SWM_ARG_ID_FLIPLAYOUT:
		ws->l_state.horizontal_flip = !ws->l_state.horizontal_flip;
		break;
	default:
		return;
	}
}

void
horizontal_stack(struct workspace *ws, struct swm_geometry *g)
{
	DNPRINTF(SWM_D_STACK, "horizontal_stack: workspace: %d\n", ws->idx);

	stack_master(ws, g, 1, ws->l_state.horizontal_flip);
}

/* fullscreen view */
void
max_stack(struct workspace *ws, struct swm_geometry *g)
{
	struct swm_geometry	gg = *g;
	struct ws_win		*win, *wintrans = NULL, *parent = NULL;
	int			winno, num_screens;

	DNPRINTF(SWM_D_STACK, "max_stack: workspace: %d\n", ws->idx);

	if (ws == NULL)
		return;

	winno = count_win(ws, 0);
	if (winno == 0 && count_win(ws, 1) == 0)
		return;

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (win->transient) {
			wintrans = win;
			parent = find_window(win->transient);
			continue;
		}

		if (win->floating && win->floatmaxed == 0 ) {
			/*
			 * retain geometry for retrieval on exit from
			 * max_stack mode
			 */
			store_float_geom(win, ws->r);
			win->floatmaxed = 1;
		}

		/* only reconfigure if necessary */
		if (X(win) != gg.x || Y(win) != gg.y || WIDTH(win) != gg.w ||
		    HEIGHT(win) != gg.h) {
			win->g = gg;
			if (bar_enabled){
				win->bordered = 1;
			} else {
				win->bordered = 0;
				WIDTH(win) += 2 * border_width;
				HEIGHT(win) += 2 * border_width;
			}

			update_window(win);
		}
		/* unmap only if we don't have multi screen */
		if (win != ws->focus)
			if (!(num_screens > 1 || outputs > 1))
				unmap_window(win);
	}

	/* put the last transient on top */
	if (wintrans) {
		if (parent)
			map_window_raised(parent->id);
		stack_floater(wintrans, ws->r);
		focus_magic(wintrans);
	}
}

void
send_to_ws(struct swm_region *r, union arg *args)
{
	int			wsid = args->id;
	struct ws_win		*win = NULL, *parent;
	struct workspace	*ws, *nws;
	xcb_atom_t		ws_idx_atom = XCB_ATOM_NONE;
	char			ws_idx_str[SWM_PROPLEN];
	union arg		a;

	if (wsid >= workspace_limit)
		return;

	if (r && r->ws && r->ws->focus)
		win = r->ws->focus;
	else
		return;
	if (win == NULL)
		return;
	if (win->ws->idx == wsid)
		return;

	DNPRINTF(SWM_D_MOVE, "send_to_ws: window: 0x%x\n", win->id);

	ws = win->ws;
	nws = &win->s->ws[wsid];

	a.id = SWM_ARG_ID_FOCUSPREV;
	focus(r, &a);
	if (win->transient) {
		parent = find_window(win->transient);
		if (parent) {
			unmap_window(parent);
			TAILQ_REMOVE(&ws->winlist, parent, entry);
			TAILQ_INSERT_TAIL(&nws->winlist, parent, entry);
			parent->ws = nws;
		}
	}
	unmap_window(win);
	TAILQ_REMOVE(&ws->winlist, win, entry);
	TAILQ_INSERT_TAIL(&nws->winlist, win, entry);
	if (TAILQ_EMPTY(&ws->winlist))
		r->ws->focus = NULL;
	win->ws = nws;

	/* Try to update the window's workspace property */
	ws_idx_atom = get_atom_from_string("_SWM_WS");
	if (ws_idx_atom &&
	    snprintf(ws_idx_str, SWM_PROPLEN, "%d", nws->idx) <
	        SWM_PROPLEN) {
		DNPRINTF(SWM_D_PROP, "send_to_ws: set property: _SWM_WS: %s\n",
		    ws_idx_str);
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win->id,
			ws_idx_atom, XCB_ATOM_STRING, 8, strlen(ws_idx_str),
			ws_idx_str);
	}

	stack();
	bar_update();
}

void
pressbutton(struct swm_region *r, union arg *args)
{
	XTestFakeButtonEvent(display, args->id, True, CurrentTime);
	XTestFakeButtonEvent(display, args->id, False, CurrentTime);
}

void
raise_toggle(struct swm_region *r, union arg *args)
{
	if (r == NULL || r->ws == NULL)
		return;

	r->ws->always_raise = !r->ws->always_raise;

	/* bring floaters back to top */
	if (r->ws->always_raise == 0)
		stack();
}

void
iconify(struct swm_region *r, union arg *args)
{
	union arg a;

	if (r->ws->focus == NULL)
		return;
	unmap_window(r->ws->focus);
	update_iconic(r->ws->focus, 1);
	stack();
	if (focus_mode == SWM_FOCUS_DEFAULT)
		drain_enter_notify();
	r->ws->focus = NULL;
	a.id = SWM_ARG_ID_FOCUSCUR;
	focus(r, &a);
}

char *
get_win_name(xcb_window_t win)
{
	char				*name = NULL;
	xcb_get_property_cookie_t	c;
	xcb_get_text_property_reply_t	r;

	c = xcb_icccm_get_wm_name(conn, win);
	if (xcb_icccm_get_wm_name_reply(conn, c, &r, NULL)) {
		name = malloc(r.name_len + 1);
		if (!name) {
			xcb_get_text_property_reply_wipe(&r);
			return (NULL);
		}
		memcpy(name, r.name, r.name_len);
		name[r.name_len] = '\0';
	}

	xcb_get_text_property_reply_wipe(&r);

	return (name);
}

void
uniconify(struct swm_region *r, union arg *args)
{
	struct ws_win		*win;
	FILE			*lfile;
	char			*name;
	int			count = 0;

	DNPRINTF(SWM_D_MISC, "uniconify\n");

	if (r == NULL || r->ws == NULL)
		return;

	/* make sure we have anything to uniconify */
	TAILQ_FOREACH(win, &r->ws->winlist, entry) {
		if (win->ws == NULL)
			continue; /* should never happen */
		if (win->iconic == 0)
			continue;
		count++;
	}
	if (count == 0)
		return;

	search_r = r;
	search_resp_action = SWM_SEARCH_UNICONIFY;

	spawn_select(r, args, "search", &searchpid);

	if ((lfile = fdopen(select_list_pipe[1], "w")) == NULL)
		return;

	TAILQ_FOREACH(win, &r->ws->winlist, entry) {
		if (win->ws == NULL)
			continue; /* should never happen */
		if (win->iconic == 0)
			continue;

		name = get_win_name(win->id);
		if (name == NULL)
			continue;
		fprintf(lfile, "%s.%u\n", name, win->id);
		free(name);
	}

	fclose(lfile);
}

void
name_workspace(struct swm_region *r, union arg *args)
{
	FILE			*lfile;

	DNPRINTF(SWM_D_MISC, "name_workspace\n");

	if (r == NULL)
		return;

	search_r = r;
	search_resp_action = SWM_SEARCH_NAME_WORKSPACE;

	spawn_select(r, args, "name_workspace", &searchpid);

	if ((lfile = fdopen(select_list_pipe[1], "w")) == NULL)
		return;

	fprintf(lfile, "%s", "");
	fclose(lfile);
}

void
search_workspace(struct swm_region *r, union arg *args)
{
	int			i;
	struct workspace	*ws;
	FILE			*lfile;

	DNPRINTF(SWM_D_MISC, "search_workspace\n");

	if (r == NULL)
		return;

	search_r = r;
	search_resp_action = SWM_SEARCH_SEARCH_WORKSPACE;

	spawn_select(r, args, "search", &searchpid);

	if ((lfile = fdopen(select_list_pipe[1], "w")) == NULL)
		return;

	for (i = 0; i < workspace_limit; i++) {
		ws = &r->s->ws[i];
		if (ws == NULL)
			continue;
		fprintf(lfile, "%d%s%s\n", ws->idx + 1,
		    (ws->name ? ":" : ""), (ws->name ? ws->name : ""));
	}

	fclose(lfile);
}

void
search_win_cleanup(void)
{
	struct search_window	*sw = NULL;

	while ((sw = TAILQ_FIRST(&search_wl)) != NULL) {
		xcb_destroy_window(conn, sw->indicator);
		xcb_free_gc(conn, sw->gc);
		TAILQ_REMOVE(&search_wl, sw, entry);
		free(sw);
	}
}

void
search_win(struct swm_region *r, union arg *args)
{
	struct ws_win		*win = NULL;
	struct search_window	*sw = NULL;
	xcb_window_t		w;
	uint32_t		gcv[1], wa[2];
	int			i;
	char			s[8];
	FILE			*lfile;
	size_t			len;
	XRectangle		ibox, lbox;
	xcb_screen_t		*screen; 
	DNPRINTF(SWM_D_MISC, "search_win\n");

	search_r = r;
	search_resp_action = SWM_SEARCH_SEARCH_WINDOW;

	spawn_select(r, args, "search", &searchpid);

	if ((lfile = fdopen(select_list_pipe[1], "w")) == NULL)
		return;

	TAILQ_INIT(&search_wl);

	i = 1;
	TAILQ_FOREACH(win, &r->ws->winlist, entry) {
		if (win->iconic == 1)
			continue;

		sw = calloc(1, sizeof(struct search_window));
		if (sw == NULL) {
			warn("search_win: calloc");
			fclose(lfile);
			search_win_cleanup();
			return;
		}
		sw->idx = i;
		sw->win = win;
		screen = get_screen(sw->idx);

		snprintf(s, sizeof s, "%d", i);
		len = strlen(s);

		XmbTextExtents(bar_fs, s, len, &ibox, &lbox);

		w = xcb_generate_id(conn);
		wa[0] = r->s->c[SWM_S_COLOR_FOCUS].color;
		wa[1] = r->s->c[SWM_S_COLOR_UNFOCUS].color;
		xcb_create_window(conn, screen->root_depth, w, win->id,
			0, 0, lbox.width + 4,
			bar_fs_extents->max_logical_extent.height, 1,
			XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
			XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL, wa);	
		
		sw->indicator = w;
		TAILQ_INSERT_TAIL(&search_wl, sw, entry);

		sw->gc = xcb_generate_id(conn);
		gcv[0] = r->s->c[SWM_S_COLOR_BAR].color;		
		xcb_create_gc(conn, sw->gc, w, XCB_GC_FOREGROUND, gcv); 
		map_window_raised(w);

		xcb_image_text_8(conn, len, w, sw->gc, 2,
		    (bar_fs_extents->max_logical_extent.height -
		    lbox.height) / 2 - lbox.y, s);

		fprintf(lfile, "%d\n", i);
		i++;
	}

	fclose(lfile);
}

void
search_resp_uniconify(char *resp, unsigned long len)
{
	char			*name;
	struct ws_win		*win;
	char			*s;

	DNPRINTF(SWM_D_MISC, "search_resp_uniconify: resp: %s\n", resp);

	TAILQ_FOREACH(win, &search_r->ws->winlist, entry) {
		if (win->iconic == 0)
			continue;
		name = get_win_name(win->id);
		if (name == NULL)
			continue;
		if (asprintf(&s, "%s.%u", name, win->id) == -1) {
			free(name);
			continue;
		}
		free(name);
		if (strncmp(s, resp, len) == 0) {
			/* XXX this should be a callback to generalize */
			update_iconic(win, 0);
			free(s);
			break;
		}
		free(s);
	}
}

void
search_resp_name_workspace(char *resp, unsigned long len)
{
	struct workspace	*ws;

	DNPRINTF(SWM_D_MISC, "search_resp_name_workspace: resp: %s\n", resp);

	if (search_r->ws == NULL)
		return;
	ws = search_r->ws;

	if (ws->name) {
		free(search_r->ws->name);
		search_r->ws->name = NULL;
	}

	if (len > 1) {
		ws->name = strdup(resp);
		if (ws->name == NULL) {
			DNPRINTF(SWM_D_MISC, "search_resp_name_workspace: "
			    "strdup: %s", strerror(errno));
			return;
		}
	}
}

void
search_resp_search_workspace(char *resp, unsigned long len)
{
	char			*p, *q;
	int			ws_idx;
	const char		*errstr;
	union arg		a;

	DNPRINTF(SWM_D_MISC, "search_resp_search_workspace: resp: %s\n", resp);

	q = strdup(resp);
	if (!q) {
		DNPRINTF(SWM_D_MISC, "search_resp_search_workspace: strdup: %s",
		    strerror(errno));
		return;
	}
	p = strchr(q, ':');
	if (p != NULL)
		*p = '\0';
	ws_idx = strtonum(q, 1, workspace_limit, &errstr);
	if (errstr) {
		DNPRINTF(SWM_D_MISC, "workspace idx is %s: %s",
		    errstr, q);
		free(q);
		return;
	}
	free(q);
	a.id = ws_idx - 1;
	switchws(search_r, &a);
}

void
search_resp_search_window(char *resp, unsigned long len)
{
	char			*s;
	int			idx;
	const char		*errstr;
	struct search_window	*sw;

	DNPRINTF(SWM_D_MISC, "search_resp_search_window: resp: %s\n", resp);

	s = strdup(resp);
	if (!s) {
		DNPRINTF(SWM_D_MISC, "search_resp_search_window: strdup: %s",
		    strerror(errno));
		return;
	}

	idx = strtonum(s, 1, INT_MAX, &errstr);
	if (errstr) {
		DNPRINTF(SWM_D_MISC, "window idx is %s: %s",
		    errstr, s);
		free(s);
		return;
	}
	free(s);

	TAILQ_FOREACH(sw, &search_wl, entry)
		if (idx == sw->idx) {
			focus_win(sw->win);
			break;
		}
}

#define MAX_RESP_LEN	1024

void
search_do_resp(void)
{
	ssize_t			rbytes;
	char			*resp;
	unsigned long		len;

	DNPRINTF(SWM_D_MISC, "search_do_resp:\n");

	search_resp = 0;
	searchpid = 0;

	if ((resp = calloc(1, MAX_RESP_LEN + 1)) == NULL) {
		warn("search: calloc");
		goto done;
	}

	rbytes = read(select_resp_pipe[0], resp, MAX_RESP_LEN);
	if (rbytes <= 0) {
		warn("search: read error");
		goto done;
	}
	resp[rbytes] = '\0';

	/* XXX:
	 * Older versions of dmenu (Atleast pre 4.4.1) do not send a
	 * newline, so work around that by sanitizing the resp now.
	 */
	resp[strcspn(resp, "\n")] = '\0';
	len = strlen(resp);

	switch (search_resp_action) {
	case SWM_SEARCH_UNICONIFY:
		search_resp_uniconify(resp, len);
		break;
	case SWM_SEARCH_NAME_WORKSPACE:
		search_resp_name_workspace(resp, len);
		break;
	case SWM_SEARCH_SEARCH_WORKSPACE:
		search_resp_search_workspace(resp, len);
		break;
	case SWM_SEARCH_SEARCH_WINDOW:
		search_resp_search_window(resp, len);
		break;
	}

done:
	if (search_resp_action == SWM_SEARCH_SEARCH_WINDOW)
		search_win_cleanup();

	search_resp_action = SWM_SEARCH_NONE;
	close(select_resp_pipe[0]);
	free(resp);
}

void
wkill(struct swm_region *r, union arg *args)
{
	DNPRINTF(SWM_D_MISC, "wkill: id: %d\n", args->id);

	if (r->ws->focus == NULL)
		return;

	if (args->id == SWM_ARG_ID_KILLWINDOW)
		xcb_kill_client(conn, r->ws->focus->id);
	else
		if (r->ws->focus->can_delete)
			client_msg(r->ws->focus, adelete);
}


int
floating_toggle_win(struct ws_win *win)
{
	struct swm_region	*r;

	if (win == NULL)
		return (0);

	if (!win->ws->r)
		return (0);

	r = win->ws->r;

	/* reject floating toggles in max stack mode */
	if (win->ws->cur_layout == &layouts[SWM_MAX_STACK])
		return (0);

	if (win->floating) {
		if (!win->floatmaxed) {
			/* retain position for refloat */
			store_float_geom(win, r);
		}
		win->floating = 0;
	} else {
		if (win->g_floatvalid) {
			/* refloat at last floating relative position */
			X(win) = win->g_float.x + X(r);
			Y(win) = win->g_float.y + Y(r);
			WIDTH(win) = win->g_float.w;
			HEIGHT(win) = win->g_float.h;
		}
		win->floating = 1;
	}

	ewmh_update_actions(win);

	return (1);
}

void
floating_toggle(struct swm_region *r, union arg *args)
{
	struct ws_win		*win = r->ws->focus;
	union arg		a;

	if (win == NULL)
		return;

	ewmh_update_win_state(win, ewmh[_NET_WM_STATE_ABOVE].atom,
	    _NET_WM_STATE_TOGGLE);

	stack();
	if (focus_mode == SWM_FOCUS_DEFAULT)
		drain_enter_notify();

	if (win == win->ws->focus) {
		a.id = SWM_ARG_ID_FOCUSCUR;
		focus(win->ws->r, &a);
	}
}

void
constrain_window(struct ws_win *win, struct swm_region *r, int resizable)
{
	if (MAX_X(win) + BORDER(win) > MAX_X(r)) {
		if (resizable)
			WIDTH(win) = MAX_X(r) - X(win) - BORDER(win);
		else
			X(win) = MAX_X(r)- WIDTH(win) - BORDER(win);
	}

	if (X(win) + BORDER(win) < X(r)) {
		if (resizable)
			WIDTH(win) -= X(r) - X(win) - BORDER(win);

		X(win) = X(r) - BORDER(win);
	}

	if (MAX_Y(win) + BORDER(win) > MAX_Y(r)) {
		if (resizable)
			HEIGHT(win) = MAX_Y(r) - Y(win) - BORDER(win);
		else
			Y(win) = MAX_Y(r) - HEIGHT(win) - BORDER(win);
	}

	if (Y(win) + BORDER(win) < Y(r)) {
		if (resizable)
			HEIGHT(win) -= Y(r) - Y(win) - BORDER(win);

		Y(win) = Y(r) - BORDER(win);
	}

	if (resizable) {
		if (WIDTH(win) < 1)
			WIDTH(win) = 1;
		if (HEIGHT(win) < 1)
			HEIGHT(win) = 1;
	}
}

void
update_window(struct ws_win *win)
{
	uint16_t	mask;
	uint32_t	wc[5];

	mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
		XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
		XCB_CONFIG_WINDOW_BORDER_WIDTH;
	wc[0] = X(win);
	wc[1] = Y(win);
	wc[2] = WIDTH(win);
	wc[3] = HEIGHT(win);
	wc[4] = BORDER(win);

	DNPRINTF(SWM_D_EVENT, "update_window: window: 0x%x, (x,y) w x h: "
	    "(%d,%d) %d x %d, bordered: %s\n", win->id, wc[0], wc[1], wc[2],
	    wc[3], YESNO(win->bordered));

	xcb_configure_window(conn, win->id, mask, wc);
}

#define SWM_RESIZE_STEPS	(50)

void
resize(struct ws_win *win, union arg *args)
{
	XEvent			ev;
	Time			time = 0;
	struct swm_region	*r = NULL;
	int			resize_step = 0;
	struct swm_geometry	g;
	int			top = 0, left = 0;
	int			dx, dy;
	unsigned int		shape; /* cursor style */
	xcb_cursor_t		cursor;
	xcb_font_t		cursor_font;
	xcb_grab_pointer_reply_t	*gpr;
	xcb_query_pointer_reply_t	*xpr;

	if (win == NULL)
		return;
	r = win->ws->r;

	if (win->ewmh_flags & EWMH_F_FULLSCREEN)
		return;

	DNPRINTF(SWM_D_MOUSE, "resize: window: 0x%x, floating: %s, "
	    "transient: 0x%x\n", win->id, YESNO(win->floating),
	    win->transient);

	if (!(win->transient != 0 || win->floating != 0))
		return;

	/* reject resizes in max mode for floaters (transient ok) */
	if (win->floatmaxed)
		return;

	win->manual = 1;
	ewmh_update_win_state(win, ewmh[_SWM_WM_STATE_MANUAL].atom,
	    _NET_WM_STATE_ADD);

	stack();

	switch (args->id) {
	case SWM_ARG_ID_WIDTHSHRINK:
		WIDTH(win) -= SWM_RESIZE_STEPS;
		resize_step = 1;
		break;
	case SWM_ARG_ID_WIDTHGROW:
		WIDTH(win) += SWM_RESIZE_STEPS;
		resize_step = 1;
		break;
	case SWM_ARG_ID_HEIGHTSHRINK:
		HEIGHT(win) -= SWM_RESIZE_STEPS;
		resize_step = 1;
		break;
	case SWM_ARG_ID_HEIGHTGROW:
		HEIGHT(win) += SWM_RESIZE_STEPS;
		resize_step = 1;
		break;
	default:
		break;
	}
	if (resize_step) {
		constrain_window(win, r, 1);
		update_window(win);
		store_float_geom(win,r);
		return;
	}

	if (focus_mode == SWM_FOCUS_DEFAULT)
		drain_enter_notify();

	/* get cursor offset from window root */
	xpr = xcb_query_pointer_reply(conn, xcb_query_pointer(conn, win->id),
		NULL);
	if (!xpr)
		return;
	
	g = win->g;

	if (xpr->win_x < WIDTH(win) / 2)
		left = 1;

	if (xpr->win_y < HEIGHT(win) / 2)
		top = 1;

	if (args->id == SWM_ARG_ID_CENTER)
		shape = XC_sizing;
	else if (top)
		shape = (left) ? XC_top_left_corner : XC_top_right_corner;
	else
		shape = (left) ? XC_bottom_left_corner : XC_bottom_right_corner;

	cursor_font = xcb_generate_id(conn);
	xcb_open_font(conn, cursor_font, strlen("cursor"), "cursor");

	cursor = xcb_generate_id(conn);
	xcb_create_glyph_cursor(conn, cursor, cursor_font, cursor_font,
		shape, shape + 1, 0, 0, 0, 0xffff, 0xffff, 0xffff);

	gpr = xcb_grab_pointer_reply(conn,
		xcb_grab_pointer(conn, False, win->id, MOUSEMASK,
		XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
		cursor, XCB_CURRENT_TIME),
		NULL);
	if (!gpr) {
		xcb_free_cursor(conn, cursor);
		xcb_close_font(conn, cursor_font);
		free(xpr);
		return;
	}

	do {
		XMaskEvent(display, MOUSEMASK | ExposureMask |
		    SubstructureRedirectMask, &ev);
		switch (ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			/* cursor offset/delta from start of the operation */
			dx = ev.xmotion.x_root - xpr->root_x;
			dy = ev.xmotion.y_root - xpr->root_y;

			/* vertical */
			if (top)
				dy = -dy;

			if (args->id == SWM_ARG_ID_CENTER) {
				if (g.h / 2 + dy < 1)
					dy = 1 - g.h / 2;

				Y(win) = g.y - dy;
				HEIGHT(win) = g.h + 2 * dy;
			} else {
				if (g.h + dy < 1)
					dy = 1 - g.h;

				if (top)
					Y(win) = g.y - dy;

				HEIGHT(win) = g.h + dy;
			}

			/* horizontal */
			if (left)
				dx = -dx;

			if (args->id == SWM_ARG_ID_CENTER) {
				if (g.w / 2 + dx < 1)
					dx = 1 - g.w / 2;

				X(win) = g.x - dx;
				WIDTH(win) = g.w + 2 * dx;
			} else {
				if (g.w + dx < 1)
					dx = 1 - g.w;

				if (left)
					X(win) = g.x - dx;

				WIDTH(win) = g.w + dx;
			}

			constrain_window(win, r, 1);

			/* not free, don't sync more than 120 times / second */
			if ((ev.xmotion.time - time) > (1000 / 120) ) {
				time = ev.xmotion.time;
				do_sync();
				update_window(win);
			}
			break;
		}
	} while (ev.type != ButtonRelease);
	if (time) {
		do_sync();
		update_window(win);
	}
	store_float_geom(win,r);

	xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
	xcb_free_cursor(conn, cursor);
	xcb_close_font(conn, cursor_font);
	free(gpr);
	free(xpr);

	/* drain events */
	drain_enter_notify();
}

void
resize_step(struct swm_region *r, union arg *args)
{
	struct ws_win		*win = NULL;

	if (r && r->ws && r->ws->focus)
		win = r->ws->focus;
	else
		return;

	resize(win, args);
}

#define SWM_MOVE_STEPS	(50)

void
move(struct ws_win *win, union arg *args)
{
	XEvent			ev;
	Time			time = 0;
	int			move_step = 0;
	struct swm_region	*r = NULL;
	xcb_font_t			cursor_font;
	xcb_cursor_t			cursor;
	xcb_grab_pointer_reply_t	*gpr;
	xcb_query_pointer_reply_t	*qpr;

	if (win == NULL)
		return;
	r = win->ws->r;

	if (win->ewmh_flags & EWMH_F_FULLSCREEN)
		return;

	DNPRINTF(SWM_D_MOUSE, "move: window: 0x%x, floating: %s, transient: "
	    "0x%x\n", win->id, YESNO(win->floating), win->transient);

	/* in max_stack mode should only move transients */
	if (win->ws->cur_layout == &layouts[SWM_MAX_STACK] && !win->transient)
		return;

	win->manual = 1;
	if (win->floating == 0 && !win->transient) {
		store_float_geom(win, r);
		ewmh_update_win_state(win, ewmh[_NET_WM_STATE_ABOVE].atom,
		    _NET_WM_STATE_ADD);
	}
	ewmh_update_win_state(win, ewmh[_SWM_WM_STATE_MANUAL].atom,
	    _NET_WM_STATE_ADD);

	stack();

	move_step = 0;
	switch (args->id) {
	case SWM_ARG_ID_MOVELEFT:
		X(win) -= (SWM_MOVE_STEPS - border_width);
		move_step = 1;
		break;
	case SWM_ARG_ID_MOVERIGHT:
		X(win) += (SWM_MOVE_STEPS - border_width);
		move_step = 1;
		break;
	case SWM_ARG_ID_MOVEUP:
		Y(win) -= (SWM_MOVE_STEPS - border_width);
		move_step = 1;
		break;
	case SWM_ARG_ID_MOVEDOWN:
		Y(win) += (SWM_MOVE_STEPS - border_width);
		move_step = 1;
		break;
	default:
		break;
	}
	if (move_step) {
		constrain_window(win, r, 0);
		update_window(win);
		store_float_geom(win, r);
		return;
	}

	cursor_font = xcb_generate_id(conn);
	xcb_open_font(conn, cursor_font, strlen("cursor"), "cursor");
	
	cursor = xcb_generate_id(conn);
	xcb_create_glyph_cursor(conn, cursor, cursor_font, cursor_font,
		XC_fleur, XC_fleur + 1, 0, 0, 0, 0xffff, 0xffff, 0xffff);

	gpr = xcb_grab_pointer_reply(conn,
		xcb_grab_pointer(conn, False, win->id, MOUSEMASK,
			XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
			XCB_WINDOW_NONE, cursor, XCB_CURRENT_TIME),
		NULL);
	if (!gpr) {
		xcb_free_cursor(conn, cursor);
		xcb_close_font(conn, cursor_font);
		return;
	}

	/* get cursor offset from window root */
	qpr = xcb_query_pointer_reply(conn, xcb_query_pointer(conn, win->id),
		NULL);
	if (!qpr) {
		xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
		xcb_free_cursor(conn, cursor);
		xcb_close_font(conn, cursor_font);
		return;
	}
	do {
		XMaskEvent(display, MOUSEMASK | ExposureMask |
		    SubstructureRedirectMask, &ev);
		switch (ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			X(win) = ev.xmotion.x_root - qpr->win_x - border_width;
			Y(win) = ev.xmotion.y_root - qpr->win_y - border_width;

			constrain_window(win, r, 0);

			/* not free, don't sync more than 120 times / second */
			if ((ev.xmotion.time - time) > (1000 / 120) ) {
				time = ev.xmotion.time;
				do_sync();
				update_window(win);
			}
			break;
		}
	} while (ev.type != ButtonRelease);
	if (time) {
		do_sync();
		update_window(win);
	}
	store_float_geom(win, r);
	free(qpr);
	xcb_free_cursor(conn, cursor);
	xcb_close_font(conn, cursor_font);
	xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);

	/* drain events */
	drain_enter_notify();
}

void
move_step(struct swm_region *r, union arg *args)
{
	struct ws_win		*win = NULL;

	if (r && r->ws && r->ws->focus)
		win = r->ws->focus;
	else
		return;

	if (!(win->transient != 0 || win->floating != 0))
		return;

	move(win, args);
}


/* user/key callable function IDs */
enum keyfuncid {
	kf_cycle_layout,
	kf_flip_layout,
	kf_stack_reset,
	kf_master_shrink,
	kf_master_grow,
	kf_master_add,
	kf_master_del,
	kf_stack_inc,
	kf_stack_dec,
	kf_swap_main,
	kf_focus_next,
	kf_focus_prev,
	kf_swap_next,
	kf_swap_prev,
	kf_quit,
	kf_restart,
	kf_focus_main,
	kf_ws_1,
	kf_ws_2,
	kf_ws_3,
	kf_ws_4,
	kf_ws_5,
	kf_ws_6,
	kf_ws_7,
	kf_ws_8,
	kf_ws_9,
	kf_ws_10,
	kf_ws_11,
	kf_ws_12,
	kf_ws_13,
	kf_ws_14,
	kf_ws_15,
	kf_ws_16,
	kf_ws_17,
	kf_ws_18,
	kf_ws_19,
	kf_ws_20,
	kf_ws_21,
	kf_ws_22,
	kf_ws_next,
	kf_ws_prev,
	kf_ws_next_all,
	kf_ws_prev_all,
	kf_ws_prior,
	kf_screen_next,
	kf_screen_prev,
	kf_mvws_1,
	kf_mvws_2,
	kf_mvws_3,
	kf_mvws_4,
	kf_mvws_5,
	kf_mvws_6,
	kf_mvws_7,
	kf_mvws_8,
	kf_mvws_9,
	kf_mvws_10,
	kf_mvws_11,
	kf_mvws_12,
	kf_mvws_13,
	kf_mvws_14,
	kf_mvws_15,
	kf_mvws_16,
	kf_mvws_17,
	kf_mvws_18,
	kf_mvws_19,
	kf_mvws_20,
	kf_mvws_21,
	kf_mvws_22,
	kf_bar_toggle,
	kf_wind_kill,
	kf_wind_del,
	kf_float_toggle,
	kf_version,
	kf_spawn_custom,
	kf_iconify,
	kf_uniconify,
	kf_raise_toggle,
	kf_button2,
	kf_width_shrink,
	kf_width_grow,
	kf_height_shrink,
	kf_height_grow,
	kf_move_left,
	kf_move_right,
	kf_move_up,
	kf_move_down,
	kf_name_workspace,
	kf_search_workspace,
	kf_search_win,
	kf_dumpwins, /* MUST BE LAST */
	kf_invalid
};

/* key definitions */
void
dummykeyfunc(struct swm_region *r, union arg *args)
{
};

struct keyfunc {
	char			name[SWM_FUNCNAME_LEN];
	void			(*func)(struct swm_region *r, union arg *);
	union arg		args;
} keyfuncs[kf_invalid + 1] = {
	/* name			function	argument */
	{ "cycle_layout",	cycle_layout,	{0} },
	{ "flip_layout",	stack_config,	{.id = SWM_ARG_ID_FLIPLAYOUT} },
	{ "stack_reset",	stack_config,	{.id = SWM_ARG_ID_STACKRESET} },
	{ "master_shrink",	stack_config,	{.id = SWM_ARG_ID_MASTERSHRINK} },
	{ "master_grow",	stack_config,	{.id = SWM_ARG_ID_MASTERGROW} },
	{ "master_add",		stack_config,	{.id = SWM_ARG_ID_MASTERADD} },
	{ "master_del",		stack_config,	{.id = SWM_ARG_ID_MASTERDEL} },
	{ "stack_inc",		stack_config,	{.id = SWM_ARG_ID_STACKINC} },
	{ "stack_dec",		stack_config,	{.id = SWM_ARG_ID_STACKDEC} },
	{ "swap_main",		swapwin,	{.id = SWM_ARG_ID_SWAPMAIN} },
	{ "focus_next",		focus,		{.id = SWM_ARG_ID_FOCUSNEXT} },
	{ "focus_prev",		focus,		{.id = SWM_ARG_ID_FOCUSPREV} },
	{ "swap_next",		swapwin,	{.id = SWM_ARG_ID_SWAPNEXT} },
	{ "swap_prev",		swapwin,	{.id = SWM_ARG_ID_SWAPPREV} },
	{ "quit",		quit,		{0} },
	{ "restart",		restart,	{0} },
	{ "focus_main",		focus,		{.id = SWM_ARG_ID_FOCUSMAIN} },
	{ "ws_1",		switchws,	{.id = 0} },
	{ "ws_2",		switchws,	{.id = 1} },
	{ "ws_3",		switchws,	{.id = 2} },
	{ "ws_4",		switchws,	{.id = 3} },
	{ "ws_5",		switchws,	{.id = 4} },
	{ "ws_6",		switchws,	{.id = 5} },
	{ "ws_7",		switchws,	{.id = 6} },
	{ "ws_8",		switchws,	{.id = 7} },
	{ "ws_9",		switchws,	{.id = 8} },
	{ "ws_10",		switchws,	{.id = 9} },
	{ "ws_11",		switchws,	{.id = 10} },
	{ "ws_12",		switchws,	{.id = 11} },
	{ "ws_13",		switchws,	{.id = 12} },
	{ "ws_14",		switchws,	{.id = 13} },
	{ "ws_15",		switchws,	{.id = 14} },
	{ "ws_16",		switchws,	{.id = 15} },
	{ "ws_17",		switchws,	{.id = 16} },
	{ "ws_18",		switchws,	{.id = 17} },
	{ "ws_19",		switchws,	{.id = 18} },
	{ "ws_20",		switchws,	{.id = 19} },
	{ "ws_21",		switchws,	{.id = 20} },
	{ "ws_22",		switchws,	{.id = 21} },
	{ "ws_next",		cyclews,	{.id = SWM_ARG_ID_CYCLEWS_UP} },
	{ "ws_prev",		cyclews,	{.id = SWM_ARG_ID_CYCLEWS_DOWN} },
	{ "ws_next_all",	cyclews,	{.id = SWM_ARG_ID_CYCLEWS_UP_ALL} },
	{ "ws_prev_all",	cyclews,	{.id = SWM_ARG_ID_CYCLEWS_DOWN_ALL} },
	{ "ws_prior",		priorws,	{0} },
	{ "screen_next",	cyclescr,	{.id = SWM_ARG_ID_CYCLESC_UP} },
	{ "screen_prev",	cyclescr,	{.id = SWM_ARG_ID_CYCLESC_DOWN} },
	{ "mvws_1",		send_to_ws,	{.id = 0} },
	{ "mvws_2",		send_to_ws,	{.id = 1} },
	{ "mvws_3",		send_to_ws,	{.id = 2} },
	{ "mvws_4",		send_to_ws,	{.id = 3} },
	{ "mvws_5",		send_to_ws,	{.id = 4} },
	{ "mvws_6",		send_to_ws,	{.id = 5} },
	{ "mvws_7",		send_to_ws,	{.id = 6} },
	{ "mvws_8",		send_to_ws,	{.id = 7} },
	{ "mvws_9",		send_to_ws,	{.id = 8} },
	{ "mvws_10",		send_to_ws,	{.id = 9} },
	{ "mvws_11",		send_to_ws,	{.id = 10} },
	{ "mvws_12",		send_to_ws,	{.id = 11} },
	{ "mvws_13",		send_to_ws,	{.id = 12} },
	{ "mvws_14",		send_to_ws,	{.id = 13} },
	{ "mvws_15",		send_to_ws,	{.id = 14} },
	{ "mvws_16",		send_to_ws,	{.id = 15} },
	{ "mvws_17",		send_to_ws,	{.id = 16} },
	{ "mvws_18",		send_to_ws,	{.id = 17} },
	{ "mvws_19",		send_to_ws,	{.id = 18} },
	{ "mvws_20",		send_to_ws,	{.id = 19} },
	{ "mvws_21",		send_to_ws,	{.id = 20} },
	{ "mvws_22",		send_to_ws,	{.id = 21} },
	{ "bar_toggle",		bar_toggle,	{0} },
	{ "wind_kill",		wkill,		{.id = SWM_ARG_ID_KILLWINDOW} },
	{ "wind_del",		wkill,		{.id = SWM_ARG_ID_DELETEWINDOW} },
	{ "float_toggle",	floating_toggle,{0} },
	{ "version",		version,	{0} },
	{ "spawn_custom",	dummykeyfunc,	{0} },
	{ "iconify",		iconify,	{0} },
	{ "uniconify",		uniconify,	{0} },
	{ "raise_toggle",	raise_toggle,	{0} },
	{ "button2",		pressbutton,	{2} },
	{ "width_shrink",	resize_step,	{.id = SWM_ARG_ID_WIDTHSHRINK} },
	{ "width_grow",		resize_step,	{.id = SWM_ARG_ID_WIDTHGROW} },
	{ "height_shrink",	resize_step,	{.id = SWM_ARG_ID_HEIGHTSHRINK} },
	{ "height_grow",	resize_step,	{.id = SWM_ARG_ID_HEIGHTGROW} },
	{ "move_left",		move_step,	{.id = SWM_ARG_ID_MOVELEFT} },
	{ "move_right",		move_step,	{.id = SWM_ARG_ID_MOVERIGHT} },
	{ "move_up",		move_step,	{.id = SWM_ARG_ID_MOVEUP} },
	{ "move_down",		move_step,	{.id = SWM_ARG_ID_MOVEDOWN} },
	{ "name_workspace",	name_workspace,	{0} },
	{ "search_workspace",	search_workspace,	{0} },
	{ "search_win",		search_win,	{0} },
	{ "dumpwins",		dumpwins,	{0} }, /* MUST BE LAST */
	{ "invalid key func",	NULL,		{0} },
};
struct key {
	RB_ENTRY(key)		entry;
	unsigned int		mod;
	KeySym			keysym;
	enum keyfuncid		funcid;
	char			*spawn_name;
};
RB_HEAD(key_tree, key);

int
key_cmp(struct key *kp1, struct key *kp2)
{
	if (kp1->keysym < kp2->keysym)
		return (-1);
	if (kp1->keysym > kp2->keysym)
		return (1);

	if (kp1->mod < kp2->mod)
		return (-1);
	if (kp1->mod > kp2->mod)
		return (1);

	return (0);
}

RB_GENERATE(key_tree, key, entry, key_cmp);
struct key_tree			keys;

/* mouse */
enum { client_click, root_click };
struct button {
	unsigned int		action;
	unsigned int		mask;
	unsigned int		button;
	void			(*func)(struct ws_win *, union arg *);
	union arg		args;
} buttons[] = {
	  /* action	key		mouse button	func	args */
	{ client_click,	MODKEY,		Button3,	resize,	{.id = SWM_ARG_ID_DONTCENTER} },
	{ client_click,	MODKEY | ShiftMask, Button3,	resize,	{.id = SWM_ARG_ID_CENTER} },
	{ client_click,	MODKEY,		Button1,	move,	{0} },
};

void
update_modkey(unsigned int mod)
{
	int			i;
	struct key		*kp;

	mod_key = mod;
	RB_FOREACH(kp, key_tree, &keys)
		if (kp->mod & ShiftMask)
			kp->mod = mod | ShiftMask;
		else
			kp->mod = mod;

	for (i = 0; i < LENGTH(buttons); i++)
		if (buttons[i].mask & ShiftMask)
			buttons[i].mask = mod | ShiftMask;
		else
			buttons[i].mask = mod;
}

/* spawn */
struct spawn_prog {
	TAILQ_ENTRY(spawn_prog)	entry;
	char			*name;
	int			argc;
	char			**argv;
};
TAILQ_HEAD(spawn_list, spawn_prog);
struct spawn_list		spawns = TAILQ_HEAD_INITIALIZER(spawns);

int
spawn_expand(struct swm_region *r, union arg *args, char *spawn_name,
    char ***ret_args)
{
	struct spawn_prog	*prog = NULL;
	int			i;
	char			*ap, **real_args;

	DNPRINTF(SWM_D_SPAWN, "spawn_expand: %s\n", spawn_name);

	/* find program */
	TAILQ_FOREACH(prog, &spawns, entry) {
		if (!strcasecmp(spawn_name, prog->name))
			break;
	}
	if (prog == NULL) {
		warnx("spawn_custom: program %s not found", spawn_name);
		return (-1);
	}

	/* make room for expanded args */
	if ((real_args = calloc(prog->argc + 1, sizeof(char *))) == NULL)
		err(1, "spawn_custom: calloc real_args");

	/* expand spawn_args into real_args */
	for (i = 0; i < prog->argc; i++) {
		ap = prog->argv[i];
		DNPRINTF(SWM_D_SPAWN, "spawn_custom: raw arg: %s\n", ap);
		if (!strcasecmp(ap, "$bar_border")) {
			if ((real_args[i] =
			    strdup(r->s->c[SWM_S_COLOR_BAR_BORDER].name))
			    == NULL)
				err(1,  "spawn_custom border color");
		} else if (!strcasecmp(ap, "$bar_color")) {
			if ((real_args[i] =
			    strdup(r->s->c[SWM_S_COLOR_BAR].name))
			    == NULL)
				err(1, "spawn_custom bar color");
		} else if (!strcasecmp(ap, "$bar_font")) {
			if ((real_args[i] = strdup(bar_fonts))
			    == NULL)
				err(1, "spawn_custom bar fonts");
		} else if (!strcasecmp(ap, "$bar_font_color")) {
			if ((real_args[i] =
			    strdup(r->s->c[SWM_S_COLOR_BAR_FONT].name))
			    == NULL)
				err(1, "spawn_custom color font");
		} else if (!strcasecmp(ap, "$color_focus")) {
			if ((real_args[i] =
			    strdup(r->s->c[SWM_S_COLOR_FOCUS].name))
			    == NULL)
				err(1, "spawn_custom color focus");
		} else if (!strcasecmp(ap, "$color_unfocus")) {
			if ((real_args[i] =
			    strdup(r->s->c[SWM_S_COLOR_UNFOCUS].name))
			    == NULL)
				err(1, "spawn_custom color unfocus");
		} else {
			/* no match --> copy as is */
			if ((real_args[i] = strdup(ap)) == NULL)
				err(1, "spawn_custom strdup(ap)");
		}
		DNPRINTF(SWM_D_SPAWN, "spawn_custom: cooked arg: %s\n",
		    real_args[i]);
	}

#ifdef SWM_DEBUG
	DNPRINTF(SWM_D_SPAWN, "spawn_custom: result: ");
	for (i = 0; i < prog->argc; i++)
		DNPRINTF(SWM_D_SPAWN, "\"%s\" ", real_args[i]);
	DNPRINTF(SWM_D_SPAWN, "\n");
#endif
	*ret_args = real_args;
	return (prog->argc);
}

void
spawn_custom(struct swm_region *r, union arg *args, char *spawn_name)
{
	union arg		a;
	char			**real_args;
	int			spawn_argc, i;

	if ((spawn_argc = spawn_expand(r, args, spawn_name, &real_args)) < 0)
		return;
	a.argv = real_args;
	if (fork() == 0)
		spawn(r->ws->idx, &a, 1);

	for (i = 0; i < spawn_argc; i++)
		free(real_args[i]);
	free(real_args);
}

void
spawn_select(struct swm_region *r, union arg *args, char *spawn_name, int *pid)
{
	union arg		a;
	char			**real_args;
	int			i, spawn_argc;

	if ((spawn_argc = spawn_expand(r, args, spawn_name, &real_args)) < 0)
		return;
	a.argv = real_args;

	if (pipe(select_list_pipe) == -1)
		err(1, "pipe error");
	if (pipe(select_resp_pipe) == -1)
		err(1, "pipe error");

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		err(1, "could not disable SIGPIPE");
	switch (*pid = fork()) {
	case -1:
		err(1, "cannot fork");
		break;
	case 0: /* child */
		if (dup2(select_list_pipe[0], 0) == -1)
			err(1, "dup2");
		if (dup2(select_resp_pipe[1], 1) == -1)
			err(1, "dup2");
		close(select_list_pipe[1]);
		close(select_resp_pipe[0]);
		spawn(r->ws->idx, &a, 0);
		break;
	default: /* parent */
		close(select_list_pipe[0]);
		close(select_resp_pipe[1]);
		break;
	}

	for (i = 0; i < spawn_argc; i++)
		free(real_args[i]);
	free(real_args);
}

void
spawn_insert(char *name, char *args)
{
	char			*arg, *cp, *ptr;
	struct spawn_prog	*sp;

	DNPRINTF(SWM_D_SPAWN, "spawn_insert: %s\n", name);

	if ((sp = calloc(1, sizeof *sp)) == NULL)
		err(1, "spawn_insert: malloc");
	if ((sp->name = strdup(name)) == NULL)
		err(1, "spawn_insert: strdup");

	/* convert the arguments to an argument list */
	if ((ptr = cp = strdup(args)) == NULL)
		err(1, "spawn_insert: strdup");
	while ((arg = strsep(&ptr, " \t")) != NULL) {
		/* empty field; skip it */
		if (*arg == '\0')
			continue;

		sp->argc++;
		if ((sp->argv = realloc(sp->argv, sp->argc *
		    sizeof *sp->argv)) == NULL)
			err(1, "spawn_insert: realloc");
		if ((sp->argv[sp->argc - 1] = strdup(arg)) == NULL)
			err(1, "spawn_insert: strdup");
	}
	free(cp);

	TAILQ_INSERT_TAIL(&spawns, sp, entry);
	DNPRINTF(SWM_D_SPAWN, "spawn_insert: leave\n");
}

void
spawn_remove(struct spawn_prog *sp)
{
	int			i;

	DNPRINTF(SWM_D_SPAWN, "spawn_remove: %s\n", sp->name);

	TAILQ_REMOVE(&spawns, sp, entry);
	for (i = 0; i < sp->argc; i++)
		free(sp->argv[i]);
	free(sp->argv);
	free(sp->name);
	free(sp);

	DNPRINTF(SWM_D_SPAWN, "spawn_remove: leave\n");
}

void
spawn_replace(struct spawn_prog *sp, char *name, char *args)
{
	DNPRINTF(SWM_D_SPAWN, "spawn_replace: %s [%s]\n", sp->name, name);

	spawn_remove(sp);
	spawn_insert(name, args);

	DNPRINTF(SWM_D_SPAWN, "spawn_replace: leave\n");
}

void
setspawn(char *name, char *args)
{
	struct spawn_prog	*sp;

	DNPRINTF(SWM_D_SPAWN, "setspawn: %s\n", name);

	if (name == NULL)
		return;

	TAILQ_FOREACH(sp, &spawns, entry) {
		if (!strcmp(sp->name, name)) {
			if (*args == '\0')
				spawn_remove(sp);
			else
				spawn_replace(sp, name, args);
			DNPRINTF(SWM_D_SPAWN, "setspawn: leave\n");
			return;
		}
	}
	if (*args == '\0') {
		warnx("error: setspawn: cannot find program: %s", name);
		return;
	}

	spawn_insert(name, args);
	DNPRINTF(SWM_D_SPAWN, "setspawn: leave\n");
}

int
setconfspawn(char *selector, char *value, int flags)
{
	DNPRINTF(SWM_D_SPAWN, "setconfspawn: [%s] [%s]\n", selector, value);

	setspawn(selector, value);

	DNPRINTF(SWM_D_SPAWN, "setconfspawn: done\n");
	return (0);
}

void
setup_spawn(void)
{
	setconfspawn("term",		"xterm",		0);
	setconfspawn("spawn_term",	"xterm",		0);
	setconfspawn("screenshot_all",	"screenshot.sh full",	0);
	setconfspawn("screenshot_wind",	"screenshot.sh window",	0);
	setconfspawn("lock",		"xlock",		0);
	setconfspawn("initscr",		"initscreen.sh",	0);
	setconfspawn("menu",		"dmenu_run"
					" -fn $bar_font"
					" -nb $bar_color"
					" -nf $bar_font_color"
					" -sb $bar_border"
					" -sf $bar_color",	0);
	setconfspawn("search",		"dmenu"
					" -i"
					" -fn $bar_font"
					" -nb $bar_color"
					" -nf $bar_font_color"
					" -sb $bar_border"
					" -sf $bar_color",	0);
	setconfspawn("name_workspace",	"dmenu"
					" -p Workspace"
					" -fn $bar_font"
					" -nb $bar_color"
					" -nf $bar_font_color"
					" -sb $bar_border"
					" -sf $bar_color",	0);
}

/* key bindings */
#define SWM_MODNAME_SIZE	32
#define	SWM_KEY_WS		"\n+ \t"
int
parsekeys(char *keystr, unsigned int currmod, unsigned int *mod, KeySym *ks)
{
	char			*cp, *name;
	KeySym			uks;
	DNPRINTF(SWM_D_KEY, "parsekeys: enter [%s]\n", keystr);
	if (mod == NULL || ks == NULL) {
		DNPRINTF(SWM_D_KEY, "parsekeys: no mod or key vars\n");
		return (1);
	}
	if (keystr == NULL || strlen(keystr) == 0) {
		DNPRINTF(SWM_D_KEY, "parsekeys: no keystr\n");
		return (1);
	}
	cp = keystr;
	*ks = NoSymbol;
	*mod = 0;
	while ((name = strsep(&cp, SWM_KEY_WS)) != NULL) {
		DNPRINTF(SWM_D_KEY, "parsekeys: key [%s]\n", name);
		if (cp)
			cp += (long)strspn(cp, SWM_KEY_WS);
		if (strncasecmp(name, "MOD", SWM_MODNAME_SIZE) == 0)
			*mod |= currmod;
		else if (!strncasecmp(name, "Mod1", SWM_MODNAME_SIZE))
			*mod |= Mod1Mask;
		else if (!strncasecmp(name, "Mod2", SWM_MODNAME_SIZE))
			*mod += Mod2Mask;
		else if (!strncmp(name, "Mod3", SWM_MODNAME_SIZE))
			*mod |= Mod3Mask;
		else if (!strncmp(name, "Mod4", SWM_MODNAME_SIZE))
			*mod |= Mod4Mask;
		else if (strncasecmp(name, "SHIFT", SWM_MODNAME_SIZE) == 0)
			*mod |= ShiftMask;
		else if (strncasecmp(name, "CONTROL", SWM_MODNAME_SIZE) == 0)
			*mod |= ControlMask;
		else {
			*ks = XStringToKeysym(name);
			XConvertCase(*ks, ks, &uks);
			if (ks == NoSymbol) {
				DNPRINTF(SWM_D_KEY,
				    "parsekeys: invalid key %s\n",
				    name);
				return (1);
			}
		}
	}
	DNPRINTF(SWM_D_KEY, "parsekeys: leave ok\n");
	return (0);
}

char *
strdupsafe(char *str)
{
	if (str == NULL)
		return (NULL);
	else
		return (strdup(str));
}

void
key_insert(unsigned int mod, KeySym ks, enum keyfuncid kfid, char *spawn_name)
{
	struct key		*kp;

	DNPRINTF(SWM_D_KEY, "key_insert: enter %s [%s]\n",
	    keyfuncs[kfid].name, spawn_name);

	if ((kp = malloc(sizeof *kp)) == NULL)
		err(1, "key_insert: malloc");

	kp->mod = mod;
	kp->keysym = ks;
	kp->funcid = kfid;
	kp->spawn_name = strdupsafe(spawn_name);
	RB_INSERT(key_tree, &keys, kp);

	DNPRINTF(SWM_D_KEY, "key_insert: leave\n");
}

struct key *
key_lookup(unsigned int mod, KeySym ks)
{
	struct key		kp;

	kp.keysym = ks;
	kp.mod = mod;

	return (RB_FIND(key_tree, &keys, &kp));
}

void
key_remove(struct key *kp)
{
	DNPRINTF(SWM_D_KEY, "key_remove: %s\n", keyfuncs[kp->funcid].name);

	RB_REMOVE(key_tree, &keys, kp);
	free(kp->spawn_name);
	free(kp);

	DNPRINTF(SWM_D_KEY, "key_remove: leave\n");
}

void
key_replace(struct key *kp, unsigned int mod, KeySym ks, enum keyfuncid kfid,
    char *spawn_name)
{
	DNPRINTF(SWM_D_KEY, "key_replace: %s [%s]\n", keyfuncs[kp->funcid].name,
	    spawn_name);

	key_remove(kp);
	key_insert(mod, ks, kfid, spawn_name);

	DNPRINTF(SWM_D_KEY, "key_replace: leave\n");
}

void
setkeybinding(unsigned int mod, KeySym ks, enum keyfuncid kfid,
    char *spawn_name)
{
	struct key		*kp;

	DNPRINTF(SWM_D_KEY, "setkeybinding: enter %s [%s]\n",
	    keyfuncs[kfid].name, spawn_name);

	if ((kp = key_lookup(mod, ks)) != NULL) {
		if (kfid == kf_invalid)
			key_remove(kp);
		else
			key_replace(kp, mod, ks, kfid, spawn_name);
		DNPRINTF(SWM_D_KEY, "setkeybinding: leave\n");
		return;
	}
	if (kfid == kf_invalid) {
		warnx("error: setkeybinding: cannot find mod/key combination");
		DNPRINTF(SWM_D_KEY, "setkeybinding: leave\n");
		return;
	}

	key_insert(mod, ks, kfid, spawn_name);
	DNPRINTF(SWM_D_KEY, "setkeybinding: leave\n");
}

int
setconfbinding(char *selector, char *value, int flags)
{
	enum keyfuncid		kfid;
	unsigned int		mod;
	KeySym			ks;
	struct spawn_prog	*sp;
	DNPRINTF(SWM_D_KEY, "setconfbinding: enter\n");
	if (selector == NULL) {
		DNPRINTF(SWM_D_KEY, "setconfbinding: unbind %s\n", value);
		if (parsekeys(value, mod_key, &mod, &ks) == 0) {
			kfid = kf_invalid;
			setkeybinding(mod, ks, kfid, NULL);
			return (0);
		} else
			return (1);
	}
	/* search by key function name */
	for (kfid = 0; kfid < kf_invalid; (kfid)++) {
		if (strncasecmp(selector, keyfuncs[kfid].name,
		    SWM_FUNCNAME_LEN) == 0) {
			DNPRINTF(SWM_D_KEY, "setconfbinding: %s: match\n",
			    selector);
			if (parsekeys(value, mod_key, &mod, &ks) == 0) {
				setkeybinding(mod, ks, kfid, NULL);
				return (0);
			} else
				return (1);
		}
	}
	/* search by custom spawn name */
	TAILQ_FOREACH(sp, &spawns, entry) {
		if (strcasecmp(selector, sp->name) == 0) {
			DNPRINTF(SWM_D_KEY, "setconfbinding: %s: match\n",
			    selector);
			if (parsekeys(value, mod_key, &mod, &ks) == 0) {
				setkeybinding(mod, ks, kf_spawn_custom,
				    sp->name);
				return (0);
			} else
				return (1);
		}
	}
	DNPRINTF(SWM_D_KEY, "setconfbinding: no match\n");
	return (1);
}

void
setup_keys(void)
{
	setkeybinding(MODKEY,		XK_space,	kf_cycle_layout,NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_backslash,	kf_flip_layout,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_space,	kf_stack_reset,	NULL);
	setkeybinding(MODKEY,		XK_h,		kf_master_shrink,NULL);
	setkeybinding(MODKEY,		XK_l,		kf_master_grow,	NULL);
	setkeybinding(MODKEY,		XK_comma,	kf_master_add,	NULL);
	setkeybinding(MODKEY,		XK_period,	kf_master_del,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_comma,	kf_stack_inc,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_period,	kf_stack_dec,	NULL);
	setkeybinding(MODKEY,		XK_Return,	kf_swap_main,	NULL);
	setkeybinding(MODKEY,		XK_j,		kf_focus_next,	NULL);
	setkeybinding(MODKEY,		XK_k,		kf_focus_prev,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_j,		kf_swap_next,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_k,		kf_swap_prev,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Return,	kf_spawn_custom,"term");
	setkeybinding(MODKEY,		XK_p,		kf_spawn_custom,"menu");
	setkeybinding(MODKEY|ShiftMask,	XK_q,		kf_quit,	NULL);
	setkeybinding(MODKEY,		XK_q,		kf_restart,	NULL);
	setkeybinding(MODKEY,		XK_m,		kf_focus_main,	NULL);
	setkeybinding(MODKEY,		XK_1,		kf_ws_1,	NULL);
	setkeybinding(MODKEY,		XK_2,		kf_ws_2,	NULL);
	setkeybinding(MODKEY,		XK_3,		kf_ws_3,	NULL);
	setkeybinding(MODKEY,		XK_4,		kf_ws_4,	NULL);
	setkeybinding(MODKEY,		XK_5,		kf_ws_5,	NULL);
	setkeybinding(MODKEY,		XK_6,		kf_ws_6,	NULL);
	setkeybinding(MODKEY,		XK_7,		kf_ws_7,	NULL);
	setkeybinding(MODKEY,		XK_8,		kf_ws_8,	NULL);
	setkeybinding(MODKEY,		XK_9,		kf_ws_9,	NULL);
	setkeybinding(MODKEY,		XK_0,		kf_ws_10,	NULL);
	setkeybinding(MODKEY,		XK_F1,		kf_ws_11,	NULL);
	setkeybinding(MODKEY,		XK_F2,		kf_ws_12,	NULL);
	setkeybinding(MODKEY,		XK_F3,		kf_ws_13,	NULL);
	setkeybinding(MODKEY,		XK_F4,		kf_ws_14,	NULL);
	setkeybinding(MODKEY,		XK_F5,		kf_ws_15,	NULL);
	setkeybinding(MODKEY,		XK_F6,		kf_ws_16,	NULL);
	setkeybinding(MODKEY,		XK_F7,		kf_ws_17,	NULL);
	setkeybinding(MODKEY,		XK_F8,		kf_ws_18,	NULL);
	setkeybinding(MODKEY,		XK_F9,		kf_ws_19,	NULL);
	setkeybinding(MODKEY,		XK_F10,		kf_ws_20,	NULL);
	setkeybinding(MODKEY,		XK_F11,		kf_ws_21,	NULL);
	setkeybinding(MODKEY,		XK_F12,		kf_ws_22,	NULL);
	setkeybinding(MODKEY,		XK_Right,	kf_ws_next,	NULL);
	setkeybinding(MODKEY,		XK_Left,	kf_ws_prev,	NULL);
	setkeybinding(MODKEY,		XK_Up,		kf_ws_next_all,	NULL);
	setkeybinding(MODKEY,		XK_Down,	kf_ws_prev_all,	NULL);
	setkeybinding(MODKEY,		XK_a,		kf_ws_prior,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Right,	kf_screen_next,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Left,	kf_screen_prev,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_1,		kf_mvws_1,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_2,		kf_mvws_2,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_3,		kf_mvws_3,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_4,		kf_mvws_4,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_5,		kf_mvws_5,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_6,		kf_mvws_6,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_7,		kf_mvws_7,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_8,		kf_mvws_8,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_9,		kf_mvws_9,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_0,		kf_mvws_10,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_F1,		kf_mvws_11,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_F2,		kf_mvws_12,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_F3,		kf_mvws_13,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_F4,		kf_mvws_14,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_F5,		kf_mvws_15,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_F6,		kf_mvws_16,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_F7,		kf_mvws_17,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_F8,		kf_mvws_18,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_F9,		kf_mvws_19,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_F10,		kf_mvws_20,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_F11,		kf_mvws_21,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_F12,		kf_mvws_22,	NULL);
	setkeybinding(MODKEY,		XK_b,		kf_bar_toggle,	NULL);
	setkeybinding(MODKEY,		XK_Tab,		kf_focus_next,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Tab,		kf_focus_prev,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_x,		kf_wind_kill,	NULL);
	setkeybinding(MODKEY,		XK_x,		kf_wind_del,	NULL);
	setkeybinding(MODKEY,		XK_s,		kf_spawn_custom,"screenshot_all");
	setkeybinding(MODKEY|ShiftMask,	XK_s,		kf_spawn_custom,"screenshot_wind");
	setkeybinding(MODKEY,		XK_t,		kf_float_toggle,NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_v,		kf_version,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Delete,	kf_spawn_custom,"lock");
	setkeybinding(MODKEY|ShiftMask,	XK_i,		kf_spawn_custom,"initscr");
	setkeybinding(MODKEY,		XK_w,		kf_iconify,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_w,		kf_uniconify,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_r,		kf_raise_toggle,NULL);
	setkeybinding(MODKEY,		XK_v,		kf_button2,	NULL);
	setkeybinding(MODKEY,		XK_equal,	kf_width_grow,	NULL);
	setkeybinding(MODKEY,		XK_minus,	kf_width_shrink,NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_equal,	kf_height_grow,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_minus,	kf_height_shrink,NULL);
	setkeybinding(MODKEY,		XK_bracketleft,	kf_move_left,	NULL);
	setkeybinding(MODKEY,		XK_bracketright,kf_move_right,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_bracketleft,	kf_move_up,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_bracketright,kf_move_down,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_slash,	kf_name_workspace,NULL);
	setkeybinding(MODKEY,		XK_slash,	kf_search_workspace,NULL);
	setkeybinding(MODKEY,		XK_f,		kf_search_win,	NULL);
#ifdef SWM_DEBUG
	setkeybinding(MODKEY|ShiftMask,	XK_d,		kf_dumpwins,	NULL);
#endif
}

void
clear_keys(void)
{
	struct key		*kp;

	while (RB_EMPTY(&keys) == 0) {
		kp = RB_ROOT(&keys);
		key_remove(kp);
	}
}

int
setkeymapping(char *selector, char *value, int flags)
{
	char			keymapping_file[PATH_MAX];
	DNPRINTF(SWM_D_KEY, "setkeymapping: enter\n");
	if (value[0] == '~')
		snprintf(keymapping_file, sizeof keymapping_file, "%s/%s",
		    pwd->pw_dir, &value[1]);
	else
		strlcpy(keymapping_file, value, sizeof keymapping_file);
	clear_keys();
	/* load new key bindings; if it fails, revert to default bindings */
	if (conf_load(keymapping_file, SWM_CONF_KEYMAPPING)) {
		clear_keys();
		setup_keys();
	}
	DNPRINTF(SWM_D_KEY, "setkeymapping: leave\n");
	return (0);
}

void
updatenumlockmask(void)
{
	unsigned int		i, j;
	XModifierKeymap		*modmap;

	DNPRINTF(SWM_D_MISC, "updatenumlockmask\n");
	numlockmask = 0;
	modmap = XGetModifierMapping(display);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
			    == XKeysymToKeycode(display, XK_Num_Lock))
				numlockmask = (1 << i);

	XFreeModifiermap(modmap);
}

void
grabkeys(void)
{
	int			num_screens;
	unsigned int		j, k;
	KeyCode			code;
	unsigned int		modifiers[] =
	    { 0, LockMask, numlockmask, numlockmask | LockMask };
	struct key		*kp;

	DNPRINTF(SWM_D_MISC, "grabkeys\n");
	updatenumlockmask();

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (k = 0; k < num_screens; k++) {
		if (TAILQ_EMPTY(&screens[k].rl))
			continue;
		xcb_ungrab_key(conn, XCB_GRAB_ANY, screens[k].root,
			XCB_MOD_MASK_ANY);
		RB_FOREACH(kp, key_tree, &keys) {
			if ((code = XKeysymToKeycode(display, kp->keysym)))
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabKey(display, code,
					    kp->mod | modifiers[j],
					    screens[k].root, True,
					    GrabModeAsync, GrabModeAsync);
		}
	}
}

void
grabbuttons(struct ws_win *win, int focused)
{
	unsigned int		i, j;
	unsigned int		modifiers[] =
	    { 0, LockMask, numlockmask, numlockmask|LockMask };

	updatenumlockmask();
	xcb_ungrab_button(conn, XCB_BUTTON_INDEX_ANY, win->id,
		XCB_BUTTON_MASK_ANY);
	if (focused) {
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].action == client_click)
				for (j = 0; j < LENGTH(modifiers); j++)
					xcb_grab_button(conn, False, win->id,
						BUTTONMASK,
						XCB_GRAB_MODE_ASYNC,
						XCB_GRAB_MODE_SYNC,
						XCB_WINDOW_NONE,
						XCB_CURSOR_NONE,
						buttons[i].button,
						buttons[i].mask);
	} else
		xcb_grab_button(conn, False, win->id, BUTTONMASK,
			XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC,
			XCB_WINDOW_NONE, XCB_CURSOR_NONE, XCB_BUTTON_INDEX_ANY,
			XCB_BUTTON_MASK_ANY);
}

const char *quirkname[] = {
	"NONE",		/* config string for "no value" */
	"FLOAT",
	"TRANSSZ",
	"ANYWHERE",
	"XTERM_FONTADJ",
	"FULLSCREEN",
	"FOCUSPREV",
};

/* SWM_Q_WS: retain '|' for back compat for now (2009-08-11) */
#define	SWM_Q_WS		"\n|+ \t"
int
parsequirks(char *qstr, unsigned long *quirk)
{
	char			*cp, *name;
	int			i;

	if (quirk == NULL)
		return (1);

	cp = qstr;
	*quirk = 0;
	while ((name = strsep(&cp, SWM_Q_WS)) != NULL) {
		if (cp)
			cp += (long)strspn(cp, SWM_Q_WS);
		for (i = 0; i < LENGTH(quirkname); i++) {
			if (!strncasecmp(name, quirkname[i], SWM_QUIRK_LEN)) {
				DNPRINTF(SWM_D_QUIRK,
				    "parsequirks: %s\n", name);
				if (i == 0) {
					*quirk = 0;
					return (0);
				}
				*quirk |= 1 << (i-1);
				break;
			}
		}
		if (i >= LENGTH(quirkname)) {
			DNPRINTF(SWM_D_QUIRK,
			    "parsequirks: invalid quirk [%s]\n", name);
			return (1);
		}
	}
	return (0);
}

void
quirk_insert(const char *class, const char *name, unsigned long quirk)
{
	struct quirk		*qp;

	DNPRINTF(SWM_D_QUIRK, "quirk_insert: %s:%s [%lu]\n", class, name,
	    quirk);

	if ((qp = malloc(sizeof *qp)) == NULL)
		err(1, "quirk_insert: malloc");
	if ((qp->class = strdup(class)) == NULL)
		err(1, "quirk_insert: strdup");
	if ((qp->name = strdup(name)) == NULL)
		err(1, "quirk_insert: strdup");

	qp->quirk = quirk;
	TAILQ_INSERT_TAIL(&quirks, qp, entry);

	DNPRINTF(SWM_D_QUIRK, "quirk_insert: leave\n");
}

void
quirk_remove(struct quirk *qp)
{
	DNPRINTF(SWM_D_QUIRK, "quirk_remove: %s:%s [%lu]\n", qp->class,
	    qp->name, qp->quirk);

	TAILQ_REMOVE(&quirks, qp, entry);
	free(qp->class);
	free(qp->name);
	free(qp);

	DNPRINTF(SWM_D_QUIRK, "quirk_remove: leave\n");
}

void
quirk_replace(struct quirk *qp, const char *class, const char *name,
    unsigned long quirk)
{
	DNPRINTF(SWM_D_QUIRK, "quirk_replace: %s:%s [%lu]\n", qp->class,
	    qp->name, qp->quirk);

	quirk_remove(qp);
	quirk_insert(class, name, quirk);

	DNPRINTF(SWM_D_QUIRK, "quirk_replace: leave\n");
}

void
setquirk(const char *class, const char *name, unsigned long quirk)
{
	struct quirk		*qp;

	DNPRINTF(SWM_D_QUIRK, "setquirk: enter %s:%s [%lu]\n", class, name,
	   quirk);

	TAILQ_FOREACH(qp, &quirks, entry) {
		if (!strcmp(qp->class, class) && !strcmp(qp->name, name)) {
			if (!quirk)
				quirk_remove(qp);
			else
				quirk_replace(qp, class, name, quirk);
			DNPRINTF(SWM_D_QUIRK, "setquirk: leave\n");
			return;
		}
	}
	if (!quirk) {
		warnx("error: setquirk: cannot find class/name combination");
		return;
	}

	quirk_insert(class, name, quirk);
	DNPRINTF(SWM_D_QUIRK, "setquirk: leave\n");
}

int
setconfquirk(char *selector, char *value, int flags)
{
	char			*cp, *class, *name;
	int			retval;
	unsigned long		quirks;
	if (selector == NULL)
		return (0);
	if ((cp = strchr(selector, ':')) == NULL)
		return (0);
	*cp = '\0';
	class = selector;
	name = cp + 1;
	if ((retval = parsequirks(value, &quirks)) == 0)
		setquirk(class, name, quirks);
	return (retval);
}

void
setup_quirks(void)
{
	setquirk("MPlayer",		"xv",		SWM_Q_FLOAT | SWM_Q_FULLSCREEN | SWM_Q_FOCUSPREV);
	setquirk("OpenOffice.org 3.2",	"VCLSalFrame",	SWM_Q_FLOAT);
	setquirk("Firefox-bin",		"firefox-bin",	SWM_Q_TRANSSZ);
	setquirk("Firefox",		"Dialog",	SWM_Q_FLOAT);
	setquirk("Gimp",		"gimp",		SWM_Q_FLOAT | SWM_Q_ANYWHERE);
	setquirk("XTerm",		"xterm",	SWM_Q_XTERM_FONTADJ);
	setquirk("xine",		"Xine Window",	SWM_Q_FLOAT | SWM_Q_ANYWHERE);
	setquirk("Xitk",		"Xitk Combo",	SWM_Q_FLOAT | SWM_Q_ANYWHERE);
	setquirk("xine",		"xine Panel",	SWM_Q_FLOAT | SWM_Q_ANYWHERE);
	setquirk("Xitk",		"Xine Window",	SWM_Q_FLOAT | SWM_Q_ANYWHERE);
	setquirk("xine",		"xine Video Fullscreen Window",	SWM_Q_FULLSCREEN | SWM_Q_FLOAT);
	setquirk("pcb",			"pcb",		SWM_Q_FLOAT);
	setquirk("SDL_App",		"SDL_App",	SWM_Q_FLOAT | SWM_Q_FULLSCREEN);
}

/* conf file stuff */
#define SWM_CONF_FILE		"spectrwm.conf"
#define SWM_CONF_FILE_OLD	"scrotwm.conf"

enum {
	SWM_S_BAR_ACTION,
	SWM_S_BAR_AT_BOTTOM,
	SWM_S_BAR_BORDER_WIDTH,
	SWM_S_BAR_DELAY,
	SWM_S_BAR_ENABLED,
	SWM_S_BAR_FONT,
	SWM_S_BAR_FORMAT,
	SWM_S_BAR_JUSTIFY,
	SWM_S_BORDER_WIDTH,
	SWM_S_CLOCK_ENABLED,
	SWM_S_CLOCK_FORMAT,
	SWM_S_CYCLE_EMPTY,
	SWM_S_CYCLE_VISIBLE,
	SWM_S_DIALOG_RATIO,
	SWM_S_DISABLE_BORDER,
	SWM_S_FOCUS_CLOSE,
	SWM_S_FOCUS_CLOSE_WRAP,
	SWM_S_FOCUS_DEFAULT,
	SWM_S_FOCUS_MODE,
	SWM_S_SPAWN_ORDER,
	SWM_S_SPAWN_TERM,
	SWM_S_SS_APP,
	SWM_S_SS_ENABLED,
	SWM_S_STACK_ENABLED,
	SWM_S_TERM_WIDTH,
	SWM_S_TITLE_CLASS_ENABLED,
	SWM_S_TITLE_NAME_ENABLED,
	SWM_S_URGENT_ENABLED,
	SWM_S_VERBOSE_LAYOUT,
	SWM_S_WINDOW_NAME_ENABLED,
	SWM_S_WORKSPACE_LIMIT
};

int
setconfvalue(char *selector, char *value, int flags)
{
	int	i;
	char	*b;

	switch (flags) {
	case SWM_S_BAR_ACTION:
		free(bar_argv[0]);
		if ((bar_argv[0] = strdup(value)) == NULL)
			err(1, "setconfvalue: bar_action");
		break;
	case SWM_S_BAR_AT_BOTTOM:
		bar_at_bottom = atoi(value);
		break;
	case SWM_S_BAR_BORDER_WIDTH:
		bar_border_width = atoi(value);
		if (bar_border_width < 0)
			bar_border_width = 0;
		break;
	case SWM_S_BAR_DELAY:
		bar_delay = atoi(value);
		break;
	case SWM_S_BAR_ENABLED:
		bar_enabled = atoi(value);
		break;
	case SWM_S_BAR_FONT:
		b = bar_fonts;
		if (asprintf(&bar_fonts, "%s,%s", value, bar_fonts) == -1)
			err(1, "setconfvalue: asprintf: failed to allocate "
				"memory for bar_fonts.");

		free(b);
		break;
	case SWM_S_BAR_FORMAT:
		free(bar_format);
		if ((bar_format = strdup(value)) == NULL)
			err(1, "setconfvalue: bar_format");
		break;
	case SWM_S_BAR_JUSTIFY:
		if (!strcmp(value, "left"))
			bar_justify = SWM_BAR_JUSTIFY_LEFT;
		else if (!strcmp(value, "center"))
			bar_justify = SWM_BAR_JUSTIFY_CENTER;
		else if (!strcmp(value, "right"))
			bar_justify = SWM_BAR_JUSTIFY_RIGHT;
		else
			errx(1, "invalid bar_justify");
		break;
	case SWM_S_BORDER_WIDTH:
		border_width = atoi(value);
		if (border_width < 0)
			border_width = 0;
		break;
	case SWM_S_CLOCK_ENABLED:
		clock_enabled = atoi(value);
		break;
	case SWM_S_CLOCK_FORMAT:
#ifndef SWM_DENY_CLOCK_FORMAT
		free(clock_format);
		if ((clock_format = strdup(value)) == NULL)
			err(1, "setconfvalue: clock_format");
#endif
		break;
	case SWM_S_CYCLE_EMPTY:
		cycle_empty = atoi(value);
		break;
	case SWM_S_CYCLE_VISIBLE:
		cycle_visible = atoi(value);
		break;
	case SWM_S_DIALOG_RATIO:
		dialog_ratio = atof(value);
		if (dialog_ratio > 1.0 || dialog_ratio <= .3)
			dialog_ratio = .6;
		break;
	case SWM_S_DISABLE_BORDER:
		disable_border = atoi(value);
		break;
	case SWM_S_FOCUS_CLOSE:
		if (!strcmp(value, "first"))
			focus_close = SWM_STACK_BOTTOM;
		else if (!strcmp(value, "last"))
			focus_close = SWM_STACK_TOP;
		else if (!strcmp(value, "next"))
			focus_close = SWM_STACK_ABOVE;
		else if (!strcmp(value, "previous"))
			focus_close = SWM_STACK_BELOW;
		else
			errx(1, "focus_close");
		break;
	case SWM_S_FOCUS_CLOSE_WRAP:
		focus_close_wrap = atoi(value);
		break;
	case SWM_S_FOCUS_DEFAULT:
		if (!strcmp(value, "last"))
			focus_default = SWM_STACK_TOP;
		else if (!strcmp(value, "first"))
			focus_default = SWM_STACK_BOTTOM;
		else
			errx(1, "focus_default");
		break;
	case SWM_S_FOCUS_MODE:
		if (!strcmp(value, "default"))
			focus_mode = SWM_FOCUS_DEFAULT;
		else if (!strcmp(value, "follow_cursor"))
			focus_mode = SWM_FOCUS_FOLLOW;
		else if (!strcmp(value, "synergy"))
			focus_mode = SWM_FOCUS_SYNERGY;
		else
			errx(1, "focus_mode");
		break;
	case SWM_S_SPAWN_ORDER:
		if (!strcmp(value, "first"))
			spawn_position = SWM_STACK_BOTTOM;
		else if (!strcmp(value, "last"))
			spawn_position = SWM_STACK_TOP;
		else if (!strcmp(value, "next"))
			spawn_position = SWM_STACK_ABOVE;
		else if (!strcmp(value, "previous"))
			spawn_position = SWM_STACK_BELOW;
		else
			errx(1, "spawn_position");
		break;
	case SWM_S_SPAWN_TERM:
		setconfspawn("term", value, 0);
		setconfspawn("spawn_term", value, 0);
		break;
	case SWM_S_SS_APP:
		break;
	case SWM_S_SS_ENABLED:
		ss_enabled = atoi(value);
		break;
	case SWM_S_STACK_ENABLED:
		stack_enabled = atoi(value);
		break;
	case SWM_S_TERM_WIDTH:
		term_width = atoi(value);
		if (term_width < 0)
			term_width = 0;
		break;
	case SWM_S_TITLE_CLASS_ENABLED:
		title_class_enabled = atoi(value);
		break;
	case SWM_S_TITLE_NAME_ENABLED:
		title_name_enabled = atoi(value);
		break;
	case SWM_S_URGENT_ENABLED:
		urgent_enabled = atoi(value);
		break;
	case SWM_S_VERBOSE_LAYOUT:
		verbose_layout = atoi(value);
		for (i = 0; layouts[i].l_stack != NULL; i++) {
			if (verbose_layout)
				layouts[i].l_string = fancy_stacker;
			else
				layouts[i].l_string = plain_stacker;
		}
		break;
	case SWM_S_WINDOW_NAME_ENABLED:
		window_name_enabled = atoi(value);
		break;
	case SWM_S_WORKSPACE_LIMIT:
		workspace_limit = atoi(value);
		if (workspace_limit > SWM_WS_MAX)
			workspace_limit = SWM_WS_MAX;
		else if (workspace_limit < 1)
			workspace_limit = 1;
		break;
	default:
		return (1);
	}
	return (0);
}

int
setconfmodkey(char *selector, char *value, int flags)
{
	if (!strncasecmp(value, "Mod1", strlen("Mod1")))
		update_modkey(Mod1Mask);
	else if (!strncasecmp(value, "Mod2", strlen("Mod2")))
		update_modkey(Mod2Mask);
	else if (!strncasecmp(value, "Mod3", strlen("Mod3")))
		update_modkey(Mod3Mask);
	else if (!strncasecmp(value, "Mod4", strlen("Mod4")))
		update_modkey(Mod4Mask);
	else
		return (1);
	return (0);
}

int
setconfcolor(char *selector, char *value, int flags)
{
	setscreencolor(value, ((selector == NULL)?-1:atoi(selector)), flags);
	return (0);
}

int
setconfregion(char *selector, char *value, int flags)
{
	custom_region(value);
	return (0);
}

int
setautorun(char *selector, char *value, int flags)
{
	int			ws_id;
	char			s[1024];
	char			*ap, *sp = s;
	union arg		a;
	int			argc = 0;
	long			pid;
	struct pid_e		*p;

	if (getenv("SWM_STARTED"))
		return (0);

	bzero(s, sizeof s);
	if (sscanf(value, "ws[%d]:%1023c", &ws_id, s) != 2)
		errx(1, "invalid autorun entry, should be 'ws[<idx>]:command'");
	ws_id--;
	if (ws_id < 0 || ws_id >= workspace_limit)
		errx(1, "autorun: invalid workspace %d", ws_id + 1);

	/*
	 * This is a little intricate
	 *
	 * If the pid already exists we simply reuse it because it means it was
	 * used before AND not claimed by manage_window.  We get away with
	 * altering it in the parent after INSERT because this can not be a race
	 */
	a.argv = NULL;
	while ((ap = strsep(&sp, " \t")) != NULL) {
		if (*ap == '\0')
			continue;
		DNPRINTF(SWM_D_SPAWN, "setautorun: arg [%s]\n", ap);
		argc++;
		if ((a.argv = realloc(a.argv, argc * sizeof(char *))) == NULL)
			err(1, "setautorun: realloc");
		a.argv[argc - 1] = ap;
	}

	if ((a.argv = realloc(a.argv, (argc + 1) * sizeof(char *))) == NULL)
		err(1, "setautorun: realloc");
	a.argv[argc] = NULL;

	if ((pid = fork()) == 0) {
		spawn(ws_id, &a, 1);
		/* NOTREACHED */
		_exit(1);
	}
	free(a.argv);

	/* parent */
	p = find_pid(pid);
	if (p == NULL) {
		p = calloc(1, sizeof *p);
		if (p == NULL)
			return (1);
		TAILQ_INSERT_TAIL(&pidlist, p, entry);
	}

	p->pid = pid;
	p->ws = ws_id;

	return (0);
}

int
setlayout(char *selector, char *value, int flags)
{
	int			ws_id, i, x, mg, ma, si, raise, f = 0;
	int			st = SWM_V_STACK, num_screens;
	char			s[1024];
	struct workspace	*ws;

	if (getenv("SWM_STARTED"))
		return (0);

	bzero(s, sizeof s);
	if (sscanf(value, "ws[%d]:%d:%d:%d:%d:%1023c",
	    &ws_id, &mg, &ma, &si, &raise, s) != 6)
		errx(1, "invalid layout entry, should be 'ws[<idx>]:"
		    "<master_grow>:<master_add>:<stack_inc>:<always_raise>:"
		    "<type>'");
	ws_id--;
	if (ws_id < 0 || ws_id >= workspace_limit)
		errx(1, "layout: invalid workspace %d", ws_id + 1);

	if (!strcasecmp(s, "vertical"))
		st = SWM_V_STACK;
	else if (!strcasecmp(s, "vertical_flip")) {
		st = SWM_V_STACK;
		f = 1;
	} else if (!strcasecmp(s, "horizontal"))
		st = SWM_H_STACK;
	else if (!strcasecmp(s, "horizontal_flip")) {
		st = SWM_H_STACK;
		f = 1;
	} else if (!strcasecmp(s, "fullscreen"))
		st = SWM_MAX_STACK;
	else
		errx(1, "invalid layout entry, should be 'ws[<idx>]:"
		    "<master_grow>:<master_add>:<stack_inc>:<always_raise>:"
		    "<type>'");

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++) {
		ws = (struct workspace *)&screens[i].ws;
		ws[ws_id].cur_layout = &layouts[st];

		ws[ws_id].always_raise = raise;
		if (st == SWM_MAX_STACK)
			continue;

		/* master grow */
		for (x = 0; x < abs(mg); x++) {
			ws[ws_id].cur_layout->l_config(&ws[ws_id],
			    mg >= 0 ?  SWM_ARG_ID_MASTERGROW :
			    SWM_ARG_ID_MASTERSHRINK);
			stack();
		}
		/* master add */
		for (x = 0; x < abs(ma); x++) {
			ws[ws_id].cur_layout->l_config(&ws[ws_id],
			    ma >= 0 ?  SWM_ARG_ID_MASTERADD :
			    SWM_ARG_ID_MASTERDEL);
			stack();
		}
		/* stack inc */
		for (x = 0; x < abs(si); x++) {
			ws[ws_id].cur_layout->l_config(&ws[ws_id],
			    si >= 0 ?  SWM_ARG_ID_STACKINC :
			    SWM_ARG_ID_STACKDEC);
			stack();
		}
		/* Apply flip */
		if (f) {
			ws[ws_id].cur_layout->l_config(&ws[ws_id],
			    SWM_ARG_ID_FLIPLAYOUT);
			stack();
		}
	}

	return (0);
}

/* config options */
struct config_option {
	char			*optname;
	int			(*func)(char*, char*, int);
	int			funcflags;
};
struct config_option configopt[] = {
	{ "bar_enabled",		setconfvalue,	SWM_S_BAR_ENABLED },
	{ "bar_at_bottom",		setconfvalue,	SWM_S_BAR_AT_BOTTOM },
	{ "bar_border",			setconfcolor,	SWM_S_COLOR_BAR_BORDER },
	{ "bar_border_width",		setconfvalue,	SWM_S_BAR_BORDER_WIDTH },
	{ "bar_color",			setconfcolor,	SWM_S_COLOR_BAR },
	{ "bar_font_color",		setconfcolor,	SWM_S_COLOR_BAR_FONT },
	{ "bar_font",			setconfvalue,	SWM_S_BAR_FONT },
	{ "bar_action",			setconfvalue,	SWM_S_BAR_ACTION },
	{ "bar_delay",			setconfvalue,	SWM_S_BAR_DELAY },
	{ "bar_justify",		setconfvalue,	SWM_S_BAR_JUSTIFY },
	{ "bar_format",			setconfvalue,	SWM_S_BAR_FORMAT },
	{ "keyboard_mapping",		setkeymapping,	0 },
	{ "bind",			setconfbinding,	0 },
	{ "stack_enabled",		setconfvalue,	SWM_S_STACK_ENABLED },
	{ "clock_enabled",		setconfvalue,	SWM_S_CLOCK_ENABLED },
	{ "clock_format",		setconfvalue,	SWM_S_CLOCK_FORMAT },
	{ "color_focus",		setconfcolor,	SWM_S_COLOR_FOCUS },
	{ "color_unfocus",		setconfcolor,	SWM_S_COLOR_UNFOCUS },
	{ "cycle_empty",		setconfvalue,	SWM_S_CYCLE_EMPTY },
	{ "cycle_visible",		setconfvalue,	SWM_S_CYCLE_VISIBLE },
	{ "workspace_limit",		setconfvalue,	SWM_S_WORKSPACE_LIMIT },
	{ "dialog_ratio",		setconfvalue,	SWM_S_DIALOG_RATIO },
	{ "verbose_layout",		setconfvalue,	SWM_S_VERBOSE_LAYOUT },
	{ "modkey",			setconfmodkey,	0 },
	{ "program",			setconfspawn,	0 },
	{ "quirk",			setconfquirk,	0 },
	{ "region",			setconfregion,	0 },
	{ "spawn_term",			setconfvalue,	SWM_S_SPAWN_TERM },
	{ "screenshot_enabled",		setconfvalue,	SWM_S_SS_ENABLED },
	{ "screenshot_app",		setconfvalue,	SWM_S_SS_APP },
	{ "window_name_enabled",	setconfvalue,	SWM_S_WINDOW_NAME_ENABLED },
	{ "urgent_enabled",		setconfvalue,	SWM_S_URGENT_ENABLED },
	{ "term_width",			setconfvalue,	SWM_S_TERM_WIDTH },
	{ "title_class_enabled",	setconfvalue,	SWM_S_TITLE_CLASS_ENABLED },
	{ "title_name_enabled",		setconfvalue,	SWM_S_TITLE_NAME_ENABLED },
	{ "focus_mode",			setconfvalue,	SWM_S_FOCUS_MODE },
	{ "focus_close",		setconfvalue,	SWM_S_FOCUS_CLOSE },
	{ "focus_close_wrap",		setconfvalue,	SWM_S_FOCUS_CLOSE_WRAP },
	{ "focus_default",		setconfvalue,	SWM_S_FOCUS_DEFAULT },
	{ "spawn_position",		setconfvalue,	SWM_S_SPAWN_ORDER },
	{ "disable_border",		setconfvalue,	SWM_S_DISABLE_BORDER },
	{ "border_width",		setconfvalue,	SWM_S_BORDER_WIDTH },
	{ "autorun",			setautorun,	0 },
	{ "layout",			setlayout,	0 },
};


int
conf_load(char *filename, int keymapping)
{
	FILE			*config;
	char			*line, *cp, *optsub, *optval;
	size_t			linelen, lineno = 0;
	int			wordlen, i, optind;
	struct config_option	*opt;

	DNPRINTF(SWM_D_CONF, "conf_load: begin\n");

	if (filename == NULL) {
		warnx("conf_load: no filename");
		return (1);
	}
	if ((config = fopen(filename, "r")) == NULL) {
		warn("conf_load: fopen: %s", filename);
		return (1);
	}

	while (!feof(config)) {
		if ((line = fparseln(config, &linelen, &lineno, NULL, 0))
		    == NULL) {
			if (ferror(config))
				err(1, "%s", filename);
			else
				continue;
		}
		cp = line;
		cp += strspn(cp, " \t\n"); /* eat whitespace */
		if (cp[0] == '\0') {
			/* empty line */
			free(line);
			continue;
		}
		/* get config option */
		wordlen = strcspn(cp, "=[ \t\n");
		if (wordlen == 0) {
			warnx("%s: line %zd: no option found",
			    filename, lineno);
			goto out;
		}
		optind = -1;
		for (i = 0; i < LENGTH(configopt); i++) {
			opt = &configopt[i];
			if (!strncasecmp(cp, opt->optname, wordlen) &&
			    strlen(opt->optname) == wordlen) {
				optind = i;
				break;
			}
		}
		if (optind == -1) {
			warnx("%s: line %zd: unknown option %.*s",
			    filename, lineno, wordlen, cp);
			goto out;
		}
		if (keymapping && strcmp(opt->optname, "bind")) {
			warnx("%s: line %zd: invalid option %.*s",
			    filename, lineno, wordlen, cp);
			goto out;
		}
		cp += wordlen;
		cp += strspn(cp, " \t\n"); /* eat whitespace */
		/* get [selector] if any */
		optsub = NULL;
		if (*cp == '[') {
			cp++;
			wordlen = strcspn(cp, "]");
			if (*cp != ']') {
				if (wordlen == 0) {
					warnx("%s: line %zd: syntax error",
					    filename, lineno);
					goto out;
				}

				if (asprintf(&optsub, "%.*s", wordlen, cp) ==
				    -1) {
					warnx("%s: line %zd: unable to allocate"
					    "memory for selector", filename,
					    lineno);
					goto out;
				}
			}
			cp += wordlen;
			cp += strspn(cp, "] \t\n"); /* eat trailing */
		}
		cp += strspn(cp, "= \t\n"); /* eat trailing */
		/* get RHS value */
		optval = strdup(cp);
		/* call function to deal with it all */
		if (configopt[optind].func(optsub, optval,
		    configopt[optind].funcflags) != 0)
			errx(1, "%s: line %zd: invalid data for %s",
			    filename, lineno, configopt[optind].optname);
		free(optval);
		free(optsub);
		free(line);
	}

	fclose(config);
	DNPRINTF(SWM_D_CONF, "conf_load: end\n");

	return (0);

out:
	free(line);
	fclose(config);
	DNPRINTF(SWM_D_CONF, "conf_load: end with error.\n");

	return (1);
}

void
set_child_transient(struct ws_win *win, Window *trans)
{
	struct ws_win		*parent, *w;
	XWMHints		*wmh = NULL;
	struct swm_region	*r;
	struct workspace	*ws;

	parent = find_window(win->transient);
	if (parent)
		parent->child_trans = win;
	else {
		DNPRINTF(SWM_D_MISC, "set_child_transient: parent doesn't exist"
		    " for 0x%x trans 0x%x\n", win->id, win->transient);

		if (win->hints == NULL) {
			warnx("no hints for 0x%x", win->id);
			return;
		}

		r = root_to_region(win->wa.root);
		ws = r->ws;
		/* parent doen't exist in our window list */
		TAILQ_FOREACH(w, &ws->winlist, entry) {
			if (wmh)
				XFree(wmh);

			if ((wmh = XGetWMHints(display, w->id)) == NULL) {
				warnx("can't get hints for 0x%x", w->id);
				continue;
			}

			if (win->hints->window_group != wmh->window_group)
				continue;

			w->child_trans = win;
			win->transient = w->id;
			*trans = w->id;
			DNPRINTF(SWM_D_MISC, "set_child_transient: asjusting "
			    "transient to 0x%x\n", win->transient);
			break;
		}
	}

	if (wmh)
		XFree(wmh);
}

long
window_get_pid(xcb_window_t win)
{
	long				ret = 0;
	const char			*errstr;
	xcb_atom_t			apid;
	xcb_get_property_cookie_t	pc;
	xcb_get_property_reply_t	*pr;

	apid = get_atom_from_string("_NET_WM_PID");
	if (apid == XCB_ATOM_NONE)
		goto tryharder;

	pc = xcb_get_property(conn, False, win, apid, XCB_ATOM_CARDINAL, 0, 1);
	pr = xcb_get_property_reply(conn, pc, NULL);
	if (!pr)
		goto tryharder;
	if (pr->type != XCB_ATOM_CARDINAL)
		goto tryharder;

	ret = *(long *)xcb_get_property_value(pr);
	free(pr);

	return (ret);

tryharder:
	apid = get_atom_from_string("_SWM_PID");
	pc = xcb_get_property(conn, False, win, apid, XCB_ATOM_STRING,
		0, SWM_PROPLEN);
	pr = xcb_get_property_reply(conn, pc, NULL);
	if (!pr)
		return (0);
	if (pr->type != XCB_ATOM_STRING)
		free(pr);
		return (0);
	ret = strtonum(xcb_get_property_value(pr), 0, UINT_MAX, &errstr);
	free(pr);

	return (ret);
}

struct ws_win *
manage_window(xcb_window_t id)
{
	Window			trans = 0;
	struct workspace	*ws;
	struct ws_win		*win, *ww;
	int			format, i, ws_idx, n, border_me = 0;
	unsigned long		nitems, bytes;
	Atom			ws_idx_atom = 0, type;
	Atom			*prot = NULL, *pp;
	unsigned char		ws_idx_str[SWM_PROPLEN], *prop = NULL;
	struct swm_region	*r;
	const char		*errstr;
	struct pid_e		*p;
	struct quirk		*qp;
	uint32_t		event_mask;

	if ((win = find_window(id)) != NULL)
		return (win);	/* already being managed */

	/* see if we are on the unmanaged list */
	if ((win = find_unmanaged_window(id)) != NULL) {
		DNPRINTF(SWM_D_MISC, "manage_window: previously unmanaged "
		    "window: 0x%x\n", win->id);
		TAILQ_REMOVE(&win->ws->unmanagedlist, win, entry);
		if (win->transient)
			set_child_transient(win, &trans);

		if (trans && (ww = find_window(trans)))
			TAILQ_INSERT_AFTER(&win->ws->winlist, ww, win, entry);
		else if ((ww = win->ws->focus) &&
		    spawn_position == SWM_STACK_ABOVE)
			TAILQ_INSERT_AFTER(&win->ws->winlist, win->ws->focus,
			    win, entry);
		else if (ww && spawn_position == SWM_STACK_BELOW)
			TAILQ_INSERT_BEFORE(win->ws->focus, win, entry);
		else switch (spawn_position) {
		default:
		case SWM_STACK_TOP:
		case SWM_STACK_ABOVE:
			TAILQ_INSERT_TAIL(&win->ws->winlist, win, entry);
			break;
		case SWM_STACK_BOTTOM:
		case SWM_STACK_BELOW:
			TAILQ_INSERT_HEAD(&win->ws->winlist, win, entry);
		}

		ewmh_update_actions(win);
		return (win);
	}

	if ((win = calloc(1, sizeof(struct ws_win))) == NULL)
		err(1, "manage_window: calloc: failed to allocate memory for "
		    "new window");

	win->id = id;
	win->bordered = 0;

	/* see if we need to override the workspace */
	p = find_pid(window_get_pid(id));

	/* Get all the window data in one shot */
	ws_idx_atom = XInternAtom(display, "_SWM_WS", False);
	if (ws_idx_atom) {
		XGetWindowProperty(display, id, ws_idx_atom, 0, SWM_PROPLEN,
		    False, XA_STRING, &type, &format, &nitems, &bytes, &prop);
	}
	XGetWindowAttributes(display, id, &win->wa);
	XGetWMNormalHints(display, id, &win->sh, &win->sh_mask);
	win->hints = XGetWMHints(display, id);
	XGetTransientForHint(display, id, &trans);
	if (trans) {
		win->transient = trans;
		set_child_transient(win, &trans);
		DNPRINTF(SWM_D_MISC, "manage_window: window: 0x%x, "
		    "transient: 0x%x\n", win->id, win->transient);
	}

	/* get supported protocols */
	if (XGetWMProtocols(display, id, &prot, &n)) {
		for (i = 0, pp = prot; i < n; i++, pp++) {
			if (*pp == takefocus)
				win->take_focus = 1;
			if (*pp == adelete)
				win->can_delete = 1;
		}
		if (prot)
			XFree(prot);
	}

	win->iconic = get_iconic(win);

	/*
	 * Figure out where to put the window. If it was previously assigned to
	 * a workspace (either by spawn() or manually moving), and isn't
	 * transient, * put it in the same workspace
	 */
	r = root_to_region(win->wa.root);
	if (p) {
		ws = &r->s->ws[p->ws];
		TAILQ_REMOVE(&pidlist, p, entry);
		free(p);
		p = NULL;
	} else if (prop && win->transient == 0) {
		DNPRINTF(SWM_D_PROP, "manage_window: get _SWM_WS: %s\n", prop);
		ws_idx = strtonum((const char *)prop, 0, workspace_limit - 1,
		    &errstr);
		if (errstr) {
			DNPRINTF(SWM_D_EVENT, "manage_window: window: #%s: %s",
			    errstr, prop);
		}
		ws = &r->s->ws[ws_idx];
	} else {
		ws = r->ws;
		/* this should launch transients in the same ws as parent */
		if (id && trans)
			if ((ww = find_window(trans)) != NULL)
				if (ws->r) {
					ws = ww->ws;
					if (ww->ws->r)
						r = ww->ws->r;
					else
						warnx("manage_window: fix this "
						    "bug mcbride");
					border_me = 1;
				}
	}

	/* set up the window layout */
	win->id = id;
	win->ws = ws;
	win->s = r->s;	/* this never changes */
	if (trans && (ww = find_window(trans)))
		TAILQ_INSERT_AFTER(&ws->winlist, ww, win, entry);
	else if (win->ws->focus && spawn_position == SWM_STACK_ABOVE)
		TAILQ_INSERT_AFTER(&win->ws->winlist, win->ws->focus, win,
		    entry);
	else if (win->ws->focus && spawn_position == SWM_STACK_BELOW)
		TAILQ_INSERT_BEFORE(win->ws->focus, win, entry);
	else switch (spawn_position) {
	default:
	case SWM_STACK_TOP:
	case SWM_STACK_ABOVE:
		TAILQ_INSERT_TAIL(&win->ws->winlist, win, entry);
		break;
	case SWM_STACK_BOTTOM:
	case SWM_STACK_BELOW:
		TAILQ_INSERT_HEAD(&win->ws->winlist, win, entry);
	}

	/* ignore window border if there is one. */
	WIDTH(win) = win->wa.width;
	HEIGHT(win) = win->wa.height;
	X(win) = win->wa.x + win->wa.border_width;
	Y(win) = win->wa.y + win->wa.border_width;
	win->bordered = 0;
	win->g_floatvalid = 0;
	win->floatmaxed = 0;
	win->ewmh_flags = 0;

	DNPRINTF(SWM_D_MISC, "manage_window: window: 0x%x, (x,y) w x h: "
	    "(%d,%d) %d x %d, ws: %d\n", win->id, X(win), Y(win), WIDTH(win),
	    HEIGHT(win), ws->idx);

	constrain_window(win, r, 0);

	/* Set window properties so we can remember this after reincarnation */
	if (ws_idx_atom && prop == NULL &&
	    snprintf((char *)ws_idx_str, SWM_PROPLEN, "%d", ws->idx) <
	        SWM_PROPLEN) {
		DNPRINTF(SWM_D_PROP, "manage_window: set _SWM_WS: %s\n",
		    ws_idx_str);
		XChangeProperty(display, win->id, ws_idx_atom, XA_STRING, 8,
		    PropModeReplace, ws_idx_str, strlen((char *)ws_idx_str));
	}
	if (prop)
		XFree(prop);

	ewmh_autoquirk(win);

	if (XGetClassHint(display, win->id, &win->ch)) {
		DNPRINTF(SWM_D_CLASS, "manage_window: class: %s, name: %s\n",
		    win->ch.res_class, win->ch.res_name);

		/* java is retarded so treat it special */
		if (strstr(win->ch.res_name, "sun-awt")) {
			win->java = 1;
			border_me = 1;
		}

		TAILQ_FOREACH(qp, &quirks, entry) {
			if (!strcmp(win->ch.res_class, qp->class) &&
			    !strcmp(win->ch.res_name, qp->name)) {
				DNPRINTF(SWM_D_CLASS, "manage_window: found: "
				    "class: %s, name: %s\n", win->ch.res_class,
				    win->ch.res_name);
				if (qp->quirk & SWM_Q_FLOAT) {
					win->floating = 1;
					border_me = 1;
				}
				win->quirks = qp->quirk;
			}
		}
	}

	/* alter window position if quirky */
	if (win->quirks & SWM_Q_ANYWHERE) {
		win->manual = 1; /* don't center the quirky windows */
		if (bar_enabled && Y(win) < bar_height)
			Y(win) = bar_height;
		if (WIDTH(win) + X(win) > WIDTH(r))
			X(win) = WIDTH(r) - WIDTH(win) - 2;
		border_me = 1;
	}

	/* Reset font sizes (the bruteforce way; no default keybinding). */
	if (win->quirks & SWM_Q_XTERM_FONTADJ) {
		for (i = 0; i < SWM_MAX_FONT_STEPS; i++)
			fake_keypress(win, XK_KP_Subtract, ShiftMask);
		for (i = 0; i < SWM_MAX_FONT_STEPS; i++)
			fake_keypress(win, XK_KP_Add, ShiftMask);
	}

	ewmh_get_win_state(win);
	ewmh_update_actions(win);
	ewmh_update_win_state(win, None, _NET_WM_STATE_REMOVE);

	/* border me */
	if (border_me) {
		win->bordered = 1;
		X(win) -= border_width;
		Y(win) -= border_width;
		update_window(win);
	}

	event_mask = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE |
			XCB_EVENT_MASK_PROPERTY_CHANGE |
			XCB_EVENT_MASK_STRUCTURE_NOTIFY;

	xcb_change_window_attributes(conn, id, XCB_CW_EVENT_MASK, &event_mask);

	/* floaters need to be mapped if they are in the current workspace */
	if ((win->floating || win->transient) && (ws->idx == r->ws->idx))
		map_window_raised(win->id);

	return (win);
}

void
free_window(struct ws_win *win)
{
	DNPRINTF(SWM_D_MISC, "free_window: window: 0x%x\n", win->id);

	if (win == NULL)
		return;

	/* needed for restart wm */
	set_win_state(win, XCB_ICCCM_WM_STATE_WITHDRAWN);

	TAILQ_REMOVE(&win->ws->unmanagedlist, win, entry);

	if (win->ch.res_class)
		XFree(win->ch.res_class);
	if (win->ch.res_name)
		XFree(win->ch.res_name);

	kill_refs(win);

	/* paint memory */
	memset(win, 0xff, sizeof *win);	/* XXX kill later */

	free(win);
}

void
unmanage_window(struct ws_win *win)
{
	struct ws_win		*parent;
	xcb_screen_t		*screen;

	if (win == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "unmanage_window: window: 0x%x\n", win->id);

	if (win->transient) {
		parent = find_window(win->transient);
		if (parent)
			parent->child_trans = NULL;
	}

	/* focus on root just in case */
	screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
	xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT,
		screen->root, XCB_CURRENT_TIME);

	focus_prev(win);

	if (win->hints) {
		XFree(win->hints);
		win->hints = NULL;
	}

	TAILQ_REMOVE(&win->ws->winlist, win, entry);
	TAILQ_INSERT_TAIL(&win->ws->unmanagedlist, win, entry);
}

void
focus_magic(struct ws_win *win)
{
	DNPRINTF(SWM_D_FOCUS, "focus_magic: window: 0x%x\n", WINID(win));

	if (win == NULL) {
		/* if there are no windows clear the status-bar */
		bar_update();
		return;
	}

	if (win->child_trans) {
		/* win = parent & has a transient so focus on that */
		if (win->java) {
			focus_win(win->child_trans);
			if (win->child_trans->take_focus)
				client_msg(win, takefocus);
		} else {
			/* make sure transient hasn't disappeared */
			if (validate_win(win->child_trans) == 0) {
				focus_win(win->child_trans);
				if (win->child_trans->take_focus)
					client_msg(win->child_trans, takefocus);
			} else {
				win->child_trans = NULL;
				focus_win(win);
				if (win->take_focus)
					client_msg(win, takefocus);
			}
		}
	} else {
		/* regular focus */
		focus_win(win);
		if (win->take_focus)
			client_msg(win, takefocus);
	}
}

void
expose(XEvent *e)
{
	DNPRINTF(SWM_D_EVENT, "expose: window: 0x%lx\n", e->xexpose.window);
}

void
keypress(XEvent *e)
{
	KeySym			keysym;
	XKeyEvent		*ev = &e->xkey;
	struct key		*kp;
	struct swm_region	*r;

	keysym = XkbKeycodeToKeysym(display, (KeyCode)ev->keycode, 0, 0);
	if ((kp = key_lookup(CLEANMASK(ev->state), keysym)) == NULL)
		return;
	if (keyfuncs[kp->funcid].func == NULL)
		return;

	r = root_to_region(ev->root);
	if (kp->funcid == kf_spawn_custom)
		spawn_custom(r, &(keyfuncs[kp->funcid].args), kp->spawn_name);
	else
		keyfuncs[kp->funcid].func(r, &(keyfuncs[kp->funcid].args));
}

void
buttonpress(XEvent *e)
{
	struct ws_win		*win;
	int			i, action;
	XButtonPressedEvent	*ev = &e->xbutton;

	if ((win = find_window(ev->window)) == NULL)
		return;

	focus_magic(win);
	action = client_click;

	for (i = 0; i < LENGTH(buttons); i++)
		if (action == buttons[i].action && buttons[i].func &&
		    buttons[i].button == ev->button &&
		    CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
			buttons[i].func(win, &buttons[i].args);
}

void
configurerequest(XEvent *e)
{
	XConfigureRequestEvent	*ev = &e->xconfigurerequest;
	struct ws_win		*win;
	int			new = 0;
	XWindowChanges		wc;

	if ((win = find_window(ev->window)) == NULL)
		if ((win = find_unmanaged_window(ev->window)) == NULL)
			new = 1;

	if (new) {
		bzero(&wc, sizeof wc);
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;

		DNPRINTF(SWM_D_EVENT, "configurerequest: new window: 0x%lx, "
		    "new: %s, (x,y) w x h: (%d,%d) %d x %d\n", ev->window,
		    YESNO(new), wc.x, wc.y, wc.width, wc.height);

		XConfigureWindow(display, ev->window, ev->value_mask, &wc);
	} else if ((!win->manual || win->quirks & SWM_Q_ANYWHERE) &&
	    !(win->sh_mask & EWMH_F_FULLSCREEN)) {
		win->g_float.x = ev->x - X(win->ws->r);
		win->g_float.y = ev->y - Y(win->ws->r);
		win->g_float.w = ev->width;
		win->g_float.h = ev->height;
		win->g_floatvalid = 1;

		if (win->floating) {
			win->g = win->g_float;
			win->g.x += X(win->ws->r);
			win->g.y += Y(win->ws->r);
			update_window(win);
		} else {
			config_win(win, ev);
		}
	} else {
		config_win(win, ev);
	}
}

void
configurenotify(XEvent *e)
{
	struct ws_win		*win;

	DNPRINTF(SWM_D_EVENT, "configurenotify: window: 0x%lx\n",
	    e->xconfigure.window);

	win = find_window(e->xconfigure.window);
	if (win) {
		XGetWMNormalHints(display, win->id, &win->sh, &win->sh_mask);
		adjust_font(win);
		if (font_adjusted)
			stack();
		if (focus_mode == SWM_FOCUS_DEFAULT)
			drain_enter_notify();
	}
}

void
destroynotify(XEvent *e)
{
	struct ws_win		*win;
	XDestroyWindowEvent	*ev = &e->xdestroywindow;

	DNPRINTF(SWM_D_EVENT, "destroynotify: window: 0x%lx\n", ev->window);

	if ((win = find_window(ev->window)) == NULL) {
		if ((win = find_unmanaged_window(ev->window)) == NULL)
			return;
		free_window(win);
		return;
	}

	/* make sure we focus on something */
	win->floating = 0;

	unmanage_window(win);
	stack();
	if (focus_mode == SWM_FOCUS_DEFAULT)
		drain_enter_notify();
	free_window(win);
}

void
enternotify(XEvent *e)
{
	XCrossingEvent		*ev = &e->xcrossing;
	XEvent			cne;
	struct ws_win		*win;
#if 0
	struct ws_win		*w;
	Window			focus_return;
	int			revert_to_return;
#endif
	DNPRINTF(SWM_D_FOCUS, "enternotify: window: 0x%lx, mode: %d, detail: "
	    "%d, root: 0x%lx, subwindow: 0x%lx, same_screen: %s, focus: %s, "
	    "state: %d\n", ev->window, ev->mode, ev->detail, ev->root,
	    ev->subwindow, YESNO(ev->same_screen), YESNO(ev->focus), ev->state);

	if (ev->mode != NotifyNormal) {
		DNPRINTF(SWM_D_EVENT, "skip enternotify: generated by "
		    "cursor grab.\n");
		return;
	}

	switch (focus_mode) {
	case SWM_FOCUS_DEFAULT:
		break;
	case SWM_FOCUS_FOLLOW:
		break;
	case SWM_FOCUS_SYNERGY:
#if 0
	/*
	 * all these checks need to be in this order because the
	 * XCheckTypedWindowEvent relies on weeding out the previous events
	 *
	 * making this code an option would enable a follow mouse for focus
	 * feature
	 */

	/*
	 * state is set when we are switching workspaces and focus is set when
	 * the window or a subwindow already has focus (occurs during restart).
	 *
	 * Only honor the focus flag if last_focus_event is not FocusOut,
	 * this allows spectrwm to continue to control focus when another
	 * program is also playing with it.
	 */
	if (ev->state || (ev->focus && last_focus_event != FocusOut)) {
		DNPRINTF(SWM_D_EVENT, "ignoring enternotify: focus\n");
		return;
	}

	/*
	 * happens when a window is created or destroyed and the border
	 * crosses the mouse pointer and when switching ws
	 *
	 * we need the subwindow test to see if we came from root in order
	 * to give focus to floaters
	 */
	if (ev->mode == NotifyNormal && ev->detail == NotifyVirtual &&
	    ev->subwindow == 0) {
		DNPRINTF(SWM_D_EVENT, "ignoring enternotify: NotifyVirtual\n");
		return;
	}

	/* this window already has focus */
	if (ev->mode == NotifyNormal && ev->detail == NotifyInferior) {
		DNPRINTF(SWM_D_EVENT, "ignoring enternotify: win has focus\n");
		return;
	}

	/* this window is being deleted or moved to another ws */
	if (XCheckTypedWindowEvent(display, ev->window, ConfigureNotify,
	    &cne) == True) {
		DNPRINTF(SWM_D_EVENT, "ignoring enternotify: configurenotify\n");
		XPutBackEvent(display, &cne);
		return;
	}

	if ((win = find_window(ev->window)) == NULL) {
		DNPRINTF(SWM_D_EVENT, "ignoring enternotify: win == NULL\n");
		return;
	}

	/*
	 * In fullstack kill all enters unless they come from a different ws
	 * (i.e. another region) or focus has been grabbed externally.
	 */
	if (win->ws->cur_layout->flags & SWM_L_FOCUSPREV &&
	    last_focus_event != FocusOut) {
		XGetInputFocus(display, &focus_return, &revert_to_return);
		if ((w = find_window(focus_return)) == NULL ||
		    w->ws == win->ws) {
			DNPRINTF(SWM_D_EVENT, "ignoring event: fullstack\n");
			return;
		}
	}
#endif
		break;
	}

	if ((win = find_window(ev->window)) == NULL) {
		DNPRINTF(SWM_D_EVENT, "skip enternotify: window is NULL\n");
		return;
	}

	/*
	 * if we have more enternotifies let them handle it in due time
	 */
	if (XCheckTypedEvent(display, EnterNotify, &cne) == True) {
		DNPRINTF(SWM_D_EVENT,
		    "ignoring enternotify: got more enternotify\n");
		XPutBackEvent(display, &cne);
		return;
	}

	focus_magic(win);
}

/* lets us use one switch statement for arbitrary mode/detail combinations */
#define MERGE_MEMBERS(a,b)	(((a & 0xffff) << 16) | (b & 0xffff))

void
focusevent(XEvent *e)
{
#if 0
	struct ws_win		*win;
	u_int32_t		mode_detail;
	XFocusChangeEvent	*ev = &e->xfocus;

	DNPRINTF(SWM_D_EVENT, "focusevent: %s window: 0x%lx mode: %d "
	    "detail: %d\n", ev->type == FocusIn ? "entering" : "leaving",
	    ev->window, ev->mode, ev->detail);

	if (last_focus_event == ev->type) {
		DNPRINTF(SWM_D_FOCUS, "ignoring focusevent: bad ordering\n");
		return;
	}

	last_focus_event = ev->type;
	mode_detail = MERGE_MEMBERS(ev->mode, ev->detail);

	switch (mode_detail) {
	/* synergy client focus operations */
	case MERGE_MEMBERS(NotifyNormal, NotifyNonlinear):
	case MERGE_MEMBERS(NotifyNormal, NotifyNonlinearVirtual):

	/* synergy server focus operations */
	case MERGE_MEMBERS(NotifyWhileGrabbed, NotifyNonlinear):

	/* Entering applications like rdesktop that mangle the pointer */
	case MERGE_MEMBERS(NotifyNormal, NotifyPointer):

		if ((win = find_window(e->xfocus.window)) != NULL && win->ws->r)
			XSetWindowBorder(display, win->id,
			    win->ws->r->s->c[ev->type == FocusIn ?
			    SWM_S_COLOR_FOCUS : SWM_S_COLOR_UNFOCUS].color);
		break;
	default:
		warnx("ignoring focusevent");
		DNPRINTF(SWM_D_FOCUS, "ignoring focusevent\n");
		break;
	}
#endif
}

void
mapnotify(XEvent *e)
{
	struct ws_win		*win;
	XMapEvent		*ev = &e->xmap;

	DNPRINTF(SWM_D_EVENT, "mapnotify: window: 0x%lx\n", ev->window);

	win = manage_window(ev->window);
	if (win)
		set_win_state(win, XCB_ICCCM_WM_STATE_NORMAL);

	/*
	 * focus_win can only set input focus on a mapped window.
	 * make sure the window really has focus since it is just being mapped.
	 */
	if (win->ws->focus == win)
		focus_win(win);
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent		*ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e)
{
	struct ws_win		*win;
	struct swm_region	*r;
	XWindowAttributes	wa;
	XMapRequestEvent	*ev = &e->xmaprequest;

	DNPRINTF(SWM_D_EVENT, "maprequest: window: 0x%lx\n",
	    e->xmaprequest.window);

	if (!XGetWindowAttributes(display, ev->window, &wa))
		return;
	if (wa.override_redirect)
		return;

	win = manage_window(e->xmaprequest.window);
	if (win == NULL)
		return; /* can't happen */

	stack();

	/* make new win focused */
	r = root_to_region(win->wa.root);
	if (win->ws == r->ws)
		focus_magic(win);
}

void
propertynotify(XEvent *e)
{
	struct ws_win		*win;
	XPropertyEvent		*ev = &e->xproperty;
#ifdef SWM_DEBUG
	char			*name;
	name = XGetAtomName(display, ev->atom);
	DNPRINTF(SWM_D_EVENT, "propertynotify: window: 0x%lx, atom: %s\n",
	    ev->window, name);
	XFree(name);
#endif

	win = find_window(ev->window);
	if (win == NULL)
		return;

	if (ev->state == PropertyDelete && ev->atom == a_swm_iconic) {
		update_iconic(win, 0);
		map_window_raised(win->id);
		stack();
		focus_win(win);
		return;
	}

	switch (ev->atom) {
#if 0
	case XA_WM_NORMAL_HINTS:
		long		mask;
		XGetWMNormalHints(display, win->id, &win->sh, &mask);
		warnx("normal hints: flag 0x%x", win->sh.flags);
		if (win->sh.flags & PMinSize) {
			WIDTH(win) = win->sh.min_width;
			HEIGHT(win) = win->sh.min_height;
			warnx("min %d %d", WIDTH(win), HEIGHT(win));
		}
		XMoveResizeWindow(display, win->id,
		    X(win), Y(win), WIDTH(win), HEIGHT(win));
#endif
	case XA_WM_CLASS:
	case XA_WM_NAME:
		bar_update();
		break;
	default:
		break;
	}
}

void
unmapnotify(XEvent *e)
{
	struct ws_win		*win;

	DNPRINTF(SWM_D_EVENT, "unmapnotify: window: 0x%lx\n", e->xunmap.window);

	/* determine if we need to help unmanage this window */
	win = find_window(e->xunmap.window);
	if (win == NULL)
		return;

	if (getstate(e->xunmap.window) == XCB_ICCCM_WM_STATE_NORMAL) {
		unmanage_window(win);
		stack();

		/* giant hack for apps that don't destroy transient windows */
		/* eat a bunch of events to prevent remanaging the window */
		XEvent			cne;
		while (XCheckWindowEvent(display, e->xunmap.window,
		    EnterWindowMask, &cne))
			;
		while (XCheckWindowEvent(display, e->xunmap.window,
		    StructureNotifyMask, &cne))
			;
		while (XCheckWindowEvent(display, e->xunmap.window,
		    SubstructureNotifyMask, &cne))
			;
		/* resend unmap because we ated it */
		XUnmapWindow(display, e->xunmap.window);
	}

	if (focus_mode == SWM_FOCUS_DEFAULT)
		drain_enter_notify();
}

void
visibilitynotify(XEvent *e)
{
	int			i, num_screens;
	struct swm_region	*r;

	DNPRINTF(SWM_D_EVENT, "visibilitynotify: window: 0x%lx\n",
	    e->xvisibility.window);

	if (e->xvisibility.state == VisibilityUnobscured) {
		num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
		for (i = 0; i < num_screens; i++)
			TAILQ_FOREACH(r, &screens[i].rl, entry)
				if (e->xvisibility.window == WINID(r->bar))
					bar_update();
	}
}

void
clientmessage(XEvent *e)
{
	XClientMessageEvent	*ev;
	struct ws_win		*win;

	ev = &e->xclient;

	win = find_window(ev->window);
	if (win == NULL) {
		if (ev->message_type == ewmh[_NET_ACTIVE_WINDOW].atom) {
			DNPRINTF(SWM_D_EVENT, "clientmessage: request focus on "
			    "unmanaged window.\n");
			e->xmaprequest.window = ev->window;
			maprequest(e);
		}
		return;
	}

	DNPRINTF(SWM_D_EVENT, "clientmessage: window: 0x%lx, type: %ld\n",
	    ev->window, ev->message_type);

	if (ev->message_type == ewmh[_NET_ACTIVE_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT, "clientmessage: _NET_ACTIVE_WINDOW\n");
		focus_win(win);
	}
	if (ev->message_type == ewmh[_NET_CLOSE_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT, "clientmessage: _NET_CLOSE_WINDOW\n");
		if (win->can_delete)
			client_msg(win, adelete);
		else
			xcb_kill_client(conn, win->id);
	}
	if (ev->message_type == ewmh[_NET_MOVERESIZE_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT,
		    "clientmessage: _NET_MOVERESIZE_WINDOW\n");
		if (win->floating) {
			if (ev->data.l[0] & (1<<8)) /* x */
				X(win) = ev->data.l[1];
			if (ev->data.l[0] & (1<<9)) /* y */
				Y(win) = ev->data.l[2];
			if (ev->data.l[0] & (1<<10)) /* width */
				WIDTH(win) = ev->data.l[3];
			if (ev->data.l[0] & (1<<11)) /* height */
				HEIGHT(win) = ev->data.l[4];

			update_window(win);
		}
		else {
			/* TODO: Change stack sizes */
			/* notify no change was made. */
			config_win(win, NULL);
		}
	}
	if (ev->message_type == ewmh[_NET_WM_STATE].atom) {
		DNPRINTF(SWM_D_EVENT, "clientmessage: _NET_WM_STATE\n");
		ewmh_update_win_state(win, ev->data.l[1], ev->data.l[0]);
		if (ev->data.l[2])
			ewmh_update_win_state(win, ev->data.l[2],
			    ev->data.l[0]);

		stack();
	}
}

int
xerror_start(Display *d, XErrorEvent *ee)
{
	other_wm = 1;
	return (-1);
}

int
xerror(Display *d, XErrorEvent *ee)
{
	/* warnx("error: %p %p", display, ee); */
	return (-1);
}

int
active_wm(void)
{
	other_wm = 0;
	xerrorxlib = XSetErrorHandler(xerror_start);

	/* this causes an error if some other window manager is running */
	XSelectInput(display, DefaultRootWindow(display),
	    SubstructureRedirectMask);
	do_sync();
	if (other_wm)
		return (1);

	XSetErrorHandler(xerror);
	do_sync();
	return (0);
}

void
new_region(struct swm_screen *s, int x, int y, int w, int h)
{
	struct swm_region	*r, *n;
	struct workspace	*ws = NULL;
	int			i;

	DNPRINTF(SWM_D_MISC, "new region: screen[%d]:%dx%d+%d+%d\n",
	     s->idx, w, h, x, y);

	/* remove any conflicting regions */
	n = TAILQ_FIRST(&s->rl);
	while (n) {
		r = n;
		n = TAILQ_NEXT(r, entry);
		if (X(r) < (x + w) &&
		    (X(r) + WIDTH(r)) > x &&
		    Y(r) < (y + h) &&
		    (Y(r) + HEIGHT(r)) > y) {
			if (r->ws->r != NULL)
				r->ws->old_r = r->ws->r;
			r->ws->r = NULL;
			bar_cleanup(r);
			TAILQ_REMOVE(&s->rl, r, entry);
			TAILQ_INSERT_TAIL(&s->orl, r, entry);
		}
	}

	/* search old regions for one to reuse */

	/* size + location match */
	TAILQ_FOREACH(r, &s->orl, entry)
		if (X(r) == x && Y(r) == y &&
		    HEIGHT(r) == h && WIDTH(r) == w)
			break;

	/* size match */
	TAILQ_FOREACH(r, &s->orl, entry)
		if (HEIGHT(r) == h && WIDTH(r) == w)
			break;

	if (r != NULL) {
		TAILQ_REMOVE(&s->orl, r, entry);
		/* try to use old region's workspace */
		if (r->ws->r == NULL)
			ws = r->ws;
	} else
		if ((r = calloc(1, sizeof(struct swm_region))) == NULL)
			err(1, "new_region: calloc: failed to allocate memory "
			    "for screen");

	/* if we don't have a workspace already, find one */
	if (ws == NULL) {
		for (i = 0; i < workspace_limit; i++)
			if (s->ws[i].r == NULL) {
				ws = &s->ws[i];
				break;
			}
	}

	if (ws == NULL)
		errx(1, "new_region: no free workspaces");

	X(r) = x;
	Y(r) = y;
	WIDTH(r) = w;
	HEIGHT(r) = h;
	r->s = s;
	r->ws = ws;
	r->ws_prior = NULL;
	ws->r = r;
	outputs++;
	TAILQ_INSERT_TAIL(&s->rl, r, entry);
}

void
scan_xrandr(int i)
{
#ifdef SWM_XRR_HAS_CRTC
	int						c;
	int						ncrtc = 0;
#endif /* SWM_XRR_HAS_CRTC */
	struct swm_region				*r;
	int						num_screens;
	xcb_randr_get_screen_resources_current_cookie_t	src;
	xcb_randr_get_screen_resources_current_reply_t	*srr;
	xcb_randr_get_crtc_info_cookie_t		cic;
	xcb_randr_get_crtc_info_reply_t			*cir = NULL;
	xcb_randr_crtc_t				*crtc;
	xcb_screen_t					*screen = get_screen(i);

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	if (i >= num_screens)
		errx(1, "scan_xrandr: invalid screen");

	/* remove any old regions */
	while ((r = TAILQ_FIRST(&screens[i].rl)) != NULL) {
		r->ws->old_r = r->ws->r = NULL;
		bar_cleanup(r);
		TAILQ_REMOVE(&screens[i].rl, r, entry);
		TAILQ_INSERT_TAIL(&screens[i].orl, r, entry);
	}
	outputs = 0;

	/* map virtual screens onto physical screens */
#ifdef SWM_XRR_HAS_CRTC
	if (xrandr_support) {
		src = xcb_randr_get_screen_resources_current(conn,
			screens[i].root);
		srr = xcb_randr_get_screen_resources_current_reply(conn, src,
			NULL);
		if (srr == NULL) {
			new_region(&screens[i], 0, 0,
			    screen->width_in_pixels,
			    screen->height_in_pixels);
			return;
		} else
			ncrtc = srr->num_crtcs;
		for (c = 0; c < ncrtc; c++) {
			crtc = xcb_randr_get_screen_resources_current_crtcs(srr);
			cic = xcb_randr_get_crtc_info(conn, crtc[c],
				XCB_CURRENT_TIME);
			cir = xcb_randr_get_crtc_info_reply(conn, cic, NULL);
			if (cir == NULL)
				continue;
			if (cir->num_outputs == 0) {
				free(cir);
				continue;
			}

			if (cir->mode == 0)
				new_region(&screens[i], 0, 0,
				    screen->width_in_pixels,
				    screen->height_in_pixels);
			else
				new_region(&screens[i],
				    cir->x, cir->y, cir->width, cir->height);
			free(cir);
		}
		free(srr);
	} else
#endif /* SWM_XRR_HAS_CRTC */
	{
		new_region(&screens[i], 0, 0, screen->width_in_pixels,
		    screen->height_in_pixels);
	}
}

void
screenchange(XEvent *e)
{
	XRRScreenChangeNotifyEvent	*xe = (XRRScreenChangeNotifyEvent *)e;
	struct swm_region		*r;
	int				i, num_screens;

	DNPRINTF(SWM_D_EVENT, "screenchange: root: 0x%lx\n", xe->root);

	if (!XRRUpdateConfiguration(e))
		return;

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	/* silly event doesn't include the screen index */
	for (i = 0; i < num_screens; i++)
		if (screens[i].root == xe->root)
			break;
	if (i >= num_screens)
		errx(1, "screenchange: screen not found");

	/* brute force for now, just re-enumerate the regions */
	scan_xrandr(i);

	/* add bars to all regions */
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			bar_setup(r);
	stack();
	if (focus_mode == SWM_FOCUS_DEFAULT)
		drain_enter_notify();
}

void
grab_windows(void)
{
	xcb_window_t		*wins	= NULL;
	int			no;
	int			i, j, num_screens;
	uint16_t		state, manage;

	xcb_query_tree_cookie_t			qtc;
	xcb_query_tree_reply_t			*qtr;
	xcb_get_window_attributes_cookie_t	c;
	xcb_get_window_attributes_reply_t	*r;
	xcb_get_property_cookie_t		pc;

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++) {
		qtc = xcb_query_tree(conn, screens[i].root);
		qtr = xcb_query_tree_reply(conn, qtc, NULL);
		if (!qtr)
			continue;
		wins = xcb_query_tree_children(qtr);
		no = xcb_query_tree_children_length(qtr);
		/* attach windows to a region */
		/* normal windows */
		for (j = 0; j < no; j++) {
			c = xcb_get_window_attributes(conn, wins[j]);
			r = xcb_get_window_attributes_reply(conn, c, NULL);
			if (!r)
				continue;
			if (r->override_redirect) {
				free(r);
				continue;
			}

			pc = xcb_icccm_get_wm_transient_for(conn, wins[j]);
			if (xcb_icccm_get_wm_transient_for_reply(conn, pc, &wins[j],
					NULL)) {
				free(r);
				continue;
			}

			state = getstate(wins[j]);
			manage = state == XCB_ICCCM_WM_STATE_ICONIC;
			if (r->map_state == XCB_MAP_STATE_VIEWABLE || manage)
				manage_window(wins[j]);
			free(r);
		}
		/* transient windows */
		for (j = 0; j < no; j++) {
			c = xcb_get_window_attributes(conn, wins[j]);
			r = xcb_get_window_attributes_reply(conn, c, NULL);
			if (!r)
				continue;
			if (r->override_redirect) {
				free(r);
				continue;
			}
			free(r);

			state = getstate(wins[j]);
			manage = state == XCB_ICCCM_WM_STATE_ICONIC;
			pc = xcb_icccm_get_wm_transient_for(conn, wins[j]);
			if (xcb_icccm_get_wm_transient_for_reply(conn, pc, &wins[j],
					NULL) && manage)
				manage_window(wins[j]);
		}
		free(qtr);
	}
}

void
setup_screens(void)
{
	int			i, j, k, num_screens;
	struct workspace	*ws;
	uint32_t		gcv[1], wa[1];
	const xcb_query_extension_reply_t *qep;
	xcb_cursor_t				cursor;
	xcb_font_t				cursor_font;
	xcb_randr_query_version_cookie_t	c;
	xcb_randr_query_version_reply_t		*r;

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	if ((screens = calloc(num_screens,
	     sizeof(struct swm_screen))) == NULL)
		err(1, "setup_screens: calloc: failed to allocate memory for "
		    "screens");

	/* initial Xrandr setup */
	xrandr_support = False;
	c = xcb_randr_query_version(conn, True, True);
	r = xcb_randr_query_version_reply(conn, c, NULL);
	if (r) {
		if (r->major_version >= 1)
			xrandr_support = True;
		free(r);
	}
	qep = xcb_get_extension_data(conn, &xcb_randr_id);
	xrandr_eventbase = qep->first_event;

	cursor_font = xcb_generate_id(conn);
	xcb_open_font(conn, cursor_font, strlen("cursor"), "cursor");

	cursor = xcb_generate_id(conn);
	xcb_create_glyph_cursor(conn, cursor, cursor_font, cursor_font,
		XC_left_ptr, XC_left_ptr + 1, 0, 0, 0, 0xffff, 0xffff, 0xffff);
	wa[0] = cursor;

	/* map physical screens */
	for (i = 0; i < num_screens; i++) {
		DNPRINTF(SWM_D_WS, "setup_screens: init screen: %d\n", i);
		screens[i].idx = i;
		TAILQ_INIT(&screens[i].rl);
		TAILQ_INIT(&screens[i].orl);
		screens[i].root = RootWindow(display, i);

		/* set default colors */
		setscreencolor("red", i + 1, SWM_S_COLOR_FOCUS);
		setscreencolor("rgb:88/88/88", i + 1, SWM_S_COLOR_UNFOCUS);
		setscreencolor("rgb:00/80/80", i + 1, SWM_S_COLOR_BAR_BORDER);
		setscreencolor("black", i + 1, SWM_S_COLOR_BAR);
		setscreencolor("rgb:a0/a0/a0", i + 1, SWM_S_COLOR_BAR_FONT);

		/* create graphics context on screen */
		screens[i].bar_gc = xcb_generate_id(conn);
		gcv[0] = 0;
		xcb_create_gc(conn, screens[i].bar_gc, screens[i].root,
			XCB_GC_GRAPHICS_EXPOSURES, gcv);

		/* set default cursor */
		xcb_change_window_attributes(conn, screens[i].root,
			XCB_CW_CURSOR, wa); 	

		/* init all workspaces */
		/* XXX these should be dynamically allocated too */
		for (j = 0; j < SWM_WS_MAX; j++) {
			ws = &screens[i].ws[j];
			ws->idx = j;
			ws->name = NULL;
			ws->focus = NULL;
			ws->r = NULL;
			ws->old_r = NULL;
			TAILQ_INIT(&ws->winlist);
			TAILQ_INIT(&ws->unmanagedlist);

			for (k = 0; layouts[k].l_stack != NULL; k++)
				if (layouts[k].l_config != NULL)
					layouts[k].l_config(ws,
					    SWM_ARG_ID_STACKINIT);
			ws->cur_layout = &layouts[0];
			ws->cur_layout->l_string(ws);
		}

		scan_xrandr(i);

		if (xrandr_support)
			xcb_randr_select_input(conn, screens[i].root,
				XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
	}
	xcb_free_cursor(conn, cursor);
	xcb_close_font(conn, cursor_font);
}

void
setup_globals(void)
{
	if ((bar_fonts = strdup(SWM_BAR_FONTS)) == NULL)
		err(1, "setup_globals: strdup: failed to allocate memory.");

	if ((clock_format = strdup("%a %b %d %R %Z %Y")) == NULL)
		err(1, "setup_globals: strdup: failed to allocate memory.");
}

void
workaround(void)
{
	int			i, num_screens;
	xcb_atom_t		netwmcheck, netwmname, utf8_string;
	xcb_window_t		root, win;
	xcb_screen_t		*screen;
	uint32_t		wa[2];

	/* work around sun jdk bugs, code from wmname */
	netwmcheck = get_atom_from_string("_NET_SUPPORTING_WM_CHECK");
	netwmname = get_atom_from_string("_NET_WM_NAME");
	utf8_string = get_atom_from_string("UTF8_STRING");

	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++) {
		root = screens[i].root;
		screen = get_screen(i);

		win = xcb_generate_id(conn);
		wa[0] = screens[i].c[SWM_S_COLOR_UNFOCUS].color;
		wa[1] = screens[i].c[SWM_S_COLOR_UNFOCUS].color;
		xcb_create_window(conn, screen->root_depth, win, 0,
			0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
			screen->root_visual,
			XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL, wa);

		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, root,
			netwmcheck, XCB_ATOM_WINDOW, 32, 1, &win);
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win,
			netwmcheck, XCB_ATOM_WINDOW, 32, 1, &win);
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win,
			netwmname, utf8_string, 8, strlen("LG3D"), "LG3D");
	}
}

int
main(int argc, char *argv[])
{
	struct swm_region	*r, *rr;
	struct ws_win		*winfocus = NULL;
	struct timeval		tv;
	union arg		a;
	char			conf[PATH_MAX], *cfile = NULL;
	struct stat		sb;
	XEvent			e;
	int			xfd, i, num_screens;
	fd_set			rd;
	struct sigaction	sact;

	start_argv = argv;
	warnx("Welcome to spectrwm V%s Build: %s", SPECTRWM_VERSION, buildstr);
	if (!setlocale(LC_CTYPE, "") || !setlocale(LC_TIME, "") ||
	    !XSupportsLocale())
		warnx("no locale support");

	if (!X_HAVE_UTF8_STRING)
		warnx("no UTF-8 support");

	if (!(display = XOpenDisplay(0)))
		errx(1, "can not open display");

	if (!(conn = XGetXCBConnection(display)))
		errx(1, "can not get XCB connection");

	if (active_wm())
		errx(1, "other wm running");

	/* handle some signals */
	bzero(&sact, sizeof(sact));
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
	sact.sa_handler = sighdlr;
	sigaction(SIGINT, &sact, NULL);
	sigaction(SIGQUIT, &sact, NULL);
	sigaction(SIGTERM, &sact, NULL);
	sigaction(SIGHUP, &sact, NULL);

	sact.sa_handler = sighdlr;
	sact.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &sact, NULL);

	astate = get_atom_from_string("WM_STATE");
	aprot = get_atom_from_string("WM_PROTOCOLS");
	adelete = get_atom_from_string("WM_DELETE_WINDOW");
	takefocus = get_atom_from_string("WM_TAKE_FOCUS");
	a_wmname = get_atom_from_string("WM_NAME");
	a_netwmname = get_atom_from_string("_NET_WM_NAME");
	a_utf8_string = get_atom_from_string("UTF8_STRING");
	a_string = get_atom_from_string("STRING");
	a_swm_iconic = get_atom_from_string("_SWM_ICONIC");

	/* look for local and global conf file */
	pwd = getpwuid(getuid());
	if (pwd == NULL)
		errx(1, "invalid user: %d", getuid());

	setup_globals();
	setup_screens();
	setup_keys();
	setup_quirks();
	setup_spawn();

	/* load config */
	for (i = 0; ; i++) {
		conf[0] = '\0';
		switch (i) {
		case 0:
			/* ~ */
			snprintf(conf, sizeof conf, "%s/.%s",
			    pwd->pw_dir, SWM_CONF_FILE);
			break;
		case 1:
			/* global */
			snprintf(conf, sizeof conf, "/etc/%s",
			    SWM_CONF_FILE);
			break;
		case 2:
			/* ~ compat */
			snprintf(conf, sizeof conf, "%s/.%s",
			    pwd->pw_dir, SWM_CONF_FILE_OLD);
			break;
		case 3:
			/* global compat */
			snprintf(conf, sizeof conf, "/etc/%s",
			    SWM_CONF_FILE_OLD);
			break;
		default:
			goto noconfig;
		}

		if (strlen(conf) && stat(conf, &sb) != -1)
			if (S_ISREG(sb.st_mode)) {
				cfile = conf;
				break;
			}
	}
noconfig:

	/* load conf (if any) */
	if (cfile)
		conf_load(cfile, SWM_CONF_DEFAULT);

	setup_ewmh();
	/* set some values to work around bad programs */
	workaround();
	/* grab existing windows (before we build the bars) */
	grab_windows();

	if (getenv("SWM_STARTED") == NULL)
		setenv("SWM_STARTED", "YES", 1);

	/* setup all bars */
	num_screens = xcb_setup_roots_length(xcb_get_setup(conn));
	for (i = 0; i < num_screens; i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			if (winfocus == NULL)
				winfocus = TAILQ_FIRST(&r->ws->winlist);
			bar_setup(r);
		}

	unfocus_all();

	grabkeys();
	stack();
	if (focus_mode == SWM_FOCUS_DEFAULT)
		drain_enter_notify();

	xfd = xcb_get_file_descriptor(conn);
	while (running) {
		while (XPending(display)) {
			XNextEvent(display, &e);
			if (running == 0)
				goto done;
			if (e.type < LASTEvent) {
				DNPRINTF(SWM_D_EVENTQ ,"XEvent: handled: %s, "
				    "window: 0x%lx, type: %s (%d), %d remaining"
				    "\n", YESNO(handler[e.type]),
				    e.xany.window, geteventname(&e),
				    e.type, QLength(display));

				if (handler[e.type])
					handler[e.type](&e);
			} else {
				DNPRINTF(SWM_D_EVENTQ, "XRandr Event: window: "
				    "0x%lx, type: %s (%d)\n", e.xany.window,
				    xrandr_geteventname(&e), e.type);

				switch (e.type - xrandr_eventbase) {
				case XCB_RANDR_SCREEN_CHANGE_NOTIFY:
					screenchange(&e);
					break;
				default:
					break;
				}
			}
		}

		/* if we are being restarted go focus on first window */
		if (winfocus) {
			rr = winfocus->ws->r;
			if (rr == NULL) {
				/* not a visible window */
				winfocus = NULL;
				continue;
			}
			/* move pointer to first screen if multi screen */
			if (num_screens > 1 || outputs > 1)
				xcb_warp_pointer(conn, XCB_WINDOW_NONE,
					rr->s[0].root, 0, 0, 0, 0, X(rr),
					Y(rr) + (bar_enabled ? bar_height : 0));

			a.id = SWM_ARG_ID_FOCUSCUR;
			focus(rr, &a);
			winfocus = NULL;
			continue;
		}

		FD_ZERO(&rd);
		FD_SET(xfd, &rd);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		if (select(xfd + 1, &rd, NULL, NULL, &tv) == -1)
			if (errno != EINTR)
				DNPRINTF(SWM_D_MISC, "select failed");
		if (restart_wm == 1)
			restart(NULL, NULL);
		if (search_resp == 1)
			search_do_resp();
		if (running == 0)
			goto done;
		if (bar_alarm) {
			bar_alarm = 0;
			bar_update();
		}
	}
done:
	teardown_ewmh();
	bar_extra_stop();

	for (i = 0; i < num_screens; ++i)
		if (screens[i].bar_gc != 0)
			xcb_free_gc(conn, screens[i].bar_gc);
	XFreeFontSet(display, bar_fs);
	xcb_disconnect(conn);
	XCloseDisplay(display);

	return (0);
}
