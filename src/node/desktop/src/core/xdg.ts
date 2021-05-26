/*
 * xdg.ts
 *
 * Copyright (C) 2021 by RStudio, PBC
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

/*
 * These routines return system and user paths for RStudio configuration and data, roughly in
 * accordance with the FreeDesktop XDG Base Directory Specification.
 *
 * https://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
 *
 * All of these can be configured with environment variables as described below.
 *
 * The values of the environment variables can include the following special variables:
 *
 * $USER  The user's name
 * $HOME  The user's home directory
 * ~      The user's home directory
 *
 * These values will be resolved against the current user by default. If you wish to
 * resolve them against a different user, supply their name and home directory using
 * the boost::optional arguments.
 */

import os from 'os';
import { Environment, expandEnvVars, getenv, setenv } from './environment';
import { username, userHomePath } from './system';
import { FilePath } from './file-path';

export enum WinFolderID {
  FOLDERID_RoamingAppData,
  FOLDERID_LocalAppData,
  FOLDERID_ProgramData
}

/**
 * Simplified implementation of Windows `SHGetKnownFolderPath` API using environment
 * variables.
 */
export function SHGetKnownFolderPath(folderId: WinFolderID): string {
  let envVar = "";
  switch (folderId) {
    case WinFolderID.FOLDERID_RoamingAppData:
      envVar = "APPDATA";
      break;
    case WinFolderID.FOLDERID_LocalAppData:
      envVar = "LOCALAPPDATA";
      break;
    case WinFolderID.FOLDERID_ProgramData:
      envVar = "ProgramData";
      break;
  }
  return getenv(envVar);
}

/**
 * Returns the hostname from the operating system
 */
function getHostname(): string {
  if (!getHostname.hostname)
    getHostname.hostname = os.hostname();
  return getHostname.hostname;
}
namespace getHostname {
   // Use a static string to store the hostname so we don't have to look it up
   // multiple times
  export let hostname = "";
}

/**
 * Resolves an XDG directory based on the user and environment.
 *
 * `rstudioEnvVer` The RStudio-specific environment variable specifying
 *   the directory (given precedence)
 * 
 * `xdgEnvVar` The XDG standard environment variable
 * 
 * `defaultDir` Fallback default directory if neither environment variable
 *   is present
 * 
 * `windowsFolderId` The ID of the Windows folder to resolve against
 * 
 * `user` Optionally, the user to return a directory for; if omitted the
 *   current user is used
 * 
 * `homeDir` Optionally, the home directory to resolve against; if omitted
 *   the current user's home directory is used
 */
function resolveXdgDir(
  rstudioEnvVar: string,
  xdgEnvVar: string,
  windowsFolderId: WinFolderID,
  defaultDir: string,
  user?: string,
  homeDir?: FilePath
): FilePath {
  let xdgHome = new FilePath();
  let finalPath = true;

   // Look for the RStudio-specific environment variable
  let env = getenv(rstudioEnvVar);
  if (!env) {
    // The RStudio environment variable specifies the final path; if it isn't
    // set we will need to append "rstudio" to the path later.
    finalPath = false;
    env = getenv(xdgEnvVar);
  }

  if (!env) {
    // No root specified for xdg home; we will need to generate one.
    if (process.platform === 'win32') {
      // On Windows, the default path is in Application Data/Roaming.
      let path = SHGetKnownFolderPath(windowsFolderId);
      if (path) {
        xdgHome = new FilePath(path);
      } else {
        // TODO: LOG_ERROR_MESSAGE
        // console.error(`Unable to retrieve app settings path (${windowsFolderId}).`);
      }
    }
    if (xdgHome.isEmpty()) {
      // Use the default subdir for POSIX. We also use this folder as a fallback on Windows
      //if we couldn't read the app settings path.
      xdgHome = new FilePath(defaultDir);
    }
  } else {
    // We have a manually specified xdg directory from an environment variable.
    xdgHome = new FilePath(env);
  }

  // expand HOME, USER, and HOSTNAME if given
  let environment: Environment = {
    "HOME": homeDir ? homeDir.getAbsolutePath() : userHomePath().getAbsolutePath(),
    "USER": user ? user : username()
  };

  // check for manually specified hostname in environment variable
  let hostname = getenv("HOSTNAME");

  // when omitted, look up the hostname using a system call
  if (!hostname)
    hostname = getHostname();

  environment.HOSTNAME = hostname;

  const expanded = expandEnvVars(environment, xdgHome.getAbsolutePath());

  // resolve aliases in the path
  xdgHome = FilePath.resolveAliasedPathSync(expanded, homeDir ? homeDir : userHomePath());

  // If this is the final path, we can return it as-is
  if (finalPath)
    return xdgHome;

  // Otherwise, it's a root folder in which we need to create our own subfolder
  const folderName = process.platform === 'win32' ? 'RStudio' : 'rstudio';
  return xdgHome.completePath(folderName);
}

export class Xdg {

  /**
   * Returns the RStudio XDG user config directory.
   * 
   * On Unix-alikes, this is `~/.config/rstudio`, or `XDG_CONFIG_HOME`.
   * On Windows, this is `FOLDERID_RoamingAppData` (typically `AppData/Roaming`).
   */
  static userConfigDir(user?: string, homeDir?: FilePath) {
    return resolveXdgDir(
      "RSTUDIO_CONFIG_HOME",
      "XDG_CONFIG_HOME",
      WinFolderID.FOLDERID_RoamingAppData,
      "~/.config",
      user,
      homeDir
    );
  }

  /**
   * Returns the RStudio XDG user data directory.
   *
   * On Unix-alikes, this is `~/.local/share/rstudio`, or `XDG_DATA_HOME`.
   * On Windows, this is `FOLDERID_LocalAppData` (typically `AppData/Local`).
   */
  static userDataDir(user?: string, homeDir?: FilePath) {
    return resolveXdgDir(
      "RSTUDIO_DATA_HOME",
      "XDG_DATA_HOME",
      WinFolderID.FOLDERID_LocalAppData,
      "~/.local/share",
      user,
      homeDir
    );
  }

  /**
   * This function verifies that the `userConfigDir()` and `userDataDir()` exist and are
   * owned by the running user.
   * 
   * It should be invoked once. Any issues with these directories will be emitted to the
   * session log.
   */
  static verifyUserDirs(
    user?: string,
    homeDir?: FilePath
  ) {
    throw Error("Xdg.verifyUserDirs is NYI");
  }


  /**
   * Returns the RStudio XDG system config directory.
   *
   * On Unix-alikes, this is `/etc/rstudio`, `XDG_CONFIG_DIRS`.
   * On Windows, this is `FOLDERID_ProgramData` (typically `C:/ProgramData`).
   */
  static systemConfigDir(): FilePath {
    let result = "";
    if (process.platform !== 'win32') {
      if (!getenv("RSTUDIO_CONFIG_DIR")) {
        // On POSIX operating systems, it's possible to specify multiple config
        // directories. We have to select one, so read the list and take the first
        // one that contains an "rstudio" folder.
        const env = getenv("XDG_CONFIG_DIRS");
        if (env.indexOf(":") >= 0) {
          const dirs = env.split(":");
          for (let dir of dirs) {
            let resolved = new FilePath(dir).completePath("rstudio");
            if (resolved.existsSync()) {
              return resolved;
            }
          }
        }
      }
    }
    return resolveXdgDir(
      "RSTUDIO_CONFIG_DIR",
      "XDG_CONFIG_DIRS",
      WinFolderID.FOLDERID_ProgramData,
      "/etc",
      undefined,  // no specific user
      undefined   // no home folder resolution
    );
  }

  /**
   * Convenience method for finding a configuration file. Checks all the
   * directories in `XDG_CONFIG_DIRS` for the file. If it doesn't find it,
   * the path where we expected to find it is returned instead.
   */
  static systemConfigFile(filename: string): FilePath {
    if (process.platform === 'win32') {
      // Passthrough on Windows
      return Xdg.systemConfigDir().completeChildPath(filename);
    }
    if (!getenv("RSTUDIO_CONFIG_DIR")) {
      // On POSIX, check for a search path.
      const env = getenv("XDG_CONFIG_DIRS");
      if (env.indexOf(":") >= 0) {
        // This is a search path; check each element for the file.
        const dirs = env.split(":");
        for (let dir of dirs) {
          let resolved = new FilePath(dir).completePath("rstudio").completeChildPath(filename);
          if (resolved.existsSync()) {
            return resolved;
          }
        }
      }
    }

    // We didn't find the file on the search path, so return the location where
    // we expected to find it.
    return Xdg.systemConfigDir().completeChildPath(filename);
  }

  /**
   * Sets relevant XDG environment variables
   */
  static forwardXdgEnvVars(pEnvironment: Environment) {
    throw Error("forwardXdgEnvVars is NYI");
  }
}