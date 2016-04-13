/*******************************************************************************
*                                                                              *
* printUtils.c -- Nirvana library Printer Menu        & Printing Routines      *
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
*                                                                              *
* April 20, 1992                                                               *
*                                                                              *
* Written by Arnulfo Zepeda-Navratil                                           *
*            Centro de Investigacion y Estudio Avanzados ( CINVESTAV )         *
*            Dept. Fisica - Mexico                                             *
*            BITNET: ZEPEDA@CINVESMX                                           *
*                                                                              *
* Modified by Donna Reid and Joy Kyriakopulos 4/8/93 - VMS port                *
*                                                                              *
*******************************************************************************/

#include <QMessageBox>

#include "printUtils.h"
#include "misc.h"
#include "prefFile.h"
#include "MotifHelper.h"

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>

/* Separator between directory references in PATH environmental variable */
#define SEPARATOR ':'

/* Number of extra pixels down to place a label even with a text widget */
#define LABEL_TEXT_DIFF 6

/* Maximum text string lengths */
#define MAX_OPT_STR   20u
#define MAX_QUEUE_STR 60u
#define MAX_INT_STR   13u
#define MAX_HOST_STR  100u
#define MAX_PCMD_STR  100u
#define MAX_NAME_STR  100u
#define MAX_CMD_STR   256u

#define N_PRINT_PREFS 7 /* must agree with number of preferences below */


#define PRINT_COMMAND_INDEX 0
#define COPIES_OPTION_INDEX 1
#define QUEUE_OPTION_INDEX  2
#define NAME_OPTION_INDEX   3
#define HOST_OPTION_INDEX   4
#define DEFAULT_QUEUE_INDEX 5
#define DEFAULT_HOST_INDEX  6

/* Maximum length of an error returned by IssuePrintCommand() */
#define MAX_PRINT_ERROR_LENGTH 1024


/* Function Prototypes */
static Widget createForm(Widget parent);
static void allowOnlyNumInput(Widget widget, XtPointer client_data, XtPointer call_data);
static void noSpaceOrPunct(Widget widget, XtPointer client_data, XtPointer call_data);
static void updatePrintCmd(Widget w, XtPointer client_data, XtPointer call_data);
static void printCmdModified(Widget w, XtPointer client_data, XtPointer call_data);
static void printButtonCB(Widget widget, XtPointer client_data, XtPointer call_data);
static void cancelButtonCB(Widget widget, XtPointer client_data, XtPointer call_data);
static void setQueueLabelText(void);
static int fileInDir(const char *filename, const char *dirpath, unsigned short mode_flags);
static int fileInPath(const char *filename, unsigned short mode_flags);
static int flprPresent(void);
static void getLprQueueDefault(char *defqueue);
static void setHostLabelText(void);

static void getFlprHostDefault(char *defhost);
static void getFlprQueueDefault(char *defqueue);

/* Module Global Variables */
static bool DoneWithDialog;
static bool PreferencesLoaded = false;
static Widget Form;
static Widget Label2;
static Widget Label3;
static Widget Text1;
static Widget Text2;
static Widget Text3;
static Widget Text4;
static std::string PrintFileName;
static std::string PrintJobName;
static char PrintCommand[MAX_PCMD_STR];  /* print command string */
static char CopiesOption[MAX_OPT_STR];   /* # of copies argument string */
static char QueueOption[MAX_OPT_STR];    /* queue name argument string */
static char NameOption[MAX_OPT_STR];     /* print job name argument string */
static char HostOption[MAX_OPT_STR];     /* host name argument string */
static char DefaultQueue[MAX_QUEUE_STR]; /* default print queue */
static char DefaultHost[MAX_HOST_STR];   /* default host name */
static char Copies[MAX_INT_STR] = "";    /* # of copies last entered by user */
static char Queue[MAX_QUEUE_STR] = "";   /* queue name last entered by user */
static char Host[MAX_HOST_STR] = "";     /* host name last entered by user */
static char CmdText[MAX_CMD_STR] = "";   /* print command last entered by user */
static bool CmdFieldModified = false;    /* user last changed the print command field, so don't trust the rest */
						
static PrefDescripRec PrintPrefDescrip[N_PRINT_PREFS] = {
    {"printCommand",      "PrintCommand",      PREF_STRING, nullptr, PrintCommand, MAX_PCMD_STR,  false},
    {"printCopiesOption", "PrintCopiesOption", PREF_STRING, nullptr, CopiesOption, MAX_OPT_STR,   false},
    {"printQueueOption",  "PrintQueueOption",  PREF_STRING, nullptr, QueueOption,  MAX_OPT_STR,   false},
    {"printNameOption",   "PrintNameOption",   PREF_STRING, nullptr, NameOption,   MAX_OPT_STR,   false},
    {"printHostOption",   "PrintHostOption",   PREF_STRING, nullptr, HostOption,   MAX_OPT_STR,   false},
    {"printDefaultQueue", "PrintDefaultQueue", PREF_STRING, nullptr, DefaultQueue, MAX_QUEUE_STR, false},
    {"printDefaultHost",  "PrintDefaultHost",  PREF_STRING, nullptr, DefaultHost,  MAX_HOST_STR,  false},
};

/*
** PrintFile(Widget parent, char *printFile, char *jobName);
**
** function to put up an application-modal style Print Panel dialog
** box.
**
**	parent		Parent widget for displaying dialog
**	printFile	File to print (assumed to be a temporary file
**			and not revealed to the user)
**	jobName		Title for the print banner page
*/
void PrintFile(Widget parent, const std::string &printFile, const std::string &jobName) {

	/* In case the program hasn't called LoadPrintPreferences, set up the
	   default values for the print preferences */
	if (!PreferencesLoaded)
		LoadPrintPreferencesEx(nullptr, "", "", true);

	/* Make the PrintFile information available to the callback routines */
	PrintFileName = printFile;
	PrintJobName = jobName;

	/* Create and display the print dialog */
	DoneWithDialog = false;
	Form = createForm(parent);
	ManageDialogCenteredOnPointer(Form);

	/* Process events until the user is done with the print dialog */
	while (!DoneWithDialog) {
		XtAppProcessEvent(XtWidgetToApplicationContext(Form), XtIMAll);
	}

	/* Destroy the dialog.  Print dialogs are not preserved across calls
	   to PrintFile so that it may be called with different parents and
	   to generally simplify the call (this, of course, makes it slower) */
	XtDestroyWidget(Form);
}

/*
** LoadPrintPreferences
**
** Read an X database to obtain print dialog preferences.
**
**	prefDB		X database potentially containing print preferences
**	appName		Application name which can be used to qualify
**			resource names for database lookup.
**	appClass	Application class which can be used to qualify
**			resource names for database lookup.
**	lookForFlpr	Check if the flpr print command is installed
**			and use that for the default if it's found.
**			(flpr is a Fermilab utility for printing on
**			arbitrary systems that support the lpr protocol)
*/
void LoadPrintPreferencesEx(XrmDatabase prefDB, const std::string &appName, const std::string &appClass, bool lookForFlpr) {
	static char defaultQueue[MAX_QUEUE_STR], defaultHost[MAX_HOST_STR];

	/* check if flpr is installed, and otherwise choose appropriate
	   printer per system type */
	if (lookForFlpr && flprPresent()) {
		getFlprQueueDefault(defaultQueue);
		getFlprHostDefault(defaultHost);
		PrintPrefDescrip[PRINT_COMMAND_INDEX].defaultString = "flpr";
		PrintPrefDescrip[COPIES_OPTION_INDEX].defaultString = "";
		PrintPrefDescrip[QUEUE_OPTION_INDEX].defaultString  = "-q";
		PrintPrefDescrip[NAME_OPTION_INDEX].defaultString   = "-j ";
		PrintPrefDescrip[HOST_OPTION_INDEX].defaultString   = "-h";
		PrintPrefDescrip[DEFAULT_QUEUE_INDEX].defaultString = defaultQueue;
		PrintPrefDescrip[DEFAULT_HOST_INDEX].defaultString  = defaultHost;
	} else {
		getLprQueueDefault(defaultQueue);
		PrintPrefDescrip[PRINT_COMMAND_INDEX].defaultString = "lpr";
		PrintPrefDescrip[COPIES_OPTION_INDEX].defaultString = "-# ";
		PrintPrefDescrip[QUEUE_OPTION_INDEX].defaultString  = "-P ";
		PrintPrefDescrip[NAME_OPTION_INDEX].defaultString   = "-J ";
		PrintPrefDescrip[HOST_OPTION_INDEX].defaultString   = "";
		PrintPrefDescrip[DEFAULT_QUEUE_INDEX].defaultString = defaultQueue;
		PrintPrefDescrip[DEFAULT_HOST_INDEX].defaultString  = "";
	}

	/* Read in the preferences from the X database using the mechanism from
	   prefFile.c (this allows LoadPrintPreferences to work before any
	   widgets are created, which is more convenient than XtGetApplication-
	   Resources for applications which have no main window) */
	RestorePreferences(nullptr, prefDB, appName, appClass, PrintPrefDescrip, N_PRINT_PREFS);

	PreferencesLoaded = true;
}

static Widget createForm(Widget parent) {
	Widget form, printOk, printCancel, label1, separator;
	Widget topWidget = nullptr;
	XmString st0;
	Arg args[65];
	int argcnt;
	Widget bwidgetarray[30];
	int bwidgetcnt = 0;

	/************************ FORM ***************************/
	argcnt = 0;
	XtSetArg(args[argcnt], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
	argcnt++;
	XtSetArg(args[argcnt], XmNdialogTitle, (st0 = XmStringCreateLtoREx("Print", XmSTRING_DEFAULT_CHARSET)));
	argcnt++;
	XtSetArg(args[argcnt], XmNautoUnmanage, False);
	argcnt++;
	form = CreateFormDialog(parent, "printForm", args, argcnt);
	XtVaSetValues(form, XmNshadowThickness, 0, nullptr);

	XmStringFree(st0);

	/*********************** LABEL 1 and TEXT BOX 1 *********************/
	if (CopiesOption[0] != '\0') {
		argcnt = 0;
		XtSetArg(args[argcnt], XmNlabelString, (st0 = XmStringCreateLtoREx("Number of copies (1)", XmSTRING_DEFAULT_CHARSET)));
		argcnt++;
		XtSetArg(args[argcnt], XmNmnemonic, 'N');
		argcnt++;
		XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM);
		argcnt++;
		XtSetArg(args[argcnt], XmNtopOffset, LABEL_TEXT_DIFF + 5);
		argcnt++;
		XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM);
		argcnt++;
		XtSetArg(args[argcnt], XmNleftOffset, 8);
		argcnt++;
		label1 = XmCreateLabelGadget(form, (char *)"label1", args, argcnt);
		XmStringFree(st0);
		bwidgetarray[bwidgetcnt] = label1;
		bwidgetcnt++;

		argcnt = 0;
		XtSetArg(args[argcnt], XmNshadowThickness, (short)2);
		argcnt++;
		XtSetArg(args[argcnt], XmNcolumns, 3);
		argcnt++;
		XtSetArg(args[argcnt], XmNrows, 1);
		argcnt++;
		XtSetArg(args[argcnt], XmNvalue, Copies);
		argcnt++;
		XtSetArg(args[argcnt], XmNmaxLength, 3);
		argcnt++;
		XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM);
		argcnt++;
		XtSetArg(args[argcnt], XmNtopOffset, 5);
		argcnt++;
		XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET);
		argcnt++;
		XtSetArg(args[argcnt], XmNleftWidget, label1);
		argcnt++;
		Text1 = XmCreateText(form, (char *)"text1", args, argcnt);
		bwidgetarray[bwidgetcnt] = Text1;
		bwidgetcnt++;
		XtAddCallback(Text1, XmNmodifyVerifyCallback, allowOnlyNumInput, nullptr);
		XtAddCallback(Text1, XmNvalueChangedCallback, updatePrintCmd, nullptr);
		RemapDeleteKey(Text1);
		topWidget = Text1;
		XtVaSetValues(label1, XmNuserData, Text1, nullptr); /* mnemonic procesing */
	}

	/************************ LABEL 2 and TEXT 2 ************************/
	if (QueueOption[0] != '\0') {
		argcnt = 0;
		XtSetArg(args[argcnt], XmNlabelString, (st0 = XmStringCreateLtoREx("  ", XmSTRING_DEFAULT_CHARSET)));
		argcnt++;
		XtSetArg(args[argcnt], XmNmnemonic, 'Q');
		argcnt++;
		XtSetArg(args[argcnt], XmNrecomputeSize, True);
		argcnt++;
		XtSetArg(args[argcnt], XmNtopAttachment, topWidget == nullptr ? XmATTACH_FORM : XmATTACH_WIDGET);
		argcnt++;
		XtSetArg(args[argcnt], XmNtopWidget, topWidget);
		argcnt++;
		XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM);
		argcnt++;
		XtSetArg(args[argcnt], XmNtopOffset, LABEL_TEXT_DIFF + 4);
		argcnt++;
		XtSetArg(args[argcnt], XmNleftOffset, 8);
		argcnt++;
		Label2 = XmCreateLabelGadget(form, (char *)"label2", args, argcnt);
		XmStringFree(st0);
		bwidgetarray[bwidgetcnt] = Label2;
		bwidgetcnt++;
		setQueueLabelText();

		argcnt = 0;
		XtSetArg(args[argcnt], XmNshadowThickness, (short)2);
		argcnt++;
		XtSetArg(args[argcnt], XmNcolumns, (short)17);
		argcnt++;
		XtSetArg(args[argcnt], XmNmaxLength, MAX_QUEUE_STR);
		argcnt++;
		XtSetArg(args[argcnt], XmNvalue, Queue);
		argcnt++;
		XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM);
		argcnt++;
		XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET);
		argcnt++;
		XtSetArg(args[argcnt], XmNleftWidget, Label2);
		argcnt++;
		XtSetArg(args[argcnt], XmNtopAttachment, topWidget == nullptr ? XmATTACH_FORM : XmATTACH_WIDGET);
		argcnt++;
		XtSetArg(args[argcnt], XmNtopWidget, topWidget);
		argcnt++;
		XtSetArg(args[argcnt], XmNrightOffset, 8);
		argcnt++;
		XtSetArg(args[argcnt], XmNtopOffset, 4);
		argcnt++;
		Text2 = XmCreateText(form, (char *)"text2", args, argcnt);
		XtAddCallback(Text2, XmNmodifyVerifyCallback, noSpaceOrPunct, nullptr);
		XtAddCallback(Text2, XmNvalueChangedCallback, updatePrintCmd, nullptr);
		bwidgetarray[bwidgetcnt] = Text2;
		bwidgetcnt++;
		RemapDeleteKey(Text2);
		XtVaSetValues(Label2, XmNuserData, Text2, nullptr); /* mnemonic procesing */
		topWidget = Text2;
	}

	/****************** LABEL 3 and TEXT 3 *********************/
	if (HostOption[0] != '\0') {
		argcnt = 0;
		XtSetArg(args[argcnt], XmNlabelString, (st0 = XmStringCreateLtoREx("  ", XmSTRING_DEFAULT_CHARSET)));
		argcnt++;
		XtSetArg(args[argcnt], XmNmnemonic, 'H');
		argcnt++;
		XtSetArg(args[argcnt], XmNrecomputeSize, True);
		argcnt++;
		XtSetArg(args[argcnt], XmNvalue, "");
		argcnt++;
		XtSetArg(args[argcnt], XmNtopAttachment, topWidget == nullptr ? XmATTACH_FORM : XmATTACH_WIDGET);
		argcnt++;
		XtSetArg(args[argcnt], XmNtopWidget, topWidget);
		argcnt++;
		XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM);
		argcnt++;
		XtSetArg(args[argcnt], XmNleftOffset, 8);
		argcnt++;
		XtSetArg(args[argcnt], XmNtopOffset, LABEL_TEXT_DIFF + 4);
		argcnt++;
		Label3 = XmCreateLabelGadget(form, (char *)"label3", args, argcnt);
		XmStringFree(st0);
		bwidgetarray[bwidgetcnt] = Label3;
		bwidgetcnt++;
		setHostLabelText();

		argcnt = 0;
		XtSetArg(args[argcnt], XmNcolumns, 17);
		argcnt++;
		XtSetArg(args[argcnt], XmNrows, 1);
		argcnt++;
		XtSetArg(args[argcnt], XmNvalue, Host);
		argcnt++;
		XtSetArg(args[argcnt], XmNmaxLength, MAX_HOST_STR);
		argcnt++;
		XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM);
		argcnt++;
		XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET);
		argcnt++;
		XtSetArg(args[argcnt], XmNleftWidget, Label3);
		argcnt++;
		XtSetArg(args[argcnt], XmNtopAttachment, topWidget == nullptr ? XmATTACH_FORM : XmATTACH_WIDGET);
		argcnt++;
		XtSetArg(args[argcnt], XmNtopWidget, topWidget);
		argcnt++;
		XtSetArg(args[argcnt], XmNrightOffset, 8);
		argcnt++;
		XtSetArg(args[argcnt], XmNtopOffset, 4);
		argcnt++;
		Text3 = XmCreateText(form, (char *)"Text3", args, argcnt);
		XtAddCallback(Text3, XmNmodifyVerifyCallback, noSpaceOrPunct, nullptr);
		XtAddCallback(Text3, XmNvalueChangedCallback, updatePrintCmd, nullptr);
		bwidgetarray[bwidgetcnt] = Text3;
		bwidgetcnt++;
		RemapDeleteKey(Text3);
		XtVaSetValues(Label3, XmNuserData, Text3, nullptr); /* mnemonic procesing */
		topWidget = Text3;
	}

	/************************** TEXT 4 ***************************/
	argcnt = 0;
	XtSetArg(args[argcnt], XmNvalue, CmdText);
	argcnt++;
	XtSetArg(args[argcnt], XmNcolumns, 50);
	argcnt++;
	XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM);
	argcnt++;
	XtSetArg(args[argcnt], XmNleftOffset, 8);
	argcnt++;
	XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET);
	argcnt++;
	XtSetArg(args[argcnt], XmNtopOffset, 8);
	argcnt++;
	XtSetArg(args[argcnt], XmNtopWidget, topWidget);
	argcnt++;
	XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM);
	argcnt++;
	XtSetArg(args[argcnt], XmNrightOffset, 8);
	argcnt++;
	Text4 = XmCreateText(form, (char *)"Text4", args, argcnt);
	XtAddCallback(Text4, XmNmodifyVerifyCallback, printCmdModified, nullptr);
	bwidgetarray[bwidgetcnt] = Text4;
	bwidgetcnt++;
	RemapDeleteKey(Text4);
	topWidget = Text4;
	if (!CmdFieldModified)
		updatePrintCmd(nullptr, nullptr, nullptr);

	/*********************** SEPARATOR **************************/
	argcnt = 0;
	XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM);
	argcnt++;
	XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET);
	argcnt++;
	XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM);
	argcnt++;
	XtSetArg(args[argcnt], XmNtopOffset, 8);
	argcnt++;
	XtSetArg(args[argcnt], XmNtopWidget, topWidget);
	argcnt++;
	separator = XmCreateSeparatorGadget(form, (char *)"separator", args, argcnt);
	bwidgetarray[bwidgetcnt] = separator;
	bwidgetcnt++;
	topWidget = separator;

	/********************** CANCEL BUTTON *************************/
	argcnt = 0;
	XtSetArg(args[argcnt], XmNlabelString, (st0 = XmStringCreateLtoREx("Cancel", XmSTRING_DEFAULT_CHARSET)));
	argcnt++;
	XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION);
	argcnt++;
	XtSetArg(args[argcnt], XmNleftPosition, 60);
	argcnt++;
	XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET);
	argcnt++;
	XtSetArg(args[argcnt], XmNtopWidget, topWidget);
	argcnt++;
	XtSetArg(args[argcnt], XmNtopOffset, 7);
	argcnt++;
	printCancel = XmCreatePushButton(form, (char *)"printCancel", args, argcnt);
	XmStringFree(st0);
	bwidgetarray[bwidgetcnt] = printCancel;
	bwidgetcnt++;
	XtAddCallback(printCancel, XmNactivateCallback, cancelButtonCB, nullptr);

	/*********************** PRINT BUTTON **************************/
	argcnt = 0;
	XtSetArg(args[argcnt], XmNlabelString, (st0 = XmStringCreateLtoREx("Print", XmSTRING_DEFAULT_CHARSET)));
	argcnt++;
	XtSetArg(args[argcnt], XmNshowAsDefault, True);
	argcnt++;
	XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION);
	argcnt++;
	XtSetArg(args[argcnt], XmNrightPosition, 40);
	argcnt++;
	XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET);
	argcnt++;
	XtSetArg(args[argcnt], XmNtopWidget, topWidget);
	argcnt++;
	XtSetArg(args[argcnt], XmNtopOffset, 7);
	argcnt++;
	printOk = XmCreatePushButton(form, (char *)"printOk", args, argcnt);
	XmStringFree(st0);
	bwidgetarray[bwidgetcnt] = printOk;
	bwidgetcnt++;
	XtAddCallback(printOk, XmNactivateCallback, printButtonCB, nullptr);

	argcnt = 0;
	XtSetArg(args[argcnt], XmNcancelButton, printCancel);
	argcnt++;
	XtSetArg(args[argcnt], XmNdefaultButton, printOk);
	argcnt++;
	XtSetValues(form, args, argcnt);

	XtManageChildren(bwidgetarray, bwidgetcnt);
	AddDialogMnemonicHandler(form, FALSE);
	return form;
}

static void setQueueLabelText(void) {
	Arg args[15];
	int argcnt;
	XmString st0;
	char tmp_buf[MAX_QUEUE_STR + 8];

	if (DefaultQueue[0] != '\0')
		sprintf(tmp_buf, "Queue (%s)", DefaultQueue);
	else
		sprintf(tmp_buf, "Queue");
	argcnt = 0;
	XtSetArg(args[argcnt], XmNlabelString, (st0 = XmStringCreateLtoREx(tmp_buf, XmSTRING_DEFAULT_CHARSET)));
	argcnt++;
	XtSetValues(Label2, args, argcnt);
	XmStringFree(st0);
}

static void setHostLabelText(void) {
	Arg args[15];
	int argcnt;
	XmString st0;
	char tmp_buf[MAX_HOST_STR + 7];

	if (strcmp(DefaultHost, ""))
		sprintf(tmp_buf, "Host (%s)", DefaultHost);
	else
		sprintf(tmp_buf, "Host");
	argcnt = 0;
	XtSetArg(args[argcnt], XmNlabelString, (st0 = XmStringCreateLtoREx(tmp_buf, XmSTRING_DEFAULT_CHARSET)));
	argcnt++;

	XtSetValues(Label3, args, argcnt);
	XmStringFree(st0);
}

static void allowOnlyNumInput(Widget widget, XtPointer client_data, XtPointer callData) {

	(void)widget;
	(void)client_data;
	
	XmTextVerifyCallbackStruct *call_data = (XmTextVerifyCallbackStruct *)callData;
	
	int textInserted, nInserted;

	nInserted = call_data->text->length;
	textInserted = (nInserted > 0);
	if ((call_data->reason == XmCR_MODIFYING_TEXT_VALUE) && textInserted) {
		for (int i = 0; i < nInserted; i++) {
			if (!isdigit((unsigned char)call_data->text->ptr[i])) {
				call_data->doit = False;
				return;
			}
		}
	}
	call_data->doit = True;
}

/*
** Prohibit a relatively random sampling of characters that will cause
** problems on command lines
*/
static void noSpaceOrPunct(Widget widget, XtPointer client_data, XtPointer callData) {

	(void)widget;
	(void)client_data;
	
	XmTextVerifyCallbackStruct *call_data = (XmTextVerifyCallbackStruct *)callData;
	
	int textInserted, nInserted;
	
	nInserted = call_data->text->length;
	textInserted = (nInserted > 0);
	if ((call_data->reason == XmCR_MODIFYING_TEXT_VALUE) && textInserted) {
		for (int i = 0; i < nInserted; i++) {
			
			static const char prohibited[] = " \t,;|<>()[]{}!@?";
			
			for (auto & elem : prohibited) {
				if (call_data->text->ptr[i] == elem) {
					call_data->doit = False;
					return;
				}
			}
		}
	}
	call_data->doit = True;
}

static void updatePrintCmd(Widget w, XtPointer client_data, XtPointer call_data) {

	(void)w;
	(void)client_data;
	(void)call_data;
	
	char command[MAX_CMD_STR], copiesArg[MAX_OPT_STR + MAX_INT_STR];
	char jobArg[MAX_NAME_STR], hostArg[MAX_OPT_STR + MAX_HOST_STR];
	char queueArg[MAX_OPT_STR + MAX_QUEUE_STR];
	int nCopies;

	/* read each text field in the dialog and generate the corresponding
	   command argument */
	if (CopiesOption[0] == '\0') {
		copiesArg[0] = '\0';
	} else {
		const QString str = XmTextGetStringEx(Text1);
		if (str.isEmpty()) {
			copiesArg[0] = '\0';
		} else {
			if (sscanf(str.toLatin1().data(), "%d", &nCopies) != 1) {
				copiesArg[0] = '\0';
			} else {
				sprintf(copiesArg, " %s%s", CopiesOption, str.toLatin1().data());
			}
		}
	}
	
	if (QueueOption[0] == '\0') {
		queueArg[0] = '\0';
	} else {
		const QString str = XmTextGetStringEx(Text2);
		if (str.isEmpty()) {
			queueArg[0] = '\0';
		} else {
			sprintf(queueArg, " %s%s", QueueOption, str.toLatin1().data());
		}
	}
	
	if (HostOption[0] == '\0') {
		hostArg[0] = '\0';
	} else {
		const QString str = XmTextGetStringEx(Text3);
		if (str.isEmpty()) {
			hostArg[0] = '\0';
		} else {
			sprintf(hostArg, " %s%s", HostOption, str.toLatin1().data());
		}
	}
	
	if (NameOption[0] == '\0') {
		jobArg[0] = '\0';
	} else {
		sprintf(jobArg, " %s\"%s\"", NameOption, PrintJobName.c_str());
	}

	/* Compose the command from the options determined above */
	sprintf(command, "%s%s%s%s%s", PrintCommand, copiesArg, queueArg, hostArg, jobArg);

	/* display it in the command text area */
	XmTextSetString(Text4, command);

	/* Indicate that the command field was synthesized from the other fields,
	   so future dialog invocations can safely re-generate the command without
	   overwriting commands specifically entered by the user */
	CmdFieldModified = false;
}

static void printCmdModified(Widget w, XtPointer client_data, XtPointer call_data) {
	(void)w;
	(void)client_data;
	(void)call_data;
	
	/* Indicate that the user has specifically modified the print command
	   and that this field should be left as is in subsequent dialogs */
	CmdFieldModified = true;
}

static void printButtonCB(Widget widget, XtPointer client_data, XtPointer call_data) {

	(void)widget;
	(void)client_data;
	(void)call_data;
	
	char command[MAX_CMD_STR];
	int nRead;
	FILE *pipe;
	char errorString[MAX_PRINT_ERROR_LENGTH], discarded[1024];

	{
		/* get the print command from the command text area */
		const QString str = XmTextGetStringEx(Text4);
	
		/* add the file name and output redirection to the print command */
		sprintf(command, "cat %s | %s 2>&1", PrintFileName.c_str(), str.toLatin1().data());
	}
	
	/* Issue the print command using a popen call and recover error messages
	   from the output stream of the command. */
	pipe = popen(command, "r");
	if(!pipe) {
		QMessageBox::warning(nullptr /*parent*/, QLatin1String("Print Error"), QString(QLatin1String("Unable to Print:\n%1")).arg(QLatin1String(strerror(errno))));
		return;
	}

	errorString[0] = 0;
	nRead = fread(errorString, sizeof(char), MAX_PRINT_ERROR_LENGTH - 1, pipe);
	/* Make sure that the print command doesn't get stuck when trying to
	   write a lot of output on stderr (pipe may fill up). We discard
	   the additional output, though. */
	while (fread(discarded, sizeof(char), 1024, pipe) > 0) {
		;
	}

	if (!ferror(pipe)) {
		errorString[nRead] = '\0';
	}

	if (pclose(pipe)) {
		QMessageBox::warning(nullptr /*parent*/, QLatin1String("Print Error"), QString(QLatin1String("Unable to Print:\n%1")).arg(QLatin1String(errorString)));
		return;
	}

	/* Print command succeeded, so retain the current print parameters */
	if (CopiesOption[0] != '\0') {
		const QString str = XmTextGetStringEx(Text1);
		strcpy(Copies, str.toLatin1().data());
	}
	if (QueueOption[0] != '\0') {
		const QString str = XmTextGetStringEx(Text2);
		strcpy(Queue, str.toLatin1().data());
	}
	if (HostOption[0] != '\0') {
		const QString str = XmTextGetStringEx(Text3);
		strcpy(Host, str.toLatin1().data());
	}
	
	const QString str = XmTextGetStringEx(Text4);
	strcpy(CmdText, str.toLatin1().data());

	/* Pop down the dialog */
	DoneWithDialog = true;
}

static void cancelButtonCB(Widget widget, XtPointer client_data, XtPointer call_data) {

	(void)widget;
	(void)client_data;
	(void)call_data;
	
	DoneWithDialog   = true;
	CmdFieldModified = false;
}

/*
** Is the filename file in the directory dirpath
** and does it have at least some of the mode_flags enabled ?
*/
static int fileInDir(const char *filename, const char *dirpath, unsigned short mode_flags) {
	
	struct stat statbuf;
	
	DIR *dfile = opendir(dirpath);
	if (dfile) {
		
		struct dirent *DirEntryPtr;	
		
		while ((DirEntryPtr = readdir(dfile))) {
			if (!strcmp(DirEntryPtr->d_name, filename)) {
				char fullname[MAXPATHLEN];
				
				snprintf(fullname, sizeof(fullname), "%s/%s", dirpath, filename);

				stat(fullname, &statbuf);
				closedir(dfile);
				return statbuf.st_mode & mode_flags;
			}
		}
		closedir(dfile);
	}
	return False;
}

/*
** Is the filename file in the environment path directories
** and does it have at least some of the mode_flags enabled ?
*/
static int fileInPath(const char *filename, unsigned short mode_flags) {
	char path[MAXPATHLEN];
	char *pathstring, *lastchar;

	/* Get environmental value of PATH */
	pathstring = getenv("PATH");
	if(!pathstring)
		return False;

	/* parse the pathstring and search on each directory found */
	do {
		/* if final path in list is empty, don't search it */
		if (!strcmp(pathstring, ""))
			return False;
		/* locate address of next : character */
		lastchar = strchr(pathstring, SEPARATOR);
		if (lastchar) {
			/* if more directories remain in pathstring, copy up to : */
			strncpy(path, pathstring, lastchar - pathstring);
			path[lastchar - pathstring] = '\0';
		} else {
			/* if it's the last directory, just copy it */
			strcpy(path, pathstring);
		}
		/* search for the file in this path */
		if (fileInDir(filename, path, mode_flags))
			return True; /* found it !! */
		/* point pathstring to start of new dir string */
		pathstring = lastchar + 1;
	} while (lastchar != nullptr);
	return False;
}

/*
** Is flpr present in the search path and is it executable ?
*/
static int flprPresent(void) {
	/* Is flpr present in the search path and is it executable ? */
	return fileInPath("flpr", 0111);
}

static int foundTag(const char *tagfilename, const char *tagname, char *result) {
	FILE *tfile;
	char tagformat[512];

	strcpy(tagformat, tagname);
	strcat(tagformat, " %s");

	tfile = fopen(tagfilename, "r");
	if (tfile) {
		while (!feof(tfile)) {
			
			char line[512];
			
			if(fgets(line, sizeof(line), tfile)) {
				if (sscanf(line, tagformat, result) != 0) {
					fclose(tfile);
					return True;
				}
			}
		}
		fclose(tfile);
	}
	return False;
}

static bool foundEnv(const char *EnvVarName, char *result) {

	if (char *dqstr = getenv(EnvVarName)) {
		strcpy(result, dqstr);
		return true;
	}
	return false;
}

static void getFlprHostDefault(char *defhost) {
	if (!foundEnv("FLPHOST", defhost))
		if (!foundTag("/usr/local/etc/flp.defaults", "host", defhost))
			strcpy(defhost, "");
}

static void getFlprQueueDefault(char *defqueue) {
	if (!foundEnv("FLPQUE", defqueue))
		if (!foundTag("/usr/local/etc/flp.defaults", "queue", defqueue))
			strcpy(defqueue, "");
}

static void getLprQueueDefault(char *defqueue) {
	if (!foundEnv("PRINTER", defqueue))
		strcpy(defqueue, "");
}
