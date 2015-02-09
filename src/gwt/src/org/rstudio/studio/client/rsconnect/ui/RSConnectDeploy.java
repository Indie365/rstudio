/*
 * RSConnectDeploy.java
 *
 * Copyright (C) 2009-14 by RStudio, Inc.
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
package org.rstudio.studio.client.rsconnect.ui;

import java.util.List;

import org.rstudio.core.client.StringUtil;
import org.rstudio.core.client.files.FileSystemItem;
import org.rstudio.core.client.widget.OperationWithInput;
import org.rstudio.studio.client.common.FilePathUtils;
import org.rstudio.studio.client.common.GlobalDisplay;
import org.rstudio.studio.client.rsconnect.model.RSConnectAccount;
import org.rstudio.studio.client.rsconnect.model.RSConnectApplicationInfo;
import org.rstudio.studio.client.rsconnect.model.RSConnectServerOperations;
import org.rstudio.studio.client.workbench.model.Session;

import com.google.gwt.core.client.GWT;
import com.google.gwt.core.client.JsArray;
import com.google.gwt.core.client.JsArrayString;
import com.google.gwt.event.dom.client.ChangeHandler;
import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.event.dom.client.KeyUpEvent;
import com.google.gwt.event.dom.client.KeyUpHandler;
import com.google.gwt.event.shared.HandlerRegistration;
import com.google.gwt.regexp.shared.RegExp;
import com.google.gwt.resources.client.ClientBundle;
import com.google.gwt.resources.client.CssResource;
import com.google.gwt.resources.client.ImageResource;
import com.google.gwt.uibinder.client.UiBinder;
import com.google.gwt.uibinder.client.UiField;
import com.google.gwt.user.client.Command;
import com.google.gwt.user.client.ui.Anchor;
import com.google.gwt.user.client.ui.Composite;
import com.google.gwt.user.client.ui.FlowPanel;
import com.google.gwt.user.client.ui.HTMLPanel;
import com.google.gwt.user.client.ui.InlineLabel;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.ListBox;
import com.google.gwt.user.client.ui.TextBox;
import com.google.gwt.user.client.ui.Widget;

public class RSConnectDeploy extends Composite
{
   private static RSConnectDeployUiBinder uiBinder = GWT
         .create(RSConnectDeployUiBinder.class);

   interface RSConnectDeployUiBinder extends UiBinder<Widget, RSConnectDeploy>
   {
   }
   
   public interface DeployStyle extends CssResource
   {
      String statusLabel();
      String normalStatus();
      String otherStatus();
      String launchCheck();
   }
   
   public interface DeployResources extends ClientBundle
   {
      @Source("DeployDialogIllustration.png")
      ImageResource deployIllustration();
   }

   public RSConnectDeploy(final RSConnectServerOperations server, 
                          final RSAccountConnector connector,    
                          final GlobalDisplay display,
                          final Session session)
   {
      accountList = new RSConnectAccountList(server, display, false);
      initWidget(uiBinder.createAndBindUi(this));

      // Validate the application name on every keystroke
      appName.addKeyUpHandler(new KeyUpHandler()
      {
         @Override
         public void onKeyUp(KeyUpEvent event)
         {
            validateAppName();
         }
      });
      // Invoke the "add account" wizard
      addAccountAnchor.addClickHandler(new ClickHandler()
      {
         @Override
         public void onClick(ClickEvent event)
         {
            connector.showAccountWizard(new OperationWithInput<Boolean>() 
            {
               @Override
               public void execute(Boolean successful)
               {
                  if (successful)
                  {
                     accountList.refreshAccountList();
                  }
               }
            });
            
            event.preventDefault();
            event.stopPropagation();
         }
      });
   }
   
   public void setSourceDir(String dir)
   {
      dir = StringUtil.shortPathName(FileSystemItem.createDir(dir), 250);
      deployLabel_.setText(dir);
      appName.setText(FilePathUtils.friendlyFileName(dir));
   }

   public void setDefaultAccount(RSConnectAccount account)
   {
      accountList.selectAccount(account);
   }
   
   public void setAccountList(JsArray<RSConnectAccount> accounts)
   {
      accountList.setAccountList(accounts);
   }
   
   public RSConnectAccount getSelectedAccount()
   {
      return accountList.getSelectedAccount();
   }
   
   public String getSelectedApp()
   {
      int idx = appList.getSelectedIndex();
      return idx >= 0 ? 
            appList.getItemText(idx) :
            null;
   }
   
   public void setAppList(List<String> apps, String selected)
   {
      appList.clear();
      int selectedIdx = 0;
      if (apps != null)
      {
         selectedIdx = Math.max(0, apps.size() - 1);
         
         for (int i = 0; i < apps.size(); i++)
         {
            String app = apps.get(i);
            appList.addItem(app);
            if (app.equals(selected))
            {
               selectedIdx = i;
            }
         }
      }
      appList.addItem("Create New");
      appList.setSelectedIndex(selectedIdx);
   }
   
   public void setFileList(JsArrayString files)
   {
      for (int i = 0; i < files.length(); i++)
      {
         Label fileLabel = new Label(files.get(i));
         fileListPanel_.add(fileLabel);
      }
   }
   
   public String getNewAppName()
   {
      return appName.getText();
   }
   
   public void showAppInfo(RSConnectApplicationInfo info)
   {
      if (info == null)
      {
         appInfoPanel.setVisible(false);
         nameLabel.setVisible(true);
         appName.setVisible(true);
         validateAppName();
         return;
      }

      setAppNameValid(true);
      urlAnchor.setText(info.getUrl());
      urlAnchor.setHref(info.getUrl());
      String status = info.getStatus();
      statusLabel.setText(status);
      statusLabel.setStyleName(style.statusLabel() + " " + 
              (status.equals("running") ?
                    style.normalStatus() :
                    style.otherStatus()));

      appInfoPanel.setVisible(true);
      nameLabel.setVisible(false);
      appName.setVisible(false);
      nameValidatePanel.setVisible(false);
   }
   
   public HandlerRegistration addAccountChangeHandler(ChangeHandler handler)
   {
      return accountList.addChangeHandler(handler);
   }

   public HandlerRegistration addAppChangeHandler(ChangeHandler handler)
   {
      return appList.addChangeHandler(handler);
   }
   
   public void setOnDeployEnabled(Command cmd)
   {
      onDeployEnabled_ = cmd;
   }
   
   public void setOnDeployDisabled(Command cmd)
   {
      onDeployDisabled_ = cmd;
   }
   
   public DeployStyle getStyle()
   {
      return style;
   }
   
   private void validateAppName()
   {
      String app = appName.getText();
      RegExp validReg = RegExp.compile("^[A-Za-z0-9_-]{4,63}$");
      setAppNameValid(validReg.test(app));
   }
   
   private void setAppNameValid(boolean isValid)
   {
      nameValidatePanel.setVisible(!isValid);
      if (isValid && onDeployEnabled_ != null)
         onDeployEnabled_.execute();
      else if (!isValid && onDeployDisabled_ != null)
         onDeployDisabled_.execute();
   }
   
   @UiField Anchor urlAnchor;
   @UiField Anchor addAccountAnchor;
   @UiField Label nameLabel;
   @UiField InlineLabel statusLabel;
   @UiField(provided=true) RSConnectAccountList accountList;
   @UiField ListBox appList;
   @UiField TextBox appName;
   @UiField HTMLPanel appInfoPanel;
   @UiField HTMLPanel nameValidatePanel;
   @UiField DeployStyle style;
   @UiField FlowPanel fileListPanel_;
   @UiField InlineLabel deployLabel_;
   
   private Command onDeployEnabled_;
   private Command onDeployDisabled_;
}
