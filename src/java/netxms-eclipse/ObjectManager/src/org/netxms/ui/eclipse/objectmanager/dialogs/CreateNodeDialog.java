/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.dialogs;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.UnknownHostException;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Zone;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for creating new node
 *
 */
public class CreateNodeDialog extends Dialog
{
	private NXCSession session;
	
	private LabeledText nameField;
	private LabeledText ipAddrField;
	private Button checkUnmanaged;
	private Button checkDisableAgent;
	private Button checkDisableSNMP;
	private Button checkDisablePing;
	private ObjectSelector agentProxySelector;
	private ObjectSelector snmpProxySelector;
	private ObjectSelector zoneSelector;
	
	private String name;
	private InetAddress ipAddress;
	private int creationFlags;
	private long agentProxy;
	private long snmpProxy;
	private long zoneId;
	
	/**
	 * @param parentShell
	 */
	public CreateNodeDialog(Shell parentShell)
	{
		super(parentShell);
		session = (NXCSession)ConsoleSharedData.getSession();
		zoneId = 0;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Create Node Object");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		dialogArea.setLayout(layout);
		
		nameField = new LabeledText(dialogArea, SWT.NONE);
		nameField.setLabel("Name");
		nameField.getTextControl().setTextLimit(255);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		nameField.setLayoutData(gd);
		
		final Composite ipAddrGroup = new Composite(dialogArea, SWT.NONE);
		layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.numColumns = 2;
		ipAddrGroup.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		ipAddrGroup.setLayoutData(gd);
		
		ipAddrField = new LabeledText(ipAddrGroup, SWT.NONE);
		ipAddrField.setLabel("Primary IP address");
		ipAddrField.getTextControl().setTextLimit(255);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		ipAddrField.setLayoutData(gd);
		
		final Button resolve = new Button(ipAddrGroup, SWT.PUSH);
		resolve.setText("&Resolve");
		gd = new GridData();
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
		gd.verticalAlignment = SWT.BOTTOM;
		resolve.setLayoutData(gd);
		resolve.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				resolveName();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText("Options");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		optionsGroup.setLayoutData(gd);
		optionsGroup.setLayout(new RowLayout(SWT.VERTICAL));
		
		checkUnmanaged = new Button(optionsGroup, SWT.CHECK);
		checkUnmanaged.setText("Create as &unmanaged object");

		checkDisableAgent = new Button(optionsGroup, SWT.CHECK);
		checkDisableAgent.setText("Disable usage of NetXMS &agent for all polls");
		
		checkDisableSNMP = new Button(optionsGroup, SWT.CHECK);
		checkDisableSNMP.setText("Disable usage of &SNMP for all polls");
		
		checkDisablePing = new Button(optionsGroup, SWT.CHECK);
		checkDisablePing.setText("Disable usage of &ICMP ping for all polls");
		
		agentProxySelector = new ObjectSelector(dialogArea, SWT.NONE);
		agentProxySelector.setLabel("Proxy for NetXMS agent");
		agentProxySelector.setObjectClass(GenericObject.OBJECT_NODE);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		agentProxySelector.setLayoutData(gd);
		
		snmpProxySelector = new ObjectSelector(dialogArea, SWT.NONE);
		snmpProxySelector.setLabel("Proxy for SNMP");
		snmpProxySelector.setObjectClass(GenericObject.OBJECT_NODE);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		snmpProxySelector.setLayoutData(gd);
		
		if (session.isZoningEnabled())
		{
			zoneSelector = new ObjectSelector(dialogArea, SWT.NONE);
			zoneSelector.setLabel("Zone");
			zoneSelector.setObjectClass(GenericObject.OBJECT_ZONE);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			zoneSelector.setLayoutData(gd);
		}
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		try
		{
			ipAddress = InetAddress.getByName(ipAddrField.getText());
		}
		catch(UnknownHostException e)
		{
			MessageDialog.openWarning(getShell(), "Warning", "Invalid IP address or host name");
			return;
		}
		
		name = nameField.getText().trim();
		if (name.isEmpty())
			name = ipAddrField.getText();
		
		creationFlags = 0;
		if (checkUnmanaged.getSelection())
			creationFlags |= NXCObjectCreationData.CF_CREATE_UNMANAGED;
		if (checkDisableAgent.getSelection())
			creationFlags |= NXCObjectCreationData.CF_DISABLE_NXCP;
		if (checkDisablePing.getSelection())
			creationFlags |= NXCObjectCreationData.CF_DISABLE_ICMP;
		if (checkDisableSNMP.getSelection())
			creationFlags |= NXCObjectCreationData.CF_DISABLE_SNMP;
		
		agentProxy = agentProxySelector.getObjectId();
		snmpProxy = snmpProxySelector.getObjectId();
		if (session.isZoningEnabled())
		{
			long zoneObjectId = zoneSelector.getObjectId();
			GenericObject object = session.findObjectById(zoneObjectId);
			if ((object != null) && (object instanceof Zone))
			{
				zoneId = ((Zone)object).getZoneId();
			}
		}
		
		super.okPressed();
	}
	
	/**
	 * Resolve entered name to IP address
	 */
	private void resolveName()
	{
		final String name = nameField.getText();
		new ConsoleJob("Resolve host name", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final InetAddress addr = Inet4Address.getByName(name);
				PlatformUI.getWorkbench().getDisplay().asyncExec(new Runnable() {
					@Override
					public void run()
					{
						ipAddrField.setText(addr.getHostAddress());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot resolve host name " + name + " to IP address";
			}
		}.start();
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the ipAddress
	 */
	public InetAddress getIpAddress()
	{
		return ipAddress;
	}

	/**
	 * @return the creationFlags
	 */
	public int getCreationFlags()
	{
		return creationFlags;
	}

	/**
	 * @return the agentProxy
	 */
	public long getAgentProxy()
	{
		return agentProxy;
	}

	/**
	 * @return the snmpProxy
	 */
	public long getSnmpProxy()
	{
		return snmpProxy;
	}

	/**
	 * @return the zoneId
	 */
	public long getZoneId()
	{
		return zoneId;
	}
}
