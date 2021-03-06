/* 
** nxethernetip - command line tool for Ethernet/IP
** Copyright (C) 2004-2020 Victor Kirhenshtein
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
** File: nxethernetip.cpp
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <ethernet_ip.h>

NETXMS_EXECUTABLE_HEADER(nxethernetip)

/**
 * Static data
 */
static uint16_t s_port = ETHERNET_IP_DEFAULT_PORT;
static uint32_t s_timeout = 5000;
static bool s_useUDP = false;
static SOCKET s_socket = INVALID_SOCKET;
static EIP_MessageReceiver *s_receiver = nullptr;

/**
 * Dump raw bytes
 */
static void DumpBytes(const uint8_t *data, size_t length)
{
   TCHAR textForm[32], buffer[64];
   for(size_t i = 0; i < length; i += 16)
   {
      const uint8_t *block = data + i;
      size_t blockSize = MIN(16, length - i);
      BinToStrEx(block, blockSize, buffer, _T(' '), 16 - blockSize);
      size_t j;
      for(j = 0; j < blockSize; j++)
      {
         uint8_t b = block[j];
         textForm[j] = ((b >= ' ') && (b < 127)) ? (TCHAR)b : _T('.');
      }
      textForm[j] = 0;
      _tprintf(_T("   %04X | %s | %s\n"), i, buffer, textForm);
   }
}

/**
 * Connect to device
 */
static bool Connect(const char *hostname)
{
   InetAddress addr = InetAddress::resolveHostName(hostname);
   if (addr.getFamily() == AF_UNSPEC)
   {
      _tprintf(_T("Cannot resolve host name %hs\n"), hostname);
      return false;
   }

   if (s_useUDP)
   {
      s_socket = ConnectToHostUDP(addr, s_port);
      if (s_socket == INVALID_SOCKET)
      {
         TCHAR ipAddrText[64], errorText[256];
         _tprintf(_T("Cannot setup UDP socket for communication with %s:%u (%s)\n"), addr.toString(ipAddrText), s_port, GetLastSocketErrorText(errorText, 256));
         return false;
      }
   }
   else
   {
      s_socket = ConnectToHost(addr, s_port, s_timeout);
      if (s_socket == INVALID_SOCKET)
      {
         TCHAR ipAddrText[64], errorText[256];
         _tprintf(_T("Cannot connected to %s:%u (%s)\n"), addr.toString(ipAddrText), s_port, GetLastSocketErrorText(errorText, 256));
         return false;
      }

      TCHAR buffer[64];
      _tprintf(_T("Connected to %s:%u\n"), addr.toString(buffer), s_port);
   }
   return true;
}

/**
 * Read command response
 */
static EIP_Message *ReadResponse(EIP_Command command, size_t cpfStartOffset)
{
   EIP_Message *response = s_receiver->readMessage(s_timeout);
   if (response == nullptr)
   {
      _tprintf(_T("Request timeout\n"));
      return nullptr;
   }

   _tprintf(_T("Raw message:\n"));
   DumpBytes(response->getBytes(), response->getSize());
   _tprintf(_T("\n"));

   if (response->getCommand() != command)
   {
      _tprintf(_T("Invalid response (command code 0x%04X)\n"), response->getCommand());
      delete response;
      return nullptr;
   }

   _tprintf(_T("Status: %02X (%s)\n"), response->getStatus(), EIP_ProtocolStatusTextFromCode(response->getStatus()));
   if (response->getStatus() != EIP_STATUS_SUCCESS)
   {
      delete response;
      return nullptr;
   }

   response->prepareCPFRead(cpfStartOffset);
   _tprintf(_T("%d item%s in response message\n\n"), response->getItemCount(), response->getItemCount() == 1 ? _T("") : _T("s"));
   return response;
}

/**
 * Generic function for sending list type command
 */
static EIP_Message *SendListCommand(EIP_Command command)
{
   EIP_Message request(command, 0);
   size_t bytes = request.getSize();
   if (SendEx(s_socket, request.getBytes(), bytes, 0, nullptr) != bytes)
   {
      TCHAR buffer[1024];
      _tprintf(_T("Request sending failed (%s)"), GetLastSocketErrorText(buffer, 1024));
      return nullptr;
   }

   return ReadResponse(command, 0);
}

/**
 * List identity
 */
static bool ListIdentity()
{
   EIP_Message *response = SendListCommand(EIP_LIST_IDENTITY);
   if (response == nullptr)
      return false;

   CPF_Item item;
   if (response->findItem(0x0C, &item))
   {
      _tprintf(_T("Protocol version.....: %u\n"), response->readDataAsUInt16(item.offset));
      _tprintf(_T("Device IP address....: %s\n"), response->readDataAsInetAddress(item.offset + 6).toString().cstr());
      _tprintf(_T("Vendor...............: %u (%s)\n"), response->readDataAsUInt16(item.offset + 18), CIP_VendorNameFromCode(response->readDataAsUInt16(item.offset + 18)));
      _tprintf(_T("Device type..........: %u (%s)\n"), response->readDataAsUInt16(item.offset + 20), CIP_DeviceTypeNameFromCode(response->readDataAsUInt16(item.offset + 20)));

      TCHAR productName[256] = _T("");
      if (response->readDataAsLengthPrefixString(item.offset + 32, productName, 256))
      {
         _tprintf(_T("Product name.........: %s\n"), productName);
      }
      _tprintf(_T("Product code.........: %u\n"), response->readDataAsUInt16(item.offset + 22));
      _tprintf(_T("Product revision.....: %u.%u\n"), response->readDataAsUInt8(item.offset + 24), response->readDataAsUInt8(item.offset + 25));
      _tprintf(_T("Serial number........: %08X\n"), response->readDataAsUInt32(item.offset + 28));

      uint16_t status = response->readDataAsUInt16(item.offset + 26);
      _tprintf(_T("Status...............: %04X (%s)\n"), status, CIP_DecodeDeviceStatus(status).cstr());
      if ((status & CIP_DEVICE_STATUS_EXTENDED_STATUS_MASK) != 0)
      {
         _tprintf(_T("Extended status......: %s\n"), CIP_DecodeExtendedDeviceStatus(status));
      }

      uint8_t state = response->readDataAsUInt8(item.offset + 33 + _tcslen(productName));
      _tprintf(_T("State................: %u (%s)\n"), state, CIP_DeviceStateTextFromCode(state));
   }
   else
   {
      _tprintf(_T("Missing identity data in response message\n"));
   }

   delete response;
   return true;
}

/**
 * List interfaces
 */
static bool ListInterfaces()
{
   EIP_Message *response = SendListCommand(EIP_LIST_INTERFACES);
   if (response == nullptr)
      return false;

   delete response;
   return true;
}

/**
 * List services
 */
static bool ListServices()
{
   EIP_Message *response = SendListCommand(EIP_LIST_SERVICES);
   if (response == nullptr)
      return false;

   CPF_Item item;
   while(response->nextItem(&item))
   {
      if (item.length < 20)
         continue;   // Invalid length

      char svcName[17];
      memcpy(svcName, &item.data[4], 16);
      svcName[16] = 0;

      _tprintf(_T("Type %04X Flags %04X - %hs\n"), item.type, response->readDataAsUInt16(item.offset + 2), svcName);
   }

   delete response;
   return true;
}

/**
 * Get attribute from device
 */
static bool GetAttribute(const char *symbolicPath)
{
   uint32_t classId, instance, attributeId;
   if (!CIP_ParseSymbolicPathA(symbolicPath, &classId, &instance, &attributeId))
   {
      _tprintf(_T("Attribute path is invalid\n"));
      return false;
   }

   CIP_EPATH path;
   CIP_EncodeAttributePath(classId, instance, attributeId, &path);
   TCHAR pathText[256];
   _tprintf(_T("Encoded EPATH: %s\n"), BinToStrEx(path.value, path.size, pathText, _T(' '), 0));

   EIP_Status status;
   EIP_Session *session = EIP_Session::connect(s_socket, s_timeout, &status);
   if (session == nullptr)
   {
      _tprintf(_T("Session registration failed (%s)\n"), status.failureReason().cstr());
      return false;
   }
   _tprintf(_T("Session registered (handle = %08X)\n"), session->getHandle());

   EIP_Message request(EIP_SEND_RR_DATA, 1024, session->getHandle());
   request.advanceWritePosition(6); // Interface ID and timeout left as 0
   request.writeDataAsUInt16(2);    // Item count
   request.advanceWritePosition(4); // Item type 0 followed by length 0 - NULL address
   request.writeDataAsUInt16(0xB2); // Type B2 - UCMM message
   request.writeDataAsUInt16(path.size + 2);
   request.writeDataAsUInt8(CIP_Get_Attribute_Single);
   request.writeDataAsUInt8(path.size / 2);
   request.writeData(path.value, path.size);
   request.completeDataWrite();

   size_t bytes = request.getSize();
   if (SendEx(s_socket, request.getBytes(), bytes, 0, nullptr) != bytes)
   {
      TCHAR buffer[1024];
      _tprintf(_T("Request sending failed (%s)"), GetLastSocketErrorText(buffer, 1024));
      delete session;
      return false;
   }

   EIP_Message *response = ReadResponse(EIP_SEND_RR_DATA, 6);
   if (response == nullptr)
   {
      delete session;
      return false;
   }

   CPF_Item item;
   if (response->findItem(0xB2, &item))
   {
      CIP_GeneralStatus generalStatus = response->readDataAsUInt8(item.offset + 2);
      _tprintf(_T("CIP General Status: %02X (%s)\n\n"), generalStatus, CIP_GeneralStatusTextFromCode(generalStatus));
      if (generalStatus == 0)
      {
         uint16_t additionalStatusSize = response->readDataAsUInt8(item.offset + 3) * 2;

         TCHAR buffer[1024];
         _tprintf(_T("Value: %s\n"),
                  CIP_DecodeAttribute(response->getRawData() + item.offset + additionalStatusSize + 4,
                           item.length - additionalStatusSize - 4, classId, attributeId, buffer, 1024));
      }
   }
   else
   {
      _tprintf(_T("Missing UCMM message data\n"));
   }

   delete session;
   return true;
}

/**
 * Startup
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   _tprintf(_T("NetXMS EtherNet/IP Diagnostic Tool Version ") NETXMS_VERSION_STRING _T("\n")
            _T("Copyright (c) 2020 Raden Solutions\n\n"));

   // Parse command line
   opterr = 1;
   bool start = true;
   int exitCode = 0;
   uint32_t value;
   char *eptr;
   int ch;
	while((ch = getopt(argc, argv, "hp:uw:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            _tprintf(_T("Usage: nxethernetip [<options>] <host> <command> [<arguments>]\n")
                     _T("\nValid commands are:\n")
                     _T("   GetAttribute <path> : read value of specific attribute (path is class.instance.attribute)\n")
                     _T("   ListIdentity        : read device identity\n")
                     _T("   ListInterfaces      : read list of supported interfaces\n")
                     _T("   ListServices        : read list of supported services\n")
                     _T("\nValid options are:\n")
                     _T("   -h                : Display help and exit\n")
                     _T("   -p <port>         : Port number (default is 44818)\n")
                     _T("   -u                : Use UDP for communication\n")
                     _T("   -w <milliseconds> : Request timeout (default is 5000 milliseconds)\n")
                     _T("\n"));
            start = false;
            break;
         case 'p':   // Port number
            value = strtoul(optarg, &eptr, 0);
            if ((*eptr != 0) || (value > 65535) || (value == 0))
            {
               _tprintf(_T("Invalid port number %hs\n"), optarg);
               start = false;
               exitCode = 1;
            }
            else
            {
               s_port = static_cast<uint16_t>(value);
            }
            break;
         case 'u':   // UDP
            s_useUDP = true;
            break;
         case 'w':   // Timeout
            value = strtoul(optarg, &eptr, 0);
            if ((*eptr != 0) || (value > 60000) || (value == 0))
            {
               _tprintf(_T("Invalid timeout value %hs\n"), optarg);
               start = false;
               exitCode = 1;
            }
            else
            {
               s_timeout = value;
            }
            break;
         case '?':
            exitCode = 1;
            start = false;
            break;
         default:
            break;
      }
   }

	if (!start)
	   return exitCode;

   if (argc - optind < 2)
   {
      _tprintf(_T("Required argument(s) missing.\nUse nxethernetip -h to get complete command line syntax.\n"));
      return 1;
   }

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(2, &wsaData);
#endif

   if (!Connect(argv[optind]))
      return 2;

   s_receiver = new EIP_MessageReceiver(s_socket);

   const char *command = argv[optind + 1];
   if (!stricmp(command, "ListIdentity"))
   {
      if (!ListIdentity())
         exitCode = 4;
   }
   else if (!stricmp(command, "ListInterfaces"))
   {
      if (!ListInterfaces())
         exitCode = 4;
   }
   else if (!stricmp(command, "ListServices"))
   {
      if (!ListServices())
         exitCode = 4;
   }
   else if (!stricmp(command, "GetAttribute"))
   {
      if (argc - optind < 3)
      {
         _tprintf(_T("Required argument(s) missing.\nUse nxethernetip -h to get complete command line syntax.\n"));
         return 1;
      }
      if (!GetAttribute(argv[optind + 2]))
         exitCode = 4;
   }
   else
   {
      _tprintf(_T("Invalid command %hs\n"), command);
      exitCode = 3;
   }

   delete s_receiver;
   shutdown(s_socket, SHUT_RDWR);
   closesocket(s_socket);

   return exitCode;
}
