/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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
package org.netxms.client.constants;

import java.util.HashMap;
import java.util.Map;
import org.netxms.base.Logger;

public enum ColumnFilterType
{
   EQUALS(0),
   RANGE(1),
   SET(2),
   LIKE(3),
   LESS(4),
   GREATER(5),
   CHILDOF(6),
   UNKNOWN(7);
   
   private int value;
   private static Map<Integer, ColumnFilterType> lookupTable = new HashMap<Integer, ColumnFilterType>();
   
   static
   {
      for(ColumnFilterType element : ColumnFilterType.values())
      {
         lookupTable.put(element.value, element);
      }
   }
   
   private ColumnFilterType(int value)
   {
      this.value = value;
   }
   
   public int getValue()
   {
      return value;
   }
   
   public static ColumnFilterType getByValue(int value)
   {
      final ColumnFilterType element = lookupTable.get(value);
      if (element == null)
      {
         Logger.warning(ColumnFilterType.class.getName(), "Unknown element " + value);
         return UNKNOWN; // fallback
      }
      return element;
   }
}
