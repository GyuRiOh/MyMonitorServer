#pragma once
#include "../NetRoot/NetServer/NetServer.h"
#include "../NetRoot/Common/DBConnector.h"
#include "../MonitorProtocol.h"
#include <set>

using namespace std;

namespace MyNetwork
{
    class MonitorServer_CS_Stub;
    class MonitorServer_SC_Proxy;
    class MonitorRelayClient;

    class MonitorServer : public NetRoot
    {
        struct MonitorUnit
        {
            int serverNum = NULL;
            multiset<int> dataSet;
            RTL_SRWLOCK lock_;

            void Lock()
            {
                AcquireSRWLockExclusive(&lock_);
            }

            void Unlock()
            {
                ReleaseSRWLockExclusive(&lock_);
            }
        };

        enum Setting
        {
            UNIT_COUNT = 45
        };

    public:
        explicit MonitorServer();
        virtual ~MonitorServer();

    private:
        //==========================
        //가상함수들
        //==========================
        bool OnConnectionRequest(const SOCKADDR_IN* const addr) override; //Accept 직후. return false시 클라이언트 거부, true시 접속 허용
        void OnClientJoin(NetSessionID NetSessionID) override; //Accept 후 접속 처리 완료 후 호출.
        void OnClientLeave(NetSessionID NetSessionID) override; //Release 후 호출
        void OnRecv(NetPacketSet* packetList) override;
        void OnSend(NetSessionID NetSessionID, int sendSize) override; //패킷 송신 완료 후
        void OnWorkerThreadBegin() override; //워커스레드 GQCS 하단에서 호출
        void OnWorkerThreadEnd() override; //워커스레드 1루프 종료 후
        void OnError(int errCode, WCHAR*)  override {}
        void OnMonitor(const MonitoringInfo* const info) override;
        void OnStart() override;
        void OnStop() override {};

        int DBSave();
        int GetAverageFromUnits(BYTE type);

        static DWORD WINAPI DBSaveThread(LPVOID arg);

    public:
        MonitorServer_CS_Stub* stub_;
        MonitorServer_SC_Proxy* proxy_;
    
        vector<NetSessionID> monitorClient_;
        vector<NetSessionID> sectorClient_;
        RTL_SRWLOCK clientLock_;
        DBConnector* dbConnector_;

        MonitorUnit units_[UNIT_COUNT];
        HANDLE dbThread_;

        bool isDBthreadRunning_;
        MonitorRelayClient* relayClient_;
    };
}