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
** File: getwsparam.cpp
**
**/

#include "nxagentd.h"
#include <curl/curl.h>
#include <netxms-regex.h>

#ifndef CURL_MAX_HTTP_HEADER
// workaround for older cURL versions
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

#define DEBUG_TAG

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
   ~ServiceEntry();

   void getParams(StringList *params, NXCPMessage *response);
   bool isDataExpired(UINT32 retentionTime) { return (time(NULL) - m_lastRequestTime) >= retentionTime; }
   UINT32 updateData(const TCHAR *url, const char *userName, const char *password, long authType, struct curl_slist *headers, bool peerVerify, const char *topLevelName);

   void lock() { m_lock.lock(); }
   void unlock() { m_lock.unlock(); }
};

/**
 * Static data
 */
Mutex s_serviceCacheLock;
StringObjectMap<ServiceEntry> s_sericeCashe(true);

/**
 * Destructor
 */
ServiceEntry::~ServiceEntry()
{
   if (m_json != NULL)
      json_decref(m_json);
}

/**
 * Get parameters from XML cashed data
 */
void ServiceEntry::getParamsFromXML(StringList *params, NXCPMessage *response)
{
   UINT32 fieldId = VID_PARAM_LIST_BASE;
   int resultCount = 0;
   AgentWriteLog(3, _T("Param list: %d"), params->size());
   AgentWriteLog(3, _T("XML: %s"), (const TCHAR *)m_xml.createXml());
   for (int i = 0; i < params->size(); i++)
   {
      const  TCHAR *root = m_xml.getEntry(_T("/"))->getName();
      const TCHAR *result = m_xml.getValue(params->get(i), NULL);
      if (result != NULL)
      {
         response->setField(fieldId++, params->get(i));
         response->setField(fieldId++, result);
         resultCount++;
      }
   }
   response->setField(VID_NUM_PARAMETERS, resultCount);
}

/**
 * Create NXSL value from json_t
 */
static bool SetValueFromJson(json_t *json, UINT32 fieldId, NXCPMessage *response)
{
   bool skip = false;
   TCHAR result[MAX_RESULT_LENGTH];
   switch(json_typeof(json))
   {
      case JSON_OBJECT:
         skip = true;
         break;
      case JSON_ARRAY:
         skip = true;
         break;
      case JSON_STRING:
         response->setFieldFromUtf8String(fieldId, json_string_value(json));
         break;
      case JSON_INTEGER:
         ret_int64(result, static_cast<INT64>(json_integer_value(json)));
         response->setField(fieldId, result);
         break;
      case JSON_REAL:
         ret_double(result, json_real_value(json));
         response->setField(fieldId, result);
         break;
      case JSON_TRUE:
         response->setField(fieldId, _T("true"));
         break;
      case JSON_FALSE:
         response->setField(fieldId, _T("false"));
         break;
      default:
         skip = true;
         break;
   }
   return !skip;
}


/**
 * Get parameters from JSON cashed data
 */
void ServiceEntry::getParamsFromJSON(StringList *params, NXCPMessage *response)
{
   UINT32 fieldId = VID_PARAM_LIST_BASE;
   int resultCount = 0;
   for (int i = 0; i < params->size(); i++)
   {
      json_t *lastObj = m_json;
#ifdef UNICODE
      char *copy = UTF8StringFromWideString(params->get(i));
#else
      char *copy = UTF8StringFromMBString(params->get(i));
#endif
      char *item = copy;
      char *separator = NULL;
      if (!strncmp(item, "/", 1))
         item++;
      do
      {
         separator = strchr(item, '/');
         if(separator != NULL)
            *separator = 0;

         lastObj = json_object_get(lastObj, item);
         if(separator != NULL)
            item = separator+1;
      } while (separator != NULL && *item != 0 && lastObj != NULL);
      MemFree(copy);
      if (lastObj != NULL)
      {
         response->setField(fieldId++, params->get(i));
         if(SetValueFromJson(lastObj, fieldId, response))
         {
            fieldId++;
            resultCount++;
         }
         else
         {
            fieldId--;
         }
      }
   }
   response->setField(VID_NUM_PARAMETERS, resultCount);
}

/**
 * Get parameters from Text cashed data
 */
void ServiceEntry::getParamsFromText(StringList *params, NXCPMessage *response)
{
   StringList *list = m_responseData.split(_T("\n"));
   UINT32 fieldId = VID_PARAM_LIST_BASE;
   int resultCount = 0;
   for (int i = 0; i < params->size(); i++)
   {
      AgentWriteLog(3, _T("Try to find"));
      const char *eptr;
      int eoffset;
      PCRE *compiledPattern = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(params->get(i)), PCRE_COMMON_FLAGS, &eptr, &eoffset, NULL);
      if (compiledPattern == NULL)
      {
         continue;
      }
      TCHAR *matchedString = NULL;
      for (int j = 0; j < list->size(); j++)
      {
         AgentWriteLog(3, _T("Try to find: %s"), list->get(j));
         int fields[30];
         if (_pcre_exec_t(compiledPattern, NULL, reinterpret_cast<const PCRE_TCHAR*>(list->get(j)), static_cast<int>(wcslen(list->get(j))), 0, 0, fields, 30) >= 0)
         {
            if (fields[2] != -1)
            {
               matchedString = MemAllocString(fields[3] + 1 - fields[2]);
               memcpy(matchedString, &list->get(j)[fields[2]], (fields[3] - fields[2]) * sizeof(TCHAR));
               matchedString[fields[3] - fields[2]] = 0;
               AgentWriteLog(3, _T("Try to find: %s"), matchedString);
            }
            break;
         }
      }

      if (matchedString != NULL)
      {
         response->setField(fieldId++, params->get(i));
         response->setField(fieldId++, matchedString);
         resultCount++;
      }
   }
   response->setField(VID_NUM_PARAMETERS, resultCount);
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
UINT32 ServiceEntry::updateData(const TCHAR *url, const char *userName, const char *password, long authType, struct curl_slist *headers, bool peerVerify, const char *topLevelName)
{
   UINT32 rcc = RCC_SUCCESS;
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

#ifdef UNICODE
      char *urlUtf8 = UTF8StringFromWideString(url);
#else
      char *urlUtf8 = UTF8StringFromMBString(url);
#endif
      AgentWriteLog(3, _T("Url: %hs"), urlUtf8);
      if (curl_easy_setopt(curl, CURLOPT_URL, urlUtf8) == CURLE_OK)
      {
         int resp = curl_easy_perform(curl);
         if (resp == CURLE_OK)
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
                  AgentWriteLog(3, _T("Top level tag: %hs"), topLevelName);
                  if (!m_xml.loadXmlConfigFromMemory(content, static_cast<int>(strlen(content)), NULL, "*", false))
                     AgentWriteLog(3, _T("Failed to load XML"));
                  AgentWriteLog(3, _T("XML loaded"));
                  MemFree(content);
               }
               else if(m_responseData.startsWith(_T("{")))
               {
                  m_type = TextType::JSON;
                  char *content = m_responseData.getUTF8String();
                  json_error_t error;
                  if (m_json != NULL)
                  {
                     json_decref(m_json);
                  }
                  m_json = json_loads(content, 0, &error);
                  MemFree(content);
               }
               else
               {
                  m_type = TextType::Text;
               }
               m_lastRequestTime = time(NULL);
               AgentWriteLog(3, _T("Type: %d"), m_type);
               AgentWriteLog(3, _T("Response data: %s"), (const TCHAR *)m_responseData);
               AgentWriteLog(3, _T("Response data lenght: %d"), m_responseData.length());
            }
            else
            {
               AgentWriteLog(3, _T("Get data from service: request returned empty document"));
               rcc = RCC_INTERNAL_ERROR; //TODO: fix all errors and debug messages
            }
         }
         else
         {
            AgentWriteLog(3, _T("Get data from service: error making curl request: %d"), resp);
            rcc = RCC_INTERNAL_ERROR; //TODO: fix all errors and debug messages
         }
      }
      else
      {
         AgentWriteLog(3, _T("Get data from service: curl_easy_setopt with url failed"));
         rcc = RCC_INTERNAL_ERROR;
      }
      MemFree(urlUtf8);
   }
   else
   {
      AgentWriteLog(3, _T("Get data from service: curl_init failed"));
      rcc = RCC_INTERNAL_ERROR;
   }

   curl_easy_cleanup(curl);
   return rcc;
}

void GetWebServiceParameters(NXCPMessage *request, NXCPMessage *response)
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
      char *topLevelName = request->getFieldAsUtf8String(VID_PARAM_LIST_BASE);
      char *separator = strchr(topLevelName+1, '/');
      if(separator != NULL)
         *separator = 0;

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
      result = cashedEntry->updateData(url, login, password, request->getFieldAsUInt64(VID_AUTH_TYPE), headers, request->getFieldAsBoolean(VID_VERIFY_CERT), topLevelName+1);

      curl_slist_free_all(headers);
      MemFree(login);
      MemFree(password);
      MemFree(topLevelName);
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
