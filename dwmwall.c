#include <unistd.h>
#include <dirent.h>

#include <stdlib.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <Imlib2.h>

#include "config.h" 

#ifdef DWMWALL_RANDOMIZE
#include <time.h>
#endif

#define ARRLEN(a) (sizeof(a)/sizeof(a[0]))

static char** imgpaths = NULL;
static int imgcnt = 0;

#ifdef DWMWALL_RANDOMIZE
static char** imgpaths_rand;
static int imgcnt_rand;
#endif


static Display* disp;
static Screen* scr;
static Window win;
static Pixmap pmap;


static void dwmwall_init()
{

#ifdef DWMWALL_RANDOMIZE
    srand(time(NULL));
#endif

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

#ifdef DWMWALL_RANDOMIZE
    imgpaths_rand = malloc(sizeof(*imgpaths) * imgcnt);
    memcpy(imgpaths_rand, imgpaths, sizeof(*imgpaths) * imgcnt);
    imgcnt_rand = imgcnt;
#endif

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
}

static void dwmwall_term()
{
    XFreePixmap(disp, pmap);
    XCloseDisplay(disp);

#ifdef DWMWALL_RANDOMIZE
    free(imgpaths_rand);
#endif
    for (int i = 0; i < imgcnt; ++i) 
        free(imgpaths[i]);

    free(imgpaths);
}

static void dwmwall_fill_pmap(const char* filepath)
{
    Imlib_Image img = imlib_load_image_immediately(filepath);

    imlib_context_set_image(img);

    imlib_render_image_on_drawable_at_size(0, 0, scr->width, scr->height); 

    imlib_free_image_and_decache();

    XClearWindow(disp, win);
    XClearArea(disp, win, 0, 0, scr->width, scr->height, True);
    XFlush(disp);
    XSync(disp, False);
}


int main()
{
    dwmwall_init();

    for (;;) {
        const char* filepath;

#ifdef DWMWALL_RANDOMIZE 
       const int idx = rand() % imgcnt_rand; 
       filepath = imgpaths_rand[idx];
       if (idx < (imgcnt_rand - 1)) {
           memmove(&imgpaths_rand[idx], &imgpaths_rand[idx + 1], sizeof(*imgpaths_rand) * (imgcnt_rand - idx - 1));
       }
       --imgcnt_rand;
       if (imgcnt_rand == 0) {
           memcpy(imgpaths_rand, imgpaths, sizeof(*imgpaths) * imgcnt);
           imgcnt_rand = imgcnt;
       }
#else
       static int i = 0;

        filepath = imgpaths[i];

        if (++i == imgcnt)
            i = 0;
#endif

        dwmwall_fill_pmap(filepath);
        sleep(dwmwall_slide_time);
    }

    dwmwall_term();
    return 0;
}

