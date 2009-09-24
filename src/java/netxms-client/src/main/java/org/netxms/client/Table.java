/**
 * 
 */
package com.radensolutions.stronghold.client.objects;

import java.util.ArrayList;
import java.util.List;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * @author Victor
 *
 */
public class Table
{
	private List<String> columns;
	private List<List<String>> data;
	
	/**
	 * Create empty table
	 */
	public Table()
	{
		columns = new ArrayList<String>(0);
		data = new ArrayList<List<String>>(0);
	}
	
	/**
	 * Create table from data in NXCP message
	 * 
	 * @param msg NXCP message
	 */
	public Table(final NXCPMessage msg)
	{
		int columnCount = msg.getVariableAsInteger(NXCPCodes.VID_TABLE_NUM_COLS);
		columns = new ArrayList<String>(columnCount);
		long varId = NXCPCodes.VID_TABLE_COLUMN_INFO_BASE;
		for(int i = 0; i < columnCount; i++, varId += 9)
			columns.add(msg.getVariableAsString(varId++));
		
		int rowCount = msg.getVariableAsInteger(NXCPCodes.VID_TABLE_NUM_ROWS);
		data = new ArrayList<List<String>>(rowCount);
		varId = NXCPCodes.VID_TABLE_DATA_BASE;
		for(int i = 0; i < rowCount; i++)
		{
			ArrayList<String> row = new ArrayList<String>(columnCount);
			for(int j = 0; j < columnCount; j++)
				row.add(msg.getVariableAsString(varId++));
			data.add(row);
		}
	}
	
	/**
	 * Add data from additional messages
	 */
	public void addDataFromMessage(final NXCPMessage msg)
	{
		int rowCount = msg.getVariableAsInteger(NXCPCodes.VID_TABLE_NUM_ROWS);
		long varId = NXCPCodes.VID_TABLE_DATA_BASE;
		for(int i = 0; i < rowCount; i++)
		{
			ArrayList<String> row = new ArrayList<String>(columns.size());
			for(int j = 0; j < columns.size(); j++)
				row.add(msg.getVariableAsString(varId++));
			data.add(row);
		}
	}
	
	/**
	 * Get number of columns in table
	 * 
	 * @return Number of columns
	 */
	public int getColumnCount()
	{
		return columns.size();
	}
	
	/**
	 * Get number of rows in table
	 * 
	 * @return Number of rows
	 */
	public int getRowCount()
	{
		return data.size();
	}
	
	/**
	 * Get column name
	 * 
	 * @param column Column index (zero-based)
	 * @return Column name
	 * @throws IndexOutOfBoundsException if column index is out of range (column < 0 || column >= getColumnCount())
	 */
	public String getColumnName(int column) throws IndexOutOfBoundsException
	{
		return columns.get(column);
	}
	
	/**
	 * Get column index by name
	 * 
	 * @param name Column name
	 * @return 0-based column index or -1 if column with given name does not exist
	 */
	public int getColumnIndex(final String name)
	{
		for(int i = 0; i < columns.size(); i++)
			if (name.equals(columns.get(i)))
				return i;
		return -1;
	}
	
	/**
	 * Get cell value at given row and column
	 * 
	 * @param row Row index (zero-based)
	 * @param column Column index (zero-based)
	 * @return Data from given cell
	 * @throws IndexOutOfBoundsException if column index is out of range (column < 0 || column >= getColumnCount())
	 *         or row index is out of range (row < 0 || row >= getRowCount())
	 */
	public String getCell(int row, int column) throws IndexOutOfBoundsException
	{
		List<String> rowData = data.get(row);
		return rowData.get(column);
	}
	
	/**
	 * Get row.
	 * 
	 * @param row Row index (zero-based)
	 * @return List of all values for given row
	 * @throws IndexOutOfBoundsException if row index is out of range (row < 0 || row >= getRowCount())
	 */
	public List<String> getRow(int row) throws IndexOutOfBoundsException
	{
		return data.get(row);
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	public String toString()
	{
		final StringBuilder sb = new StringBuilder();
		sb.append("Table");
		sb.append("{columns=").append(columns);
		sb.append(", data=").append(data);
		sb.append('}');
		return sb.toString();
	}
}
