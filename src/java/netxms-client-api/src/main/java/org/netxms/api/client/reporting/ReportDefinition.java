package org.netxms.api.client.reporting;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class ReportDefinition
{
	private UUID id;
	private String name;
	private int numberOfColumns = 1;
	private List<ReportParameter> parameters = new ArrayList<ReportParameter>(0);

	public UUID getId()
	{
		return id;
	}

	public void setId(UUID id)
	{
		this.id = id;
	}

	public String getName()
	{
		return name;
	}

	public void setName(String name)
	{
		this.name = name;
	}

	public List<ReportParameter> getParameters()
	{
		return parameters;
	}

	public void addParameter(ReportParameter parameter)
	{
		parameters.add(parameter);
	}

	public int getNumberOfColumns()
	{
		return numberOfColumns;
	}

	public void setNumberOfColumns(int numberOfColumns)
	{
		if (numberOfColumns > 0)
		{
			this.numberOfColumns = numberOfColumns;
		}
		else
		{
			this.numberOfColumns = 1;
		}
	}

	@Override
	public String toString()
	{
		return "ReportDefinition [id=" + id + ", name=" + name + ", numberOfColumns=" + numberOfColumns + ", parameters="
				+ parameters + "]";
	}
}
