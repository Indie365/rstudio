/*
 * DesktopOptions.cpp
 *
 * Copyright (C) 2022 by Posit Software, PBC
 *
 * Unless you have received this program directly from Posit Software pursuant
 * to the terms of a commercial license agreement with Posit Software, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#ifdef _WIN32
# define _WINSOCK_DEPRECATED_NO_WARNINGS
# include <windows.h>
# include <winsock2.h>
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netinet/in.h>
# include <netinet/ip.h>
#endif

#include "DesktopOptions.hpp"

#include <QtGui>

#include <QApplication>
#include <QDesktopWidget>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/scope_exit.hpp>

#include <shared_core/SafeConvert.hpp>

#include <core/Random.hpp>
#include <core/StringUtils.hpp>
#include <core/system/System.hpp>
#include <core/system/Environment.hpp>

#include "DesktopInfo.hpp"
#include "DesktopUtils.hpp"


#define kRStudioDesktopSessionPortValidationEnabled "RSTUDIO_DESKTOP_SESSION_PORT_VALIDATION_ENABLED"
#define kRStudioDesktopSessionPortRange "RSTUDIO_DESKTOP_SESSION_PORT_RANGE"
#define kRStudioDesktopSessionPort "RSTUDIO_DESKTOP_SESSION_PORT"

#define kDefaultPortRangeStart 49152
#define kDefaultPortRangeEnd 65535

using namespace rstudio::core;

namespace rstudio {
namespace desktop {

namespace {

bool portIsOpen(int port)
{
   // Disable validation checks if requested; mainly for testing in cases
   // where QTcpSocket is unable to successfully bind to an open port
   std::string envvar = core::system::getenv(kRStudioDesktopSessionPortValidationEnabled);
   bool needsValidation = core::string_utils::isTruthy(envvar, true);
   if (!needsValidation)
      return true;

#ifdef _WIN32
   // NOTE: Ideally, we'd just use QTcpSocket or some other helper from
   // Qt or Boost, but we had trouble getting these to work in the presence
   // of Windows proxy servers, so we just hand-roll some winsock below.

   // define a listener socket
   SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (listener == INVALID_SOCKET)
   {
      DLOGF("Error in socket() [error code {}]; assuming port available", WSAGetLastError());
      return true;
   }

   // request exclusive access (similar to session behavior)
   std::string enableExclusiveAddrUse = core::system::getenv("RSTUDIO_DESKTOP_EXCLUSIVE_ADDR_USE");
   if (core::string_utils::isTruthy(enableExclusiveAddrUse, true))
   {
      int exclusiveAddrUse = 1;
      setsockopt(listener, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*) &exclusiveAddrUse, sizeof(exclusiveAddrUse));
   }

   // define the address we want to bind to
   sockaddr_in address;
   address.sin_family = AF_INET;
   address.sin_port = htons(port);
   address.sin_addr.s_addr = inet_addr("127.0.0.1");
   memset(address.sin_zero, 0, sizeof(address.sin_zero));

   // try to bind the socket
   int bindResult = bind(listener, (SOCKADDR*) &address, sizeof(address));
   if (bindResult == SOCKET_ERROR)
   {
      int error = WSAGetLastError();
      DLOGF("Error binding to port {} [error code {}]", port, error);
   }
   else
   {
      DLOGF("Successfully bound to port {}", port);
   }

   // close socket
   closesocket(listener);

   return bindResult != SOCKET_ERROR;

#else

   // create a socket
   int fd = socket(AF_INET, SOCK_STREAM, 0);
   if (fd == -1)
   {
      int errorNumber = errno;
      DLOGF("Error in socket() [error code {}]; assuming port open", errorNumber);
      return true;
   }

   // define the address we want to bind to
   struct sockaddr_in address;
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = inet_addr("127.0.0.1");
   address.sin_port = htons(port);
   memset(address.sin_zero, 0, sizeof(address.sin_zero));

   // try to bind to the address
   int bindResult = bind(fd, (struct sockaddr *) &address, sizeof(address));
   if (bindResult == -1)
   {
      int errorNumber = errno;
      DLOGF("Error binding to port {} [error code {}]", port, errorNumber);
   }
   else
   {
      DLOGF("Successfully bound to port {}", port);
   }

   close(fd);
   return bindResult != -1;
#endif
}

int findOpenPort()
{
   // allow for override in non-package builds, for testing
#ifndef RSTUDIO_PACKAGE_BUILD
   std::string override = core::system::getenv(kRStudioDesktopSessionPort);
   if (!override.empty())
   {
      auto result = safe_convert::stringTo<int>(override);
      if (result)
      {
         DLOGF("Using session port override '{}'", *result);
         return *result;
      }
      else
      {
         DLOGF("Ignoring invalid session port override '{}'", override);
      }
   }
#endif

   // RFC 6335 suggests the range 49152-65535 for dynamic / private ephemeral ports
   int portRangeStart = kDefaultPortRangeStart;
   int portRangeEnd = kDefaultPortRangeEnd;

   // allow override if necessary
   std::string portRangeOverride = core::system::getenv(kRStudioDesktopSessionPortRange);
   if (!portRangeOverride.empty())
   {
      std::vector<std::string> parts;
      boost::algorithm::split(parts, portRangeOverride, boost::is_any_of(":-,"));

      if (parts.size() == 2)
      {
         auto start = safe_convert::stringTo<int>(parts[0], portRangeStart);
         auto end   = safe_convert::stringTo<int>(parts[1], portRangeEnd);

         // If the ports appear invalid, fall back to defaults
         if (start >= end || start < 0 || start > 65535 || end < 0 || end > 65535)
         {
            DLOGF("Ignoring invalid port range value '{}'", portRangeOverride);
         }
         else
         {
            portRangeStart = start;
            portRangeEnd = end;
         }
      }
      else
      {
         DLOGF("Unexpected value '{}' for {}", portRangeOverride, kRStudioDesktopSessionPortRange);
      }
   }

   DLOGF("Using port range [{}, {})", portRangeStart, portRangeEnd);
   for (int i = 0; i < 100; i++)
   {
      // generate a port number
      int port = core::random::uniformRandomInteger<int>(portRangeStart, portRangeEnd);
      if (portIsOpen(port))
         return port;
   }

   // we couldn't find an open port; just choose a random port and hope for the best
   int port = core::random::uniformRandomInteger<int>(portRangeStart, portRangeEnd);
   DLOGF("Failed to find open port; defaulting to port {}", port);
   return port;
}

} // end anonymous namespace

#ifdef _WIN32
// Defined in DesktopRVersion.cpp
QString binDirToHomeDir(QString binDir);
#endif

#define kMainWindowGeometry (QStringLiteral("mainwindow/geometry"))
#define kFixedWidthFont     (QStringLiteral("font.fixedWidth"))
#define kProportionalFont   (QStringLiteral("font.proportional"))

QString scratchPath;

Options& options()
{
   static Options singleton;
   return singleton;
}

Options::Options() :
   settings_(FORMAT,
             QSettings::UserScope,
             QString::fromUtf8("RStudio"),
             QString::fromUtf8("desktop")),
   runDiagnostics_(false)
{
#ifndef _WIN32
   // ensure that the options file is only readable by this user
   FilePath optionsFile(settings_.fileName().toStdString());
   if (!optionsFile.exists())
   {
      // file doesn't yet exist - QT can lazily write to the settings file
      // create an empty file so we can set its permissions before it's created by QT
      std::shared_ptr<std::ostream> pOfs;
      Error error = optionsFile.openForWrite(pOfs, false);
      if (error)
         LOG_ERROR(error);
   }

   Error error = optionsFile.changeFileMode(FileMode::USER_READ_WRITE);
   if (error)
      LOG_ERROR(error);
#endif
}

void Options::initFromCommandLine(const QStringList& arguments)
{
   for (int i=1; i<arguments.size(); i++)
   {
      const QString &arg = arguments.at(i);
      if (arg == QString::fromUtf8(kRunDiagnosticsOption))
         runDiagnostics_ = true;
   }

   // synchronize zoom level with desktop frame
   desktopInfo().setZoomLevel(zoomLevel());
}

void Options::restoreMainWindowBounds(QMainWindow* win)
{
   // NOTE: on macOS, if the display configuration has changed, the attempt to
   // restore window geometry can fail and the use can be left with a tiny
   // RStudio window.
   //
   // to avoid this, we always first attempt to resize to a 'good' size, and
   // then restore a saved window geometry (which may just silently fail if the
   // display configuration has indeed changed)
   //
   // https://github.com/rstudio/rstudio/issues/3498
   // https://github.com/rstudio/rstudio/issues/3159
   //
   
   QSize size = QSize(1200, 900).boundedTo(
            QApplication::primaryScreen()->availableGeometry().size());
   if (size.width() > 800 && size.height() > 500)
   {
      // Only use default size if it seems sane; otherwise let Qt set it
      win->resize(size);
   }

   if (settings_.contains(kMainWindowGeometry))
   {
      // try to restore the geometry
      win->restoreGeometry(settings_.value(kMainWindowGeometry).toByteArray());

      // double-check that we haven't accidentally restored a geometry that
      // places the Window off-screen (can happen if the screen configuration
      // changes between the time geometry was saved and loaded)
      QRect desktopRect = QApplication::primaryScreen()->availableGeometry();
      QRect winRect = win->geometry();
      
      // shrink the window rectangle a bit just to capture cases like RStudio
      // too close to edge of window and hardly showing at all
      QRect checkRect(
               winRect.topLeft() + QPoint(5, 5),
               winRect.bottomRight() - QPoint(5, 5));
      
      // check for intersection
      if (!desktopRect.intersects(checkRect))
      {
         // restore size and center the window
         win->resize(size);
         win->move(
                  desktopRect.width() / 2 - size.width() / 2,
                  desktopRect.height() / 2 - size.height() / 2);
      }
   }
   
   // ensure a minimum width, height for the window on restore
   win->resize(
            std::max(300, win->width()),
            std::max(200, win->height()));
      
}

void Options::saveMainWindowBounds(QMainWindow* win)
{
   settings_.setValue(kMainWindowGeometry, win->saveGeometry());
}

QString Options::portNumber() const
{
   // if we already have a port number, use it
   if (!portNumber_.isEmpty())
   {
      LOG_DEBUG_MESSAGE("Using port: " + portNumber_.toStdString());
      return portNumber_;
   }

   // compute the port number to use
   DLOGF("Finding open port for communication with rsession");
   int port = findOpenPort();

   DLOGF("Using port: {}", port);
   portNumber_ = QString::number(port);

   // recalculate the local peer and set RS_LOCAL_PEER so that
   // rsession and its children can use it
#ifdef _WIN32
   QString localPeer =  QStringLiteral("\\\\.\\pipe\\%1-rsession").arg(portNumber_);
   localPeer_ = localPeer.toUtf8().constData();
   core::system::setenv("RS_LOCAL_PEER", localPeer_);
#endif

   return portNumber_;
}

QString Options::newPortNumber()
{
   portNumber_.clear();
   return portNumber();
}

std::string Options::localPeer() const
{
   return localPeer_;
}

QString Options::desktopRenderingEngine() const
{
   return settings_.value(QStringLiteral("desktop.renderingEngine")).toString();
}

void Options::setDesktopRenderingEngine(QString engine)
{
   settings_.setValue(QStringLiteral("desktop.renderingEngine"), engine);
}

namespace {

QString findFirstMatchingFont(const QStringList& fonts,
                              QString defaultFont,
                              bool fixedWidthOnly)
{
   for (int i = 0; i < fonts.size(); i++)
   {
      QFont font(fonts.at(i));
      if (font.exactMatch())
         if (!fixedWidthOnly || isFixedWidthFont(QFont(fonts.at(i))))
            return fonts.at(i);
   }
   return defaultFont;
}

} // anonymous namespace

void Options::setFont(QString key, QString font)
{
   if (font.isEmpty())
      settings_.remove(key);
   else
      settings_.setValue(key, font);
}

void Options::setProportionalFont(QString font)
{
   setFont(kProportionalFont, font);
}

QString Options::proportionalFont() const
{
   static QString detectedFont;

   QString font =
         settings_.value(kProportionalFont).toString();
   if (!font.isEmpty())
   {
      return font;
   }

   if (!detectedFont.isEmpty())
      return detectedFont;

   QStringList fontList;
#if defined(_WIN32)
   fontList <<
           QString::fromUtf8("Segoe UI") << QString::fromUtf8("Verdana") <<  // Windows
           QString::fromUtf8("Lucida Sans") << QString::fromUtf8("DejaVu Sans") <<  // Linux
           QString::fromUtf8("Lucida Grande") <<          // Mac
           QString::fromUtf8("Helvetica");
#elif defined(__APPLE__)
   fontList <<
           QString::fromUtf8("Lucida Grande") <<          // Mac
           QString::fromUtf8("Lucida Sans") << QString::fromUtf8("DejaVu Sans") <<  // Linux
           QString::fromUtf8("Segoe UI") << QString::fromUtf8("Verdana") <<  // Windows
           QString::fromUtf8("Helvetica");
#else
   fontList <<
           QString::fromUtf8("Lucida Sans") << QString::fromUtf8("DejaVu Sans") <<  // Linux
           QString::fromUtf8("Lucida Grande") <<          // Mac
           QString::fromUtf8("Segoe UI") << QString::fromUtf8("Verdana") <<  // Windows
           QString::fromUtf8("Helvetica");
#endif

   QString sansSerif = QStringLiteral("sans-serif");
   QString selectedFont = findFirstMatchingFont(fontList, sansSerif, false);

   // NOTE: browsers will refuse to render a default font if the name is in
   // quotes; e.g. "sans-serif" is a signal to look for a font called sans-serif
   // rather than use the default sans-serif font!
   if (selectedFont == sansSerif)
      return sansSerif;
   else
      return QStringLiteral("\"%1\"").arg(selectedFont);
}

void Options::setFixedWidthFont(QString font)
{
   setFont(kFixedWidthFont, font);
}

QString Options::fixedWidthFont() const
{
   static QString detectedFont;

   QString font =
         settings_.value(kFixedWidthFont).toString();
   if (!font.isEmpty())
   {
      return QString::fromUtf8("\"") + font + QString::fromUtf8("\"");
   }

   if (!detectedFont.isEmpty())
      return detectedFont;

   QStringList fontList;
   fontList <<
#if defined(Q_OS_MAC)
           QString::fromUtf8("Monaco")
#elif defined (Q_OS_LINUX)
           QString::fromUtf8("Ubuntu Mono") << QString::fromUtf8("Droid Sans Mono") << QString::fromUtf8("DejaVu Sans Mono") << QString::fromUtf8("Monospace")
#else
           QString::fromUtf8("Lucida Console") << QString::fromUtf8("Consolas") // Windows;
#endif
           ;

   // NOTE: browsers will refuse to render a default font if the name is in
   // quotes; e.g. "monospace" is a signal to look for a font called monospace
   // rather than use the default monospace font!
   QString monospace = QStringLiteral("monospace");
   QString matchingFont = findFirstMatchingFont(fontList, monospace, true);
   if (matchingFont == monospace)
      return monospace;
   else
      return QStringLiteral("\"%1\"").arg(matchingFont);
}


double Options::zoomLevel() const
{
   QVariant zoom = settings_.value(QString::fromUtf8("view.zoomLevel"), 1.0);
   return zoom.toDouble();
}

void Options::setZoomLevel(double zoomLevel)
{
   desktopInfo().setZoomLevel(zoomLevel);
   settings_.setValue(QString::fromUtf8("view.zoomLevel"), zoomLevel);
}

bool Options::enableAccessibility() const
{
   QVariant accessibility = settings_.value(QString::fromUtf8("view.accessibility"), false);
   return accessibility.toBool();
}

void Options::setEnableAccessibility(bool enable)
{
   settings_.setValue(QString::fromUtf8("view.accessibility"), enable);
}

bool Options::clipboardMonitoring() const
{
   QVariant monitoring = settings_.value(QString::fromUtf8("clipboard.monitoring"), true);
   return monitoring.toBool();
}

void Options::setClipboardMonitoring(bool monitoring)
{
   settings_.setValue(QString::fromUtf8("clipboard.monitoring"), monitoring);
}

bool Options::ignoreGpuExclusionList() const
{
   QVariant ignore = settings_.value(QStringLiteral("general.ignoreGpuExclusionList"), false);
   return ignore.toBool();
}

void Options::setIgnoreGpuExclusionList(bool ignore)
{
   settings_.setValue(QStringLiteral("general.ignoreGpuExclusionList"), ignore);
}

bool Options::disableGpuDriverBugWorkarounds() const
{
   QVariant disable = settings_.value(QStringLiteral("general.disableGpuDriverBugWorkarounds"), false);
   return disable.toBool();
}

void Options::setDisableGpuDriverBugWorkarounds(bool disable)
{
   settings_.setValue(QStringLiteral("general.disableGpuDriverBugWorkarounds"), disable);
}

bool Options::useFontConfigDatabase() const
{
   QVariant use = settings_.value(QStringLiteral("general.useFontConfigDatabase"), true);
   return use.toBool();
}

void Options::setUseFontConfigDatabase(bool use)
{
   settings_.setValue(QStringLiteral("general.useFontConfigDatabase"), use);
}

#ifdef _WIN32
QString Options::rBinDir() const
{
   // HACK: If RBinDir doesn't appear at all, that means the user has never
   // specified a preference for R64 vs. 32-bit R. In this situation we should
   // accept either. We'll distinguish between this case (where preferR64
   // should be ignored) and the other case by using null for this case and
   // empty string for the other.
   if (!settings_.contains(QString::fromUtf8("RBinDir")))
      return QString();

   QString value = settings_.value(QString::fromUtf8("RBinDir")).toString();
   return value.isNull() ? QString() : value;
}

void Options::setRBinDir(QString path)
{
   settings_.setValue(QString::fromUtf8("RBinDir"), path);
}

bool Options::preferR64() const
{
   if (!core::system::isWin64())
      return false;

   if (!settings_.contains(QString::fromUtf8("PreferR64")))
      return true;
   return settings_.value(QString::fromUtf8("PreferR64")).toBool();
}

void Options::setPreferR64(bool preferR64)
{
   settings_.setValue(QString::fromUtf8("PreferR64"), preferR64);
}
#endif

FilePath Options::scriptsPath() const
{
   return scriptsPath_;
}

void Options::setScriptsPath(const FilePath& scriptsPath)
{
   scriptsPath_ = scriptsPath;
}

FilePath Options::executablePath() const
{
   if (executablePath_.isEmpty())
   {
      Error error = core::system::executablePath(QApplication::arguments().at(0).toUtf8(),
                                                 &executablePath_);
      if (error)
         LOG_ERROR(error);
   }
   return executablePath_;
}

FilePath Options::supportingFilePath() const
{
   if (supportingFilePath_.isEmpty())
   {
      // default to install path
      core::system::installPath("..",
                                QApplication::arguments().at(0).toUtf8(),
                                &supportingFilePath_);

      // adapt for OSX resource bundles
#ifdef __APPLE__
         if (supportingFilePath_.completePath("Info.plist").exists())
            supportingFilePath_ = supportingFilePath_.completePath("Resources");
#endif
   }
   return supportingFilePath_;
}

FilePath Options::resourcesPath() const
{
   if (resourcesPath_.isEmpty())
   {
#ifdef RSTUDIO_PACKAGE_BUILD
      // release configuration: the 'resources' folder is
      // part of the supporting files folder
      resourcesPath_ = supportingFilePath().completePath("resources");
#else
      // developer configuration: the 'resources' folder is
      // a sibling of the RStudio executable
      resourcesPath_ = scriptsPath().completePath("resources");
#endif
   }

   return resourcesPath_;
}

FilePath Options::wwwDocsPath() const
{
   FilePath supportingFilePath = desktop::options().supportingFilePath();
   FilePath wwwDocsPath = supportingFilePath.completePath("www/docs");
   if (!wwwDocsPath.exists())
      wwwDocsPath = supportingFilePath.completePath("../gwt/www/docs");
#ifdef __APPLE__
   if (!wwwDocsPath.exists())
      wwwDocsPath = supportingFilePath.completePath("../../../../../gwt/www/docs");
#endif
   return wwwDocsPath;
}

#ifdef _WIN32

FilePath Options::urlopenerPath() const
{
   FilePath parentDir = scriptsPath();

   // detect dev configuration
   if (parentDir.getFilename() == "desktop")
      parentDir = parentDir.completePath("urlopener");

   return parentDir.completePath("urlopener.exe");
}

FilePath Options::rsinversePath() const
{
   FilePath parentDir = scriptsPath();

   // detect dev configuration
   if (parentDir.getFilename() == "desktop")
      parentDir = parentDir.completePath("synctex/rsinverse");

   return parentDir.completePath("rsinverse.exe");
}

#endif

QStringList Options::ignoredUpdateVersions() const
{
   return settings_.value(QString::fromUtf8("ignoredUpdateVersions"), QStringList()).toStringList();
}

void Options::setIgnoredUpdateVersions(const QStringList& ignoredVersions)
{
   settings_.setValue(QString::fromUtf8("ignoredUpdateVersions"), ignoredVersions);
}

core::FilePath Options::scratchTempDir(core::FilePath defaultPath)
{
   core::FilePath dir(scratchPath.toUtf8().constData());

   if (!dir.isEmpty() && dir.exists())
   {
      dir = dir.completeChildPath("tmp");
      core::Error error = dir.ensureDirectory();
      if (!error)
         return dir;
   }
   return defaultPath;
}

void Options::cleanUpScratchTempDir()
{
   core::FilePath temp = scratchTempDir(core::FilePath());
   if (!temp.isEmpty())
      temp.removeIfExists();
}

QString Options::lastRemoteSessionUrl(const QString& serverUrl)
{
   settings_.beginGroup(QString::fromUtf8("remote-sessions-list"));
   QString sessionUrl = settings_.value(serverUrl).toString();
   settings_.endGroup();
   return sessionUrl;
}

void Options::setLastRemoteSessionUrl(const QString& serverUrl, const QString& sessionUrl)
{
   settings_.beginGroup(QString::fromUtf8("remote-sessions-list"));
   settings_.setValue(serverUrl, sessionUrl);
   settings_.endGroup();
}

QList<QNetworkCookie> Options::cookiesFromList(const QStringList& cookieStrs) const
{
   QList<QNetworkCookie> cookies;

   for (const QString& cookieStr : cookieStrs)
   {
      QByteArray cookieArr = QByteArray::fromStdString(cookieStr.toStdString());
      QList<QNetworkCookie> innerCookies = QNetworkCookie::parseCookies(cookieArr);
      for (const QNetworkCookie& cookie : innerCookies)
      {
         cookies.push_back(cookie);
      }
   }

   return cookies;
}

QList<QNetworkCookie> Options::authCookies() const
{
   QStringList cookieStrs = settings_.value(QString::fromUtf8("cookies"), QStringList()).toStringList();
   return cookiesFromList(cookieStrs);
}

QList<QNetworkCookie> Options::tempAuthCookies() const
{
   QStringList cookieStrs = settings_.value(QString::fromUtf8("temp-auth-cookies"), QStringList()).toStringList();
   return cookiesFromList(cookieStrs);
}

QStringList Options::cookiesToList(const QList<QNetworkCookie>& cookies) const
{
   QStringList cookieStrs;
   for (const QNetworkCookie& cookie : cookies)
   {
      cookieStrs.push_back(QString::fromStdString(cookie.toRawForm().toStdString()));
   }

   return cookieStrs;
}

void Options::setAuthCookies(const QList<QNetworkCookie>& cookies)
{
   settings_.setValue(QString::fromUtf8("cookies"), cookiesToList(cookies));
}

void Options::setTempAuthCookies(const QList<QNetworkCookie>& cookies)
{
   settings_.setValue(QString::fromUtf8("temp-auth-cookies"), cookiesToList(cookies));
}

} // namespace desktop
} // namespace rstudio

