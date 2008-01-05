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

#ifndef __QApplicationArgb_h__
#define __QApplicationArgb_h__

#include <QtGui/QApplication>

class QApplicationArgb : public QApplication
{
    public:
        QApplicationArgb(int & argc, char ** argv);
        ~QApplicationArgb();

    private:
        void init();
        inline Display *display() { init(); return m_display; }
        inline Qt::HANDLE visual() { init(); return m_visual; }
        inline Qt::HANDLE colormap() { init(); return m_colormap; }
        Display *m_display;
        Qt::HANDLE m_visual;
        Qt::HANDLE m_colormap;
};

#endif // __QApplicationArgb_h__
