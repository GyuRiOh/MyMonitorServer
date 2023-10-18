
#pragma once
#include "../NetRoot/NetServer/NetPacket.h"
#include "../NetRoot/NetServer/NetSessionID.h"
#include "MonitorServer.h"

namespace MyNetwork
{
	class MonitorServer_SC_Proxy
	{
	public:
		explicit MonitorServer_SC_Proxy(MonitorServer* server) : server_(nullptr)
		{
			server_ = server;
		}

		void ResLogin(BYTE status, NetSessionID sessionID)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << (unsigned short)en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;
			*msg << status;

			server_->AsyncSendPacket(sessionID, msg);
			NetPacket::Free(msg);
		}

		void ResSectorLogin(BYTE status, NetSessionID sessionID)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << (unsigned short)en_PACKET_CS_SECTOR_TOOL_RES_LOGIN;
			*msg << status;

			server_->AsyncSendPacket(sessionID, msg);
			NetPacket::Free(msg);
		}

		void DataUpdate(BYTE dataType, int dataValue, int timeStamp)
		{

			server_->units_[dataType].Lock();
			server_->units_[dataType].dataSet.insert(dataValue);
			server_->units_[dataType].Unlock();

			NetPacket* msg = NetPacket::Alloc();

			*msg << (unsigned short)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;
			*msg << (BYTE)server_->units_[dataType].serverNum;
			*msg << dataType;
			*msg << dataValue;
			*msg << timeStamp;

			NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
			AcquireSRWLockExclusive(&server_->clientLock_);

			auto iter = server_->monitorClient_.begin();
			for (; iter != server_->monitorClient_.end(); ++iter)
			{
				idSet->Enqueue((*iter));
			}
			ReleaseSRWLockExclusive(&server_->clientLock_);

			server_->AsyncSendPacket(idSet, msg);

			NetPacket::Free(msg);
		}

		void SectorDataUpdate(BYTE* sectorData)
		{
			NetPacket* msg = NetPacket::Alloc();

			*msg << (unsigned short)en_PACKET_CS_SECTOR_TOOL_DATA_UPDATE;
			msg->EnqData((char*)sectorData, 2500);

			NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
			AcquireSRWLockExclusive(&server_->clientLock_);

			auto iter = server_->sectorClient_.begin();
			for (; iter != server_->sectorClient_.end(); ++iter)
			{
				idSet->Enqueue((*iter));
			}
			ReleaseSRWLockExclusive(&server_->clientLock_);

			server_->AsyncSendPacket(idSet, msg);

			NetPacket::Free(msg);
		}


	private:
		MonitorServer* server_;
	};
}
