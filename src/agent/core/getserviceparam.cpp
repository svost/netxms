/*
** NetXMS multiplatform core agent
** Copyright (C) 2020 Raden Solutions
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
** File: getserviceparam.cpp
**
**/

#include "nxagentd.h"
#include <curl/curl.h>
#include <netxms-regex.h>

#ifndef CURL_MAX_HTTP_HEADER
// workaround for older cURL versions
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

enum class TextType
{
   XML = 0,
   JSON = 1,
   Text = 2
};

/**
 * One cashed service entry
 */
class ServiceEntry
{
private:
   time_t m_lastRequestTime;
   StringBuffer m_responseData;
   Mutex m_lock;
   TextType m_type;
   Config m_xml;
   json_t *m_json;

   void getParamsFromXML(StringList *params, NXCPMessage *response);
   void getParamsFromJSON(StringList *params, NXCPMessage *response);
   void getParamsFromText(StringList *params, NXCPMessage *response);

public:
   ServiceEntry() { m_lastRequestTime = 0; m_type = TextType::Text; m_json = NULL; }

   void getParams(StringList *params, NXCPMessage *response);
   bool isDataExpired(UINT32 retentionTime) { return (time(NULL) - m_lastRequestTime) >= retentionTime; }
   UINT32 updateData(const TCHAR *url, const char *userName, const char *password, long authType, struct curl_slist *headers, bool peerVerify);

   void lock() { m_lock.lock(); }
   void unlock() { m_lock.unlock(); }
};

/**
 * Static data
 */
Mutex s_serviceCacheLock;
StringObjectMap<ServiceEntry> s_sericeCashe(true);

/**
 * Get parameters from XML cashed data
 */
void ServiceEntry::getParamsFromXML(StringList *params, NXCPMessage *response)
{
   UINT32 fieldId = VID_PARAM_LIST_BASE;
   for (int i = 0; i < params->size(); i++)
   {
      response->setField(fieldId++, params->get(i));
      response->setField(fieldId++, m_xml.getValue(params->get(i), NULL));
   }
   response->setField(VID_NUM_PARAMETERS, params->size());
}

/**
 * Get parameters from JSON cashed data
 */
void ServiceEntry::getParamsFromJSON(StringList *params, NXCPMessage *response)
{
   UINT32 fieldId = VID_PARAM_LIST_BASE;
   for (int i = 0; i < params->size(); i++)
   {
      response->setField(fieldId++, params->get(i));

      json_t *lastObj = m_json;
      TCHAR *item = MemCopyString(params->get(i));
      TCHAR *separator = NULL;
      do
      {
         separator = _tcschr(item, _T('\\'));
         if(separator != NULL)
            *separator = 0;

         char attr[256];
#ifdef UNICODE
         WideCharToMultiByte(CP_UTF8, 0, item, -1, attr, 256, NULL, NULL);
#else
         mb_to_utf8(item, -1, attr, 256);
#endif
         attr[255] = 0;

         lastObj = json_object_get(lastObj, attr);
         if(separator != NULL)
            item = separator+1;
      } while (separator != NULL && *item != 0);
      MemFree(item);
      response->setFieldFromUtf8String(fieldId++, lastObj != NULL ? json_string_value(lastObj) : NULL);
   }
   response->setField(VID_NUM_PARAMETERS, params->size());
}

/**
 * Get parameters from Text cashed data
 */
void ServiceEntry::getParamsFromText(StringList *params, NXCPMessage *response)
{
   StringList *list = m_responseData.split(_T("\n"));
   UINT32 fieldId = VID_PARAM_LIST_BASE;
   for (int i = 0; i < params->size(); i++)
   {
      response->setField(fieldId++, params->get(i));

      const char *eptr;
      int eoffset;
      PCRE *compiledPattern = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(params->get(i)), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, NULL);
      if (compiledPattern == NULL)
      {
         response->setField(fieldId++, static_cast<const TCHAR *>(NULL));
         continue;
      }
      TCHAR *matchedString = NULL;
      for (int j = 0; j < list->size(); j++)
      {
         int fields[30];
         if (_pcre_exec_t(compiledPattern, NULL, reinterpret_cast<const PCRE_TCHAR*>(list->get(j)), static_cast<int>(wcslen(list->get(j))), 0, 0, fields, 30) >= 0)
         {
            if (fields[2] != -1)
            {
               matchedString = MemAllocString(fields[3] + 1 - fields[2]);
               memcpy(matchedString, &list->get(j)[fields[2]], (fields[3] - fields[2]) * sizeof(TCHAR));
               matchedString[fields[3] - fields[2]] = 0;
            }
            break;
         }
      }
      response->setField(fieldId++, matchedString);
   }
   delete list;
}

/**
 * Get parameters from cashed data
 */
void ServiceEntry::getParams(StringList *params, NXCPMessage *response)
{
   if(m_type == TextType::XML)
   {
      getParamsFromXML(params, response);
   }
   else if(m_type == TextType::JSON)
   {
      getParamsFromJSON(params, response);
   }
   else
   {
      getParamsFromText(params, response);
   }
}

/**
 * Callback for processing data received from cURL
 */
static size_t OnCurlDataReceived(char *ptr, size_t size, size_t nmemb, void *userdata)
{
   ByteStream *data = (ByteStream *)userdata;
   size_t bytes = size * nmemb;
   data->write(ptr, bytes);
   return bytes;
}

/**
 * Update casched data
 */
UINT32 ServiceEntry::updateData(const TCHAR *url, const char *userName, const char *password, long authType, struct curl_slist *headers, bool peerVerify)
{
   UINT32 rcc;
   CURL *curl = curl_easy_init();
   if (curl != NULL)
   {
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_USERNAME, userName);
      curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
      curl_easy_setopt(curl, CURLOPT_HTTPAUTH, authType); //Use CURLAUTH_ANY by default
      curl_easy_setopt(curl, CURLOPT_HEADER, (long)0);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &OnCurlDataReceived);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS agent/" NETXMS_VERSION_STRING_A);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, peerVerify ? 1 : 0);

      // Receiving buffer
      ByteStream data(32768);
      data.setAllocationStep(32768);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

      //convert url to char
      if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
      {
         if (curl_easy_perform(curl) == 0)
         {
            if(data.size() > 0)
            {
               data.write('\0');
               size_t size;
               m_responseData.clear();
#ifdef UNICODE
               WCHAR *wtext = WideStringFromUTF8String((char *)data.buffer(&size));
               m_responseData.appendPreallocated(wtext);
#else
               char *text = MBStringFromUTF8String((char *)data.buffer(&size));
               m_responseData.appendPreallocated(text);
#endif
               if(m_responseData.startsWith(_T("<")))
               {
                  m_type = TextType::XML;
                  char *content = m_responseData.getUTF8String();
                  m_xml.loadXmlConfigFromMemory(content, strlen(content), NULL, NULL, false);
                  MemFree(content);
               }
               else if(m_responseData.startsWith(_T("{")))
               {
                  m_type = TextType::JSON;
                  char *content = m_responseData.getUTF8String();
                  json_error_t error;
                  if(m_json != NULL)
                  {
                     json_decref(m_json);
                  }
                  json_t *json = json_loads(content, 0, &error);
                  MemFree(content);
               }
               else
               {
                  m_type = TextType::Text;
               }
            }
            else
            {
               AgentWriteLog(3, _T("Get data from service: request returned empty document"));
               rcc = RCC_INTERNAL_ERROR; //TODO: fix all errors and debug messages
            }
         }
         else
         {
            AgentWriteLog(3, _T("Get data from service: error making curl request"));
            rcc = RCC_INTERNAL_ERROR; //TODO: fix all errors and debug messages
         }
      }
      else
      {
         AgentWriteLog(3, _T("Get data from service: curl_easy_setopt with url failed"));
         rcc = RCC_INTERNAL_ERROR;
      }
   }
   else
   {
      AgentWriteLog(3, _T("Get data from service: curl_init failed"));
      rcc = RCC_INTERNAL_ERROR;
   }

   curl_easy_cleanup(curl);
   return rcc;
}

void GetServiceParameters(NXCPMessage *request, NXCPMessage *response)
{
   TCHAR *url = request->getFieldAsString(VID_URL);

   s_serviceCacheLock.lock();
   ServiceEntry *cashedEntry = s_sericeCashe.get(url);
   if(cashedEntry == NULL)
   {
      cashedEntry = new ServiceEntry();
      s_sericeCashe.set(url, cashedEntry);
   }
   s_serviceCacheLock.unlock();

   cashedEntry->lock();
   UINT32 retentionTime = request->getFieldAsUInt32(VID_RETENTION_TIME);
   UINT32 result = RCC_SUCCESS;
   if (cashedEntry->isDataExpired(retentionTime))
   {
      char *login = request->getFieldAsUtf8String(VID_LOGIN_NAME);
      char *password = request->getFieldAsUtf8String(VID_PASSWORD);
      struct curl_slist *headers = NULL;
      UINT32 headerSize = request->getFieldAsUInt32(VID_NUM_HEADERS);
      UINT32 fieldId = VID_HEADER_BASE;
      char header[CURL_MAX_HTTP_HEADER];
      for(int i = 0; i < headerSize; i++)
      {
         headers = curl_slist_append(headers, request->getFieldAsUtf8String(fieldId++, header, CURL_MAX_HTTP_HEADER));
      }
      result = cashedEntry->updateData(url, login, password, request->getFieldAsUInt64(VID_AUTH_TYPE), headers, request->getFieldAsBoolean(VID_VERIFY_CERT));

      curl_slist_free_all(headers);
      MemFree(login);
      MemFree(password);
   }

   if(result == RCC_SUCCESS)
   {
      StringList params(request, VID_PARAM_LIST_BASE, VID_NUM_PARAMETERS);
      cashedEntry->getParams(&params, response);
   }
   cashedEntry->unlock();

   response->setField(VID_RCC, result);
   MemFree(url);
}
