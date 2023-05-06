/*
 * CopilotServerOperations.java
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
package org.rstudio.studio.client.workbench.copilot.server;

import org.rstudio.studio.client.server.ServerRequestCallback;
import org.rstudio.studio.client.workbench.copilot.model.CopilotResponse.CopilotCodeCompletionResponse;
import org.rstudio.studio.client.workbench.copilot.model.CopilotResponse.CopilotInstallAgentResponse;
import org.rstudio.studio.client.workbench.copilot.model.CopilotResponse.CopilotVerifyInstalledResponse;

public interface CopilotServerOperations
{
   public void copilotVerifyInstalled(ServerRequestCallback<CopilotVerifyInstalledResponse> requestCallback);
   
   public void copilotInstallAgent(ServerRequestCallback<CopilotInstallAgentResponse> requestCallback);
   
   public void copilotCodeCompletion(String documentId,
                                     int cursorRow,
                                     int cursorColumn,
                                     ServerRequestCallback<CopilotCodeCompletionResponse> requestCallback);
}
