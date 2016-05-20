/*******************************************************************************
*                                                                              *
* server.c -- Nirvana Editor edit-server component                             *
*                                                                              *
* Copyright (C) 1999 Mark Edel                                                 *
*                                                                              *
* This is free software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version. In addition, you may distribute version of this program linked to   *
* Motif or Open Motif. See README for details.                                 *
*                                                                              *
* This software is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
* for more details.                                                            *
*                                                                              *
* You should have received a copy of the GNU General Public License along with *
* software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
* Place, Suite 330, Boston, MA  02111-1307 USA                                 *
*                                                                              *
* Nirvana Text Editor                                                          *
* November, 1995                                                               *
*                                                                              *
* Written by Mark Edel                                                         *
*                                                                              *
*******************************************************************************/

#include <QApplication>

#include "server.h"
#include "TextBuffer.h"
#include "nedit.h"
#include "window.h"
#include "file.h"
#include "selection.h"
#include "Document.h"
#include "macro.h"
#include "menu.h"
#include "preferences.h"
#include "server_common.h"
#include "../util/fileUtils.h"
#include "../util/utils.h"
#include "../util/misc.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>

#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include <unistd.h>
#include <pwd.h>

static void processServerCommand(void);
static void cleanUpServerCommunication(void);
static void processServerCommandString(char *string);
static void getFileClosedProperty(Document *window);
static int isLocatedOnDesktop(Document *window, long currentDesktop);
static Document *findWindowOnDesktop(int tabbed, long currentDesktop);

static Atom ServerRequestAtom = 0;
static Atom ServerExistsAtom = 0;

/*
** Set up inter-client communication for NEdit server end, expected to be
** called only once at startup time
*/
void InitServerCommunication(void) {
	Window rootWindow = RootWindow(TheDisplay, DefaultScreen(TheDisplay));

	// Create the server property atoms on the current DISPLAY. 
	CreateServerPropertyAtoms(GetPrefServerName(), &ServerExistsAtom, &ServerRequestAtom);

	/* Pay attention to PropertyChangeNotify events on the root window.
	   Do this before putting up the server atoms, to avoid a race
	   condition (when nc sees that the server exists, it sends a command,
	   so we must make sure that we already monitor properties). */
	XSelectInput(TheDisplay, rootWindow, PropertyChangeMask);

	/* Create the server-exists property on the root window to tell clients
	   whether to try a request (otherwise clients would always have to
	   try and wait for their timeouts to expire) */
	XChangeProperty(TheDisplay, rootWindow, ServerExistsAtom, XA_STRING, 8, PropModeReplace, (unsigned char *)"True", 4);

	// Set up exit handler for cleaning up server-exists property 
	atexit(cleanUpServerCommunication);
}

static void deleteProperty(Atom *atom) {
	if (!IsServer) {
		*atom = None;
		return;
	}

	if (*atom != None) {
		XDeleteProperty(TheDisplay, RootWindow(TheDisplay, DefaultScreen(TheDisplay)), *atom);
		*atom = None;
	}
}

/*
** Exit handler.  Removes server-exists property on root window
*/
static void cleanUpServerCommunication(void) {

	/* Delete any per-file properties that still exist
	 * (and that server knows about)
	 */
	for(Document *w: WindowList) {
		DeleteFileClosedProperty(w);
	}

	/* Delete any per-file properties that still exist
	 * (but that that server doesn't know about)
	 */
	DeleteServerFileAtoms(GetPrefServerName(), RootWindow(TheDisplay, DefaultScreen(TheDisplay)));

	/* Delete the server-exists property from the root window (if it was
	   assigned) and don't let the process exit until the X server has
	   processed the delete request (otherwise it won't be done) */
	deleteProperty(&ServerExistsAtom);
	XSync(TheDisplay, False);
}

/*
** Special event loop for NEdit servers.  Processes PropertyNotify events on
** the root window (this would not be necessary if it were possible to
** register an Xt event-handler for a window, rather than only a widget).
** Invokes server routines when a server-request property appears,
** re-establishes server-exists property when another server exits and
** this server is still alive to take over.
*/
void ServerMainLoop(XtAppContext context) {
	while (TRUE) {
		XEvent event;
		XtAppNextEvent(context, &event);
		ServerDispatchEvent(&event);
	}
}

static void processServerCommand(void) {
	Atom dummyAtom;
	unsigned long nItems, dummyULong;
	unsigned char *propValue;
	int getFmt;

	// Get the value of the property, and delete it from the root window 
	if (XGetWindowProperty(TheDisplay, RootWindow(TheDisplay, DefaultScreen(TheDisplay)), ServerRequestAtom, 0, INT_MAX, True, XA_STRING, &dummyAtom, &getFmt, &nItems, &dummyULong, &propValue) != Success || getFmt != 8)
		return;

	// Invoke the command line processor on the string to process the request 
	processServerCommandString((char *)propValue);
	XFree(propValue);
}

Boolean ServerDispatchEvent(XEvent *event) {
	if (IsServer) {
		Window rootWindow = RootWindow(TheDisplay, DefaultScreen(TheDisplay));
		if (event->xany.window == rootWindow && event->xany.type == PropertyNotify) {
			const XPropertyEvent *e = &event->xproperty;

			if (e->type == PropertyNotify && e->window == rootWindow) {
				if (e->atom == ServerRequestAtom && e->state == PropertyNewValue)
					processServerCommand();
				else if (e->atom == ServerExistsAtom && e->state == PropertyDelete)
					XChangeProperty(TheDisplay, rootWindow, ServerExistsAtom, XA_STRING, 8, PropModeReplace, (unsigned char *)"True", 4);
			}
		}
	}
	return XtDispatchEvent(event);
}

// Try to find existing 'FileOpen' property atom for path. 
static Atom findFileOpenProperty(const char *filename, const char *pathname) {

	if (!IsServer) {
		return (None);
	}

	QString path = QString(QLatin1String("%1%2")).arg(QLatin1String(pathname)).arg(QLatin1String(filename));
	
	return CreateServerFileOpenAtom(GetPrefServerName(), path.toLatin1().data());
}

/* Destroy the 'FileOpen' atom to inform nc that this file has
** been opened.
*/
static void deleteFileOpenProperty(Document *window) {
	if (window->filenameSet_) {
		Atom atom = findFileOpenProperty(window->filename_.toLatin1().data(), window->path_.toLatin1().data());
		deleteProperty(&atom);
	}
}

static void deleteFileOpenProperty2(const char *filename, const char *pathname) {
	Atom atom = findFileOpenProperty(filename, pathname);
	deleteProperty(&atom);
}

// Try to find existing 'FileClosed' property atom for path. 
static Atom findFileClosedProperty(const char *filename, const char *pathname) {

	if (!IsServer) {
		return (None);
	}

	QString path = QString(QLatin1String("%1%2")).arg(QLatin1String(pathname)).arg(QLatin1String(filename));

	return CreateServerFileClosedAtom(GetPrefServerName(), path.toLatin1().data(), True); // don't create 
}

// Get hold of the property to use when closing the file. 
static void getFileClosedProperty(Document *window) {
	if (window->filenameSet_) {
		window->fileClosedAtom_ = findFileClosedProperty(window->filename_.toLatin1().data(), window->path_.toLatin1().data());
	}
}

/* Delete the 'FileClosed' atom to inform nc that this file has
** been closed.
*/
void DeleteFileClosedProperty(Document *window) {
	if (window->filenameSet_) {
		deleteProperty(&window->fileClosedAtom_);
	}
}

static void deleteFileClosedProperty2(const char *filename, const char *pathname) {
	Atom atom = findFileClosedProperty(filename, pathname);
	deleteProperty(&atom);
}

static int isLocatedOnDesktop(Document *window, long currentDesktop) {
	long windowDesktop;
	if (currentDesktop == -1)
		return True; // No desktop information available 

	windowDesktop = QueryDesktop(TheDisplay, window->shell_);
	// Sticky windows have desktop 0xFFFFFFFF by convention 
	if (windowDesktop == currentDesktop || windowDesktop == 0xFFFFFFFFL)
		return True; // Desktop matches, or window is sticky 

	return False;
}

static Document *findWindowOnDesktop(int tabbed, long currentDesktop) {

	if (tabbed == 0 || (tabbed == -1 && GetPrefOpenInTab() == 0)) {
		/* A new window is requested, unless we find an untitled unmodified
		    document on the current desktop */
		for (Document *window: WindowList) {
			if (window->filenameSet_ || window->fileChanged_ || window->macroCmdData_) {
				continue;
			}
			// No check for top document here! 
			if (isLocatedOnDesktop(window, currentDesktop)) {
				return window;
			}
		}
	} else {
		// Find a window on the current desktop to hold the new document 
		for (Document *window: WindowList) {
			// Avoid unnecessary property access (server round-trip) 
			if (!window->IsTopDocument()) {
				continue;
			}
			if (isLocatedOnDesktop(window, currentDesktop)) {
				return window;
			}
		}
	}

	return nullptr; // No window found on current desktop -> create new window 
}

static void processServerCommandString(char *string) {
	char *fullname, filename[MAXPATHLEN], pathname[MAXPATHLEN];
	char *doCommand, *geometry, *langMode, *inPtr;
	int editFlags, stringLen = strlen(string);
	int lineNum, createFlag, readFlag, iconicFlag, lastIconic = 0, tabbed = -1;
	int fileLen, doLen, lmLen, geomLen, charsRead, itemsRead;
	Document *lastFile = nullptr;
	long currentDesktop = QueryCurrentDesktop(TheDisplay, RootWindow(TheDisplay, DefaultScreen(TheDisplay)));

	/* If the command string is empty, put up an empty, Untitled window
	   (or just pop one up if it already exists) */
	if (string[0] == '\0') {

		auto it = std::find_if(WindowList.begin(), WindowList.end(), [currentDesktop](Document *w) {
			return (!w->filenameSet_ && !w->fileChanged_ && isLocatedOnDesktop(w, currentDesktop));
		});

		if(it == WindowList.end()) {
			EditNewFile(findWindowOnDesktop(tabbed, currentDesktop), nullptr, False, nullptr, nullptr);
			CheckCloseDim();
		} else {
			(*it)->RaiseDocument();
			WmClientMsg(TheDisplay, XtWindow((*it)->shell_), "_NET_ACTIVE_WINDOW", 0, 0, 0, 0, 0);
			XMapRaised(TheDisplay, XtWindow((*it)->shell_));
		}
		return;
	}

	/*
	** Loop over all of the files in the command list
	*/
	inPtr = string;
	while (true) {

		if (*inPtr == '\0')
			break;

		/* Read a server command from the input string.  Header contains:
		   linenum createFlag fileLen doLen\n, followed by a filename and -do
		   command both followed by newlines.  This bit of code reads the
		   header, and converts the newlines following the filename and do
		   command to nulls to terminate the filename and doCommand strings */
		itemsRead = sscanf(inPtr, "%d %d %d %d %d %d %d %d %d%n", &lineNum, &readFlag, &createFlag, &iconicFlag, &tabbed, &fileLen, &doLen, &lmLen, &geomLen, &charsRead);
		if (itemsRead != 9)
			goto readError;
		inPtr += charsRead + 1;
		if (inPtr - string + fileLen > stringLen)
			goto readError;
		fullname = inPtr;
		inPtr += fileLen;
		*inPtr++ = '\0';
		if (inPtr - string + doLen > stringLen)
			goto readError;
		doCommand = inPtr;
		inPtr += doLen;
		*inPtr++ = '\0';
		if (inPtr - string + lmLen > stringLen)
			goto readError;
		langMode = inPtr;
		inPtr += lmLen;
		*inPtr++ = '\0';
		if (inPtr - string + geomLen > stringLen)
			goto readError;
		geometry = inPtr;
		inPtr += geomLen;
		*inPtr++ = '\0';

		/* An empty file name means:
		 *   put up an empty, Untitled window, or use an existing one
		 *   choose a random window for executing the -do macro upon
		 */
		if (fileLen <= 0) {
			
			
			auto it = std::find_if(WindowList.begin(), WindowList.end(), [currentDesktop](Document *w) {
				return (!w->filenameSet_ && !w->fileChanged_ && isLocatedOnDesktop(w, currentDesktop));
			});			
			
		
			if (*doCommand == '\0') {
				if(it == WindowList.end()) {
					EditNewFile(findWindowOnDesktop(tabbed, currentDesktop), nullptr, iconicFlag, lmLen == 0 ? nullptr : langMode, nullptr);
				} else {
					if (iconicFlag) {
						(*it)->RaiseDocument();
					} else {
						(*it)->RaiseDocumentWindow();
					}
				}
			} else {
				
				/* Starting a new command while another one is still running
				   in the same window is not possible (crashes). */
				   
				   
				// TODO(eteran): I *think* this searches for the first window
				// where win->macroCmdData_ is nullptr				
				auto win = WindowList.begin();
				while (win != WindowList.end() && (*win)->macroCmdData_) {
					++win;
				}

				if (win == WindowList.end()) {
					QApplication::beep();
				} else {
					// Raise before -do (macro could close window). 
					if (iconicFlag) {
						(*win)->RaiseDocument();
					} else {
						(*win)->RaiseDocumentWindow();
					}
					DoMacro(*win, doCommand, "-do macro");
				}
			}
			CheckCloseDim();
			return;
		}

		/* Process the filename by looking for the files in an
		   existing window, or opening if they don't exist */
		editFlags = (readFlag ? PREF_READ_ONLY : 0) | CREATE | (createFlag ? SUPPRESS_CREATE_WARN : 0);
		if (ParseFilename(fullname, filename, pathname) != 0) {
			fprintf(stderr, "NEdit: invalid file name\n");
			deleteFileClosedProperty2(filename, pathname);
			break;
		}

		Document *window = Document::FindWindowWithFile(QLatin1String(filename), QLatin1String(pathname));
		if(!window) {
			/* Files are opened in background to improve opening speed
			   by defering certain time  consuiming task such as syntax
			   highlighting. At the end of the file-opening loop, the
			   last file opened will be raised to restore those deferred
			   items. The current file may also be raised if there're
			   macros to execute on. */
			window = EditExistingFile(findWindowOnDesktop(tabbed, currentDesktop), QLatin1String(filename), QLatin1String(pathname), editFlags, geometry, iconicFlag, lmLen == 0 ? nullptr : langMode, tabbed == -1 ? GetPrefOpenInTab() : tabbed, True);

			if (window) {
				window->CleanUpTabBarExposeQueue();
				if (lastFile && window->shell_ != lastFile->shell_) {
					lastFile->CleanUpTabBarExposeQueue();
					lastFile->RaiseDocument();
				}
			}
		}

		/* Do the actions requested (note DoMacro is last, since the do
		   command can do anything, including closing the window!) */
		if (window) {
			deleteFileOpenProperty(window);
			getFileClosedProperty(window);

			if (lineNum > 0)
				SelectNumberedLine(window, lineNum);

			if (*doCommand != '\0') {
				window->RaiseDocument();

				if (!iconicFlag) {
					WmClientMsg(TheDisplay, XtWindow(window->shell_), "_NET_ACTIVE_WINDOW", 0, 0, 0, 0, 0);
					XMapRaised(TheDisplay, XtWindow(window->shell_));
				}

				/* Starting a new command while another one is still running
				   in the same window is not possible (crashes). */
				if (window->macroCmdData_) {
					QApplication::beep();
				} else {
					DoMacro(window, doCommand, "-do macro");
					/* in case window is closed by macro functions
					   such as close() or detach_document() */
					if (!window->IsValidWindow())
						window = nullptr;
					if (lastFile && !lastFile->IsValidWindow())
						lastFile = nullptr;
				}
			}

			// register the last file opened for later use 
			if (window) {
				lastFile = window;
				lastIconic = iconicFlag;
			}
		} else {
			deleteFileOpenProperty2(filename, pathname);
			deleteFileClosedProperty2(filename, pathname);
		}
	}

	// Raise the last file opened 
	if (lastFile) {
		lastFile->CleanUpTabBarExposeQueue();
		if (lastIconic) {
			lastFile->RaiseDocument();
		} else {
			lastFile->RaiseDocumentWindow();
		}
		CheckCloseDim();
	}
	return;

readError:
	fprintf(stderr, "NEdit: error processing server request\n");
	return;
}
