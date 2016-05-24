
#include "DialogAbout.h"
#include <QLocale>
#include <QSysInfo>
#include <QtGlobal>

namespace {

//------------------------------------------------------------------------------
// Name: buildPlatform
//------------------------------------------------------------------------------
QString buildPlatform() {
#if defined(Q_OS_AIX)
	return QLatin1String("AIX");
#elif defined(Q_OS_BSD4)
	return QLatin1String("BSD4");
#elif defined(Q_OS_BSDI)
	return QLatin1String("BSDI");
#elif defined(Q_OS_CYGWIN)
	return QLatin1String("Cygwin");
#elif defined(Q_OS_DARWIN)
	return QLatin1String("Darwin");
#elif defined(Q_OS_DGUX)
	return QLatin1String("DGUX");
#elif defined(Q_OS_DYNIX)
	return QLatin1String("Dynix");
#elif defined(Q_OS_FREEBSD)
	return QLatin1String("FreeBSD");
#elif defined(Q_OS_HPUX)
	return QLatin1String("HPUX");
#elif defined(Q_OS_HURD)
	return QLatin1String("HURD");
#elif defined(Q_OS_IRIX)
	return QLatin1String("IRIX");
#elif defined(Q_OS_LINUX)
	return QLatin1String("Linux");
#elif defined(Q_OS_LYNX)
	return QLatin1String("Lynx");
#elif defined(Q_OS_MAC)
	return QLatin1String("OSX");
#elif defined(Q_OS_MSDOS)
	return QLatin1String("MS-DOS");
#elif defined(Q_OS_NETBSD)
	return QLatin1String("NetBSD");
#elif defined(Q_OS_OS2)
	return QLatin1String("OS/2");
#elif defined(Q_OS_OPENBSD)
	return QLatin1String("OpenBSD");
#elif defined(Q_OS_OS2EMX)
	return QLatin1String("OS/2 EMX");
#elif defined(Q_OS_OSF)
	return QLatin1String("OSF");
#elif defined(Q_OS_QNX)
	return QLatin1String("QNX");
#elif defined(Q_OS_RELIANT)
	return QLatin1String("Reliant");
#elif defined(Q_OS_SCO)
	return QLatin1String("SCO");
#elif defined(Q_OS_SOLARIS)
	return QLatin1String("Solaris");
#elif defined(Q_OS_SYMBIAN)
	return QLatin1String("Symbian");
#elif defined(Q_OS_ULTRIX)
	return QLatin1String("Ultrix");
#elif defined(Q_OS_UNIX)
	return QLatin1String("Unix");
#elif defined(Q_OS_UNIXWARE)
	return QLatin1String("Unixware");
#elif defined(Q_OS_WIN32)
	return QLatin1String("Windows");
#elif defined(Q_OS_WINCE)
	return QLatin1String("WinCE");
#else
	return QLatin1String("<Unknown OS>");
#endif
}

//------------------------------------------------------------------------------
// Name: buildCompiler
// Note: copied from QtCreator ICore
//------------------------------------------------------------------------------
QString buildCompiler() {
#if defined(Q_CC_CLANG) // must be before GNU, because clang claims to be GNU too
    QString isAppleString;
#if defined(__apple_build_version__) // Apple clang has other version numbers
    isAppleString = QLatin1String(" (Apple)");
#endif
    return QLatin1String("Clang " ) + QString::number(__clang_major__) + QLatin1Char('.') + QString::number(__clang_minor__) + isAppleString;
#elif defined(Q_CC_GNU)
    return QLatin1String("GCC " ) + QLatin1String(__VERSION__);
#elif defined(Q_CC_MSVC)
    if (_MSC_VER >= 1500) { // 1500: MSVC 2008, 1600: MSVC 2010, ...
        return QLatin1String("MSVC ") + QString::number(2008 + 2 * ((_MSC_VER / 100) - 15));
	}
#else
	return QLatin1String("<Unknown Compiler>");
#endif
}

}

//------------------------------------------------------------------------------
// Name: DialogAbout
//------------------------------------------------------------------------------
DialogAbout::DialogAbout(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f) {
	ui.setupUi(this);

	ui.textAbout->setText(tr(
		"NEdit-ng version %1\n"
		"\n"
		"     Built on: %2, %3, %4\n"
		"     Built at: %5, %6\n"
		"      With Qt: %7\n"
		"   Running Qt: %8\n"
		"       Locale: %9\n"
		"\n"
		"NEdit-ng was written by Evan Teran. It is intended to be a modern replacement for the Nirvana Editor (aka NEdit). The author has been using NEdit as his primary code editor for many years, and while it continues to be a superior editor in many ways, it is unfortunately showing its age. So NEdit-ng was born out of a design to have an editor that functions as close to the original as possible, but utilizing a modern toolkit (Qt). This will allow NEdit-ng to enjoy the benefit of modern features such as:\n"
		"\n"
		" * A modern/native look and feel.\n\n"
		" * Antialiased fonts\n\n"
		" * Unicode support!\n\n"
		" * More cross platform, it will run on Windows, OSX, Linux and any other platform Qt supports.\n\n"
		" * By using a modern toolkit, more developers will be able to be involved in furthering the evolution of the editor\n\n"
	)
	.arg(tr("1.0.0"))
	.arg(buildPlatform())
	.arg(tr("%1 bit").arg(QSysInfo::WordSize))
	.arg(buildCompiler())
	.arg(QLatin1String(__DATE__))
	.arg(QLatin1String(__TIME__))
	.arg(QLatin1String(QT_VERSION_STR))
	.arg(QLatin1String(qVersion()))
	.arg(QLocale::system().bcp47Name())
	);
}

//------------------------------------------------------------------------------
// Name: ~DialogAbout
//------------------------------------------------------------------------------
DialogAbout::~DialogAbout() {
}
