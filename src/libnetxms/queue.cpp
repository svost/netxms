/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: queue.cpp
**
**/

#include "libnetxms.h"
#include <nxqueue.h>

/**
 * Queue constructor
 */
Queue::Queue(size_t regionCapacity, bool owner) : m_elements(regionCapacity, true)
{
   m_owner = owner;
	commonInit();
}

/**
 * Default queue constructor
 */
Queue::Queue(bool owner) : m_elements(256, true)
{
   m_owner = owner;
	commonInit();
}

/**
 * Common initialization (used by all constructors)
 */
void Queue::commonInit()
{
   m_headLock = MutexCreate();
   m_tailLock = MutexCreate();
   m_condWakeup = ConditionCreate(FALSE);
   m_head = m_elements.allocate();
   memset(m_head, 0, sizeof(QueueElement));
   m_tail = m_head;
   m_size = 0;
	m_shutdownFlag = false;
	m_destructor = MemFree;
}

/**
 * Destructor
 */
Queue::~Queue()
{
   if (m_owner)
   {
      for(QueueElement *e = m_head->next; e != NULL; e = e->next)
         m_destructor(e->value);
   }
   MutexDestroy(m_headLock);
   MutexDestroy(m_tailLock);
   ConditionDestroy(m_condWakeup);
}

/**
 * Put new element into queue
 */
void Queue::put(void *value)
{
   QueueElement *element = m_elements.allocate();
   element->next = NULL;
   element->value = value;

   MutexLock(m_tailLock);
   m_tail->next = element;
   m_tail = element;

   if (InterlockedIncrement(&m_size) == 1)
      ConditionSet(m_condWakeup);

   MutexUnlock(m_tailLock);
}

/**
 * Insert new element into the beginning of a queue
 */
void Queue::insert(void *value)
{
   QueueElement *element = m_elements.allocate();
   element->value = value;

   MutexLock(m_headLock);
   MutexLock(m_tailLock);

   element->next = m_head->next;
   m_head->next = element;

   if (InterlockedIncrement(&m_size) == 1)
      ConditionSet(m_condWakeup);

   MutexUnlock(m_tailLock);
   MutexUnlock(m_headLock);
}

/**
 * Get object from queue. Return NULL if queue is empty
 */
void *Queue::get()
{
   if (m_shutdownFlag)
      return INVALID_POINTER_VALUE;

   MutexLock(m_headLock);
   QueueElement *head = m_head;
   QueueElement *newHead = head->next;

   void *value;
   if (newHead != NULL)
   {
      value = newHead->value;
      m_head = newHead;
      m_elements.free(head);
      InterlockedDecrement(&m_size);
   }
   else
   {
      value = NULL;
   }
   MutexUnlock(m_headLock);
   return value;
}

/**
 * Get object from queue or block with timeout if queue if empty
 */
void *Queue::getOrBlock(UINT32 timeout)
{
   void *value = get();
   if (value != NULL)
   {
      return value;
   }

   do
   {
      if (!ConditionWait(m_condWakeup, timeout))
         break;
      value = get();
   } while(value == NULL);
   return value;
}

/**
 * Clear queue
 */
void Queue::clear()
{
   MutexLock(m_headLock);
   MutexLock(m_tailLock);

   if (m_owner)
   {
      for(QueueElement *e = m_head->next; e != NULL; e = e->next)
         m_destructor(e->value);
   }

   m_elements.clear();

   m_head = m_elements.allocate();
   memset(m_head, 0, sizeof(QueueElement));
   m_tail = m_head;
   m_size = 0;

   MutexUnlock(m_tailLock);
   MutexUnlock(m_headLock);
}

/**
 * Set shutdown flag
 * When this flag is set, Get() always return INVALID_POINTER_VALUE
 */
void Queue::setShutdownMode()
{
   m_shutdownFlag = true;
   ConditionSet(m_condWakeup);
}

/**
 * Find element in queue using given key and comparator
 * Returns pointer to element (optionally transformed) or NULL if element was not found.
 * Element remains in the queue
 */
void *Queue::find(const void *key, QueueComparator comparator, void *(*transform)(void*))
{
   void *value = NULL;

   MutexLock(m_headLock);
   MutexLock(m_tailLock);

   for(QueueElement *e = m_head->next; e != NULL; e = e->next)
   {
      if (comparator(key, e->value))
      {
         value = (transform != NULL) ? transform(e->value) : e->value;
         break;
      }
   }

   MutexUnlock(m_tailLock);
   MutexUnlock(m_headLock);

   return value;
}

/**
 * Find element in queue using given key and comparator and remove it.
 * Returns true if element was removed.
 */
bool Queue::remove(const void *key, QueueComparator comparator)
{
	bool success = false;

   MutexLock(m_headLock);
   MutexLock(m_tailLock);

   QueueElement *prev = m_head;
   for(QueueElement *e = m_head->next; e != NULL; e = e->next)
   {
      if (comparator(key, e->value))
      {
         prev->next = e->next;
         if (m_tail == e)
            m_tail = prev;
         if (m_owner)
            m_destructor(e->value);
         m_elements.free(e);
         InterlockedDecrement(&m_size);
         success = true;
         break;
      }
      prev = e;
   }

   MutexUnlock(m_tailLock);
   MutexUnlock(m_headLock);

	return success;
}

/**
 * Enumerate queue elements
 */
void Queue::forEach(QueueEnumerationCallback callback, void *context)
{
   MutexLock(m_headLock);
   MutexLock(m_tailLock);

   for(QueueElement *e = m_head->next; e != NULL; e = e->next)
   {
      if (callback(e->value, context) == _STOP)
         break;
   }

   MutexUnlock(m_tailLock);
   MutexUnlock(m_headLock);
}
