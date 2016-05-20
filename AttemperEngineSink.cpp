#include "StdAfx.h"
#include "ServiceUnits.h"
#include "ControlPacket.h"
#include "AttemperEngineSink.h"

//////////////////////////////////////////////////////////////////////////////////

//时间标识
#define IDI_LOAD_GAME_LIST			1									//加载列表
#define IDI_CONNECT_CORRESPOND		2									//重连标识
#define IDI_COLLECT_ONLINE_INFO		3									//统计在线

//////////////////////////////////////////////////////////////////////////////////

//构造函数
CAttemperEngineSink::CAttemperEngineSink()
{
	//状态变量
	m_bNeekCorrespond=true;
	m_bShowServerStatus=false;

	//状态变量
	m_pInitParameter=NULL;
	m_pBindParameter=NULL;

	//组件变量
	m_pITimerEngine=NULL;
	m_pIDataBaseEngine=NULL;
	m_pITCPNetworkEngine=NULL;
	m_pITCPSocketService=NULL;

	//视频配置
	m_wAVServerPort=0;
	m_dwAVServerAddr=0;
	return;
}

//析构函数
CAttemperEngineSink::~CAttemperEngineSink()
{
}

//接口查询
VOID * CAttemperEngineSink::QueryInterface(REFGUID Guid, DWORD dwQueryVer)
{
	QUERYINTERFACE(IAttemperEngineSink,Guid,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(IAttemperEngineSink,Guid,dwQueryVer);
	return NULL;
}

//启动事件
bool CAttemperEngineSink::OnAttemperEngineStart(IUnknownEx * pIUnknownEx)
{
	//绑定参数
	m_pBindParameter=new tagBindParameter[m_pInitParameter->m_wMaxConnect];
	ZeroMemory(m_pBindParameter,sizeof(tagBindParameter)*m_pInitParameter->m_wMaxConnect);

	//设置时间
	ASSERT(m_pITimerEngine!=NULL);
	m_pITimerEngine->SetTimer(IDI_COLLECT_ONLINE_INFO,m_pInitParameter->m_wCollectTime*1000L,TIMES_INFINITY,0);

	//获取目录
	TCHAR szPath[MAX_PATH]=TEXT("");
	CString szFileName;
	GetModuleFileName(AfxGetInstanceHandle(),szPath,sizeof(szPath));
	szFileName=szPath;
	int nIndex = szFileName.ReverseFind(TEXT('\\'));
	szFileName = szFileName.Left(nIndex);
	szFileName += TEXT("\\PlazaOptionConfig.ini");

	//读取配置
	m_bShowServerStatus = (GetPrivateProfileInt(TEXT("ServerStatus"),TEXT("ShowServerStatus"),0,szFileName) != 0);

	//获取目录
	TCHAR szServerAddr[LEN_SERVER]=TEXT("");
	GetCurrentDirectory(sizeof(szPath),szPath);

	//读取配置
	TCHAR szVideoFileName[MAX_PATH]=TEXT("");
	_sntprintf(szVideoFileName,CountArray(szVideoFileName),TEXT("%s\\VideoOption.ini"),szPath);
	m_wAVServerPort=GetPrivateProfileInt(TEXT("VideoOption"),TEXT("ServerPort"), 0,szVideoFileName);
	DWORD dwAddrLen=GetPrivateProfileString(TEXT("VideoOption"),TEXT("ServerAddr"), TEXT(""), szServerAddr,LEN_SERVER,szVideoFileName);
	if(dwAddrLen>0)
	{
		CT2CA strServerDomain(szServerAddr);
		m_dwAVServerAddr=inet_addr(strServerDomain);
	}
	else
	{
		m_dwAVServerAddr=0;
	}

	return true;
}

//停止事件
bool CAttemperEngineSink::OnAttemperEngineConclude(IUnknownEx * pIUnknownEx)
{
	//状态变量
	m_bNeekCorrespond=true;

	//组件变量
	m_pITimerEngine=NULL;
	m_pIDataBaseEngine=NULL;
	m_pITCPNetworkEngine=NULL;
	m_pITCPSocketService=NULL;

	//删除数据
	SafeDeleteArray(m_pBindParameter);

	//列表组件
	m_ServerListManager.ResetServerList();

	return true;
}

//控制事件
bool CAttemperEngineSink::OnEventControl(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	switch (wIdentifier)
	{
	case CT_LOAD_DB_GAME_LIST:		//加载列表
		{
			//加载列表
			m_ServerListManager.DisuseKernelItem();
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_GAME_LIST,0,NULL,0);

			return true;
		}
	case CT_CONNECT_CORRESPOND:		//连接协调
		{
			//发起连接
			tagAddressInfo * pCorrespondAddress=&m_pInitParameter->m_CorrespondAddress;
			m_pITCPSocketService->Connect(pCorrespondAddress->szAddress,m_pInitParameter->m_wCorrespondPort);

			//构造提示
			TCHAR szString[512]=TEXT("");
			_sntprintf(szString,CountArray(szString),TEXT("正在连接协调服务器 [ %s:%d ]"),pCorrespondAddress->szAddress,m_pInitParameter->m_wCorrespondPort);

			//提示消息
			CTraceService::TraceString(szString,TraceLevel_Normal);

			return true;
		}
	}

	return false;
}

//调度事件
bool CAttemperEngineSink::OnEventAttemperData(WORD wRequestID, VOID * pData, WORD wDataSize)
{
	return false;
}

//时间事件
bool CAttemperEngineSink::OnEventTimer(DWORD dwTimerID, WPARAM wBindParam)
{
	switch (dwTimerID)
	{
	case IDI_LOAD_GAME_LIST:		//加载列表
		{
			//加载列表
			m_ServerListManager.DisuseKernelItem();
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOAD_GAME_LIST,0,NULL,0);

			return true;
		}
	case IDI_CONNECT_CORRESPOND:	//连接协调
		{
			//发起连接
			tagAddressInfo * pCorrespondAddress=&m_pInitParameter->m_CorrespondAddress;
			m_pITCPSocketService->Connect(pCorrespondAddress->szAddress,m_pInitParameter->m_wCorrespondPort);

			//构造提示
			TCHAR szString[512]=TEXT("");
			_sntprintf(szString,CountArray(szString),TEXT("正在连接协调服务器 [ %s:%d ]"),pCorrespondAddress->szAddress,m_pInitParameter->m_wCorrespondPort);

			//提示消息
			CTraceService::TraceString(szString,TraceLevel_Normal);

			return true;
		}
	case IDI_COLLECT_ONLINE_INFO:	//统计在线
		{
			//变量定义
			DBR_GP_OnLineCountInfo OnLineCountInfo;
			ZeroMemory(&OnLineCountInfo,sizeof(OnLineCountInfo));

			//获取总数
			OnLineCountInfo.dwOnLineCountSum=m_ServerListManager.CollectOnlineInfo();

			//获取类型
			POSITION KindPosition=NULL;
			do
			{
				//获取类型
				CGameKindItem * pGameKindItem=m_ServerListManager.EmunGameKindItem(KindPosition);

				//设置变量
				if (pGameKindItem!=NULL)
				{
					WORD wKindIndex=OnLineCountInfo.wKindCount++;
					OnLineCountInfo.OnLineCountKind[wKindIndex].wKindID=pGameKindItem->m_GameKind.wKindID;
					OnLineCountInfo.OnLineCountKind[wKindIndex].dwOnLineCount=pGameKindItem->m_GameKind.dwOnLineCount;
				}

				//溢出判断
				if (OnLineCountInfo.wKindCount>=CountArray(OnLineCountInfo.OnLineCountKind))
				{
					ASSERT(FALSE);
					break;
				}

			} while (KindPosition!=NULL);

			//发送请求
			WORD wHeadSize=sizeof(OnLineCountInfo)-sizeof(OnLineCountInfo.OnLineCountKind);
			WORD wSendSize=wHeadSize+OnLineCountInfo.wKindCount*sizeof(OnLineCountInfo.OnLineCountKind[0]);
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_ONLINE_COUNT_INFO,0,&OnLineCountInfo,wSendSize);

			return true;
		}
	}

	return false;
}

//应答事件
bool CAttemperEngineSink::OnEventTCPNetworkBind(DWORD dwClientAddr, DWORD dwSocketID)
{
	//获取索引
	ASSERT(LOWORD(dwSocketID)<m_pInitParameter->m_wMaxConnect);
	if (LOWORD(dwSocketID)>=m_pInitParameter->m_wMaxConnect) return false;

	//变量定义
	WORD wBindIndex=LOWORD(dwSocketID);
	tagBindParameter * pBindParameter=(m_pBindParameter+wBindIndex);

	//设置变量
	pBindParameter->dwSocketID=dwSocketID;
	pBindParameter->dwClientAddr=dwClientAddr;
	pBindParameter->dwActiveTime=(DWORD)time(NULL);

	return true;
}

//关闭事件
bool CAttemperEngineSink::OnEventTCPNetworkShut(DWORD dwClientAddr, DWORD dwActiveTime, DWORD dwSocketID)
{
	//清除信息
	WORD wBindIndex=LOWORD(dwSocketID);
	ZeroMemory((m_pBindParameter+wBindIndex),sizeof(tagBindParameter));

	return false;
}

//读取事件
bool CAttemperEngineSink::OnEventTCPNetworkRead(TCP_Command Command, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	switch (Command.wMainCmdID)
	{
	case MDM_GP_LOGON:			//登录命令
		{
			return OnTCPNetworkMainPCLogon(Command.wSubCmdID,pData,wDataSize,dwSocketID);
		}
	}

	return false;
}

//数据库事件
bool CAttemperEngineSink::OnEventDataBase(WORD wRequestID, DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	switch (wRequestID)
	{
	case DBO_GP_LOGON_SUCCESS:			//登录成功
		{
			return OnDBPCLogonSuccess(dwContextID,pData,wDataSize);
		}
	case DBO_GP_LOGON_FAILURE:			//登录失败
		{
			return OnDBPCLogonFailure(dwContextID,pData,wDataSize);
		}
	case DBR_GP_VALIDATE_MBCARD:
		{
			return OnDBPCLogonValidateMBCard(dwContextID,pData,wDataSize);
		}
	case DBO_GP_USER_FACE_INFO:			//用户头像
		{
			return OnDBPCUserFaceInfo(dwContextID,pData,wDataSize);
		}
	case DBO_GP_USER_INDIVIDUAL:		//用户信息
		{
			return OnDBPCUserIndividual(dwContextID,pData,wDataSize);
		}
	case DBO_GP_USER_INSURE_INFO:		//银行资料
		{
			return OnDBPCUserInsureInfo(dwContextID,pData,wDataSize);
		}
	case DBO_GP_USER_INSURE_SUCCESS:	//银行成功
		{
			return OnDBPCUserInsureSuccess(dwContextID,pData,wDataSize);
		}
	case DBO_GP_USER_INSURE_FAILURE:	//银行失败
		{
			return OnDBPCUserInsureFailure(dwContextID,pData,wDataSize);
		}
	case DBO_GP_USER_INSURE_USER_INFO:  //用户信息
		{
			return OnDBPCUserInsureUserInfo(dwContextID,pData,wDataSize);
		}
	case DBO_GP_OPERATE_SUCCESS:		//操作成功
		{
			return OnDBPCOperateSuccess(dwContextID,pData,wDataSize);
		}
	case DBO_GP_OPERATE_FAILURE:		//操作失败
		{
			return OnDBPCOperateFailure(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_TYPE_ITEM:			//类型子项
		{
			return OnDBPCGameTypeItem(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_KIND_ITEM:			//类型子项
		{
			return OnDBPCGameKindItem(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_NODE_ITEM:			//节点子项
		{
			return OnDBPCGameNodeItem(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_PAGE_ITEM:			//定制子项
		{
			return OnDBPCGamePageItem(dwContextID,pData,wDataSize);
		}
	case DBO_GP_GAME_LIST_RESULT:		//加载结果
		{
			return OnDBPCGameListResult(dwContextID,pData,wDataSize);
		}
	case DBO_MB_LOGON_SUCCESS:			//登录成功
		{
			return OnDBMBLogonSuccess(dwContextID,pData,wDataSize);
		}
	case DBO_MB_LOGON_FAILURE:			//登录失败
		{
			return OnDBMBLogonFailure(dwContextID,pData,wDataSize);
		}
	}

	return false;
}

//关闭事件
bool CAttemperEngineSink::OnEventTCPSocketShut(WORD wServiceID, BYTE cbShutReason)
{
	//协调连接
	if (wServiceID==NETWORK_CORRESPOND)
	{
		//重连判断
		if (m_bNeekCorrespond==true)
		{
			//构造提示
			TCHAR szDescribe[128]=TEXT("");
			_sntprintf(szDescribe,CountArray(szDescribe),TEXT("与协调服务器的连接关闭了，%ld 秒后将重新连接"),m_pInitParameter->m_wConnectTime);

			//提示消息
			CTraceService::TraceString(szDescribe,TraceLevel_Warning);

			//设置时间
			ASSERT(m_pITimerEngine!=NULL);
			m_pITimerEngine->SetTimer(IDI_CONNECT_CORRESPOND,m_pInitParameter->m_wConnectTime*1000L,1,0);

			return true;
		}
	}

	return false;
}

//连接事件
bool CAttemperEngineSink::OnEventTCPSocketLink(WORD wServiceID, INT nErrorCode)
{
	//协调连接
	if (wServiceID==NETWORK_CORRESPOND)
	{
		//提示消息
		CTraceService::TraceString(TEXT("正在注册游戏登录服务器..."),TraceLevel_Normal);

		//变量定义
		CMD_CS_C_RegisterPlaza RegisterPlaza;
		ZeroMemory(&RegisterPlaza,sizeof(RegisterPlaza));

		//设置变量
		lstrcpyn(RegisterPlaza.szServerName,m_pInitParameter->m_szServerName,CountArray(RegisterPlaza.szServerName));
		lstrcpyn(RegisterPlaza.szServerAddr,m_pInitParameter->m_ServiceAddress.szAddress,CountArray(RegisterPlaza.szServerAddr));

		//发送数据
		m_pITCPSocketService->SendData(MDM_CS_REGISTER,SUB_CS_C_REGISTER_PLAZA,&RegisterPlaza,sizeof(RegisterPlaza));

		return true;
	}

	return true;
}

//读取事件
bool CAttemperEngineSink::OnEventTCPSocketRead(WORD wServiceID, TCP_Command Command, VOID * pData, WORD wDataSize)
{
	
	return true;
}

//登录处理
bool CAttemperEngineSink::OnTCPNetworkMainPCLogon(WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	switch (wSubCmdID)
	{
	case SUB_GP_LOGON_GAMEID:		//I D 登录
		{
			return OnTCPNetworkSubPCLogonGameID(pData,wDataSize,dwSocketID);
		}
	case SUB_GP_LOGON_ACCOUNTS:		//帐号登录
		{
			return OnTCPNetworkSubPCLogonAccounts(pData,wDataSize,dwSocketID);
		}
	case SUB_GP_REGISTER_ACCOUNTS:	//帐号注册
		{
			return OnTCPNetworkSubPCRegisterAccounts(pData,wDataSize,dwSocketID);
		}
	}

	return false;
}

//I D 登录
bool CAttemperEngineSink::OnTCPNetworkSubPCLogonGameID(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize>=sizeof(CMD_GP_LogonGameID));
	if (wDataSize<sizeof(CMD_GP_LogonGameID))
	{
		if (wDataSize<sizeof(CMD_GP_LogonGameID)-sizeof(BYTE))
			return false;
	}

	//变量定义
	WORD wBindIndex=LOWORD(dwSocketID);
	tagBindParameter * pBindParameter=(m_pBindParameter+wBindIndex);

	//处理消息
	CMD_GP_LogonGameID * pLogonGameID=(CMD_GP_LogonGameID *)pData;
	pLogonGameID->szPassword[CountArray(pLogonGameID->szPassword)-1]=0;
	pLogonGameID->szMachineID[CountArray(pLogonGameID->szMachineID)-1]=0;

	//设置连接
	pBindParameter->cbClientKind=CLIENT_KIND_COMPUTER;
	pBindParameter->dwPlazaVersion=pLogonGameID->dwPlazaVersion;

	//效验版本
	if (CheckPlazaVersion(DEVICE_TYPE_PC,pLogonGameID->dwPlazaVersion,dwSocketID,((pLogonGameID->cbValidateFlags&LOW_VER_VALIDATE_FLAGS)!=0))==false)
	{
		return true;
	}

	//变量定义
	DBR_GP_LogonGameID LogonGameID;
	ZeroMemory(&LogonGameID,sizeof(LogonGameID));

	//附加信息
	LogonGameID.pBindParameter=(m_pBindParameter+LOWORD(dwSocketID));

	//构造数据
	LogonGameID.dwGameID=pLogonGameID->dwGameID;
	LogonGameID.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
	lstrcpyn(LogonGameID.szPassword,pLogonGameID->szPassword,CountArray(LogonGameID.szPassword));
	lstrcpyn(LogonGameID.szMachineID,pLogonGameID->szMachineID,CountArray(LogonGameID.szMachineID));
	LogonGameID.cbNeeValidateMBCard=(pLogonGameID->cbValidateFlags&MB_VALIDATE_FLAGS);

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOGON_GAMEID,dwSocketID,&LogonGameID,sizeof(LogonGameID));

	return true;
}

//帐号登录
bool CAttemperEngineSink::OnTCPNetworkSubPCLogonAccounts(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize>=sizeof(CMD_GP_LogonAccounts));
	if (wDataSize<sizeof(CMD_GP_LogonAccounts))
	{
		if (wDataSize<sizeof(CMD_GP_LogonAccounts)-sizeof(BYTE))
			return false;
	}

	//变量定义
	WORD wBindIndex=LOWORD(dwSocketID);
	tagBindParameter * pBindParameter=(m_pBindParameter+wBindIndex);

	//处理消息
	CMD_GP_LogonAccounts * pLogonAccounts=(CMD_GP_LogonAccounts *)pData;
	pLogonAccounts->szAccounts[CountArray(pLogonAccounts->szAccounts)-1]=0;
	pLogonAccounts->szPassword[CountArray(pLogonAccounts->szPassword)-1]=0;
	pLogonAccounts->szMachineID[CountArray(pLogonAccounts->szMachineID)-1]=0;

	//设置连接
	pBindParameter->cbClientKind=CLIENT_KIND_COMPUTER;
	pBindParameter->dwPlazaVersion=pLogonAccounts->dwPlazaVersion;

	//版本判断
	if (CheckPlazaVersion(DEVICE_TYPE_PC,pLogonAccounts->dwPlazaVersion,dwSocketID,((pLogonAccounts->cbValidateFlags&LOW_VER_VALIDATE_FLAGS)!=0))==false)
	{
		return true;
	}

	//变量定义
	DBR_GP_LogonAccounts LogonAccounts;
	ZeroMemory(&LogonAccounts,sizeof(LogonAccounts));

	//附加信息
	LogonAccounts.pBindParameter=(m_pBindParameter+LOWORD(dwSocketID));

	//构造数据
	LogonAccounts.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
	lstrcpyn(LogonAccounts.szAccounts,pLogonAccounts->szAccounts,CountArray(LogonAccounts.szAccounts));
	lstrcpyn(LogonAccounts.szPassword,pLogonAccounts->szPassword,CountArray(LogonAccounts.szPassword));
	lstrcpyn(LogonAccounts.szMachineID,pLogonAccounts->szMachineID,CountArray(LogonAccounts.szMachineID));
	LogonAccounts.cbNeeValidateMBCard=(pLogonAccounts->cbValidateFlags&MB_VALIDATE_FLAGS);

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOGON_ACCOUNTS,dwSocketID,&LogonAccounts,sizeof(LogonAccounts));

	return true;
}

//帐号注册
bool CAttemperEngineSink::OnTCPNetworkSubPCRegisterAccounts(VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	//效验参数
	ASSERT(wDataSize>=sizeof(CMD_GP_RegisterAccounts));
	if (wDataSize<sizeof(CMD_GP_RegisterAccounts))
	{
		if (wDataSize<sizeof(CMD_GP_RegisterAccounts)-sizeof(BYTE))
			return false;
	}

	//变量定义
	WORD wBindIndex=LOWORD(dwSocketID);
	tagBindParameter * pBindParameter=(m_pBindParameter+wBindIndex);

	//处理消息
	CMD_GP_RegisterAccounts * pRegisterAccounts=(CMD_GP_RegisterAccounts *)pData;
	pRegisterAccounts->szAccounts[CountArray(pRegisterAccounts->szAccounts)-1]=0;
	pRegisterAccounts->szNickName[CountArray(pRegisterAccounts->szNickName)-1]=0;
	pRegisterAccounts->szSpreader[CountArray(pRegisterAccounts->szSpreader)-1]=0;
	pRegisterAccounts->szMachineID[CountArray(pRegisterAccounts->szMachineID)-1]=0;
	pRegisterAccounts->szLogonPass[CountArray(pRegisterAccounts->szLogonPass)-1]=0;
	pRegisterAccounts->szInsurePass[CountArray(pRegisterAccounts->szInsurePass)-1]=0;
	pRegisterAccounts->szPassPortID[CountArray(pRegisterAccounts->szPassPortID)-1]=0;
	pRegisterAccounts->szCompellation[CountArray(pRegisterAccounts->szCompellation)-1]=0;

	//设置连接
	pBindParameter->cbClientKind=CLIENT_KIND_COMPUTER;
	pBindParameter->dwPlazaVersion=pRegisterAccounts->dwPlazaVersion;

	//效验版本
	if (CheckPlazaVersion(DEVICE_TYPE_PC,pRegisterAccounts->dwPlazaVersion,dwSocketID,((pRegisterAccounts->cbValidateFlags&LOW_VER_VALIDATE_FLAGS)!=0))==false)
	{
		return true;
	}

	//变量定义
	DBR_GP_RegisterAccounts RegisterAccounts;
	ZeroMemory(&RegisterAccounts,sizeof(RegisterAccounts));

	//附加信息
	RegisterAccounts.pBindParameter=(m_pBindParameter+LOWORD(dwSocketID));

	//构造数据
	RegisterAccounts.wFaceID=pRegisterAccounts->wFaceID;
	RegisterAccounts.cbGender=pRegisterAccounts->cbGender;
	RegisterAccounts.dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientAddr;
	lstrcpyn(RegisterAccounts.szAccounts,pRegisterAccounts->szAccounts,CountArray(RegisterAccounts.szAccounts));
	lstrcpyn(RegisterAccounts.szNickName,pRegisterAccounts->szNickName,CountArray(RegisterAccounts.szNickName));
	lstrcpyn(RegisterAccounts.szSpreader,pRegisterAccounts->szSpreader,CountArray(RegisterAccounts.szSpreader));
	lstrcpyn(RegisterAccounts.szMachineID,pRegisterAccounts->szMachineID,CountArray(RegisterAccounts.szMachineID));
	lstrcpyn(RegisterAccounts.szLogonPass,pRegisterAccounts->szLogonPass,CountArray(RegisterAccounts.szLogonPass));
	lstrcpyn(RegisterAccounts.szInsurePass,pRegisterAccounts->szInsurePass,CountArray(RegisterAccounts.szInsurePass));
	lstrcpyn(RegisterAccounts.szPassPortID,pRegisterAccounts->szPassPortID,CountArray(RegisterAccounts.szPassPortID));
	lstrcpyn(RegisterAccounts.szCompellation,pRegisterAccounts->szCompellation,CountArray(RegisterAccounts.szCompellation));

	//投递请求
	m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_REGISTER_ACCOUNTS,dwSocketID,&RegisterAccounts,sizeof(RegisterAccounts));

	return true;
}

//版本检测
bool CAttemperEngineSink::CheckPlazaVersion(BYTE cbDeviceType, DWORD dwPlazaVersion, DWORD dwSocketID, bool bCheckLowVer)
{
	//变量定义
	bool bMustUpdate=false;
	bool bAdviceUpdate=false;
	DWORD dwVersion=VERSION_PLAZA;

	//手机版本
	if(cbDeviceType >= DEVICE_TYPE_IPAD) dwVersion=VERSION_MOBILE_IOS;
	else if(cbDeviceType >= DEVICE_TYPE_IPHONE) dwVersion=VERSION_MOBILE_IOS;
	else if(cbDeviceType >= DEVICE_TYPE_ITOUCH) dwVersion=VERSION_MOBILE_IOS;
	else if(cbDeviceType >= DEVICE_TYPE_ANDROID) dwVersion=VERSION_MOBILE_ANDROID;
	else if(cbDeviceType == DEVICE_TYPE_PC) dwVersion=VERSION_PLAZA;

	//版本判断
	if (bCheckLowVer && GetSubVer(dwPlazaVersion)<GetSubVer(dwVersion)) bAdviceUpdate=true;
	if (GetMainVer(dwPlazaVersion)!=GetMainVer(dwVersion)) bMustUpdate=true;
	if (GetProductVer(dwPlazaVersion)!=GetProductVer(dwVersion)) bMustUpdate=true;

	//升级判断
	if ((bMustUpdate==true)||(bAdviceUpdate==true))
	{
		//变量定义
		CMD_GP_UpdateNotify UpdateNotify;
		ZeroMemory(&UpdateNotify,sizeof(UpdateNotify));

		//变量定义
		UpdateNotify.cbMustUpdate=bMustUpdate;
		UpdateNotify.cbAdviceUpdate=bAdviceUpdate;
		UpdateNotify.dwCurrentVersion=dwVersion;

		//发送消息
		m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_LOGON,SUB_GP_UPDATE_NOTIFY,&UpdateNotify,sizeof(UpdateNotify));

		//中断判断
		if (bMustUpdate==true) 
		{
			m_pITCPNetworkEngine->ShutDownSocket(dwSocketID);
			return false;
		}
	}

	return true;
}

//发送请求
bool CAttemperEngineSink::SendUIControlPacket(WORD wRequestID, VOID * pData, WORD wDataSize)
{
	//发送数据
	CServiceUnits * pServiceUnits=CServiceUnits::g_pServiceUnits;
	pServiceUnits->PostControlRequest(wRequestID,pData,wDataSize);

	return true;
}

//发送类型
VOID CAttemperEngineSink::SendGameTypeInfo(DWORD dwSocketID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGameTypeItem * pGameTypeItem=NULL;

	//枚举数据
	for (DWORD i=0;i<m_ServerListManager.GetGameTypeCount();i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagGameType))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_TYPE,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGameTypeItem=m_ServerListManager.EmunGameTypeItem(Position);
		if (pGameTypeItem==NULL) break;

		//拷贝数据
		CopyMemory(cbDataBuffer+wSendSize,&pGameTypeItem->m_GameType,sizeof(tagGameType));
		wSendSize+=sizeof(tagGameType);
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_TYPE,cbDataBuffer,wSendSize);

	return;
}

//发送种类
VOID CAttemperEngineSink::SendGameKindInfo(DWORD dwSocketID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGameKindItem * pGameKindItem=NULL;

	//枚举数据
	for (DWORD i=0;i<m_ServerListManager.GetGameKindCount();i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagGameKind))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_KIND,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGameKindItem=m_ServerListManager.EmunGameKindItem(Position);
		if (pGameKindItem==NULL) break;

		//拷贝数据
		CopyMemory(cbDataBuffer+wSendSize,&pGameKindItem->m_GameKind,sizeof(tagGameKind));
		wSendSize+=sizeof(tagGameKind);
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_KIND,cbDataBuffer,wSendSize);

	return;
}

//发送节点
VOID CAttemperEngineSink::SendGameNodeInfo(DWORD dwSocketID, WORD wKindID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGameNodeItem * pGameNodeItem=NULL;

	//枚举数据
	for (DWORD i=0;i<m_ServerListManager.GetGameNodeCount();i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagGameNode))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_NODE,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGameNodeItem=m_ServerListManager.EmunGameNodeItem(Position);
		if (pGameNodeItem==NULL) break;

		//拷贝数据
		if ((wKindID==INVALID_WORD)||(pGameNodeItem->m_GameNode.wKindID==wKindID))
		{
			CopyMemory(cbDataBuffer+wSendSize,&pGameNodeItem->m_GameNode,sizeof(tagGameNode));
			wSendSize+=sizeof(tagGameNode);
		}
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_NODE,cbDataBuffer,wSendSize);

	return;
}

//发送定制
VOID CAttemperEngineSink::SendGamePageInfo(DWORD dwSocketID, WORD wKindID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGamePageItem * pGamePageItem=NULL;

	//枚举数据
	for (DWORD i=0;i<m_ServerListManager.GetGamePageCount();i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagGamePage))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_PAGE,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGamePageItem=m_ServerListManager.EmunGamePageItem(Position);
		if (pGamePageItem==NULL) break;

		//拷贝数据
		if ((wKindID==INVALID_WORD)||(pGamePageItem->m_GamePage.wKindID==wKindID))
		{
			CopyMemory(cbDataBuffer+wSendSize,&pGamePageItem->m_GamePage,sizeof(tagGamePage));
			wSendSize+=sizeof(tagGamePage);
		}
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_PAGE,cbDataBuffer,wSendSize);

	return;
}

//发送房间
VOID CAttemperEngineSink::SendGameServerInfo(DWORD dwSocketID, WORD wKindID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGameServerItem * pGameServerItem=NULL;

	//枚举数据
	for (DWORD i=0;i<m_ServerListManager.GetGameServerCount();i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagGameServer))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_SERVER,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGameServerItem=m_ServerListManager.EmunGameServerItem(Position);
		if (pGameServerItem==NULL) break;

		//拷贝数据
		if ((wKindID==INVALID_WORD)||(pGameServerItem->m_GameServer.wKindID==wKindID))
		{
			CopyMemory(cbDataBuffer+wSendSize,&pGameServerItem->m_GameServer,sizeof(tagGameServer));
			wSendSize+=sizeof(tagGameServer);
		}
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_LIST_SERVER,cbDataBuffer,wSendSize);

	if(m_wAVServerPort!=0 && m_dwAVServerAddr!=0)
	{
		//变量定义
		tagAVServerOption AVServerOption;
		AVServerOption.wAVServerPort=m_wAVServerPort;
		AVServerOption.dwAVServerAddr=m_dwAVServerAddr;

		//发送配置
		m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SERVER_LIST,SUB_GP_VIDEO_OPTION,&AVServerOption,sizeof(AVServerOption));
	};

	return;
}

//发送类型
VOID CAttemperEngineSink::SendMobileKindInfo(DWORD dwSocketID, WORD wModuleID)
{
	return;
}

//发送房间
VOID CAttemperEngineSink::SendMobileServerInfo(DWORD dwSocketID, WORD wModuleID)
{
	//网络数据
	WORD wSendSize=0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET];

	//枚举数据
	POSITION Position=NULL;
	CGameServerItem * pGameServerItem=NULL;

	//枚举数据
	for (DWORD i=0;i<m_ServerListManager.GetGameServerCount();i++)
	{
		//发送数据
		if ((wSendSize+sizeof(tagGameServer))>sizeof(cbDataBuffer))
		{
			m_pITCPNetworkEngine->SendData(dwSocketID,MDM_MB_SERVER_LIST,SUB_MB_LIST_SERVER,cbDataBuffer,wSendSize);
			wSendSize=0;
		}

		//获取数据
		pGameServerItem=m_ServerListManager.EmunGameServerItem(Position);
		if (pGameServerItem==NULL) break;

		//拷贝数据
		if (pGameServerItem->m_GameServer.wKindID==wModuleID)
		{
			CopyMemory(cbDataBuffer+wSendSize,&pGameServerItem->m_GameServer,sizeof(tagGameServer));
			wSendSize+=sizeof(tagGameServer);
		}
	}

	//发送剩余
	if (wSendSize>0) m_pITCPNetworkEngine->SendData(dwSocketID,MDM_MB_SERVER_LIST,SUB_MB_LIST_SERVER,cbDataBuffer,wSendSize);

	return;
}

//////////////////////////////////////////////////////////////////////////////////
