/* 
 ** Java-Bridge NetXMS subagent
 ** Copyright (C) 2013 TEMPEST a.s.
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **
 ** File: TestPlugin.java
 **
 **/
package org.netxms.agent;


public class TestPlugin extends Plugin {

	public TestPlugin(Config config) {
		super(config);
	}

	public String getName() {
		return "Dummy";
	}

	public String getVersion() {
		return "0.1";
	}

	public void init(Config config) {
		// TODO Auto-generated method stub

	}

	public void shutdown() {
		// TODO Auto-generated method stub

	}

	public Parameter[] getParameters() {
		return new Parameter[] {
				new Parameter() {

					public String getName() {
						return "Parameter1";
					}

					public String getDescription() {
						return "Parameter1 description";
					}

					public ParameterType getType() {
						return ParameterType.INT;
					}

					public String getValue(String argument) {
						return "1";
					}
					
				}
				, new Parameter() {
					public String getName() {
						return "Parameter2";
					}

					public String getDescription() {
						return "Parameter2 description";
					}

					public ParameterType getType() {
						return ParameterType.INT64;
					}

					public String getValue(String argument) {
						return "2";
					}
				}, new Parameter() {
					public String getName() {
						return "Parameter3";
					}

					public String getDescription() {
						return "Parameter3 description";
					}

					public ParameterType getType() {
						return ParameterType.STRING;
					}

					public String getValue(String argument) {
						return "String";
					}				
				}
		};
	}

	public PushParameter[] getPushParameters() {
		return new PushParameter[] {
			new PushParameter() {

				public String getName() {
					return "PushParameter1";
				}

				public String getDescription() {
					return "PushParameter1 description";
				}

				public ParameterType getType() {
					return ParameterType.INT;
				}
				
				public String getValue() {
					return "666";
				}
				
			}
		};
	}

	public ListParameter[] getListParameters() {
		return new ListParameter[] {
			new ListParameter() {

				public String getName() {
					return "ListParameter1";
				}

				public String[] getValues(String value) {
					return new String[] { "1", "2"};
				}
				
			}
		};
	}

	public TableParameter[] getTableParameters() {
		return new TableParameter[] {
				new TableParameter() {

					public String getName() {
						return "TableParameter1";
					}

					public String getDescription() {
						return "TableParameter1 description";
					}

					public TableColumn[] getColumns() {
						return new TableColumn[] {
								new TableColumn("Column1", ParameterType.STRING, true),
								new TableColumn("Column2", ParameterType.INT, false)
						};
					}

					public String[][] getValues(String parameter) {
						return new String[][] {
								{"Row1", "1"},
								{"Row2", "2"} 
						};
					}
				}
		};
	}

	public Action[] getActions() {
		return new Action[] {
				new Action() {
					
					public String getName() {
						return "Action1";
					}
					
					public String getDescription() {
						return "Action1 description";
					}
					
					public void execute(String action, String[] args) {
						// TODO Auto-generated method stub
						
					}
				}
		};
	}

}