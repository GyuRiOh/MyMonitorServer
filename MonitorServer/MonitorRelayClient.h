#pragma once
#include <Windows.h>
#include "../NetRoot/LanClient/LanClient.h"
#include "MonitorServer_CS_Stub.h"
#include "MonitorServer_SC_Proxy.h"

namespace MyNetwork
{
	class MonitorRelayClient : public LanClient
	{
	public:
		void RegisterMonitorServer(MonitorServer* server) 
		{
			server_ = server;
		}

	protected:
		bool OnRecv(LanPacketSet* packetList)
		{
			switch (packetList->GetType())
			{
			case eNET_RECVED_PACKET_SET:
			{
				while (packetList->GetSize() > 0)
				{
					LanDummyPacket* packet = nullptr;
					if (packetList->Dequeue(&packet) == false)
						CrashDump::Crash();

					if (!PacketProc(packetList->GetSessionID(), packet))
						return false;
				}
				break;
			}
			default:
				CrashDump::Crash();
				break;
			}

			LanPacketSet::Free(packetList);

			return true;
		}

		void OnConnect(){

			LanPacket* packet = LanPacket::Alloc();

			*packet << (WORD)en_PACKET_SS_MONITOR_LOGIN;
			*packet << (int)eMONITOR_SERVER_NO;

			SendPacket(packet);
			LanPacket::Free(packet);

		}

		bool PacketProc(LanSessionID sessionID, LanDummyPacket* packet)
		{
			WORD type;
			*packet >> type;
			switch (type)
			{
			case en_PACKET_SS_MONITOR_DATA_UPDATE:
			{
				BYTE dataType;
				int dataValue;
				int timeStamp;
				*packet >> dataType;
				*packet >> dataValue;
				*packet >> timeStamp;
				ReqDataUpdate(dataType, dataValue, timeStamp);
				return true;
			}

			case en_PACKET_CS_SECTOR_TOOL_DATA_UPDATE:
			{
				BYTE sectorBuffer[2500] = { 0 };
				packet->DeqData((char*)sectorBuffer, 2500);
			}

			}
			return false;
		}

		void ReqDataUpdate(BYTE dataType, int dataValue, int timeStamp)
		{
			server_->stub_->ReqDataUpdate(dataType, dataValue, timeStamp);
		}

		void ReqSectorDataUpdate()
		{

		}


	private:
		MonitorServer* server_ = nullptr;
	};
}
