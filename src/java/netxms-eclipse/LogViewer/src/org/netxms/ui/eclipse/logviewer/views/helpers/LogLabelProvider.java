/**
 * 
 */
package org.netxms.ui.eclipse.logviewer.views.helpers;

import java.text.DateFormat;
import java.util.Collection;
import java.util.Date;
import java.util.List;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCObject;
import org.netxms.client.NXCSession;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.shared.StatusDisplayInfo;

/**
 * @author Victor
 *
 */
public class LogLabelProvider implements ITableLabelProvider
{
	private Log logHandle;
	private LogColumn[] columns;
	private NXCSession session;
	private Image[] statusImages;
	
	public LogLabelProvider(Log logHandle)
	{
		this.logHandle = logHandle;
		Collection<LogColumn> c = logHandle.getColumns();
		columns = c.toArray(new LogColumn[c.size()]);
		session = NXMCSharedData.getInstance().getSession();
		
		statusImages = new Image[9];
		for(int i = 0; i < 9; i++)
		{
			ImageDescriptor d = StatusDisplayInfo.getStatusImageDescriptor(i);
			statusImages[i] = (d != null) ? d.createImage() : null;
		}
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@SuppressWarnings("unchecked")
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		final String value = ((List<String>)element).get(columnIndex);
		switch(columns[columnIndex].getType())
		{
			case LogColumn.LC_SEVERITY:
				try
				{
					int severity = Integer.parseInt(value);
					return statusImages[severity];
				}
				catch(NumberFormatException e)
				{
					return null;
				}
			default:
				return null;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@SuppressWarnings("unchecked")
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		final String value = ((List<String>)element).get(columnIndex);
		switch(columns[columnIndex].getType())
		{
			case LogColumn.LC_TIMESTAMP:
				try
				{
					long timestamp = Long.parseLong(value);
					Date date = new Date(timestamp * 1000);
					return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.MEDIUM).format(date);
				}
				catch(NumberFormatException e)
				{
					return "<error>";
				}
			case LogColumn.LC_OBJECT_ID:
				try
				{
					long id = Long.parseLong(value);
					if (id == 0)
						return "";
					NXCObject object = session.findObjectById(id);
					return (object != null) ? object.getObjectName() : "<unknown>";
				}
				catch(NumberFormatException e)
				{
					return "<error>";
				}
			case LogColumn.LC_SEVERITY:
				try
				{
					int severity = Integer.parseInt(value);
					return StatusDisplayInfo.getStatusText(severity);
				}
				catch(NumberFormatException e)
				{
					return "<error>";
				}
			default:
				return value;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#addListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void addListener(ILabelProviderListener listener)
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		for(int i = 0; i < statusImages.length; i++)
			if (statusImages[i] != null)
				statusImages[i].dispose();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#isLabelProperty(java.lang.Object, java.lang.String)
	 */
	@Override
	public boolean isLabelProperty(Object element, String property)
	{
		// TODO Auto-generated method stub
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#removeListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void removeListener(ILabelProviderListener listener)
	{
		// TODO Auto-generated method stub

	}

}
