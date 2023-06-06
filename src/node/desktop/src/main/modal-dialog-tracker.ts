/*
 * modal-dialog-tracker.ts
 *
 * Copyright (C) 2023 by Posit Software, PBC
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

import { ModalDialog } from '../ui/modal-dialog';
import { appState } from './app-state';

/**
 * Track modals that extend ModalDialog (src/node/desktop/src/ui/modal-dialog.ts)
 * Based on src/gwt/src/org/rstudio/core/client/widget/ModalDialogTracker.java
 */
export class ModalDialogTracker {
  private modals: ModalDialog<any>[] = [];
  private electronModalsShowing = 0;
  private gwtModalsShowing = 0;

  public async addModal(modal: ModalDialog<any>) {
    this.modals.push(modal);
    appState().gwtCallback?.mainWindow.menuCallback.setMainMenuEnabled(false);
  }

  public async removeModal(modal: ModalDialog<any>) {
    this.modals = this.modals.filter((m) => m !== modal);
    this.maybeReenableMainMenu();
  }

  public numModalsShowing(): number {
    return this.modals.length + this.gwtModalsShowing + this.electronModalsShowing;
  }

  public maybeReenableMainMenu() {
    if (this.numModalsShowing() === 0) {
      appState().gwtCallback?.mainWindow.menuCallback.setMainMenuEnabled(true);
    }
  }

  public setNumGwtModalsShowing(gwtModalsShowing: number) {
    this.gwtModalsShowing = gwtModalsShowing;
  }

  public async trackElectronModalAsync<T>(func: () => Promise<T>): Promise<T> {
    this.addElectronModal();
    return func().finally(() => this.removeElectronModal());
  }

  public trackElectronModalSync<T>(func: () => T): T {
    this.addElectronModal();
    const retVal = func();
    this.removeElectronModal();
    return retVal;
  }

  private addElectronModal() {
    this.electronModalsShowing++;
    appState().gwtCallback?.mainWindow.menuCallback.setMainMenuEnabled(false);
  }

  private removeElectronModal() {
    if (this.electronModalsShowing > 0) this.electronModalsShowing--;
    this.maybeReenableMainMenu();
  }
}
