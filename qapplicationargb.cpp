/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>
    Copyright (C) 2007 Enrico Ros <enrico.ros@gmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "qapplicationargb.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

QApplicationArgb::QApplicationArgb(int & argc, char ** args)
    : QApplication(display(), argc, args, visual(), colormap())
{
}

QApplicationArgb::~QApplicationArgb()
{
}

void QApplicationArgb::init()
{
    static bool alreadyCalled = false;
    if (alreadyCalled) {
        return;
    }
    alreadyCalled = true;

    bool argbVisual = false;
    Display *dpy = XOpenDisplay(0); // open default display
    if (!dpy) {
        qWarning("Cannot connect to the X server");
        exit(1);
    }

    const int screen = DefaultScreen(dpy);
    Colormap colormap = 0;
    Visual *visual = 0;
    int eventBase, errorBase;

    if (XRenderQueryExtension(dpy, &eventBase, &errorBase)) {
        int nvi;
        XVisualInfo templ;
        templ.screen = screen;
        templ.depth = 32;
        templ.c_class = TrueColor;
        XVisualInfo *xvi = XGetVisualInfo(dpy, VisualScreenMask | VisualDepthMask | VisualClassMask,
                &templ, &nvi);

        for (int i = 0; i < nvi; ++i) {
            XRenderPictFormat *format = XRenderFindVisualFormat(dpy, xvi[i].visual);
            if (format->type == PictTypeDirect && format->direct.alphaMask) {
                visual = xvi[i].visual;
                colormap = XCreateColormap(dpy, RootWindow(dpy, screen), visual, AllocNone);
                argbVisual = true;
                break;
            }
        }
    }
    if (argbVisual) {
        m_display = dpy;
        m_visual = Qt::HANDLE(visual);
        m_colormap = Qt::HANDLE(colormap);
    } else {
        m_display = 0;
        m_visual = 0;
        m_colormap = 0;
        XCloseDisplay(dpy);
    }
}
