#pragma once

#include "../NetRoot/NetServer/NetSessionID.h"
#include "../NetRoot/NetServer/NetPacketSet.h"
#include "../NetRoot/NetServer/NetUser.h"
#include "MonitorServer.h"
#include "MonitorServer_SC_Proxy.h"
#include "../MonitorProtocol.h"

namespace MyNetwork
{
	class MonitorServer_CS_Stub
	{
	public:
		explicit MonitorServer_CS_Stub(MonitorServer* server) : server_(nullptr)
		{
			server_ = server;
		}

		bool PacketProc(NetPacketSet* msgPack)
		{
			switch (msgPack->GetType())
			{
			case eNET_RECVED_PACKET_SET:
			{
				while (msgPack->GetSize() > 0)
				{
					NetDummyPacket* packet = nullptr;
					if (msgPack->Dequeue(&packet) == false)
						CrashDump::Crash();

					if (!PacketProc(msgPack->GetSessionID(), packet))
						return false;
				}
				break;
			}
			default:
				CrashDump::Crash();
				break;
			}

			return true;
		}

		bool PacketProc(NetSessionID sessionID, NetDummyPacket* msg)
		{
			WORD type;
			*msg >> type;
			switch (type)
			{
			case en_PACKET_SS_MONITOR_DATA_UPDATE:
			{
				BYTE dataType;
				int dataValue;
				int timeStamp;
				*msg >> dataType;
				*msg >> dataValue;
				*msg >> timeStamp;
				ReqDataUpdate(dataType, dataValue, timeStamp);
				return true;
			}

			case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
			{
				char loginSessionKey[32] = {0};
				msg->DeqData((char*)loginSessionKey, 32);
				ReqLogin(loginSessionKey, sessionID);
				return true;
			}


			case en_PACKET_CS_SECTOR_TOOL_REQ_LOGIN:
			{	
				ReqSectorLogin(sessionID);
				return true;
			}

			case en_PACKET_CS_SECTOR_TOOL_DATA_UPDATE:
			{
				BYTE sectorBuffer[2500] = { 0 };
				msg->DeqData((char*)sectorBuffer, 2500);
				ReqSectorDataUpdate(sectorBuffer);
				return true;
			}

			default:
				SystemLogger::GetInstance()->LogText(L"MonitorServerStub", LEVEL_ERROR, L"Undefined Packet Recved");
				break;
			}
			return false;
		}

		void ReqLogin(char* loginSessionKey, NetSessionID sessionID)
		{

			if (strncmp(loginSessionKey, "ajfw@!cv980dSZ[fje#@fdj123948djf", 32) != 0)
				return;
			
			SystemLogger::GetInstance()->Console(L"MonitorServer", LEVEL_DEBUG, L"MonitorClientTool Login");

			AcquireSRWLockExclusive(&server_->clientLock_);
			server_->monitorClient_.push_back(sessionID);
			ReleaseSRWLockExclusive(&server_->clientLock_);

			server_->proxy_->ResLogin(dfMONITOR_TOOL_LOGIN_OK, sessionID);
		}

		void ReqDataUpdate(BYTE dataType, int dataValue, int timeStamp)
		{
			server_->proxy_->DataUpdate(dataType, dataValue, timeStamp);
		}

		void ReqSectorLogin(NetSessionID sessionID)
		{
			SystemLogger::GetInstance()->Console(L"MonitorServer", LEVEL_DEBUG, L"SectorClientTool Login");

			AcquireSRWLockExclusive(&server_->clientLock_);
			server_->sectorClient_.push_back(sessionID);
			ReleaseSRWLockExclusive(&server_->clientLock_);

			server_->proxy_->ResSectorLogin(dfSECTOR_TOOL_LOGIN_OK, sessionID);
		}

		void ReqSectorDataUpdate(BYTE* sectorData)
		{
			server_->proxy_->SectorDataUpdate(sectorData);
		}


	private:
		MonitorServer* server_;
	};
}
