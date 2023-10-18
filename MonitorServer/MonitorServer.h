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
        //�����Լ���
        //==========================
        bool OnConnectionRequest(const SOCKADDR_IN* const addr) override; //Accept ����. return false�� Ŭ���̾�Ʈ �ź�, true�� ���� ���
        void OnClientJoin(NetSessionID NetSessionID) override; //Accept �� ���� ó�� �Ϸ� �� ȣ��.
        void OnClientLeave(NetSessionID NetSessionID) override; //Release �� ȣ��
        void OnRecv(NetPacketSet* packetList) override;
        void OnSend(NetSessionID NetSessionID, int sendSize) override; //��Ŷ �۽� �Ϸ� ��
        void OnWorkerThreadBegin() override; //��Ŀ������ GQCS �ϴܿ��� ȣ��
        void OnWorkerThreadEnd() override; //��Ŀ������ 1���� ���� ��
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