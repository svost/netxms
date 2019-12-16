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
   clear();
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
   MutexUnlock(m_tailLock);

   if (InterlockedIncrement(&m_size) == 1)
      ConditionSet(m_condWakeup);
}

/**
 * Insert new element into the beginning of a queue
 */
void Queue::insert(void *value)
{
}

/**
 * Get object from queue. Return NULL if queue is empty
 */
void *Queue::get()
{
   MutexLock(m_headLock);
   QueueElement *head = m_head;
   QueueElement *newHead = head->next;

   void *value;
   if (newHead != NULL)
   {
      value = newHead->value;
      m_head = newHead;
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
}

/**
 * Set shutdown flag
 * When this flag is set, Get() always return INVALID_POINTER_VALUE
 */
void Queue::setShutdownMode()
{
}

/**
 * Find element in queue using given key and comparator
 * Returns pointer to element (optionally transformed) or NULL if element was not found.
 * Element remains in the queue
 */
void *Queue::find(const void *key, QueueComparator comparator, void *(*transform)(void*))
{
   return NULL;
}

/**
 * Find element in queue using given key and comparator and remove it.
 * Returns true if element was removed.
 */
bool Queue::remove(const void *key, QueueComparator comparator)
{
	bool success = false;

	return success;
}

/**
 * Enumerate queue elements
 */
void Queue::forEach(QueueEnumerationCallback callback, void *context)
{
}
