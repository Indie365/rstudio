/*
 * CppCompletionRequest.java
 *
 * Copyright (C) 2009-12 by RStudio, Inc.
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

package org.rstudio.studio.client.workbench.views.source.editors.text.cpp;

import org.rstudio.core.client.CommandWithArg;
import org.rstudio.core.client.Invalidation;
import org.rstudio.studio.client.RStudioGinjector;
import org.rstudio.studio.client.server.ServerError;
import org.rstudio.studio.client.server.ServerRequestCallback;
import org.rstudio.studio.client.workbench.prefs.model.UIPrefs;
import org.rstudio.studio.client.workbench.views.console.shell.editor.InputEditorSelection;
import org.rstudio.studio.client.workbench.views.source.editors.text.DocDisplay;
import org.rstudio.studio.client.workbench.views.source.editors.text.ace.Position;
import org.rstudio.studio.client.workbench.views.source.editors.text.ace.Range;
import org.rstudio.studio.client.workbench.views.source.model.CppCompletion;
import org.rstudio.studio.client.workbench.views.source.model.CppCompletionResult;
import org.rstudio.studio.client.workbench.views.source.model.CppServerOperations;

import com.google.gwt.core.client.JsArray;
import com.google.gwt.core.client.Scheduler;
import com.google.gwt.core.client.Scheduler.ScheduledCommand;
import com.google.gwt.event.logical.shared.CloseEvent;
import com.google.gwt.event.logical.shared.CloseHandler;
import com.google.gwt.user.client.ui.PopupPanel;
import com.google.inject.Inject;

public class CppCompletionRequest 
                           extends ServerRequestCallback<CppCompletionResult>
{
   public CppCompletionRequest(String docPath,
                               CompletionPosition completionPosition,
                               DocDisplay docDisplay, 
                               Invalidation.Token token,
                               boolean explicit)
   {
      RStudioGinjector.INSTANCE.injectMembers(this);
      
      docDisplay_ = docDisplay;
      completionPosition_ = completionPosition;
      invalidationToken_ = token;
      explicit_ = explicit;
      
      Position pos = completionPosition_.getPosition();
      
      server_.getCppCompletions(docPath, 
                                pos.getRow() + 1, 
                                pos.getColumn() + 1, 
                                completionPosition_.getUserText(),
                                this);
   }

   @Inject
   void initialize(CppServerOperations server, UIPrefs uiPrefs)
   {
      server_ = server;
      uiPrefs_ = uiPrefs;
   }
   
   public boolean isExplicit()
   {
      return explicit_;
   }
   
   public CompletionPosition getCompletionPosition()
   {
      return completionPosition_;
   }
   
   public CppCompletionPopupMenu getCompletionPopup()
   {
      return popup_;
   }
     
   public void updateUI(boolean autoAccept)
   {
      if (invalidationToken_.isInvalid())
         return;
      
      // if we don't have the completion list back from the server yet
      // then just ignore this (this function will get called again when
      // the request completes)
      if (completions_ != null)
      {  
         // discover text already entered
         String userTypedText = getUserTypedText();
         
         // build list of entries (filter on text already entered)
         JsArray<CppCompletion> filtered = JsArray.createArray().cast();
         for (int i = 0; i < completions_.length(); i++)
         {
            CppCompletion completion = completions_.get(i);
            String typedText = completion.getTypedText();
            if ((userTypedText.length() == 0) || 
                 typedText.startsWith(userTypedText))
            {
               // be more picky for member scope completions because clang
               // returns a bunch of noise like constructors, destructors, 
               // compiler generated assignments, etc.
               if (completionPosition_.getScope() == 
                                 CompletionPosition.Scope.Member)
               {
                  if (completion.getType() == CppCompletion.VARIABLE ||
                      (completion.getType() == CppCompletion.FUNCTION &&
                       !typedText.startsWith("operator=")))
                  {
                     filtered.push(completion);
                  }
                 
               }
               else
               {
                  filtered.push(completion);
               }
            }
         }
         
         // check for auto-accept
         if ((filtered.length() == 1) && autoAccept && explicit_)
         {
            applyValue(filtered.get(0));
         }
         // check for one completion that's already present
         else if (filtered.length() == 1 && 
                  filtered.get(0).getTypedText() == getUserTypedText())
         {
            terminate();
         }
         else
         {
            showCompletionPopup(filtered);
         }
      }
   }
   
   public void terminate()
   {
      closeCompletionPopup();
      terminated_ = true;
   }
   
   public boolean isTerminated()
   {
      return terminated_;
   }
   
   @Override
   public void onResponseReceived(CppCompletionResult result)
   {
      if (invalidationToken_.isInvalid())
         return;
      
      // null result means that completion is not supported for this file
      if (result == null)
         return;    
       
      // get the completions
      completions_ = result.getCompletions();
      
      // check for none found condition on explicit completion
      if ((completions_.length() == 0) && explicit_)
      {
         showCompletionPopup("(No matches)");
      }
      // otherwise just update the ui (apply filter, etc.)
      else
      {
         updateUI(true);
      }
      
      // show diagnostics
      /*
      JsArray<CppDiagnostic> diagnostics = result.getDiagnostics();
      for (int i = 0; i < diagnostics.length(); i++)
         Debug.prettyPrint(diagnostics.get(i));
      */
   }
   
   private void showCompletionPopup(String message)
   {
      if (popup_ == null)
         popup_ = createCompletionPopup();
      popup_.setText(message);
     
   }
   
   private void showCompletionPopup(JsArray<CppCompletion> completions)
   {
      // clear any existing signature tips
      if (completions.length() > 0)
         CppCompletionSignatureTip.hideAll();
        
      if (popup_ == null)
         popup_ = createCompletionPopup();
      popup_.setCompletions(completions, new CommandWithArg<CppCompletion>() {
         @Override
         public void execute(CppCompletion completion)
         {
            applyValue(completion);
         } 
      });
   }
      
   
   private CppCompletionPopupMenu createCompletionPopup()
   {
      CppCompletionPopupMenu popup = new CppCompletionPopupMenu(
          docDisplay_, completionPosition_);
      
      popup.addCloseHandler(new CloseHandler<PopupPanel>() {

         @Override
         public void onClose(CloseEvent<PopupPanel> event)
         {
            popup_ = null; 
            terminated_ = true;
            Scheduler.get().scheduleDeferred(new ScheduledCommand()
            {
               @Override
               public void execute()
               {
                  docDisplay_.setPopupVisible(false);
               }
            });
         } 
      });
      
      return popup;
   }
   
   private void closeCompletionPopup()
   {
      if (popup_ != null)
      {
         popup_.hide();
         popup_ = null;
      }
   }
   
   @Override
   public void onError(ServerError error)
   {
      if (invalidationToken_.isInvalid())
         return;
      
      if (explicit_)
         showCompletionPopup(error.getUserMessage());
   }
   
   private void applyValue(CppCompletion completion)
   {
      if (invalidationToken_.isInvalid())
         return;
      
      terminate();
     
      String insertText = completion.getTypedText();
      if (completion.getType() == CppCompletion.FUNCTION &&
            uiPrefs_.insertParensAfterFunctionCompletion().getValue())
      {
         if (uiPrefs_.insertMatching().getValue())
            insertText = insertText + "()";
         else
            insertText = insertText + "(";
      }
      
      docDisplay_.setFocus(true); 
      docDisplay_.setSelection(getReplacementSelection());
      docDisplay_.replaceSelection(insertText, true);
      
      if (completion.hasParameters() &&
            uiPrefs_.insertParensAfterFunctionCompletion().getValue() &&
            uiPrefs_.insertMatching().getValue())
      {
         Position pos = docDisplay_.getCursorPosition();
         pos = Position.create(pos.getRow(), pos.getColumn() - 1);
         docDisplay_.setSelectionRange(Range.fromPoints(pos, pos));
      }
      
      if (completion.hasParameters() &&
          uiPrefs_.showSignatureTooltips().getValue())
      {
         new CppCompletionSignatureTip(completion, docDisplay_);
      }
   }
   
   private InputEditorSelection getReplacementSelection()
   {
      return docDisplay_.createSelection(completionPosition_.getPosition(), 
                                         docDisplay_.getCursorPosition());
   }
   
   private String getUserTypedText()
   {
      return docDisplay_.getCode(
        completionPosition_.getPosition(), docDisplay_.getCursorPosition());
   }
   
   private CppServerOperations server_;
   private UIPrefs uiPrefs_;
  
   private final DocDisplay docDisplay_; 
   private final boolean explicit_;
   private final Invalidation.Token invalidationToken_;
   
   private final CompletionPosition completionPosition_;
   
   private CppCompletionPopupMenu popup_;
   private JsArray<CppCompletion> completions_;
   
   private boolean terminated_ = false;
}
