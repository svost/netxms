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
package org.netxms.ui.eclipse.datacollection.widgets;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.widgets.internal.LastValuesComparator;
import org.netxms.ui.eclipse.datacollection.widgets.internal.LastValuesFilter;
import org.netxms.ui.eclipse.datacollection.widgets.internal.LastValuesLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Viewer for last values of given object
 */
public class LastValuesWidget extends Composite
{
	public static final String JOB_FAMILY = "LastValuesViewJob";
	
	// Columns
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_DESCRIPTION = 1;
	public static final int COLUMN_VALUE = 2;
	public static final int COLUMN_TIMESTAMP = 3;
	
	private final ViewPart viewPart;
	private Node node;
	private NXCSession session;
	private boolean filterEnabled = false;
	private FilterText filterText;
	private SortableTableViewer dataViewer;
	private LastValuesLabelProvider labelProvider;
	private LastValuesFilter filter;
	private boolean autoRefreshEnabled = false;
	private int autoRefreshInterval = 30000;	// in milliseconds
	private Runnable refreshTimer;
	private Action actionUseMultipliers;
	
	/**
	 * Create "last values" widget
	 * 
	 * @param viewPart owning view part
	 * @param parent parent widget
	 * @param style style
	 * @param _node node to display data for
	 * @param configPrefix configuration prefix for saving/restoring viewer settings
	 */
	public LastValuesWidget(ViewPart viewPart, Composite parent, int style, Node _node, final String configPrefix)
	{
		super(parent, style);
		session = (NXCSession)ConsoleSharedData.getSession();
		this.viewPart = viewPart;		
		this.node = _node;
		
		final IDialogSettings ds = Activator.getDefault().getDialogSettings();
		
		refreshTimer = new Runnable() {
			@Override
			public void run()
			{
				if (isDisposed())
					return;
				
				getDataFromServer();
				getDisplay().timerExec(autoRefreshInterval, this);
			}
		};
		
		setLayout(new FormLayout());
		
		// Create filter area
		filterText = new FilterText(this, SWT.NONE);
		filterText.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				onFilterModify();
			}
		});
		filterText.setCloseAction(new Action() {
			@Override
			public void run()
			{
				enableFilter(false);
			}
		});
		
		// Setup table columns
		final String[] names = { "ID", "Description", "Value", "Timestamp" };
		final int[] widths = { 70, 250, 150, 100 };
		dataViewer = new SortableTableViewer(this, names, widths, 0, SWT.DOWN, SortableTableViewer.DEFAULT_STYLE);
	
		labelProvider = new LastValuesLabelProvider();
		filter = new LastValuesFilter();
		dataViewer.setLabelProvider(labelProvider);
		dataViewer.setContentProvider(new ArrayContentProvider());
		dataViewer.setComparator(new LastValuesComparator());
		dataViewer.addFilter(filter);
		WidgetHelper.restoreTableViewerSettings(dataViewer, ds, configPrefix);
		
		actionUseMultipliers = new Action("Use &multipliers", Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				setUseMultipliers(!areMultipliersUsed());
			}
		};

		createPopupMenu();

		dataViewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(dataViewer, ds, configPrefix);
				ds.put(configPrefix + ".autoRefresh", autoRefreshEnabled);
				ds.put(configPrefix + ".autoRefreshInterval", autoRefreshEnabled);
				ds.put(configPrefix + ".useMultipliers", labelProvider.areMultipliersUsed());
			}
		});

		// Setup layout
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(filterText);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		dataViewer.getTable().setLayoutData(fd);
		
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		filterText.setLayoutData(fd);
		
		// Finalise widget creation
		getDataFromServer();
		
		try
		{
			ds.getInt(configPrefix + ".autoRefreshInterval");
		}
		catch(NumberFormatException e)
		{
		}
		setAutoRefreshEnabled(ds.getBoolean(configPrefix + ".autoRefresh"));
		if (ds.get(configPrefix + ".useMultipliers") != null)
			labelProvider.setUseMultipliers(ds.getBoolean(configPrefix + ".useMultipliers"));
		else
			labelProvider.setUseMultipliers(true);
		actionUseMultipliers.setChecked(areMultipliersUsed());
		
		// Set initial focus to filter input line
		if (filterEnabled)
			filterText.setFocus();
		else
			enableFilter(false);	// Will hide filter area correctly
	}
	
	/**
	 * Create pop-up menu for alarm list
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(dataViewer.getControl());
		dataViewer.getControl().setMenu(menu);

		// Register menu for extension.
		if (viewPart != null)
			viewPart.getSite().registerContextMenu(menuMgr, dataViewer);
	}
	
	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(new Separator());
		mgr.add(actionUseMultipliers);
	}
	
	/**
	 * Get data from server
	 */
	private void getDataFromServer()
	{
		if (node == null)
		{
			dataViewer.setInput(new DciValue[0]);
			return;
		}

		ConsoleJob job = new ConsoleJob("Get DCI values for node " + node.getObjectName(), viewPart, Activator.PLUGIN_ID, LastValuesWidget.JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot get DCI values for node " + node.getObjectName();
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final DciValue[] data = session.getLastValues(node.getObjectId());
				new UIJob("Update last values view") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						if (!isDisposed())
							dataViewer.setInput(data);
						return Status.OK_STATUS;
					}
				}.schedule();
			}
		};
		job.setUser(false);
		job.start();
	}
	
	/**
	 * Change node object
	 * 
	 * @param _node new node object
	 */
	public void setNode(Node _node)
	{
		this.node = _node;
		getDataFromServer();
	}
	
	/**
	 * Refresh view
	 */
	public void refresh()
	{
		getDataFromServer();
	}

	/**
	 * @return the autoRefreshEnabled
	 */
	public boolean isAutoRefreshEnabled()
	{
		return autoRefreshEnabled;
	}

	/**
	 * @param autoRefreshEnabled the autoRefreshEnabled to set
	 */
	public void setAutoRefreshEnabled(boolean autoRefreshEnabled)
	{
		this.autoRefreshEnabled = autoRefreshEnabled;
		getDisplay().timerExec(autoRefreshEnabled ? autoRefreshInterval : -1, refreshTimer);
	}

	/**
	 * @return the autoRefreshInterval
	 */
	public int getAutoRefreshInterval()
	{
		return autoRefreshInterval;
	}

	/**
	 * @param autoRefreshInterval the autoRefreshInterval to set
	 */
	public void setAutoRefreshInterval(int autoRefreshInterval)
	{
		this.autoRefreshInterval = autoRefreshInterval;
	}
	
	/**
	 * @return the useMultipliers
	 */
	public boolean areMultipliersUsed()
	{
		return (labelProvider != null) ? labelProvider.areMultipliersUsed() : false;
	}

	/**
	 * @param useMultipliers the useMultipliers to set
	 */
	public void setUseMultipliers(boolean useMultipliers)
	{
		if (labelProvider != null)
		{
			labelProvider.setUseMultipliers(useMultipliers);
			if (dataViewer != null)
			{
				dataViewer.refresh(true);
			}
		}
	}

	/**
	 * Enable or disable filter
	 * 
	 * @param enable New filter state
	 */
	public void enableFilter(boolean enable)
	{
		filterEnabled = enable;
		filterText.setVisible(filterEnabled);
		FormData fd = (FormData)dataViewer.getTable().getLayoutData();
		fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
		layout();
		if (enable)
			filterText.setFocus();
		else
			setFilter("");
	}

	/**
	 * @return the filterEnabled
	 */
	public boolean isFilterEnabled()
	{
		return filterEnabled;
	}

	/**
	 * Set filter text
	 * 
	 * @param text New filter text
	 */
	public void setFilter(final String text)
	{
		filterText.setText(text);
		onFilterModify();
	}

	/**
	 * Handler for filter modification
	 */
	private void onFilterModify()
	{
		final String text = filterText.getText();
		filter.setFilterString(text);
		dataViewer.refresh(false);
	}
	
	/**
	 * Set action to be executed when user press "Close" button in object filter.
	 * Default implementation will hide filter area without notifying parent.
	 * 
	 * @param action
	 */
	public void setFilterCloseAction(Action action)
	{
		filterText.setCloseAction(action);
	}
}
