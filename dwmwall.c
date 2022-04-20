#include <unistd.h>
#include <dirent.h>
#include <signal.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <Imlib2.h>

#include "dhust-nx-config.h"

#define STATIC_ASSERT(cond, msg) extern void static_assert_##msg(int hack[(cond) ? 1 : -1])
#define ARRLEN(a) (sizeof(a)/sizeof(a[0]))


STATIC_ASSERT(ARRLEN(dwmwall_dirs) > 0, dirs_must_not_be_empty);


static char** imgpaths = NULL;
static int imgcnt = 0;

static Display* disp;
static Screen* scr;
static Window win;
static Pixmap pmap;
static bool terminate = false;

void dwmwall_sighandler(int signum)
{
	switch (signum) {
	case SIGTERM:
	case SIGINT:
		terminate = true;
		break;
	}
}

static void dwmwall_init()
{
	if (dwmwall_randomize)
		srand(time(NULL));

    for (size_t i = 0; i < ARRLEN(dwmwall_dirs); ++i) {
        const char* basepath = dwmwall_dirs[i];
        const int basepathlen = strlen(basepath);

        DIR* dir = opendir(basepath);
        struct dirent* ent;

        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type != DT_REG)
                continue;

            for (size_t j = 0; j < ARRLEN(dwmwall_exts); ++j) {
                const int dnamelen = strlen(ent->d_name);
                const int extlen = strlen(dwmwall_exts[j]);
                if (dnamelen <= extlen)
                    continue;

                if (strcmp(dwmwall_exts[j], &ent->d_name[dnamelen - extlen]) == 0) {
                    imgpaths = realloc(imgpaths, sizeof(char*) * (imgcnt + 1));
                    imgpaths[imgcnt] = malloc(basepathlen + dnamelen + 1);
                    sprintf(imgpaths[imgcnt], "%s/%s", basepath, ent->d_name);
                    ++imgcnt;
                    break;
                }
            }

        }

        closedir(dir);
    }

    disp = XOpenDisplay(NULL);

    const int scrid = DefaultScreen(disp);
    Visual* vis = DefaultVisual(disp, scrid);
    int depth = DefaultDepth(disp, scrid);

    scr = ScreenOfDisplay(disp, scrid);
    win = RootWindow(disp, scrid);
    pmap = XCreatePixmap(disp, win, scr->width, scr->height, depth);

    imlib_context_set_display(disp);
    imlib_context_set_visual(vis);
    imlib_context_set_drawable(pmap);

    const Atom prop_root = XInternAtom(disp, "_XROOTPMAP_ID", False);
    const Atom prop_esetroot = XInternAtom(disp, "ESETROOT_PMAP_ID", False);

    XChangeProperty(
            disp,
            win,
            prop_root,
            XA_PIXMAP,
            32,
            PropModeReplace,
            (unsigned char*)&pmap,
            1
    );

    XChangeProperty(
            disp,
            win,
            prop_esetroot,
            XA_PIXMAP,
            32,
            PropModeReplace,
            (unsigned char*)&pmap,
            1
    );

	signal(SIGINT, dwmwall_sighandler);
	signal(SIGTERM, dwmwall_sighandler);
}

static void dwmwall_term()
{
    XFreePixmap(disp, pmap);
    XCloseDisplay(disp);

    for (int i = 0; i < imgcnt; ++i) 
        free(imgpaths[i]);

    free(imgpaths);
}

static void dwmwall_fill_pmap(const char* imgpath)
{
    Imlib_Image img = imlib_load_image_immediately(imgpath);

    imlib_context_set_image(img);

    imlib_render_image_on_drawable_at_size(0, 0, scr->width, scr->height); 

    imlib_free_image();

    XClearWindow(disp, win);
    XClearArea(disp, win, 0, 0, scr->width, scr->height, True);
    XFlush(disp);
    XSync(disp, False);

}


int main()
{
    dwmwall_init();

	for (int i = 0; !terminate; ++i) {
		if (i == imgcnt)
			i = 0;

		char* imgpath;

		if (dwmwall_randomize) {
			const int idx = rand() % (imgcnt - i); 
			imgpath = imgpaths[idx];
			/* move selected img to the end */
			memmove(
				&imgpaths[idx],
				&imgpaths[idx + 1],
				sizeof(*imgpaths) * (imgcnt - 1 - idx)
			);
			imgpaths[imgcnt - 1] = imgpath;
		} else {
			imgpath = imgpaths[i];
		}

		dwmwall_fill_pmap(imgpath);
		sleep(dwmwall_slide_time);
	}

    dwmwall_term();
    return 0;
}

