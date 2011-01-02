#
# Routingfor navi
#

#/****************************************************************
# * This file is distributed under the following license:
# *
# * Copyright (C) 2010, Bernd Stramm
# *
# *  This program is free software; you can redistribute it and/or
# *  modify it under the terms of the GNU General Public License
# *  as published by the Free Software Foundation; either version 2
# *  of the License, or (at your option) any later version.
# *
# *  This program is distributed in the hope that it will be useful,
# *  but WITHOUT ANY WARRANTY; without even the implied warranty of
# *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# *  GNU General Public License for more details.
# *
# *  You should have received a copy of the GNU General Public License
# *  along with this program; if not, write to the Free Software
# *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
# *  Boston, MA  02110-1301, USA.
# ****************************************************************/

MYNAME = asroute

TEMPLATE = app

QT += core gui sql webkit network xml 
CONFIG += debug_and_release

MAKEFILE = Make_$${MYNAME}
!include ("options.pri") {
  message ("no options.pri, using defaults")
  EXTRA_INCLUDE += /usr/include
  SQLRUN_LIBDIR = /usr/lib64
} else {
  EXTRA_INCLUDE += $${SQLRUN}/include
  SQLRUN_LIBDIR = $${SQLRUN}/lib$${BITS_SUFFIX}
}

INCLUDEPATH += $$EXTRA_INCLUDE
LIBS += -L$$SQLRUN_LIBDIR -lsqlrunner

CONFIG(debug, debug|release) {
  DEFINES += DELIBERATE_DEBUG=1
  TARGET = bin/$${MYNAME}_d
  OBJECTS_DIR = tmp/debug/obj
  message ("DEBUG cxx-flags used $${QMAKE_CXXFLAGS_DEBUG}")
  message ("DEBUG c-flags used $${QMAKE_CFLAGS_DEBUG}")
} else {
  DEFINES += DELIBERATE_DEBUG=0
  TARGET = bin/$${MYNAME}
  OBJECTS_DIR = tmp/release/obj
  QMAKE_CXXFLAGS_RELEASE -= -g
  QMAKE_CFLAGS_RELEASE -= -g
  message ("RELEASE cxx-flags used $${QMAKE_CXXFLAGS_RELEASE}")
  message ("RELEASE c-flags used $${QMAKE_CFLAGS_RELEASE}")
}

INCLUDEPATH += .
INCLUDEPATH += src/


UI_DIR = tmp/ui
MOC_DIR = tmp/moc
RCC_DIR = tmp/rcc
RESOURCES = navi.qrc

FORMS = \
        ui/$${MYNAME}.ui \
        ui/DebugLog.ui \
        ui/config-edit.ui \
        ui/helpwin.ui \  
        ui/map-display.ui
        

HEADERS = \
          src/$${MYNAME}.h \
          src/gpl2.h \
          src/cmdoptions.h \
          src/config-edit.h \
          src/delib-debug.h \
          src/deliberate.h \
          src/version.h \
          src/helpview.h \
          src/as-db-manager.h \
          src/navi-global.h \
          src/route-cell-menus.h \
          src/map-display.h \
          src/move-button.h \


SOURCES = \
          src/$${MYNAME}.cpp \
          src/asroute-main.cpp \
          src/cmdoptions.cpp \
          src/config-edit.cpp \
          src/delib-debug.cpp \
          src/deliberate.cpp \
          src/version.cpp \
          src/helpview.cpp \
          src/as-db-manager.cpp \
          src/navi-global.cpp \
          src/route-cell-menus.cpp \
          src/map-display.cpp \
          src/move-button.cpp \

