package org.netxms.client;

import java.util.Date;
import org.netxms.base.NXCPMessage;

public class ScheduledTask
{
   private long id;
   private String scheduledTaskId;
   private String schedule;
   private String parameters;
   private Date executionTime;
   private Date lastExecutionTime;
   private int flags;
   private long owner;
   
   ScheduledTask(final NXCPMessage msg, long base)
   {
      id = msg.getFieldAsInt64(base);
      scheduledTaskId = msg.getFieldAsString(base+1);
      schedule = msg.getFieldAsString(base+2);
      parameters = msg.getFieldAsString(base+3);
      executionTime = msg.getFieldAsDate(base+4);
      lastExecutionTime = msg.getFieldAsDate(base+5);
      flags = msg.getFieldAsInt32(base+6);
      owner = msg.getFieldAsInt64(base+7);
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @param id the id to set
    */
   public void setId(long id)
   {
      this.id = id;
   }

   /**
    * @return the scheduledTaskId
    */
   public String getScheduledTaskId()
   {
      return scheduledTaskId;
   }

   /**
    * @param scheduledTaskId the scheduledTaskId to set
    */
   public void setScheduledTaskId(String scheduledTaskId)
   {
      this.scheduledTaskId = scheduledTaskId;
   }

   /**
    * @return the schedule
    */
   public String getSchedule()
   {
      return schedule;
   }

   /**
    * @param schedule the schedule to set
    */
   public void setSchedule(String schedule)
   {
      this.schedule = schedule;
   }

   /**
    * @return the parameters
    */
   public String getParameters()
   {
      return parameters;
   }

   /**
    * @param parameters the parameters to set
    */
   public void setParameters(String parameters)
   {
      this.parameters = parameters;
   }

   /**
    * @return the executionTime
    */
   public Date getExecutionTime()
   {
      return executionTime;
   }

   /**
    * @param executionTime the executionTime to set
    */
   public void setExecutionTime(Date executionTime)
   {
      this.executionTime = executionTime;
   }

   /**
    * @return the lastExecutionTime
    */
   public Date getLastExecutionTime()
   {
      return lastExecutionTime;
   }

   /**
    * @param lastExecutionTime the lastExecutionTime to set
    */
   public void setLastExecutionTime(Date lastExecutionTime)
   {
      this.lastExecutionTime = lastExecutionTime;
   }

   /**
    * @return the flags
    */
   public int getFlags()
   {
      return flags;
   }

   /**
    * @param flags the flags to set
    */
   public void setFlags(int flags)
   {
      this.flags = flags;
   }

   /**
    * @return the owner
    */
   public long getOwner()
   {
      return owner;
   }

   /**
    * @param owner the owner to set
    */
   public void setOwner(long owner)
   {
      this.owner = owner;
   }
}
