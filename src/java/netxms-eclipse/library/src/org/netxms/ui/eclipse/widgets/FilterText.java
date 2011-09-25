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
package org.netxms.ui.eclipse.widgets;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.jface.action.Action;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Filter text widgets. Shows labelled text entry field, filtering 
 * attribute selection buttons, and close button.
 */
public class FilterText extends Composite
{
	private Text text;
	private Composite buttonArea;
	private List<Button> attrButtons = new ArrayList<Button>(4);
	private Label closeButton;
	private Action closeAction = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public FilterText(Composite parent, int style)
	{
		super(parent, style);
		GridLayout layout = new GridLayout();
		layout.numColumns = 4;
		setLayout(layout);
		
		final Label label = new Label(this, SWT.NONE);
		label.setText("Filter:");
		GridData gd = new GridData();
		gd.verticalAlignment = SWT.CENTER;
		label.setLayoutData(gd);
		
		text = new Text(this, SWT.BORDER);
		text.setTextLimit(64);
		text.setMessage("Filter is empty");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.CENTER;
		text.setLayoutData(gd);
		
		buttonArea = new Composite(this, SWT.NONE);
		RowLayout buttonLayout = new RowLayout();
		buttonLayout.type = SWT.HORIZONTAL;
		buttonLayout.wrap = true;
		buttonLayout.marginBottom = 0;
		buttonLayout.marginTop = 0;
		buttonLayout.marginLeft = 0;
		buttonLayout.marginRight = 0;
		buttonLayout.spacing = WidgetHelper.OUTER_SPACING;
		buttonLayout.pack = false;
		buttonArea.setLayout(buttonLayout);
		gd = new GridData();
		gd.verticalAlignment = SWT.CENTER;
		buttonArea.setLayoutData(gd);
		
		closeButton = new Label(this, SWT.NONE);
		closeButton.setBackground(getBackground());
		closeButton.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
		closeButton.setImage(SharedIcons.IMG_CLOSE);
		closeButton.setToolTipText("Close filter");
		gd = new GridData();
		gd.verticalAlignment = SWT.CENTER;
		closeButton.setLayoutData(gd);
		closeButton.addMouseListener(new MouseListener() {
			private boolean doAction = false;
			
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
				if (e.button == 1)
					doAction = false;
			}

			@Override
			public void mouseDown(MouseEvent e)
			{
				if (e.button == 1)
					doAction = true;
			}

			@Override
			public void mouseUp(MouseEvent e)
			{
				if ((e.button == 1) && doAction)
					closeFilter();
			}
		});
	}
	
	/**
	 * Close filter widget
	 */
	private void closeFilter()
	{
		if (closeAction != null)
			closeAction.run();
	}
	
	/**
	 * Set filtering attribute list
	 * 
	 * @param attributes
	 */
	public void setAttributeList(String[] attributes)
	{
		for(Button b : attrButtons)
		{
			b.dispose();
		}
		attrButtons.clear();
		
		for(String attr : attributes)
		{
			final Button b = new Button(buttonArea, SWT.TOGGLE);
			b.setText(attr);
			attrButtons.add(b);
			b.addSelectionListener(new SelectionListener () {
				@Override
				public void widgetSelected(SelectionEvent e)
				{
					onAttrButtonSelection(b);
				}

				@Override
				public void widgetDefaultSelected(SelectionEvent e)
				{
					widgetSelected(e);
				}
			});
		}
		
		layout(true, true);
	}
	
	/**
	 * Handler for attribute button selection
	 * 
	 * @param b button object
	 */
	private void onAttrButtonSelection(Button b)
	{
		
	}
	
	/**
	 * Add text modify listener
	 * 
	 * @param listener
	 */
	public void addModifyListener(ModifyListener listener)
	{
		text.addModifyListener(listener);
	}
	
	/**
	 * Remove text modify listener
	 * 
	 * @param listener
	 */
	public void removeModifyListener(ModifyListener listener)
	{
		text.removeModifyListener(listener);
	}
	
	/**
	 * Get current filter text
	 * 
	 * @return current filter text
	 */
	public String getText()
	{
		return text.getText();
	}
	
	/**
	 * Set filter text
	 * 
	 * @param value new filter text
	 */
	public void setText(String value)
	{
		text.setText(value);
	}

	/**
	 * @return the closeAction
	 */
	public Action getCloseAction()
	{
		return closeAction;
	}

	/**
	 * @param closeAction the closeAction to set
	 */
	public void setCloseAction(Action closeAction)
	{
		this.closeAction = closeAction;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#setFocus()
	 */
	@Override
	public boolean setFocus()
	{
		return text.setFocus();
	}
}
