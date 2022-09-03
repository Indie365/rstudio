/*
 * main-window.ts
 *
 * Copyright (C) 2022 by RStudio, PBC
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

import { ChildProcess } from 'child_process';
import { BrowserWindow, dialog, Menu, session } from 'electron';

import { Err } from '../core/err';
import { logger } from '../core/logger';

import i18next from 'i18next';
import { setDockLabel } from '../native/dock.node';
import { appState } from './app-state';
import { ApplicationLaunch, LaunchRStudioOptions } from './application-launch';
import { DesktopBrowserWindow } from './desktop-browser-window';
import { GwtCallback, PendingQuit } from './gwt-callback';
import { GwtWindow } from './gwt-window';
import { MenuCallback, showPlaceholderMenu } from './menu-callback';
import { ElectronDesktopOptions } from './preferences/electron-desktop-options';
import { RCommandEvaluator } from './r-command-evaluator';
import { RemoteDesktopSessionLauncher } from './remote-desktop-session-launcher-overlay';
import { SessionLauncher } from './session-launcher';
import { CloseServerSessions } from './session-servers-overlay';
import { waitForUrlWithTimeout } from './url-utils';

export function closeAllSatellites(mainWindow: BrowserWindow): void {
  const topLevels = BrowserWindow.getAllWindows();
  for (const win of topLevels) {
    if (win !== mainWindow) {
      win.close();
    }
  }
}

// This allows TypeScript to pick up the magic constants auto-generated by Forge's Webpack
// plugin that tells the Electron app where to look for the Webpack-bundled app code (depending on
// whether you're running in development or production).
declare const LOADING_WINDOW_WEBPACK_ENTRY: string;
declare const CONNECT_WINDOW_WEBPACK_ENTRY: string;

// number of times we've tried to reload in startup
let reloadCount = 0;

// amount of time to wait before each reload, in milliseconds
const reloadWaitDuration = 200;

export class MainWindow extends GwtWindow {
  static FIRST_WORKBENCH_INITIALIZED = 'main-window-first_workbench_initialized';
  static URL_CHANGED = 'main-window-url_changed';

  sessionLauncher?: SessionLauncher;
  remoteSessionLauncher?: RemoteDesktopSessionLauncher;
  appLauncher?: ApplicationLaunch;
  menuCallback: MenuCallback;
  quitConfirmed = false;
  geometrySaved = false;
  workbenchInitialized = false;

  private sessionProcess?: ChildProcess;
  private isErrorDisplayed = false;

  // if loading fails and emits `did-fail-load` it will be followed by a
  // 'did-finish-load'; use this bool to differentiate
  private didMainFrameLoadSuccessfully = false;

  // TODO
  //#ifdef _WIN32
  // HWINEVENTHOOK eventHook_ = nullptr;
  //#endif

  constructor(url: string, public isRemoteDesktop = false) {
    super({
      name: '',
      baseUrl: url,
      allowExternalNavigate: isRemoteDesktop,
      addApiKeys: ['desktop', 'desktopMenuCallback'],
    });

    appState().gwtCallback = new GwtCallback(this, isRemoteDesktop);
    this.menuCallback = new MenuCallback();

    RCommandEvaluator.setMainWindow(this);

    if (this.isRemoteDesktop) {
      // TODO - determine if we need to replicate this
      // since the object registration is asynchronous, during the GWT setup code
      // there is a race condition where the initialization can happen before the
      // remoteDesktop object is registered, making the GWT application think that
      // it should use regular desktop objects - to circumvent this, we use a custom
      // user agent string that the GWT code can detect with 100% success rate to
      // get around this race condition
      // QString userAgent = webPage()->profile()->httpUserAgent().append(QStringLiteral("; RStudio Remote Desktop"));
      // webPage()->profile()->setHttpUserAgent(userAgent);
      // channel->registerObject(QStringLiteral("remoteDesktop"), &gwtCallback_);
    }

    showPlaceholderMenu();

    this.menuCallback.on(MenuCallback.MENUBAR_COMPLETED, (menu: Menu) => {
      Menu.setApplicationMenu(menu);
    });
    this.menuCallback.on(MenuCallback.COMMAND_INVOKED, (commandId) => {
      this.invokeCommand(commandId);
    });

    // TODO -- see comment in menu-callback.ts about: "probably need to not use the roles here"
    // connect(&menuCallback_, SIGNAL(zoomActualSize()), this, SLOT(zoomActualSize()));
    // connect(&menuCallback_, SIGNAL(zoomIn()), this, SLOT(zoomIn()));
    // connect(&menuCallback_, SIGNAL(zoomOut()), this, SLOT(zoomOut()));

    appState().gwtCallback?.on(GwtCallback.WORKBENCH_INITIALIZED, () => {
      this.emit(MainWindow.FIRST_WORKBENCH_INITIALIZED);
    });
    appState().gwtCallback?.on(GwtCallback.WORKBENCH_INITIALIZED, () => {
      this.onWorkbenchInitialized();
    });
    appState().gwtCallback?.on(GwtCallback.SESSION_QUIT, () => {
      this.onSessionQuit();
    });

    this.on(DesktopBrowserWindow.CLOSE_WINDOW_SHORTCUT, this.onCloseWindowShortcut.bind(this));

    // connect(webView(), &WebView::urlChanged,
    //         this, &MainWindow::onUrlChanged);

    this.window.webContents.on('did-start-loading', () => {
      logger().logDebug('did-start-loading');
    });

    this.window.webContents.on('did-start-navigation', (event, url, isInPlace, isMainFrame) => {
      logger().logDebug(`did-start-navigation: ${url} [${isMainFrame ? 'main frame' : 'sub frame'}]`);

      // If we're about to attempt navigation in the main frame, unset
      // the loaded successfully flag and then let it get reset after
      // the following success / failure.
      if (isMainFrame) {
        this.didMainFrameLoadSuccessfully = false;
      }
    });

    // NOTE: This callback will be executed when sub-frames (e.g. those in the
    // Viewer pane) fail to navigate as well, so we need to keep track of
    // whether the main frame succeeded or failed to load and ignore
    // 'did-fail-load' events that come after a successful load.
    this.window.webContents.on('did-fail-load', () => {
      logger().logDebug('did-fail-load');
      if (!this.didMainFrameLoadSuccessfully) {
        this.onLoadFinished(false);
      }
    });

    this.window.webContents.on('did-finish-load', () => {
      logger().logDebug('did-finish-load');
      this.didMainFrameLoadSuccessfully = true;
      this.menuCallback.cleanUpActions();
      this.onLoadFinished(true);
    });

    // connect(&desktopInfo(), &DesktopInfo::fixedWidthFontListChanged, [this]() {
    //    QString js = QStringLiteral(
    //       "if (typeof window.onFontListReady === 'function') window.onFontListReady()");
    //    this->webPage()->runJavaScript(js);
    // });

    // connect(qApp, SIGNAL(commitDataRequest(QSessionManager&)),
    //         this, SLOT(commitDataRequest(QSessionManager&)),
    //         Qt::DirectConnection);

    // setWindowIcon(QIcon(QString::fromUtf8(":/icons/RStudio.ico")));
    this.window.setTitle(appState().activation().editionName());

    // Error error = pLauncher_->initialize();
    // if (error) {
    //   LOG_ERROR(error);
    //   showError(nullptr,
    //             QStringLiteral("Initialization error"),
    //             QStringLiteral("Could not initialize Job Launcher"),
    //             QString());
    //   ::_exit(EXIT_FAILURE);
    // }
  }

  launchSession(reload: boolean): void {
    // we're about to start another session, so clear the workbench init flag
    // (will get set again once the new session has initialized the workbench)
    this.workbenchInitialized = false;

    const error = this.sessionLauncher?.launchNextSession(reload);
    if (error) {
      logger().logError(error);

      dialog.showMessageBoxSync(this.window, {
        message: i18next.t('mainWindowTs.rSessionFailedToStart'),
        type: 'error',
        title: appState().activation().editionName(),
      });
      this.quit();
    }
  }

  launchRStudio(options: LaunchRStudioOptions): void {
    this.appLauncher?.launchRStudio(options);
  }

  saveRemoteAuthCookies(): void {
    // TODO
  }

  launchRemoteRStudio(): void {
    // TODO
  }

  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  launchRemoteRStudioProject(projectUrl: string): void {
    // TODO
  }

  onWorkbenchInitialized(): void {
    // reset state (in case this occurred in response to a manual reload
    // or reload for a new project context)
    this.quitConfirmed = false;
    this.geometrySaved = false;
    this.workbenchInitialized = true;

    this.executeJavaScript('window.desktopHooks.getActiveProjectDir()')
      .then((projectDir) => {
        if (projectDir.length > 0) {
          this.window.setTitle(`${projectDir} - ${appState().activation().editionName()}`);
          setDockLabel(projectDir);
        } else {
          this.window.setTitle(appState().activation().editionName());
          setDockLabel('');
        }
        this.avoidMoveCursorIfNecessary();
      })
      .catch((error) => {
        logger().logError(error);
      });
  }

  // TODO - REVIEW
  // https://github.com/electron/electron/issues/9613
  // https://github.com/electron/electron/issues/8762
  // this notification occurs when windows or X11 is shutting
  // down -- in this case we want to be a good citizen and just
  // exit right away so we notify the gwt callback that a legit
  // quit and exit is on the way and we set the quitConfirmed_
  // flag so no prompting occurs (note that source documents
  // have already been auto-saved so will be restored next time
  // the current project context is opened)
  // commitDataRequest(QSessionManager &manager) {
  //   gwtCallback_.setPendingQuit(PendingQuitAndExit);
  //   quitConfirmed_ = true;
  // }

  async loadUrl(url: string): Promise<void> {
    // pass along the shared secret with every request
    const filter = {
      urls: [`${url}/*`],
    };
    session.defaultSession.webRequest.onBeforeSendHeaders(filter, (details, callback) => {
      details.requestHeaders['X-Shared-Secret'] = process.env.RS_SHARED_SECRET ?? '';
      callback({ requestHeaders: details.requestHeaders });
    });

    logger().logDebug(`Setting base URL: ${url}`);
    this.options.baseUrl = url;

    this.window.loadURL(url).catch((reason) => {
      logger().logErrorMessage(`Failed to load ${url}: ${reason}`);
    });
  }

  quit(): void {
    RCommandEvaluator.setMainWindow(null);
    this.quitConfirmed = true;
    this.window.close();
  }

  invokeCommand(cmdId: string): void {
    let cmd = '';
    if (process.platform === 'darwin') {
      cmd = `
        var wnd;
        try {
          wnd = window.$RStudio.last_focused_window;
        } catch (e) {
          wnd = window;
        }
        (wnd || window).desktopHooks.invokeCommand('${cmdId}');`;
    } else {
      cmd = `window.desktopHooks.invokeCommand("${cmdId}")`;
    }
    this.executeJavaScript(cmd).catch((error) => {
      logger().logError(error);
    });
  }

  onSessionQuit(): void {
    if (this.isRemoteDesktop) {
      const pendingQuit = this.collectPendingQuitRequest();
      if (pendingQuit === PendingQuit.PendingQuitAndExit || this.quitConfirmed) {
        closeAllSatellites(this.window);
        this.quit();
      }
    }
  }

  setSessionProcess(sessionProcess: ChildProcess | undefined): void {
    this.sessionProcess = sessionProcess;

    // TODO implement Win32 eventHook
    // when R creates dialogs (e.g. through utils::askYesNo), their first
    // invocation might show behind the RStudio window. this allows us
    // to detect when those Windows are opened and focused, and raise them
    // to the front.
    // if (process.platform === 'win32') {
    //   if (eventHook_) {
    //     ::UnhookWinEvent(eventHook_);
    //   }

    //   if (this.sessionProcess) {
    //     eventHook_ = ::SetWinEventHook(
    //       EVENT_SYSTEM_DIALOGSTART, EVENT_SYSTEM_DIALOGSTART,
    //       nullptr,
    //       onDialogStart,
    //       pSessionProcess -> processId(),
    //       0,
    //       WINEVENT_OUTOFCONTEXT);
    //   }
    // }
  }

  closeEvent(event: Electron.Event): void {
    if (process.platform === 'win32') {
      // TODO
      // if (eventHook_) {
      // :: UnhookWinEvent(eventHook_);
      // }
    }

    // desktopInfo().onClose();
    // saveRemoteAuthCookies(boost::bind(&Options::authCookies, &options()),
    //                      boost::bind(&Options::setAuthCookies, &options(), _1),
    //                      false);

    if (!this.geometrySaved) {
      const bounds = this.window.getBounds();
      ElectronDesktopOptions().saveWindowBounds(bounds);
      this.geometrySaved = true;
    }

    const close: CloseServerSessions = 'Always'; // TODO sessionServerSettings().closeServerSessionsOnExit();

    if (
      this.quitConfirmed ||
      (!this.isRemoteDesktop && !this.sessionProcess) ||
      (!this.isRemoteDesktop && (!this.sessionProcess || this.sessionProcess.exitCode !== null))
    ) {
      closeAllSatellites(this.window);
      return;
    }

    const quit = () => {
      closeAllSatellites(this.window);
      this.quit();
    };

    event.preventDefault();
    this.executeJavaScript('!!window.desktopHooks')
      .then((hasQuitR: boolean) => {
        if (!hasQuitR) {
          logger().logErrorMessage('Main window closed unexpectedly');

          // exit to avoid user having to kill/force-close the application
          quit();
        } else {
          // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
          if (!this.isRemoteDesktop || close === 'Always') {
            this.executeJavaScript('window.desktopHooks.quitR()')
              .then(() => (this.quitConfirmed = true))
              .catch(logger().logError);
          } else if (close === 'Never') {
            quit();
          } else {
            this.executeJavaScript('window.desktopHooks.promptToQuitR()')
              .then(() => (this.quitConfirmed = true))
              .catch(logger().logError);
          }
        }
      })
      .catch(logger().logError);
  }

  collectPendingQuitRequest(): PendingQuit {
    return appState().gwtCallback?.collectPendingQuitRequest() ?? PendingQuit.PendingQuitNone;
  }

  onActivated(): void {
    // intentionally left blank
  }

  reload(): void {
    if (this.isErrorDisplayed) {
      return;
    }
    reloadCount++;
    this.loadUrl(this.options.baseUrl ?? '').catch(logger().logError);
  }

  onLoadFinished(ok: boolean): void {
    if (ok) {
      // we've successfully loaded!
    } else if (this.isErrorDisplayed) {
      // the session failed to launch and we're already showing
      // an error page to the user; nothing else to do here.
    } else {
      if (reloadCount === 0) {
        // the load failed, but we haven't yet received word that the
        // session has failed to load. let the user know that the R
        // session is still initializing, and then reload the page.
        this.loadUrl(LOADING_WINDOW_WEBPACK_ENTRY).catch(logger().logError);
        waitForUrlWithTimeout(this.options.baseUrl ?? '', reloadWaitDuration, reloadWaitDuration, 10)
          .then((error: Err) => {
            if (error) {
              logger().logError(error);
            }
          })
          .catch((error) => {
            logger().logError(error);
          })
          .finally(() => {
            this.reload();
          });
      } else {
        reloadCount = 0;
        this.onLoadFailed();
      }
    }
  }

  onLoadFailed(): void {
    if (this.remoteSessionLauncher || this.isErrorDisplayed) {
      return;
    }

    const vars = new Map<string, string>();
    vars.set('retry_url', this.options.baseUrl ?? '');
    appState().gwtCallback?.setErrorPageInfo(vars);
    this.loadUrl(CONNECT_WINDOW_WEBPACK_ENTRY).catch(logger().logError);
  }

  setErrorDisplayed(): void {
    this.isErrorDisplayed = true;
  }
}
