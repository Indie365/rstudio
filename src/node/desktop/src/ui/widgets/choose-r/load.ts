/*
 * load.ts
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

/* eslint-disable @typescript-eslint/no-implicit-any-catch */
// eslint-disable-next-line @typescript-eslint/no-unused-vars

import { Callbacks, CallbackData } from './preload';
import { initI18n } from '../../../main/i18n-manager';
import i18next from 'i18next';

import './styles.css';
import { logger } from '../../../core/logger';

declare global {
  interface Window {
    callbacks: Callbacks;
  }
}

// ensure that the custom select box is only enabled when the associated
// radio button is checked
const selectWidget = document.getElementById('select') as HTMLSelectElement;
const radioChooseCustom = document.getElementById('use-custom') as HTMLInputElement;
const radioButtons = document.querySelectorAll('input[type="radio"]');

selectWidget.disabled = !radioChooseCustom.checked;
radioButtons.forEach((radioButton) => {
  radioButton.addEventListener('click', () => {
    selectWidget.disabled = !radioChooseCustom.checked;
  });
});

// set up callbacks for OK + Cancel buttons
const buttonOk = document.getElementById('button-ok') as HTMLButtonElement;
const buttonCancel = document.getElementById('button-cancel') as HTMLButtonElement;
const buttonBrowse = document.getElementById('button-browse') as HTMLButtonElement;

// get reference to rendering engine select widget
const renderingEngineSelect = document.getElementById('rendering-engine') as HTMLSelectElement;

// helper function for building response
function callbackData(binaryPath?: string): CallbackData {
  return {
    binaryPath: binaryPath,
    renderingEngine: renderingEngineSelect.value,
  };
}

buttonOk.addEventListener('click', () => {
  const useDefault32Radio = document.getElementById('use-default-32') as HTMLInputElement;
  if (useDefault32Radio.checked) {
    window.callbacks.useDefault32bit(callbackData());
    window.close();
    return;
  }

  const useDefault64Radio = document.getElementById('use-default-64') as HTMLInputElement;
  if (useDefault64Radio.checked) {
    window.callbacks.useDefault64bit(callbackData());
    window.close();
    return;
  }

  const useCustomRadio = document.getElementById('use-custom') as HTMLInputElement;
  if (useCustomRadio.checked) {
    const selectWidget = document.getElementById('select') as HTMLSelectElement;
    const selection = selectWidget.value;
    window.callbacks.use(callbackData(selection));
    window.close();
  }
});

buttonCancel.addEventListener('click', () => {
  window.callbacks.cancel();
  window.close();
});

buttonBrowse.addEventListener('click', async () => {
  try {
    const shouldCloseDialog = await window.callbacks.browse(callbackData());
    if (shouldCloseDialog) {
      window.close();
    }
  } catch (err) {
    logger().logDebug(`Error occurred when trying to browse for R: ${err}`);
  }
});

const loadPageLocalization = () => {
  initI18n();

  window.addEventListener('load', () => {
    const i18nIds = [
      'chooseRInstallation',
      'rstudioRequiresAnExistingRInstallationTitle',
      'rstudioRequiresAnExistingRInstallationSubtitle',
      'useYourMachineDefault64BitR',
      'useYourMachineDefault32BitR',
      'chooseASpecificVersionOfR',
      'browseDots',
      'okCaps',
      'cancel',
      'customizeRenderingEngine',
      'renderingEngineLabel',
    ].map((id) => 'i18n-' + id);

    try {
      document.title = i18next.t('uiFolder.chooseRInstallation');

      i18nIds.forEach((id) => {
        const reducedId = id.replace('i18n-', '');

        switch (reducedId) {
          case 'browseDots':
            id = 'button-browse';
            break;
          case 'okCaps':
            id = 'button-ok';
            break;
          case 'cancel':
            id = 'button-cancel';
            break;
          default:
            break;
        }

        const elements = document.querySelectorAll(`[id=${id}]`);

        elements.forEach((element) => {
          element.innerHTML = i18next.t('uiFolder.' + reducedId);
        });
      });
    } catch (err) {
      console.log('Error occurred when loading i18n: ', err);
    }
  });
};

loadPageLocalization();
