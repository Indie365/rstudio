/*
 * user.ts
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

import os from 'os';

import { getenv } from './environment';
import { FilePath } from './file-path';

import desktop from '../native/desktop.node';

export function userHomePath(): FilePath {
  const user = getenv('R_USER');
  if (checkPath(user)) return new FilePath(user);
  const home = getenv('HOME');
  if (checkPath(home)) return new FilePath(home);
  if (process.platform === 'win32') {
    const currentHome = desktop.currentCSIDLPersonalHomePath();
    if (checkPath(currentHome)) return new FilePath(currentHome);
    const defaultHome = desktop.defaultCSIDLPersonalHomePath();
    if (checkPath(defaultHome)) return new FilePath(defaultHome);
  }
  return new FilePath(os.homedir());
}

export function userHomePathString(): string {
  return userHomePath().getAbsolutePath();
}

export function username(): string {
  try {
    return os.userInfo().username;
  } catch (err: unknown) {
    return '';
  }
}

function checkPath(path: string): boolean {
  if (path === '') return false;
  const fp = new FilePath(removeTrailingSlashes(path));
  return fp.existsSync();
}

// This seems to be a false positive from eslint ¯\_(ツ)_/¯
// eslint-disable-next-line no-useless-escape
const TRAILING_SLASH_REGEX = /[\\\/]+$/;

function removeTrailingSlashes(pathString: string): string {
  const trailingSlashes = pathString.match(TRAILING_SLASH_REGEX);
  if (trailingSlashes && trailingSlashes.length > 0) {
    const slashStartIndex = pathString.lastIndexOf(trailingSlashes[0]);
    return pathString.substring(0, slashStartIndex);
  }
  return pathString;
}
