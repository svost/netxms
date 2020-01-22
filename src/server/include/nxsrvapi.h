/*
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2020 Reden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: nxsrvapi.h
**
**/

#ifndef _nxsrvapi_h_
#define _nxsrvapi_h_

#ifdef LIBNXSRV_EXPORTS
#define LIBNXSRV_EXPORTABLE __EXPORT
#define LIBNXSRV_EXPORTABLE_VAR(v) __EXPORT_VAR(v)
#else
#define LIBNXSRV_EXPORTABLE __IMPORT
#define LIBNXSRV_EXPORTABLE_VAR(v) __IMPORT_VAR(v)
#endif

#include <nxcpapi.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <nxsnmp.h>
#include <netxms_isc.h>
#include <nxcldefs.h>
#include <nxsl.h>

/**
 * Default files
 */
#ifdef _WIN32

#define DEFAULT_LOG_FILE      _T("C:\\netxmsd.log")
#define DEFAULT_DUMP_DIR      _T("C:\\")

#define LDIR_NCD              _T("\\ncdrv")
#define LDIR_NDD              _T("\\ndd")
#define LDIR_PDSDRV           _T("\\pdsdrv")

#define DDIR_PACKAGES         _T("\\packages")
#define DDIR_BACKGROUNDS      _T("\\backgrounds")
#define DFILE_KEYS            _T("\\server_key")
#define DFILE_COMPILED_MIB    _T("\\netxms.mib")
#define DDIR_IMAGES           _T("\\images")
#define DDIR_FILES            _T("\\files")

#define SDIR_SCRIPTS          _T("\\scripts")
#define SDIR_TEMPLATES        _T("\\templates")
#define SFILE_RADDICT         _T("\\radius.dict")

#else    /* _WIN32 */

#ifndef DATADIR
#define DATADIR              _T("/usr/share/netxms")
#endif

#ifndef STATEDIR
#define STATEDIR             _T("/var/lib/netxms")
#endif

#ifndef LIBDIR
#define LIBDIR               _T("/usr/lib")
#endif

#ifndef PKGLIBDIR
#define PKGLIBDIR            _T("/usr/lib/netxms")
#endif

#define DEFAULT_LOG_FILE      _T("/var/log/netxmsd.log")
#define DEFAULT_DUMP_DIR      _T("/var/tmp")

#define LDIR_NCD              _T("/ncdrv")
#define LDIR_NDD              _T("/ndd")
#define LDIR_PDSDRV           _T("/pdsdrv")

#define DDIR_PACKAGES         _T("/packages")
#define DDIR_BACKGROUNDS      _T("/backgrounds")
#define DFILE_KEYS            _T("/.server_key")
#define DFILE_COMPILED_MIB    _T("/netxms.mib")
#define DDIR_IMAGES           _T("/images")
#define DDIR_FILES            _T("/files")

#define SDIR_SCRIPTS          _T("/scripts")
#define SDIR_TEMPLATES        _T("/templates")
#define SFILE_RADDICT         _T("/radius.dict")

#endif   /* _WIN32 */

/**
 * Debug tags
 */
#define DEBUG_TAG_TOPO_ARP    _T("topology.arp")

/**
 * Application flags
 */
#define AF_DAEMON                              _ULL(0x0000000000000001)
#define AF_USE_SYSLOG                          _ULL(0x0000000000000002)
#define AF_PASSIVE_NETWORK_DISCOVERY           _ULL(0x0000000000000004)
#define AF_ACTIVE_NETWORK_DISCOVERY            _ULL(0x0000000000000008)
#define AF_LOG_SQL_ERRORS                      _ULL(0x0000000000000010)
#define AF_DELETE_EMPTY_SUBNETS                _ULL(0x0000000000000020)
#define AF_ENABLE_SNMP_TRAPD                   _ULL(0x0000000000000040)
#define AF_ENABLE_ZONING                       _ULL(0x0000000000000080)
#define AF_SYNC_NODE_NAMES_WITH_DNS            _ULL(0x0000000000000100)
#define AF_CHECK_TRUSTED_NODES                 _ULL(0x0000000000000200)
#define AF_ENABLE_NXSL_CONTAINER_FUNCTIONS     _ULL(0x0000000000000400)
#define AF_USE_FQDN_FOR_NODE_NAMES             _ULL(0x0000000000000800)
#define AF_APPLY_TO_DISABLED_DCI_FROM_TEMPLATE _ULL(0x0000000000001000)
#define AF_DEBUG_CONSOLE_DISABLED              _ULL(0x0000000000002000)
#define AF_ENABLE_OBJECT_TRANSACTIONS          _ULL(0x0000000000004000)
#define AF_WRITE_FULL_DUMP                     _ULL(0x0000000000080000)
#define AF_RESOLVE_NODE_NAMES                  _ULL(0x0000000000100000)
#define AF_CATCH_EXCEPTIONS                    _ULL(0x0000000000200000)
#define AF_HELPDESK_LINK_ACTIVE                _ULL(0x0000000000400000)
#define AF_DB_LOCKED                           _ULL(0x0000000001000000)
#define AF_DB_CONNECTION_LOST                  _ULL(0x0000000004000000)
#define AF_NO_NETWORK_CONNECTIVITY             _ULL(0x0000000008000000)
#define AF_EVENT_STORM_DETECTED                _ULL(0x0000000010000000)
#define AF_SNMP_TRAP_DISCOVERY                 _ULL(0x0000000020000000)
#define AF_TRAPS_FROM_UNMANAGED_NODES          _ULL(0x0000000040000000)
#define AF_RESOLVE_IP_FOR_EACH_STATUS_POLL     _ULL(0x0000000080000000)
#define AF_PERFDATA_STORAGE_DRIVER_LOADED      _ULL(0x0000000100000000)
#define AF_BACKGROUND_LOG_WRITER               _ULL(0x0000000200000000)
#define AF_CASE_INSENSITIVE_LOGINS             _ULL(0x0000000400000000)
#define AF_TRAP_SOURCES_IN_ALL_ZONES           _ULL(0x0000000800000000)
#define AF_SYSLOG_DISCOVERY                    _ULL(0x0000001000000000)
#define AF_ENABLE_LOCAL_CONSOLE                _ULL(0x0000002000000000)
#define AF_CACHE_DB_ON_STARTUP                 _ULL(0x0000004000000000)
#define AF_ENABLE_NXSL_FILE_IO_FUNCTIONS       _ULL(0x0000008000000000)
#define AF_ENABLE_EMBEDDED_PYTHON              _ULL(0x0000010000000000)
#define AF_DB_SUPPORTS_MERGE                   _ULL(0x0000020000000000)
#define AF_PARALLEL_NETWORK_DISCOVERY          _ULL(0x0000040000000000)
#define AF_SINGLE_TABLE_PERF_DATA              _ULL(0x0000080000000000)
#define AF_MERGE_DUPLICATE_NODES               _ULL(0x0000100000000000)
#define AF_SYSTEMD_DAEMON                      _ULL(0x0000200000000000)
#define AF_USE_SYSTEMD_JOURNAL                 _ULL(0x0000400000000000)
#define AF_COLLECT_ICMP_STATISTICS             _ULL(0x0000800000000000)
#define AF_LOG_IN_JSON_FORMAT                  _ULL(0x0001000000000000)
#define AF_LOG_TO_STDOUT                       _ULL(0x0002000000000000)
#define AF_SERVER_INITIALIZED                  _ULL(0x4000000000000000)
#define AF_SHUTDOWN                            _ULL(0x8000000000000000)

/**
 * Encryption usage policies
 */
#define ENCRYPTION_DISABLED   0
#define ENCRYPTION_ALLOWED    1
#define ENCRYPTION_PREFERRED  2
#define ENCRYPTION_REQUIRED   3

/**
 * Data collection errors
 */
enum DataCollectionError
{
   DCE_SUCCESS           = 0,
   DCE_COMM_ERROR        = 1,
   DCE_NOT_SUPPORTED     = 2,
   DCE_IGNORE            = 3,
   DCE_NO_SUCH_INSTANCE  = 4,
   DCE_COLLECTION_ERROR  = 5,
   DCE_ACCESS_DENIED     = 6
};

/**
 * Agent action output callback events
 */
enum ActionCallbackEvent
{
   ACE_CONNECTED = 0,
   ACE_DATA = 1,
   ACE_DISCONNECTED = 2
};

/**
 * Win32 service and syslog constants
 */
#ifdef _WIN32

#define CORE_SERVICE_NAME     _T("NetXMSCore")
#define CORE_EVENT_SOURCE     _T("NetXMSCore")
#define NETXMSD_SYSLOG_NAME   CORE_EVENT_SOURCE

#else

#define NETXMSD_SYSLOG_NAME   _T("netxmsd")

#endif   /* _WIN32 */

/**
 * Single ARP cache entry
 */
struct ArpEntry
{
   UINT32 ifIndex;       // Interface index, 0 if unknown
   InetAddress ipAddr;
   MacAddress macAddr;

   ArpEntry(const InetAddress& _ipAddr, const MacAddress& _macAddr, UINT32 _ifIndex) : ipAddr(_ipAddr), macAddr(_macAddr) { ifIndex = _ifIndex; }
};

/**
 * ARP cache structure used by discovery functions and AgentConnection class
 */
class LIBNXSRV_EXPORTABLE ArpCache : public RefCountObject
{
private:
   ObjectArray<ArpEntry> *m_entries;
   HashMap<InetAddress, ArpEntry> *m_ipIndex;
   time_t m_timestamp;

protected:
   virtual ~ArpCache();

public:
   ArpCache();

   void addEntry(ArpEntry *entry);
   void addEntry(const InetAddress& ipAddr, const MacAddress& macAddr, UINT32 ifIndex = 0) { addEntry(new ArpEntry(ipAddr, macAddr, ifIndex)); }

   int size() const { return m_entries->size(); }
   time_t timestamp() const { return m_timestamp; }
   const ArpEntry *get(int index) const { return m_entries->get(index); }
   const ArpEntry *findByIP(const InetAddress& addr);

   void dumpToLog() const;
};

/**
 * Interface physical location
 */
struct LIBNXSRV_EXPORTABLE InterfacePhysicalLocation
{
   UINT32 chassis;
   UINT32 module;
   UINT32 pic;
   UINT32 port;

   InterfacePhysicalLocation()
   {
      chassis = 0;
      module = 0;
      pic = 0;
      port = 0;
   }

   InterfacePhysicalLocation(UINT32 _chassis, UINT32 _module, UINT32 _pic, UINT32 _port)
   {
      chassis = _chassis;
      module = _module;
      pic = _pic;
      port = _port;
   }

   bool equals(const InterfacePhysicalLocation& l)
   {
      return (port == l.port) && (pic == l.pic) && (module == l.module) && (chassis == l.chassis);
   }

   TCHAR *toString(TCHAR *buffer, size_t size)
   {
      _sntprintf(buffer, size, _T("%u/%u/%u/%u"), chassis, module, pic, port);
      return buffer;
   }
};

/**
 * Interface information structure used by discovery functions and AgentConnection class
 */
class InterfaceInfo
{
private:
   void init()
   {
      name[0] = 0;
      description[0] = 0;
      alias[0] = 0;
      type = IFTYPE_OTHER;
      mtu = 0;
      speed = 0;
      bridgePort = 0;
      memset(macAddr, 0, sizeof(macAddr));
      isPhysicalPort = false;
      isSystem = false;
      parentIndex = 0;
   }

public:
   UINT32 index;
   TCHAR name[MAX_DB_STRING];			// Interface display name
	TCHAR description[MAX_DB_STRING];	// Value of ifDescr MIB variable for SNMP agents
	TCHAR alias[MAX_DB_STRING];	// Value of ifDescr MIB variable for SNMP agents
   UINT32 type;
   UINT32 mtu;
   UINT64 speed;  // interface speed in bits/sec
	UINT32 bridgePort;
	InterfacePhysicalLocation location;
   InetAddressList ipAddrList;
   BYTE macAddr[MAC_ADDR_LENGTH];
	bool isPhysicalPort;
   bool isSystem;
   UINT32 ifTableSuffix[16];   // actual ifTable suffix
   int ifTableSuffixLength;
   UINT32 parentIndex;

   InterfaceInfo(UINT32 ifIndex)
   {
      index = ifIndex;
      ifTableSuffixLength = 0;
      init();
   }

   InterfaceInfo(UINT32 ifIndex, int suffixLen, const UINT32 *suffix)
   {
      index = ifIndex;
      ifTableSuffixLength = ((suffixLen >= 0) && (suffixLen < 16)) ? suffixLen : 0;
      memcpy(ifTableSuffix, suffix, ifTableSuffixLength * sizeof(UINT32));
      init();
   }

   bool hasAddress(const InetAddress& addr) { return ipAddrList.hasAddress(addr); }
};

/**
 * Interface list used by discovery functions and AgentConnection class
 */
class LIBNXSRV_EXPORTABLE InterfaceList
{
private:
   ObjectArray<InterfaceInfo> *m_interfaces;
   void *m_data;                  // Can be used by custom enumeration handlers
   bool m_needPrefixWalk;

public:
	InterfaceList(int initialAlloc = 8);
	~InterfaceList();

   void add(InterfaceInfo *iface) { m_interfaces->add(iface); }
   void remove(int index) { m_interfaces->remove(index); }

	int size() { return m_interfaces->size(); }
	InterfaceInfo *get(int index) { return m_interfaces->get(index); }
	InterfaceInfo *findByIfIndex(UINT32 ifIndex);
   InterfaceInfo *findByPhysicalLocation(const InterfacePhysicalLocation &loc);
   InterfaceInfo *findByPhysicalLocation(int chassis, int module, int pic, int port) { return findByPhysicalLocation(InterfacePhysicalLocation(chassis, module, pic, port)); }

	void setData(void *data) { m_data = data; }
	void *getData() { return m_data; }

   bool isPrefixWalkNeeded() { return m_needPrefixWalk; }
   void setPrefixWalkNeeded() { m_needPrefixWalk = true; }
};

#define VLAN_PRM_IFINDEX   0
#define VLAN_PRM_PHYLOC    1
#define VLAN_PRM_BPORT     2

/**
 * VLAN port reference
 */
struct VlanPortInfo
{
   UINT32 portId;    // device or driver specific port ID
   UINT32 objectId;
   UINT32 ifIndex;
   InterfacePhysicalLocation location;
};

/**
 * Vlan information
 */
class LIBNXSRV_EXPORTABLE VlanInfo
{
private:
	int m_vlanId;
	TCHAR *m_name;
	int m_portRefMode;	// Port reference mode - (by ifIndex, physical location, or bridge port number)
	int m_allocated;
	int m_numPorts;	// Number of ports in VLAN
	VlanPortInfo *m_ports;	// member ports (slot/port pairs or ifIndex)
	UINT32 m_nodeId;

public:
	VlanInfo(int vlanId, int prm);
	VlanInfo(const VlanInfo *src, UINT32 nodeId);
	~VlanInfo();

	int getVlanId() const { return m_vlanId; }
	int getPortReferenceMode() const { return m_portRefMode; }
	const TCHAR *getName() const { return CHECK_NULL_EX(m_name); }
	int getNumPorts() const { return m_numPorts; }
	VlanPortInfo *getPorts() { return m_ports; }
	UINT32 getNodeId() const { return m_nodeId; }

	void add(const InterfacePhysicalLocation& location);
	void add(UINT32 chassis, UINT32 module, UINT32 pic, UINT32 port) { add(InterfacePhysicalLocation(chassis, module, pic, port)); }
	void add(UINT32 portId);
	void setName(const TCHAR *name);

	void resolvePort(int index, const InterfacePhysicalLocation& location, UINT32 ifIndex, UINT32 id);
};

/**
 * Vlan list
 */
class LIBNXSRV_EXPORTABLE VlanList
{
private:
   int m_size;          // Number of valid entries
	int m_allocated;     // Number of allocated entries
   void *m_data;        // Can be used by custom enumeration handlers
   VlanInfo **m_vlans;  // VLAN entries

public:
	VlanList(int initialAlloc = 8);
	~VlanList();

	void add(VlanInfo *vlan);
	void addMemberPort(int vlanId, UINT32 portId);
   void addMemberPort(int vlanId, const InterfacePhysicalLocation& location);

	int size() { return m_size; }
	VlanInfo *get(int index) { return ((index >= 0) && (index < m_size)) ? m_vlans[index] : NULL; }
	VlanInfo *findById(int id);
	VlanInfo *findByName(const TCHAR *name);

	void setData(void *data) { m_data = data; }
	void *getData() { return m_data; }

	void fillMessage(NXCPMessage *msg);
};

#define CAF_INHERITABLE 1
#define CAF_REDEFINED   2

struct CustomAttribute
{
   SharedString value;
   UINT32 sourceObject;//original object of inheritance
   UINT32 flags;

   CustomAttribute(SharedString value, UINT32 flags, UINT32 sourceObject = 0)
   {
      this->value = value;
      this->flags = flags;
      this->sourceObject = sourceObject;
   }

   bool isInheritable() const
   {
      return (flags & CAF_INHERITABLE) > 0;
   }

   bool isRedefined() const
   {
      return (flags & CAF_REDEFINED) > 0;
   }

   bool isInherited() const
   {
      return sourceObject != 0;
   }

   json_t *toJson(const TCHAR *name)const
   {
      json_t *root = json_object();
      json_object_set_new(root, "name", json_string_t(name));
      json_object_set_new(root, "value", json_string_t(value));
      json_object_set_new(root, "flags", json_integer(flags));
      json_object_set_new(root, "sourceObject", json_integer(sourceObject));
      return root;
   }
};

class LIBNXSRV_EXPORTABLE NObject
{
   DISABLE_COPY_CTOR(NObject)

private:
   StringObjectMap<CustomAttribute> *m_customAttributes;
   MUTEX m_customAttributeLock;

   ObjectArray<NObject> *m_childList;     // Array of pointers to child objects
   ObjectArray<NObject> *m_parentList;    // Array of pointers to parent objects

   SharedString getCustomAttributeFromParent(const TCHAR *name);
   bool setCustomAttributeFromMessage(NXCPMessage *msg, UINT32 base);
   void setCustomAttribute(const TCHAR *name, SharedString value, UINT32 parent);
   void deletePopulatedCustomAttribute(const TCHAR *name);
   void populate(const TCHAR *name, SharedString value, UINT32 parentId);
   void populateRemove(const TCHAR *name);

protected:
   UINT32 m_id;
   uuid m_guid;
   TCHAR m_name[MAX_OBJECT_NAME];

   RWLOCK m_rwlockParentList; // Lock for parent list
   RWLOCK m_rwlockChildList;  // Lock for child list

   const ObjectArray<NObject> *getChildList() { return m_childList; }
   const ObjectArray<NObject> *getParentList() { return m_parentList; }

   void clearChildList() { m_childList->clear(); }
   void clearParentList();

   void lockCustomAttributes() const { MutexLock(m_customAttributeLock); }
   void unlockCustomAttributes() const { MutexUnlock(m_customAttributeLock); }

   void lockParentList(bool writeLock)
   {
      if (writeLock)
         RWLockWriteLock(m_rwlockParentList, INFINITE);
      else
         RWLockReadLock(m_rwlockParentList, INFINITE);
   }
   void unlockParentList() { RWLockUnlock(m_rwlockParentList); }
   void lockChildList(bool writeLock)
   {
      if (writeLock)
         RWLockWriteLock(m_rwlockChildList, INFINITE);
      else
         RWLockReadLock(m_rwlockChildList, INFINITE);
   }
   void unlockChildList() { RWLockUnlock(m_rwlockChildList); }

   virtual void onChildAdd();
   virtual void onParentRemove();
   virtual void onCustomAttributeChange();

public:
   NObject();
   virtual ~NObject();

   UINT32 getId() const { return m_id; }
   const uuid& getGuid() const { return m_guid; }
   const TCHAR *getName() const { return m_name; }

   void addChild(NObject *object);     // Add reference to child object
   void addParent(NObject *object);    // Add reference to parent object

   void deleteChild(NObject *object);  // Delete reference to child object
   void deleteParent(NObject *object); // Delete reference to parent object

   bool isChild(UINT32 id);
   bool isDirectChild(UINT32 id);
   bool isParent(UINT32 id);
   bool isDirectParent(UINT32 id);

   int getChildCount() const { return m_childList->size(); }
   int getParentCount() const { return m_parentList->size(); }

   TCHAR *getCustomAttribute(const TCHAR *name, TCHAR *buffer, size_t size) const;
   SharedString getCustomAttribute(const TCHAR *name) const;
   TCHAR *getCustomAttributeCopy(const TCHAR *name) const;
   INT32 getCustomAttributeAsInt32(const TCHAR *key, INT32 defaultValue) const;
   UINT32 getCustomAttributeAsUInt32(const TCHAR *key, UINT32 defaultValue) const;
   INT64 getCustomAttributeAsInt64(const TCHAR *key, INT64 defaultValue) const;
   UINT64 getCustomAttributeAsUInt64(const TCHAR *key, UINT64 defaultValue) const;
   double getCustomAttributeAsDouble(const TCHAR *key, double defaultValue) const;
   bool getCustomAttributeAsBoolean(const TCHAR *key, bool defaultValue) const;

   StringMap *getCustomAttributes(bool (*filter)(const TCHAR *, const CustomAttribute *, void *) = NULL, void *context = NULL) const;
   StringMap *getCustomAttributes(const TCHAR *regexp) const;

   void setCustomAttribute(const TCHAR *name, SharedString value, StateChange inheritable);
   void setCustomAttribute(const TCHAR *key, INT32 value);
   void setCustomAttribute(const TCHAR *key, UINT32 value);
   void setCustomAttribute(const TCHAR *key, INT64 value);
   void setCustomAttribute(const TCHAR *key, UINT64 value);

   void setCustomAttributesFromMessage(NXCPMessage *msg);
   void setCustomAttributesFromDatabase(DB_RESULT hResult);
   void deleteCustomAttribute(const TCHAR *name, bool force = false);
   NXSL_Value *getCustomAttributeForNXSL(NXSL_VM *vm, const TCHAR *name) const;
   NXSL_Value *getCustomAttributesForNXSL(NXSL_VM *vm) const;
   int getCustomAttributeSize() const { return m_customAttributes->size(); }

   template <typename C>
   EnumerationCallbackResult forEachCustomAttribute(EnumerationCallbackResult (*cb)(const TCHAR *, const CustomAttribute *, C *), C *context) const
   {
      lockCustomAttributes();
      EnumerationCallbackResult result =  m_customAttributes->forEach(reinterpret_cast<EnumerationCallbackResult (*)(const TCHAR*, const void*, void*)>(cb), (void *)context);
      unlockCustomAttributes();
      return result;
   }

   // Debug methods
   const TCHAR *dbgGetParentList(TCHAR *szBuffer);
   const TCHAR *dbgGetChildList(TCHAR *szBuffer);
};

/**
 * Route information
 */
typedef struct
{
   UINT32 dwDestAddr;
   UINT32 dwDestMask;
   UINT32 dwNextHop;
   UINT32 dwIfIndex;
   UINT32 dwRouteType;
} ROUTE;

/**
 * Routing table
 */
typedef struct
{
   int iNumEntries;     // Number of entries
   ROUTE *pRoutes;      // Route list
} ROUTING_TABLE;

/**
 * Information about policies installed on agent
 */
class LIBNXSRV_EXPORTABLE AgentPolicyInfo
{
private:
   bool m_newPolicyType;
	int m_size;
	BYTE *m_guidList;
	TCHAR **m_typeList;
   TCHAR **m_serverInfoList;
	UINT64 *m_serverIdList;
	int *m_version;

public:
	AgentPolicyInfo(NXCPMessage *msg);
	~AgentPolicyInfo();

	int size() { return m_size; }
	uuid getGuid(int index);
	const TCHAR *getType(int index) { return ((index >= 0) && (index < m_size)) ? m_typeList[index] : NULL; }
	const TCHAR *getServerInfo(int index) { return ((index >= 0) && (index < m_size)) ? m_serverInfoList[index] : NULL; }
	UINT64 getServerId(int index) { return ((index >= 0) && (index < m_size)) ? m_serverIdList[index] : 0; }
	int getVersion(int index) { return ((index >= 0) && (index < m_size)) ? m_version[index] : -1; }
	bool isNewPolicyType() { return m_newPolicyType; }
};

/**
 * Agent parameter definition
 */
class LIBNXSRV_EXPORTABLE AgentParameterDefinition
{
private:
   TCHAR *m_name;
   TCHAR *m_description;
   int m_dataType;

public:
   AgentParameterDefinition(NXCPMessage *msg, UINT32 baseId);
   AgentParameterDefinition(AgentParameterDefinition *src);
   AgentParameterDefinition(const TCHAR *name, const TCHAR *description, int dataType);
   ~AgentParameterDefinition();

   UINT32 fillMessage(NXCPMessage *msg, UINT32 baseId);

   const TCHAR *getName() { return m_name; }
   const TCHAR *getDescription() { return m_description; }
   int getDataType() { return m_dataType; }
};

/**
 * Agent table column definition
 */
struct AgentTableColumnDefinition
{
   TCHAR m_name[MAX_COLUMN_NAME];
   int m_dataType;

   AgentTableColumnDefinition(AgentTableColumnDefinition *src)
   {
      nx_strncpy(m_name, src->m_name, MAX_COLUMN_NAME);
      m_dataType = src->m_dataType;
   }
};

/**
 * Agent table definition
 */
class LIBNXSRV_EXPORTABLE AgentTableDefinition
{
private:
   TCHAR *m_name;
   TCHAR *m_description;
   StringList *m_instanceColumns;
   ObjectArray<AgentTableColumnDefinition> *m_columns;

public:
   AgentTableDefinition(NXCPMessage *msg, UINT32 baseId);
   AgentTableDefinition(AgentTableDefinition *src);
   ~AgentTableDefinition();

   UINT32 fillMessage(NXCPMessage *msg, UINT32 baseId);

   const TCHAR *getName() { return m_name; }
   const TCHAR *getDescription() { return m_description; }
};

/**
 * Remote file information
 */
class LIBNXSRV_EXPORTABLE RemoteFileInfo
{
private:
   TCHAR *m_name;
   UINT64 m_size;
   time_t m_mtime;
   BYTE m_hash[MD5_DIGEST_SIZE];
   UINT32 m_status;

public:
   RemoteFileInfo(NXCPMessage *msg, UINT32 baseId, const TCHAR *name);
   ~RemoteFileInfo();

   const TCHAR *name() const { return m_name; }
   UINT32 status() const { return m_status; }
   bool isValid() const { return m_status == ERR_SUCCESS; }
   UINT64 size() const { return m_size; }
   time_t modificationTime() const { return m_mtime; }
   const BYTE *hash() const { return m_hash; }
};

/**
 * Agent connection
 */
class LIBNXSRV_EXPORTABLE AgentConnection
{
private:
   VolatileCounter m_userRefCount;
   VolatileCounter m_internalRefCount;
   UINT32 m_debugId;
   InetAddress m_addr;
   int m_nProtocolVersion;
   bool m_controlServer;
   bool m_masterServer;
   int m_iAuthMethod;
   char m_szSecret[MAX_SECRET_LENGTH];
   time_t m_tLastCommandTime;
   AbstractCommChannel *m_channel;
   VolatileCounter m_requestId;
   UINT32 m_dwCommandTimeout;
	UINT32 m_connectionTimeout;
   UINT32 m_dwRecvTimeout;
   MsgWaitQueue *m_pMsgWaitQueue;
   bool m_isConnected;
   MUTEX m_mutexDataLock;
	MUTEX m_mutexSocketWrite;
   THREAD m_hReceiverThread;
   NXCPEncryptionContext *m_pCtx;
   int m_iEncryptionPolicy;
   bool m_useProxy;
   InetAddress m_proxyAddr;
   WORD m_wPort;
   WORD m_wProxyPort;
   int m_iProxyAuth;
   char m_szProxySecret[MAX_SECRET_LENGTH];
	int m_hCurrFile;
	TCHAR m_currentFileName[MAX_PATH];
	UINT32 m_dwDownloadRequestId;
	CONDITION m_condFileDownload;
	bool m_fileDownloadSucceeded;
	void (*m_downloadProgressCallback)(size_t, void *);
	void *m_downloadProgressCallbackArg;
	bool m_deleteFileOnDownloadFailure;
	void (*m_sendToClientMessageCallback)(NXCP_MESSAGE *, void *);
	bool m_fileUploadInProgress;
	bool m_allowCompression;
	VolatileCounter m_bulkDataProcessing;

   void receiverThread();
   static THREAD_RESULT THREAD_CALL receiverThreadStarter(void *);

   UINT32 setupEncryption(RSA *pServerKey);
   UINT32 authenticate(BOOL bProxyData);
   UINT32 setupProxyConnection();
   UINT32 prepareFileDownload(const TCHAR *fileName, UINT32 rqId, bool append,
            void (*downloadProgressCallback)(size_t, void *), void (*fileResendCallback)(NXCP_MESSAGE *, void *), void *cbArg);

   void processCollectedDataCallback(NXCPMessage *msg);
   void onDataPushCallback(NXCPMessage *msg);
   void onSnmpTrapCallback(NXCPMessage *msg);
   void onTrapCallback(NXCPMessage *msg);
   void onSyslogMessageCallback(NXCPMessage *msg);
   void postRawMessageCallback(NXCP_MESSAGE *msg);

protected:
   virtual ~AgentConnection();

   virtual AbstractCommChannel *createChannel();
   virtual void onTrap(NXCPMessage *pMsg);
   virtual void onSyslogMessage(NXCPMessage *pMsg);
   virtual void onDataPush(NXCPMessage *msg);
   virtual void onFileMonitoringData(NXCPMessage *msg);
   virtual void onSnmpTrap(NXCPMessage *pMsg);
   virtual void onFileDownload(bool success);
   virtual UINT32 processCollectedData(NXCPMessage *msg);
   virtual UINT32 processBulkCollectedData(NXCPMessage *request, NXCPMessage *response);
   virtual bool processCustomMessage(NXCPMessage *pMsg);
   virtual void processTcpProxyData(UINT32 channelId, const void *data, size_t size);

   const InetAddress& getIpAddr() const { return m_addr; }

	void debugPrintf(int level, const TCHAR *format, ...);

   void lock() { MutexLock(m_mutexDataLock); }
   void unlock() { MutexUnlock(m_mutexDataLock); }
	NXCPEncryptionContext *acquireEncryptionContext();
   AbstractCommChannel *acquireChannel();

   UINT32 waitForRCC(UINT32 dwRqId, UINT32 dwTimeOut);

   void incInternalRefCount() { InterlockedIncrement(&m_internalRefCount); }
   void decInternalRefCount() { if (InterlockedDecrement(&m_internalRefCount) == 0) delete this; }

public:
   AgentConnection(const InetAddress& addr, WORD port = AGENT_LISTEN_PORT, int authMethod = AUTH_NONE, const TCHAR *secret = NULL, bool allowCompression = true);

   void incRefCount() { InterlockedIncrement(&m_userRefCount); }
   void decRefCount() { if (InterlockedDecrement(&m_userRefCount) == 0) { disconnect(); decInternalRefCount(); } }

   bool connect(RSA *pServerKey = NULL, UINT32 *pdwError = NULL, UINT32 *pdwSocketError = NULL, UINT64 serverId = 0);
   void disconnect();
   bool isConnected() const { return m_isConnected; }
   bool isProxyMode() { return m_useProxy; }
	int getProtocolVersion() const { return m_nProtocolVersion; }
	bool isControlServer() const { return m_controlServer; }
	bool isMasterServer() const { return m_masterServer; }
	bool isCompressionAllowed() const { return m_allowCompression && (m_nProtocolVersion >= 4); }

   bool sendMessage(NXCPMessage *msg);
   bool sendRawMessage(NXCP_MESSAGE *msg);
   void postRawMessage(NXCP_MESSAGE *msg);
   NXCPMessage *waitForMessage(WORD code, UINT32 id, UINT32 timeout) { return m_pMsgWaitQueue->waitForMessage(code, id, timeout); }

   ArpCache *getArpCache();
   InterfaceList *getInterfaceList();
   ROUTING_TABLE *getRoutingTable();
   UINT32 getParameter(const TCHAR *pszParam, UINT32 dwBufSize, TCHAR *pszBuffer);
   UINT32 getList(const TCHAR *param, StringList **list);
   UINT32 getTable(const TCHAR *param, Table **table);
   UINT32 getServiceParameter(const TCHAR *url, UINT32 retentionTime, const TCHAR *login, const TCHAR *password, long authType, StringList *headers, StringList *parameters, bool verifyCert, StringMap *results);
   UINT32 nop();
   UINT32 setServerCapabilities();
   UINT32 setServerId(UINT64 serverId);
   UINT32 execAction(const TCHAR *action, const StringList &list, bool withOutput = false,
            void (* outputCallback)(ActionCallbackEvent, const TCHAR *, void *) = NULL, void *cbData = NULL);
   UINT32 uploadFile(const TCHAR *localFile, const TCHAR *destinationFile = NULL,
            void (* progressCallback)(INT64, void *) = NULL, void *cbArg = NULL,
            NXCPStreamCompressionMethod compMethod = NXCP_STREAM_COMPRESSION_NONE);
   UINT32 getFileSetInfo(const StringList &fileSet, ObjectArray<RemoteFileInfo> **info);
   UINT32 startUpgrade(const TCHAR *pszPkgName);
   UINT32 checkNetworkService(UINT32 *pdwStatus, const InetAddress& addr, int iServiceType, WORD wPort = 0,
                              WORD wProto = 0, const TCHAR *pszRequest = NULL, const TCHAR *pszResponse = NULL, UINT32 *responseTime = NULL);
   UINT32 getSupportedParameters(ObjectArray<AgentParameterDefinition> **paramList, ObjectArray<AgentTableDefinition> **tableList);
   UINT32 getConfigFile(TCHAR **ppszConfig, UINT32 *pdwSize);
   UINT32 updateConfigFile(const TCHAR *pszConfig);
   UINT32 enableTraps();
   UINT32 enableFileUpdates();
	UINT32 getPolicyInventory(AgentPolicyInfo **info);
	UINT32 uninstallPolicy(const uuid& guid);
   UINT32 takeScreenshot(const TCHAR *sessionName, BYTE **data, size_t *size);
   TCHAR *getHostByAddr(const InetAddress& ipAddr, TCHAR *buffer, size_t bufLen);
   UINT32 setupTcpProxy(const InetAddress& ipAddr, UINT16 port, UINT32 *channelId);
   UINT32 closeTcpProxy(UINT32 channelId);

	UINT32 generateRequestId() { return (UINT32)InterlockedIncrement(&m_requestId); }
	NXCPMessage *customRequest(NXCPMessage *pRequest, const TCHAR *recvFile = NULL, bool append = false,
	         void (*downloadProgressCallback)(size_t, void *) = NULL,
	         void (*fileResendCallback)(NXCP_MESSAGE *, void *) = NULL, void *cbArg = NULL);

   void setConnectionTimeout(UINT32 dwTimeout) { m_connectionTimeout = MAX(dwTimeout, 1000); }
	UINT32 getConnectionTimeout() { return m_connectionTimeout; }
   void setCommandTimeout(UINT32 dwTimeout) { m_dwCommandTimeout = MAX(dwTimeout, 500); }
	UINT32 getCommandTimeout() { return m_dwCommandTimeout; }
   void setRecvTimeout(UINT32 dwTimeout) { m_dwRecvTimeout = MAX(dwTimeout, 10000); }
   void setEncryptionPolicy(int iPolicy) { m_iEncryptionPolicy = iPolicy; }
   void setProxy(InetAddress addr, WORD wPort = AGENT_LISTEN_PORT,
                 int iAuthMethod = AUTH_NONE, const TCHAR *pszSecret = NULL);
   void setPort(WORD wPort) { m_wPort = wPort; }
   void setAuthData(int method, const TCHAR *secret);

   void setDeleteFileOnDownloadFailure(bool flag) { m_deleteFileOnDownloadFailure = flag; }
   UINT32 cancelFileDownload();
};

/**
 * Proxy SNMP transport
 */
class LIBNXSRV_EXPORTABLE SNMP_ProxyTransport : public SNMP_Transport
{
protected:
	AgentConnection *m_agentConnection;
	NXCPMessage *m_response;
	InetAddress m_ipAddr;
	UINT16 m_port;
	bool m_waitForResponse;

public:
	SNMP_ProxyTransport(AgentConnection *conn, const InetAddress& ipAddr, UINT16 port);
	virtual ~SNMP_ProxyTransport();

   virtual int readMessage(SNMP_PDU **ppData, UINT32 timeout = INFINITE,
                           struct sockaddr *pSender = NULL, socklen_t *piAddrSize = NULL,
	                        SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t) = NULL) override;
   virtual int sendMessage(SNMP_PDU *pdu, UINT32 timeout) override;
   virtual InetAddress getPeerIpAddress() override;
   virtual UINT16 getPort() override;
   virtual bool isProxyTransport() override;

   void setWaitForResponse(bool wait) { m_waitForResponse = wait; }
};

/**
 * ISC flags
 */
#define ISCF_IS_CONNECTED        ((UINT32)0x00000001)
#define ISCF_REQUIRE_ENCRYPTION  ((UINT32)0x00000002)

/**
 * Inter-server connection (ISC)
 */
class LIBNXSRV_EXPORTABLE ISC
{
private:
	UINT32 m_flags;
   InetAddress m_addr;
	WORD m_port;
   SOCKET m_socket;
   int m_protocolVersion;
	VolatileCounter m_requestId;
	UINT32 m_recvTimeout;
   MsgWaitQueue *m_msgWaitQueue;
   MUTEX m_mutexDataLock;
	MUTEX m_socketLock;
   THREAD m_hReceiverThread;
   NXCPEncryptionContext *m_ctx;
	UINT32 m_commandTimeout;

   void receiverThread();
   static THREAD_RESULT THREAD_CALL receiverThreadStarter(void *);

protected:
   UINT32 setupEncryption(RSA *pServerKey);
	UINT32 connectToService(UINT32 service);

   void lock() { MutexLock(m_mutexDataLock); }
   void unlock() { MutexUnlock(m_mutexDataLock); }

   virtual void printMessage(const TCHAR *format, ...);
   virtual void onBinaryMessage(NXCP_MESSAGE *rawMsg);
   virtual bool onMessage(NXCPMessage *msg);

public:
   ISC();
   ISC(const InetAddress& addr, WORD port = NETXMS_ISC_PORT);
   virtual ~ISC();

   UINT32 connect(UINT32 service, RSA *serverKey = NULL, BOOL requireEncryption = FALSE);
	void disconnect();
   bool connected() { return m_flags & ISCF_IS_CONNECTED; };

   BOOL sendMessage(NXCPMessage *msg);
   NXCPMessage *waitForMessage(WORD code, UINT32 id, UINT32 timeOut) { return m_msgWaitQueue->waitForMessage(code, id, timeOut); }
   UINT32 waitForRCC(UINT32 rqId, UINT32 timeOut);
   UINT32 generateMessageId() { return (UINT32)InterlockedIncrement(&m_requestId); }

   UINT32 nop();
};


//
// Functions
//

void LIBNXSRV_EXPORTABLE DestroyRoutingTable(ROUTING_TABLE *pRT);
void LIBNXSRV_EXPORTABLE SortRoutingTable(ROUTING_TABLE *pRT);
const TCHAR LIBNXSRV_EXPORTABLE *AgentErrorCodeToText(UINT32 err);
UINT32 LIBNXSRV_EXPORTABLE AgentErrorToRCC(UINT32 err);

// for compatibility - new code should use nxlog_debug
#define DbgPrintf nxlog_debug

void LIBNXSRV_EXPORTABLE SetAgentDEP(int iPolicy);

const TCHAR LIBNXSRV_EXPORTABLE *ISCErrorCodeToText(UINT32 code);

/**
 * Variables
 */
extern LIBNXSRV_EXPORTABLE_VAR(UINT64 g_flags);
extern LIBNXSRV_EXPORTABLE_VAR(ThreadPool *g_agentConnectionThreadPool);

/**
 * Helper finctions for checking server flags
 */
inline bool IsStandalone()
{
	return !(g_flags & AF_DAEMON) ? true : false;
}

inline bool IsZoningEnabled()
{
	return (g_flags & AF_ENABLE_ZONING) ? true : false;
}

#endif   /* _nxsrvapi_h_ */
