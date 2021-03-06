/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.agentmanager.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.UserAgentNotification;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;

/**
 * User agent notification filter
 */
public class UserAgentNotificationFilter extends ViewerFilter
{
   private String filterString = null;

   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      UserAgentNotification uam = (UserAgentNotification)element;
      if ((filterString == null) || (filterString.isEmpty()))
         return true;
      else if (Long.toString(uam.getId()).toLowerCase().contains(filterString))
         return true;
      else if (uam.getMessage().toLowerCase().contains(filterString))
         return true;
      else if (uam.getObjectNames().toLowerCase().contains(filterString))
         return true;
      else if (RegionalSettings.getDateTimeFormat().format(uam.getEndTime()).contains(filterString))
         return true;
      else if (RegionalSettings.getDateTimeFormat().format(uam.getStartTime()).contains(filterString))
         return true;
      else if (uam.isRecalled() ? "yes".contains(filterString) : "no".contains(filterString))
         return true;
      else if (uam.isStartupNotification() ? "yes".contains(filterString) : "no".contains(filterString))
         return true;
      return false;
   }
   
   /**
    * @param filterString the filterString to set
    */
   public void setFilterString(String filterString)
   {
      this.filterString = filterString.toLowerCase();
   }
}
