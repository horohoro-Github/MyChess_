// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconClient.h"
#include "Interfaces/IHttpRequest.h"
#include "MyPlayerState.h"
#include "Containers/UnrealString.h"
#include "MyPlayerController.h"
#include "MyChatStruct.h"
#include "MyOnlineBeaconClient.generated.h"


UCLASS()
class MYCHESS__API AMyOnlineBeaconClient : public AOnlineBeaconClient
{
	GENERATED_BODY()
public:
	AMyOnlineBeaconClient();

	enum ENextAction : uint8
	{
		NONEACTION,
		FRIENDSLIST,
		INVITE
	};

	UPROPERTY(BlueprintReadWrite)
		FString MyPID = "";
	UPROPERTY(BlueprintReadWrite)
		FString PlayerName = "";
	UPROPERTY(BlueprintReadWrite)
		bool bLogined = false;
	UPROPERTY(BlueprintReadWrite)
		int PlayerWin = 0;
	UPROPERTY(BlueprintReadWrite)
		int PlayerDraw = 0;
	UPROPERTY(BlueprintReadWrite)
		int PlayerLose = 0;
	UPROPERTY(BlueprintReadWrite)
		FString Userid = "";
	UPROPERTY(BlueprintReadWrite)
		FString Userpass = "";
	UPROPERTY(BlueprintReadWrite)
		FString Friends = "";
	UPROPERTY(BlueprintReadWrite)
		FString FriendsRes = "";
	UPROPERTY(BlueprintReadWrite)
		FString FriendsReq = "";

	TArray<FServerDataBase> ServerDataArray;
	UPROPERTY(BlueprintReadWrite)
		TArray<FServerDataBase> ServerPortArray;

	UFUNCTION(BlueprintCallable)
		bool ConnectToServer(const FString& address);

	UFUNCTION(Client, Reliable)
		void OpenLevelClient(const FString& url, const FString& id, const FString& pass);
	void OpenLevelClient_Implementation(const FString& port, const FString& id, const FString& pass);
	
	UPROPERTY()
		class AMyPlayerController* PC = NULL;
	UPROPERTY()
		class AMyPlayerState* PS = NULL;
	
	UPROPERTY()
		bool bGameServerBeacon = false;

	virtual void OnFailure()override;
	virtual void OnConnected()override;
	UPROPERTY()
		class UMyGameInstance* gameins = NULL;

	UFUNCTION(BlueprintCallable, Server, Reliable)
		void OpenLevelWithPlayerData(const FString& url);
	void OpenLevelWithPlayerData_Implementation(const FString& url);
	
	UFUNCTION(Server, Reliable)
		void RenamePlayer_Server(const FString& name);
	void RenamePlayer_Server_Implementation(const FString& name);

	UFUNCTION(Client, Reliable)
		void RenamePlayer_Client(const FString& name);
	void RenamePlayer_Client_Implementation(const FString& name);

	UFUNCTION(Server, Reliable)
		void GetServerPort_Server(int MatchNumber);
	void GetServerPort_Server_Implementation(int MatchNumber);

	UFUNCTION(Client, Reliable)
		void ServerSearchClient(const FString& response);
	void ServerSearchClient_Implementation(const FString& response);

	UFUNCTION(Server, Reliable)
		void FindServerPortFromServer(const FString& port);
	void FindServerPortFromServer_Implementation(const FString& port);

	UFUNCTION(Client, Reliable)
		void LoadingStreamLevel(bool bLoad);
	void LoadingStreamLevel_Implementation(bool bLoad);

	UFUNCTION(Client, Reliable)
		void RejectedLogin(bool bLogin, int RejectedType); // 0. 서버가 아님, 1. 계정생성 시, 2. 로그인 시, 3. 로그인 시 (아이디가 없을때), 4. 닉네임이 있을때 
	void RejectedLogin_Implementation(bool bLogin, int RejectedType);

	//로그인 계정
	UFUNCTION(Server, Reliable)
		void GetAccount(const FString& id, const FString& pass);
	void GetAccount_Implementation(const FString& id, const FString& pass);

	UFUNCTION(Client, Reliable)
		void CreateAccount(const FString& id, const FString& Pass);
	void CreateAccount_Implementation(const FString& id, const FString& Pass);

	UFUNCTION(Server, Reliable)
		void AccountRegistration(const FString& id, const FString& pass);
	void AccountRegistration_Implementation(const FString& id, const FString& pass);

	UFUNCTION(Server, Reliable)
		void LoginAttempt(const FString& id, const FString& pass, int LoginState, bool bTravel, const FString& uid);
	void LoginAttempt_Implementation(const FString& id, const FString& pass, int LoginState, bool bTravel, const FString& uid);

	UFUNCTION(Client, Reliable)
		void Login_C(const FString& id, const FString& pass, int LoginState);
	void Login_C_Implementation(const FString& id, const FString& pass, int LoginState);

	UFUNCTION(Client, Reliable)
		void RegisterComplete(const FString& tempid);
	void RegisterComplete_Implementation(const FString& tempid);

	UPROPERTY()
		bool bTraveling = false;

	UFUNCTION(Client, Reliable)
		void SetPlayerData(const FString& name, int win, int draw, int lose, const FString& id, const FString& Strfriends, const FString& Strfriendsres, const FString& Strfriendsreq, bool bOnlyFriendsData);
	void SetPlayerData_Implementation(const FString& name, int win, int draw, int lose, const FString& id, const FString& Strfriends, const FString& Strfriendsres, const FString& Strfriendsreq, bool bOnlyFriendsData);

	UFUNCTION(Client, Reliable)
		void LobbyWidgetChangerIndex(int index);
	void LobbyWidgetChangerIndex_Implementation(int index);

	UFUNCTION(Client, Reliable)
		void AddMessageDataWidget(const FString& id, const FString& name, bool bInvite, const FString& message, bool bSender);
	void AddMessageDataWidget_Implementation(const FString& id, const FString& name, bool bInvite, const FString& message, bool bSender);

	UFUNCTION(Client, Reliable)
		void GetFriendsList(const TArray<FString>& idarray, const TArray<FString>& namearray);
	void GetFriendsList_Implementation(const TArray<FString>& idarray, const TArray<FString>& namearray);

	UFUNCTION(Server, Reliable)
		void GetFriendsList_Server(const FString& id, const FString& StrFriends, const FString& StrFriendsres, const FString& StrFriendsreq);
	void GetFriendsList_Server_Implementation(const FString& id, const FString& StrFriends, const FString& StrFriendsres, const FString& StrFriendsreq);

	UFUNCTION(Server, Reliable)
		void UpdateFriendListData(const FString& Targetid, const FString& id, const FString& name, const FString& StrFriends, const FString& StrFriendsres, const FString& StrFriendsreq, int IFriend, const FString& Messages);
	void UpdateFriendListData_Implementation(const FString& Targetid, const FString& id, const FString& name, const FString& StrFriends, const FString& StrFriendsres, const FString& StrFriendsreq, int IFriend, const FString& Messages);

	UFUNCTION(Server, Reliable)
		void OpenSubDedicatedCheck();
	void OpenSubDedicatedCheck_Implementation();

	UFUNCTION(Server, Reliable)
		void ReLoginBeacon(const FString& url, const FString& id, const FString& pass);
	void ReLoginBeacon_Implementation(const FString& url, const FString& id, const FString& pass);

	UPROPERTY()
		TArray<FMessageStruct> messages;
	UPROPERTY()
		FString LevelName = "";
	UPROPERTY()
		FString PortNum = "";
	UPROPERTY()
		FString InstanceUID = "-1";

	UFUNCTION(Server, Reliable)
		void SetLevelName(const FString& name, const FString& port);
	void SetLevelName_Implementation(const FString& name, const FString& port);

};
