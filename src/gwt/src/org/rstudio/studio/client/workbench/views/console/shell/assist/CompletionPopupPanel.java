/*
 * CompletionPopupPanel.java
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
package org.rstudio.studio.client.workbench.views.console.shell.assist;

import java.util.Map;

import com.google.gwt.event.dom.client.*;
import com.google.gwt.event.logical.shared.CloseEvent;
import com.google.gwt.event.logical.shared.CloseHandler;
import com.google.gwt.event.logical.shared.SelectionEvent;
import com.google.gwt.event.logical.shared.SelectionHandler;
import com.google.gwt.event.shared.HandlerRegistration;
import com.google.gwt.user.client.ui.HTML;
import com.google.gwt.user.client.ui.PopupPanel;

import org.rstudio.core.client.ElementIds;
import org.rstudio.core.client.Rectangle;
import org.rstudio.core.client.events.SelectionCommitEvent;
import org.rstudio.core.client.events.SelectionCommitHandler;
import org.rstudio.core.client.widget.ThemedPopupPanel;
import org.rstudio.studio.client.workbench.views.console.ConsoleResources;
import org.rstudio.studio.client.workbench.views.console.shell.assist.CompletionRequester.QualifiedName;
import org.rstudio.studio.client.workbench.views.help.model.HelpInfo.ParsedInfo;

public class CompletionPopupPanel extends ThemedPopupPanel
      implements CompletionPopupDisplay
{
   public CompletionPopupPanel()
   {
      super() ;
      styles_ = ConsoleResources.INSTANCE.consoleStyles();
      help_ = new HelpInfoPopupPanel();
      help_.setWidth("400px");
      help_.setHeight("200px");
      help_.setStylePrimaryName(styles_.helpPopup());
      help_.show();
      
      setStylePrimaryName(styles_.completionPopup()) ;
      
      hideAll();
      
      addCloseHandler(new CloseHandler<PopupPanel>() {
         
         @Override
         public void onClose(CloseEvent<PopupPanel> event)
         {
            hideAll();
         }
      });
   }
   
   private void hideAll()
   {
      // Throw everything off-screen to reduce flickering
      help_.setPopupPosition(-1000, -1000);
      setPopupPosition(-1000, -1000);
      help_.clearHelp(false);
      help_.setVisible(false);
   }

   public void showProgress(String progress, PositionCallback callback)
   {
      setText(progress) ;
      show(callback) ;
   }
   
   public void showErrorMessage(String error, PositionCallback callback)
   {
      setText(error) ;
      show(callback) ;
   }
   
   @Override
   public void clearCompletions()
   {
      list_ = null;
   }

   @Override
   public void showCompletionValues(QualifiedName[] values, 
                                    PositionCallback callback)
   {
      CompletionList<QualifiedName> list = new CompletionList<QualifiedName>(
                                       values,
                                       6,
                                       true,
                                       true) ;

      list.addSelectionCommitHandler(new SelectionCommitHandler<QualifiedName>() {
         public void onSelectionCommit(SelectionCommitEvent<QualifiedName> event)
         {
            SelectionCommitEvent.fire(CompletionPopupPanel.this, 
                                      event.getSelectedItem()) ;
         }
      });
      
      list.addSelectionHandler(new SelectionHandler<QualifiedName>() {
         public void onSelection(SelectionEvent<QualifiedName> event)
         {
            SelectionEvent.fire(CompletionPopupPanel.this, 
                                event.getSelectedItem()) ;
         }
      });
      
      list_ = list ;
      setWidget(list_);
      
      help_.setVisible(values.length > 0);
      
      ElementIds.assignElementId(list_.getElement(), 
            ElementIds.POPUP_COMPLETIONS);
      
      show(callback) ;
   }
   
   public boolean hasCompletions() {
      if (list_ == null)
         return false;
      return list_.getItemCount() > 0;
   }

   private void show(PositionCallback callback)
   {
      if (callback != null)
         setPopupPositionAndShow(callback) ;
      else
         show() ;
   }
   
   public QualifiedName getSelectedValue()
   {
      if (list_ == null || !list_.isAttached())
         return null ;
      
      return list_.getSelectedItem() ;
   }
   
   public Rectangle getSelectionRect()
   {
      return list_.getSelectionRect() ;
   }
   
   public boolean selectNext()
   {
      return list_.selectNext() ;
   }
   
   public boolean selectPrev()
   {
      return list_.selectPrev() ;
   }
   
   public boolean selectPrevPage()
   {
      return list_.selectPrevPage() ;
   }

   public boolean selectNextPage()
   {
      return list_.selectNextPage() ;
   }
   
   public boolean selectFirst()
   {
      return list_.selectFirst() ;
   }
   
   public boolean selectLast()
   {
      return list_.selectLast() ;
   }
   
   public void setHelpVisible(boolean visible)
   {
      help_.setVisible(visible);
   }

   @Override
   public void displayHelp(ParsedInfo help)
   {
      resolveHelpPosition(help.hasInfo());
      help_.displayHelp(help) ;
   }
   
   @Override
   public void displayParameterHelp(Map<String, String> map, String parameterName)
   {
      resolveHelpPosition(map.get(parameterName) != null);
      help_.displayParameterHelp(map, parameterName) ;
   }
   
   @Override
   public void displayPackageHelp(ParsedInfo help)
   {
      resolveHelpPosition(help.hasInfo());
      help_.displayPackageHelp(help) ;
   }
   
   private void resolveHelpPosition(boolean setVisible)
   {
      int top = getAbsoluteTop();
      int left = getAbsoluteLeft();
      int width = getOffsetWidth();
      help_.setPopupPosition(left + width - 6, top + 3);
      help_.setVisible(setVisible);
   }
   
   @Override
   public void displayDataHelp(ParsedInfo help)
   {
      displayPackageHelp(help);
   }

   public void clearHelp(boolean downloadOperationPending)
   {
      help_.clearHelp(downloadOperationPending) ;
   }

   public HandlerRegistration addSelectionHandler(
         SelectionHandler<QualifiedName> handler)
   {
      return addHandler(handler, SelectionEvent.getType()) ;
   }

   public HandlerRegistration addSelectionCommitHandler(
         SelectionCommitHandler<QualifiedName> handler)
   {
      return addHandler(handler, SelectionCommitEvent.getType()) ;
   }

   public HandlerRegistration addMouseDownHandler(MouseDownHandler handler)
   {
      return addDomHandler(handler, MouseDownEvent.getType()) ;
   }

   private HTML setText(String text)
   {
      HTML contents = new HTML() ;
      contents.setText(text) ;
      setWidget(contents) ;
      return contents ;
   }
   
   private CompletionList<QualifiedName> list_ ;
   private HelpInfoPopupPanel help_ ;
   private final ConsoleResources.ConsoleStyles styles_;
}
