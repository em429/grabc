/*  A program to pick a color by clicking the mouse.
 *
 *  RCS:
 *      $Revision$
 *      $Date$
 *
 *  Description:
 *
 *  When this program is run, the mouse pointer is grabbed and changed to
 *  a cross hair and when the mouse is clicked, the color of the clicked
 *  pixel is written to stdout in hex prefixed with #
 *
 *  This program can be useful when you see a color and want to use the
 *  color in xterm or your window manager's border but no clue what the 
 *  name of the color is. It's silly to use a image processing software
 *  to find it out.
 *
 * Example: 
 *   cpick
 *          #ffffff
 *   xterm -bg `cpick` -fg `cpick` (silly but esoteric!) 
 *
 *  Development History:
 *      who                  when               why
 *      ma_muquit@fccc.edu   march-16-19997     first cut
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <time.h>

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#ifndef True
#define True    1
#endif

#ifndef False
#define False   0
#endif

/* private function prototypes */
static Window selectWindow (Display *,int *x,int *y);

static Window findSubWindow(Display *display,Window top_winodw,
    Window window_to_check,int *x,int *y);

static int getWindowColor(Display *display,XColor *color);
static int MXError(Display *display,XErrorEvent *error);

int main(int argc,char **argv)
{
    Display
        *display;

    int
        status;

    XColor  
        color;

    Colormap
        cmap;

    int
        i,
        r,
        g,
        b;

    for (i=1; i < argc; i++)
    {
        if (strncmp(argv[i],"-h",2) == 0)
        {
            (void) fprintf (stderr,"grabc 1.1 by Muhammad A Muquit\n");
            exit(1);
        }
    }
    display=XOpenDisplay((char *) NULL);
    cmap=DefaultColormap(display,DefaultScreen(display));

    XSetErrorHandler(MXError);

    if (display == (Display *) NULL)
    {
        (void) fprintf (stderr,"Failed to open local DISPLAY!'n");
        exit(1);
    }

    status=getWindowColor(display,&color);
    if (status == True)
    {
        XQueryColor(display,cmap,&color);
        r=(color.red >> 8);
        g=(color.green >> 8);
        b=(color.blue >> 8);
        (void) fprintf (stdout,"#%02x%02x%02x\n",r,g,b);
        (void) fflush(stdout);
        /*
        ** write the values in decimal on stderr
        */
        (void) fprintf(stderr,"%d,%d,%d\n",r,g,b);
    }
    else
    {
        (void) fprintf (stderr,"Failed to grab color!\n");
    }
    return (0);
}

/*
** function to select a window
** output parameters: x,y (coordinate of the point of click)
** reutrns Window
** exits if mouse can not be grabbed
*/
static Window selectWindow(Display *display,int *x,int *y)
{
    Cursor
        target_cursor;

    static Cursor
        cross_cursor=(Cursor) NULL;
    int
        status;

    Window
        target_window,
        root_window;

    XEvent
        event;

    target_window=(Window) NULL;

    if (cross_cursor == (Cursor) NULL)
    {
        cross_cursor=XCreateFontCursor(display,XC_tcross);
        if (cross_cursor == (Cursor) NULL)
        {
            (void) fprintf (stderr,"Failed to create Cross Cursor!\n");
            return ((Window) NULL);
        }
    }
    target_cursor=cross_cursor;
    root_window=XRootWindow(display,XDefaultScreen(display));

    status=XGrabPointer(display,root_window,False,
        (unsigned int) ButtonPressMask,GrabModeSync,
        GrabModeAsync,root_window,target_cursor,CurrentTime);

    if (status == GrabSuccess)
    {
        XAllowEvents(display,SyncPointer,CurrentTime);
        XWindowEvent(display,root_window,ButtonPressMask,&event);

        if (event.type == ButtonPress)
        {
            target_window=findSubWindow(display,root_window,
                event.xbutton.subwindow,
                &event.xbutton.x,
                &event.xbutton.y );

            if (target_window == (Window) NULL)
            {
                (void) fprintf (stderr,
                    "Failed to get target window, getting root window!\n");
                target_window=root_window;
            }
            XUngrabPointer(display,CurrentTime);
        }
    }
    else
    {
        (void) fprintf (stderr,"Failed to grab mouse!\n");
        exit(1);
    }

    /* free things we do not need, always a good practice */
    XFreeCursor(display,cross_cursor);

    (*x)=event.xbutton.x;
    (*y)=event.xbutton.y;

    return (target_window);
}

/* find a window */
static Window findSubWindow(Display *display,Window top_window,
    Window window_to_check,int *x,int *y)
{
    int 
        newx,
        newy;

    Window
        window;

    if (top_window == (Window) NULL)
        return ((Window) NULL);

    if (window_to_check == (Window) NULL)
        return ((Window) NULL);

    /* initialize automatics */
    window=window_to_check;

    while ((XTranslateCoordinates(display,top_window,window_to_check,
        *x,*y,&newx,&newy,&window) != 0) &&
           (window != (Window) NULL))
    {
        if (window != (Window) NULL)
        {
            top_window=window_to_check;
            window_to_check=window;
            (*x)=newx;
            (*y)=newy;
        }
    }

    if (window == (Window) NULL)
        window=window_to_check;


    (*x)=newx;
    (*y)=newy;

    return (window);
}

/*
 * get the color of the pixel of the point of mouse click
 * output paramter: XColor *color
 *
 * returns True if succeeds
 *          False if fails
 */

static int getWindowColor(Display *display,XColor *color)
{
    Window
        root_window,
        target_window;

    XImage
        *ximage;

    int
        x,
        y;

    Status
        status;

    root_window=XRootWindow(display,XDefaultScreen(display));
    target_window=selectWindow(display,&x,&y);
    
    if (target_window == (Window) NULL)
        return (False);

    ximage=XGetImage(display,target_window,x,y,1,1,AllPlanes,ZPixmap);
    if (ximage == (XImage *) NULL)
        return (False);

    color->pixel=XGetPixel(ximage,0,0);
    XDestroyImage(ximage);

    return (True);
}

/* forgiving X error handler */

static int MXError (Display *display, XErrorEvent *error)
{
    int
        xerrcode;
 
    xerrcode = error->error_code;
 
    if (xerrcode == BadAlloc || 
        (xerrcode == BadAccess && error->request_code==88)) 
    {
        return (False);
    }
    else
    {
        switch (error->request_code)
        {
            case X_GetGeometry:
            {
                if (error->error_code == BadDrawable)
                    return (False);
                break;
            }

            case X_GetWindowAttributes:
            case X_QueryTree:
            {
                if (error->error_code == BadWindow)
                    return (False);
                break;
            }

            case X_QueryColors:
            {
                if (error->error_code == BadValue)
                    return(False);
                break;
            }
        }
    }
    return (True);
}

