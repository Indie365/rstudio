/*
 * preload.ts
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
import { contextBridge, ipcRenderer } from 'electron';
import { existsSync } from 'fs';
import path from 'path';

// initialize select input
ipcRenderer.on('initialize', (event, data) => {

  // cast received data
  const rInstalls = data as string[];
  console.log(rInstalls);

  // get access to the select element
  const selectEl = document.getElementById('select') as HTMLSelectElement;

  // keep track of which R installations we've already visited,
  // in case the registry had multiple or duplicate copies
  const visitedInstallations: { [index: string]: boolean } = {};

  // sort so that newer versions are shown first
  rInstalls.sort((lhs, rhs) => {
    return rhs.localeCompare(lhs);
  });

  rInstalls.forEach(rInstall => {

    // normalize separators, etc
    rInstall = path.normalize(rInstall).replaceAll(/[/\\]+$/g, '');

    // skip if we've already seen this
    if (visitedInstallations[rInstall]) {
      return;
    }
    visitedInstallations[rInstall] = true;

    // check for 64 bit executable
    const r64 = `${rInstall}/bin/x64/R.exe`;
    if (existsSync(r64)) {
      const optionEl = window.document.createElement('option');
      optionEl.value = r64;
      optionEl.innerText = `[64-bit] ${rInstall}`;
      selectEl.appendChild(optionEl);
    }

    // check for 32 bit executable
    const r32 = `${rInstall}/bin/i386/R.exe`;
    if (existsSync(r32)) {
      const optionEl = window.document.createElement('option');
      optionEl.value = r32;
      optionEl.innerText = `[32-bit] ${rInstall}`;
      selectEl.appendChild(optionEl);
    }

  });

});

// export callbacks
contextBridge.exposeInMainWorld('callbacks', {
  
  useDefault32bit: () => {
    ipcRenderer.send('use-default-32bit');
  },

  useDefault64bit: () => {
    ipcRenderer.send('use-default-64bit');
  },

  use: (version: string) => {
    ipcRenderer.send('use-custom', version);
  },

  cancel: () => {
    ipcRenderer.send('cancel');
  },

});

/*
import { BrowserWindow, ipcMain } from 'electron';
import { existsSync } from 'fs';
import path from 'path';
import { ModalWindow } from 'src/ui/modal';
import { logger } from '../../../core/logger';
import { findDefault32Bit, findDefault64Bit, findRInstallationsWin32 } from '../../../main/detect-r';

function buildHtmlContent(rInstalls: string[]): string {

}

export async function chooseRInstallation(): Promise<string> {

  const dialog = new ModalWindow();
  dialog.setWidth(400);
  dialog.setHeight(300);

  // find R installations, and generate the HTML using that
  const rInstalls = findRInstallationsWin32();
  const html = buildHtmlContent(rInstalls);

  // load the HTML
  await dialog.loadURL(`data:text/html;charset=utf-8,${html}`);

  // show the page
  dialog.show();


}
*/
