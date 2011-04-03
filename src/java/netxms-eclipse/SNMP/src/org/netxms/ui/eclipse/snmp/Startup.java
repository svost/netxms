/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.snmp;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.osgi.service.datalocation.Location;
import org.eclipse.ui.IStartup;
import org.netxms.client.NXCSession;
import org.netxms.client.snmp.MibTree;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Early startup class
 *
 */
public class Startup implements IStartup
{
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IStartup#earlyStartup()
	 */
	@Override
	public void earlyStartup()
	{
		// wait for connect
		ConsoleJob job = new ConsoleJob("Load MIB file on startup", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				while(ConsoleSharedData.getSession() == null)
				{
					try
					{
						Thread.sleep(1000);
					}
					catch(InterruptedException e)
					{
					}
				}
				
				NXCSession session = (NXCSession)ConsoleSharedData.getSession();
				Location loc = Platform.getInstanceLocation();
				if (loc != null)
				{
					File targetDir = new File(loc.getURL().toURI());
					File mibFile = new File(targetDir, "netxms.mib");
					
					Date serverMibTimestamp = session.getMibFileTimestamp();
					if (!mibFile.exists() || (serverMibTimestamp.getTime() > mibFile.lastModified()))
					{
						File file = session.downloadMibFile();

						if (mibFile.exists())
							mibFile.delete();
						
						if (!file.renameTo(mibFile))
						{
							// Rename failed, try to copy file
							InputStream in = null;
							OutputStream out = null;
							try
							{
								in = new FileInputStream(file);
								out = new FileOutputStream(mibFile);
								byte[] buffer = new byte[16384];
						      int len;
						      while((len = in.read(buffer)) > 0)
						      	out.write(buffer, 0, len);
							}
							catch(Exception e)
							{
								throw e;
							}
							finally
							{
						      in.close();
						      out.close();
							}
					      
					      file.delete();
						}
					}
					
					MibTree mibTree = new MibTree(mibFile);
					Activator.setMibTree(mibTree);
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot load MIB file from server";
			}
		};
		job.setUser(false);
		job.start();
	}
}
