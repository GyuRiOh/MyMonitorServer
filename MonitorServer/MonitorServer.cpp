
#include "MonitorServer.h"
#include "MonitorServer_CS_Stub.h"
#include "MonitorServer_SC_Proxy.h"
#include "../NetRoot/Common/Parser.h"
#include "MonitorRelayClient.h"
#include <algorithm>
#include <thread>

server_baby::MonitorServer::MonitorServer() : isDBthreadRunning_(false), dbThread_(INVALID_HANDLE_VALUE)
{

    stub_ = new MonitorServer_CS_Stub(this);
    proxy_ = new MonitorServer_SC_Proxy(this);

    dbConnector_ = new DBConnector(L"127.0.0.1",
        L"root",
        L"1234",
        L"logdb",
        3306);

    InitializeSRWLock(&clientLock_);

    for (int i = 0; i < UNIT_COUNT; i++)
    {
        InitializeSRWLock(&units_[i].lock_);
    }

    
    for (int i = 0; i < dfMONITOR_DATA_TYPE_GAME_SERVER_RUN; i++)
    {
        units_[i].serverNum = eLOGIN_SERVER_NO;
    }


    for (int i = dfMONITOR_DATA_TYPE_GAME_SERVER_RUN; i < dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN; i++)
    {
        units_[i].serverNum = eGAME_SERVER_NO;
    }


    for (int i = dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN; i < dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL; i++)
    {
        units_[i].serverNum = eCHAT_SERVER_NO;
    }


    for (int i = dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL; i < dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY; i++)
    {
        units_[i].serverNum = eHARDWARE_NO;
    }


    isDBthreadRunning_ = true;

    unsigned int threadID = NULL;

    dbThread_ = (HANDLE)_beginthreadex(
        NULL,
        0,
        (_beginthreadex_proc_type)&DBSaveThread,
        (LPVOID)this,
        0,
        (unsigned int*)&threadID);

    if (!dbThread_)
        ErrorQuit(L"UpdateThread Start Failed");


}

server_baby::MonitorServer::~MonitorServer()
{

    isDBthreadRunning_ = false;

    delete stub_;
    delete proxy_;
    delete dbConnector_;
    delete relayClient_;
}

bool server_baby::MonitorServer::OnConnectionRequest(const SOCKADDR_IN* const addr)
{
    return true;
}

void server_baby::MonitorServer::OnClientJoin(NetSessionID sessionID)
{

}

void server_baby::MonitorServer::OnClientLeave(NetSessionID sessionID)
{
    AcquireSRWLockExclusive(&clientLock_);

    auto iter = monitorClient_.begin();
    for (; iter != monitorClient_.end(); ++iter)
    {
        NetSessionID clientID = (*iter);
        if (clientID.total_ == sessionID.total_)
        {
            monitorClient_.erase(iter);
            break;
        }
    }

    ReleaseSRWLockExclusive(&clientLock_);
}

void server_baby::MonitorServer::OnRecv(NetPacketSet* packetList)
{
    if (!stub_->PacketProc(packetList))
        Disconnect(packetList->GetSessionID());

    NetPacketSet::Free(packetList);
}

void server_baby::MonitorServer::OnSend(NetSessionID NetSessionID, int sendSize)
{

}

void server_baby::MonitorServer::OnWorkerThreadBegin()
{

}

void server_baby::MonitorServer::OnWorkerThreadEnd()
{

}

void server_baby::MonitorServer::OnMonitor(const MonitoringInfo* const info)
{

}

void server_baby::MonitorServer::OnStart()
{

    relayClient_ = new MonitorRelayClient;

    int relayPort = 0;
    Parser::GetInstance()->GetValue("RelayServerPort", (int*)&relayPort);
    SystemLogger::GetInstance()->Console(L"NetServer", LEVEL_DEBUG, L"Relay Server Port : %d", relayPort);

    char IP[16] = "106.245.38.107";
    relayClient_->Start(IP, relayPort);
    relayClient_->RegisterMonitorServer(this);
}

int server_baby::MonitorServer::DBSave()
{
    int min = NULL;
    int max = NULL;
    int avr = NULL;

    while (isDBthreadRunning_)
    {
        time_t timer;
        struct tm t;
        timer = time(NULL);
        localtime_s(&t, &timer);

        WCHAR timeBuf[256] = { 0 };

        swprintf(timeBuf, 256, L"%d-%02d-%02d %02d:%02d:%02d",
            t.tm_year + 1900,
            t.tm_mon + 1,
            t.tm_mday,
            t.tm_hour,
            t.tm_min,
            t.tm_sec);

        for (int i = 1; i < UNIT_COUNT; i++)
        {         


            if(!units_[i].dataSet.empty())
            {
                SystemLogger::GetInstance()->Console(L"MonitorServer", LEVEL_DEBUG, L"%s - DB Save [%d]", timeBuf, i);

                units_[i].Lock();
                min = *(units_[i].dataSet.begin());
                max = *(units_[i].dataSet.rbegin());
                avr = GetAverageFromUnits(i);

                units_[i].dataSet.clear();
                units_[i].Unlock();

                dbConnector_->Query_Save(L"insert into monitorlog values (NULL, '%s', '%d', '%s' , '%d', '%d', '%d', '%d')",
                    timeBuf,
                    units_[i].serverNum,
                    L"Empty",
                    i,
                    avr,
                    min,
                    max);
            }

        }

        Sleep(60000);
    }
   
    return 0;
}

int server_baby::MonitorServer::GetAverageFromUnits(BYTE type)
{

    int total = NULL;
    auto function = [&total](int value) {
        total += value;
    };

    for_each(units_[type].dataSet.begin(), units_[type].dataSet.end(), function);    
    
    int ret = NULL;
    if(total)
        ret = total / static_cast<int>((units_[type].dataSet.size()));
    
    return ret;
}

DWORD __stdcall server_baby::MonitorServer::DBSaveThread(LPVOID arg)
{
    MonitorServer* server = reinterpret_cast<MonitorServer*>(arg);
    return server->DBSave();
}
