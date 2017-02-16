
#ifndef DOCUMENT_WIDGET_H_
#define DOCUMENT_WIDGET_H_


#include "Bookmark.h"
#include "IndentStyle.h"
#include "LockReasons.h"
#include "MenuItem.h"
#include "SearchDirection.h"
#include "SearchType.h"
#include "ShowMatchingStyle.h"
#include "string_view.h"
#include "tags.h"
#include "userCmds.h"
#include "util/FileFormats.h"

#include <QDialog>
#include <QPointer>
#include <QProcess>
#include <QWidget>

#include "ui_DocumentWidget.h"

class MainWindow;
class QFrame;
class QLabel;
class QMenu;
class QSplitter;
class QTimer;
class TextArea;
class TextBuffer;
class UndoInfo;
struct HighlightData;
struct WindowHighlightData;
struct dragEndCBStruct;
struct smartIndentCBStruct;

class DocumentWidget : public QWidget {
	Q_OBJECT
	friend class MainWindow;
public:
	DocumentWidget(const QString &name, QWidget *parent = 0, Qt::WindowFlags f = 0);
	virtual ~DocumentWidget();

private Q_SLOTS:
    void flashTimerTimeout();
    void customContextMenuRequested(const QPoint &pos);
    void mergedReadProc();
    void stdoutReadProc();
    void stderrReadProc();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

public Q_SLOTS:
    void setLanguageMode(const QString &mode);
    void open(const char *fullpath);
    void bannerTimeoutProc();
    void findIncrAP(const QString &searchString, SearchDirection direction, SearchType searchType, bool searchWraps, bool isContinue);
    void findAP(const QString &searchString, SearchDirection direction, SearchType searchType, bool searchWraps);
    void replaceAP(const QString &searchString, const QString &replaceString, SearchDirection direction, SearchType searchType, bool searchWraps);
    void replaceFindAP(const QString &searchString, const QString &replaceString, SearchDirection direction, SearchType searchType, bool searchWraps);
    void replaceAllAP(const QString &searchString, const QString &replaceString, SearchType searchType);
    void replaceInSelAP(const QString &searchString, const QString &replaceString, SearchType searchType);
    void markAP(QChar ch);
    void gotoMarkAP(QChar label, bool extendSel);
    void GotoMatchingCharacter(TextArea *area);
    void SelectToMatchingCharacter(TextArea *area);
    void FindDefCalltip(TextArea *area, const char *arg);
    void findDefinitionHelper(TextArea *area, const char *arg, Mode search_type);
    int findDef(TextArea *area, const char *value, Mode search_type);
    void FindDefinition(TextArea *area, const char *arg);
    void execAP(TextArea *area, const QString &command);
    void ExecShellCommandEx(TextArea *area, const QString &command, bool fromMacro);
    void PrintWindow(TextArea *area, bool selectedOnly);
    void PrintStringEx(const std::string &string, const QString &jobName);
    void splitPane();
    void closePane();
    void BeginSmartIndentEx(int warn);
    void moveDocument(MainWindow *fromWindow);
    void ShowStatsLine(bool state);
    void gotoAP(TextArea *area, QStringList args);
    void SetColors(const QString &textFg, const QString &textBg, const QString &selectFg, const QString &selectBg, const QString &hiliteFg, const QString &hiliteBg, const QString &lineNoFg, const QString &cursorFg);

public:
	void movedCallback(TextArea *area);
	void dragStartCallback(TextArea *area);
	void dragEndCallback(TextArea *area, dragEndCBStruct *data);
	void smartIndentCallback(TextArea *area, smartIndentCBStruct *data);
    void modifiedCallback(int pos, int nInserted, int nDeleted, int nRestyled, view::string_view deletedText);

public:
    static DocumentWidget *documentFrom(TextArea *area);

public:
    TextArea *createTextArea(TextBuffer *buffer);
    QList<TextArea *> textPanes() const;
    TextArea *firstPane() const;
	void SetWindowModified(bool modified);
	void RefreshTabState();
	void DetermineLanguageMode(bool forceNewDefaults);
	void SetLanguageMode(int mode, bool forceNewDefaults);
	int matchLanguageMode();
	void UpdateStatsLine(TextArea *area);
    void RaiseDocument();
    void documentRaised();
    void reapplyLanguageMode(int mode, bool forceDefaults);
    void setWrapMargin(int margn);
    void SetAutoWrap(int state);
    void SetAutoIndent(IndentStyle state);
    void SetEmTabDist(int emTabDist);
    void SetTabDist(int tabDist);
    bool IsTopDocument() const;
    QString getWindowsMenuEntry();
    MainWindow *toWindow() const;
    void UpdateMarkTable(int pos, int nInserted, int nDeleted);
    void StopHighlightingEx();
    void SetBacklightChars(const QString &applyBacklightTypes);
    void freeHighlightData(WindowHighlightData *hd);
    void freePatterns(HighlightData *patterns);
    QString GetWindowDelimiters() const;
    void DimSelectionDepUserMenuItems(bool enabled);
    void dimSelDepItemsInMenu(QMenu *menuPane, const QVector<MenuData> &menuList, bool enabled);
    void RaiseFocusDocumentWindow(bool focus);
    void RaiseDocumentWindow();
    void SaveUndoInformation(int pos, int nInserted, int nDeleted, view::string_view deletedText);
    void ClearRedoList();
    void ClearUndoList();
    void appendDeletedText(view::string_view deletedText, int deletedLen, int direction);
    void removeRedoItem();
    void removeUndoItem();
    void addRedoItem(UndoInfo *redo);
    void addUndoItem(UndoInfo *undo);
    void trimUndoList(int maxLength);
    void Undo();
    void Redo();
    bool CheckReadOnly() const;
    void MakeSelectionVisible(TextArea *textPane);
    void RemoveBackupFile();
    QString backupFileNameEx();
    void CheckForChangesToFileEx();
    QString FullPath() const;
    int cmpWinAgainstFile(const QString &fileName);
    void RevertToSaved();
    int WriteBackupFile();
    int SaveWindow();
    bool doSave();
    int SaveWindowAs(const QString &newName, bool addWrap);
    void addWrapNewlines();
    bool writeBckVersion();
    bool bckError(const QString &errString, const QString &file);
    int fileWasModifiedExternally();
    int CloseFileAndWindow(int preResponse);
    void CloseWindow();
    bool doOpen(const QString &name, const QString &path, int flags);
    void RefreshWindowStates();
    void refreshMenuBar();
    void RefreshMenuToggleStates();
    void executeNewlineMacroEx(smartIndentCBStruct *cbInfo);
    void SetShowMatching(ShowMatchingStyle state);
    int textPanesCount() const;
    void executeModMacroEx(smartIndentCBStruct *cbInfo);
    void actionClose(const QString &mode);
    bool includeFile(const QString &name);
    bool findMatchingCharEx(char toMatch, void *styleToMatch, int charPos, int startLimit, int endLimit, int *matchPos);
    void SetFonts(const QString &fontName, const QString &italicName, const QString &boldName, const QString &boldItalicName);
    void SetOverstrike(bool overstrike);
    void SetModeMessageEx(const QString &message);
    void ClearModeMessageEx();
    void safeCloseEx();
    void UnloadLanguageModeTipsFileEx();
    void issueCommandEx(MainWindow *window, TextArea *area, const QString &command, const QString &input, int flags, int replaceLeft, int replaceRight, bool fromMacro);
    void AbortShellCommandEx();
    void ExecCursorLineEx(TextArea *area, bool fromMacro);
    void filterSelection(const QString &filterText);
    void FilterSelection(const QString &command, bool fromMacro);
    bool DoNamedShellMenuCmd(TextArea *area, const QString &name, bool fromMacro);
    void DoShellMenuCmd(MainWindow *inWindow, TextArea *area, const QString &command, InSrcs input, OutDests output, bool outputReplacesInput, bool saveFirst, bool loadAfter, bool fromMacro);
    bool DoNamedMacroMenuCmd(TextArea *area, const QString &name, bool fromMacro);
    void repeatMacro(const QString &macro, int how);
    bool DoNamedBGMenuCmd(TextArea *area, const QString &name, bool fromMacro);
    int WidgetToPaneIndex(TextArea *area) const;
    void ShellCmdToMacroStringEx(const std::string &command, const std::string &input);
    void SetAutoScroll(int margin);

public:
    static DocumentWidget *EditExistingFileEx(DocumentWidget *inWindow, const QString &name, const QString &path, int flags, const QString &geometry, int iconic, const QString &languageMode, bool tabbed, bool bgOpen);
    static QList<DocumentWidget *> allDocuments();

private:
	// TODO(eteran): are these dialog's per window or per text document?
	QPointer<QDialog> dialogColors_;
	QPointer<QDialog> dialogFonts_; /* nullptr, unless font dialog is up */	

public:
	Bookmark markTable_[MAX_MARKS];    // marked locations in window
	FileFormats fileFormat_;           // whether to save the file straight (Unix format), or convert it to MS DOS style with \r\n line breaks
	LockReasons lockReasons_;          // all ways a file can be locked
	QString backlightCharTypes_;       // what backlighting to use
	QString boldFontName_;
	QString boldItalicFontName_;
	QString filename_;                 // name component of file being edited
	QString fontName_;                 // names of the text fonts in use
	QString italicFontName_;
	QString path_;                     // path component of file being edited
	TextBuffer *buffer_;               // holds the text being edited

    QFont fontStruct_;
    QFont boldFontStruct_;
    QFont boldItalicFontStruct_;
    QFont italicFontStruct_;    // fontStructs for highlighting fonts

    QTimer *flashTimer_;               // timer for getting rid of highlighted matching paren.
    QMenu *contextMenu_;

	bool autoSave_;                    // is autosave turned on?
	bool backlightChars_;              // is char backlighting turned on?
	bool fileChanged_;                 // has window been modified?
	bool fileMissing_;                 // is the window's file gone?
	bool filenameSet_;                 // is the window still "Untitled"?
	bool highlightSyntax_;             // is syntax highlighting turned on?
	bool ignoreModify_;                // ignore modifications to text area
	bool modeMessageDisplayed_;        // special stats line banner for learn and shell command executing modes
	bool multiFileBusy_;               // suppresses multiple beeps/dialogs during multi-file replacements
	bool multiFileReplSelected_;       // selected during last multi-window replacement operation (history)
	bool overstrike_;                  // is overstrike mode turned on ?
	bool replaceFailed_;               // flags replacements failures during multi-file replacements
    bool saveOldVersion_;              // keep old version in filename.bc
    QString modeMessage_;              // stats line banner content for learn and shell command executing modes
    IndentStyle indentStyle_;          // whether/how to auto indent
	char matchSyntaxBased_;            // Use syntax info to show matching
    ShowMatchingStyle showMatchingStyle_;           // How to show matching parens: NO_FLASH, FLASH_DELIMIT, or FLASH_RANGE
	char wrapMode_;                    // line wrap style: NO_WRAP, NEWLINE_WRAP or CONTINUOUS_WRAP
	dev_t device_;                     // device where the file resides
	gid_t fileGid_;                    // last recorded group id of the file
	ino_t inode_;                      // file's inode
	int autoSaveCharCount_;            // count of single characters typed since last backup file generated
	int autoSaveOpCount_;              // count of editing operations ""    
	int flashPos_;                     // position saved for erasing matching paren highlight (if one is drawn)
	int languageMode_;                 // identifies language mode currently selected in the window
	int nMarks_;                       // number of active bookmarks
	int undoMemUsed_;                  // amount of memory (in bytes) dedicated to the undo list
	std::list<UndoInfo *> redo_;       // info for redoing last undone op
	std::list<UndoInfo *> undo_;       // info for undoing last operation
	time_t lastModTime_;               // time of last modification to file
	uid_t fileUid_;                    // last recorded user id of the file
	unsigned fileMode_;                // permissions of file being edited
    void *highlightData_;              // info for syntax highlighting                                          // TODO(eteran): why void* ?
    void *macroCmdData_;               // same for macro commands                                               // TODO(eteran): why void* ?
    void *shellCmdData_;               // when a shell command is executing, info. about it, otherwise, nullptr // TODO(eteran): why void* ?
    void *smartIndentData_;            // compiled macros for smart indent                                      // TODO(eteran): why void* ?
    bool  showStats_;                  // is stats line supposed to be shown

private:
	QSplitter *splitter_;
	Ui::DocumentWidget ui;
};

#endif