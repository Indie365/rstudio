/*
 * DataTable.java
 *
 * Copyright (C) 2009-15 by RStudio, Inc.
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

package org.rstudio.studio.client.dataviewer;

import java.util.ArrayList;

import org.rstudio.core.client.dom.IFrameElementEx;
import org.rstudio.core.client.dom.WindowEx;
import org.rstudio.core.client.widget.LatchingToolbarButton;
import org.rstudio.core.client.widget.RStudioFrame;
import org.rstudio.core.client.widget.SearchWidget;
import org.rstudio.core.client.widget.Toolbar;

import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.event.logical.shared.ValueChangeEvent;
import com.google.gwt.event.logical.shared.ValueChangeHandler;
import com.google.gwt.user.client.ui.SuggestOracle;

public class DataTable
{
   public interface Host 
   {
      RStudioFrame getDataTableFrame();
   }

   public DataTable(Host host)
   {
      host_ = host;
   }
   
   public void initToolbar(Toolbar toolbar)
   {
      filterButton_ = new LatchingToolbarButton(
              "Filter",
              DataViewerResources.INSTANCE.filterIcon(),
              new ClickHandler() {
                 public void onClick(ClickEvent event)
                 {
                    filtered_ = !filtered_;
                    setFilterUIVisible(filtered_);
                 }
              });
      toolbar.addLeftWidget(filterButton_);

      searchWidget_ = new SearchWidget(new SuggestOracle() {
         @Override
         public void requestSuggestions(Request request, Callback callback)
         {
            // no suggestions
            callback.onSuggestionsReady(
                  request,
                  new Response(new ArrayList<Suggestion>()));
         }
      });
      searchWidget_.addValueChangeHandler(new ValueChangeHandler<String>() {
         @Override
         public void onValueChange(ValueChangeEvent<String> event)
         {
            applySearch(getWindow(), event.getValue());
         }
      });

      toolbar.addRightWidget(searchWidget_);
   }
   
   private WindowEx getWindow()
   {
      IFrameElementEx frameEl = (IFrameElementEx) host_.getDataTableFrame().getElement().cast();
      return frameEl.getContentWindow();
   }

   public void setFilterUIVisible(boolean visible)
   {
      setFilterUIVisible(getWindow(), visible);
   }
   
   public void refreshData(boolean structureChanged, boolean sizeChanged)
   {
      // if the structure of the data changed, the old search/filter data is
      // discarded, as it may no longer be applicable to the data's new shape.
      if (structureChanged)
      {
         filtered_= false;
         if (searchWidget_ != null)
            searchWidget_.setText("", false);
         if (filterButton_ != null)
            filterButton_.setLatched(false);
      }

      refreshData(getWindow(), structureChanged, sizeChanged);
   }
   
   public void applySizeChange()
   {
      applySizeChange(getWindow());
   }
   
   private static final native void setFilterUIVisible (WindowEx frame, boolean visible) /*-{
      if (frame && frame.setFilterUIVisible)
         frame.setFilterUIVisible(visible);
   }-*/;
   
   private static final native void refreshData(WindowEx frame, 
         boolean structureChanged,
         boolean sizeChanged) /*-{
      if (frame && frame.refreshData)
         frame.refreshData(structureChanged, sizeChanged);
   }-*/;

   private static final native void applySearch(WindowEx frame, String text) /*-{
      if (frame && frame.applySearch)
         frame.applySearch(text);
   }-*/;
   
   private static final native void applySizeChange(WindowEx frame) /*-{
      if (frame && frame.applySizeChange)
         frame.applySizeChange();
   }-*/;

   private Host host_;
   private LatchingToolbarButton filterButton_;
   private SearchWidget searchWidget_;
   private boolean filtered_ = false;
}