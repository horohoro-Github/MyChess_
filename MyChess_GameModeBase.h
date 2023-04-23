// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyPlayerController.h"
#include "MyServerSearchDataWidget.h"

#include "Http.h"
//#include "MyChess_.h"
/*
#pragma warning(push)
#pragma warning(disable: 4996)
#include "Steam/steam_api.h"
#pragma warning(pop)
*/

#include "MyChess_GameModeBase.generated.h"

//#define RAW_APP_ID "480"

class AMyPlayerState;

/*
 * 
 */
USTRUCT()
struct FOptions
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FString Options = "";
	UPROPERTY()
		FUniqueNetIdRepl uniqueid;
};

USTRUCT(BlueprintType)
struct FLoginData
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FString UID = "-1";
	UPROPERTY()
		FString userid = "";
	UPROPERTY()
		FString userpassword = "";
};

USTRUCT(BlueprintType)
struct FUserdData
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FString userid = "";
	UPROPERTY()
		FString userpassword = "";
	UPROPERTY()
		bool login = false;
};

USTRUCT(BlueprintType)
struct FPlayerData
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FString pid = "-1";
	UPROPERTY()
		bool login = false;
	UPROPERTY()
		int win = 0;
	UPROPERTY()
		int lose = 0;
	UPROPERTY()
		int draw = 0;
	UPROPERTY()
		FString username = "";
	UPROPERTY()
		FString userid = "";
	UPROPERTY()
		FString friends = "";
	UPROPERTY()
		FString friendsres = "";
	UPROPERTY()
		FString friendsreq = "";
};

USTRUCT(BlueprintType)
struct FServerData
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FString ServerIP = "172.30.1.26";
	UPROPERTY()
		FString Ports = "-1";
	UPROPERTY()
		int Players = 0;
	UPROPERTY()
		int users = 0;
	UPROPERTY(BlueprintReadWrite)
		FString servername = "";
};


USTRUCT(BlueprintType)
struct FPlayerLists
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
		FString PID = "";
	UPROPERTY(BlueprintReadWrite)
		FString PlayerName = "";
	UPROPERTY(BlueprintReadWrite)
		int Win = 0;

	UPROPERTY(BlueprintReadWrite)
		int Draw = 0;
	UPROPERTY(BlueprintReadWrite)
		int Lose = 0;
	UPROPERTY(BlueprintReadWrite)
		FString id = "";
	UPROPERTY(BlueprintReadWrite)
		bool Spectator = false;
	UPROPERTY(BlueprintReadWrite)
		bool MasterPlayer = false;
	UPROPERTY(BlueprintReadWrite)
		bool ME = false;
};

UCLASS()
class MYCHESS__API AMyChess_GameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AMyChess_GameModeBase();
	virtual void BeginPlay()override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
		bool CreateHostBeacon();

	class AMyOnlineBeaconHostObject* HostObject = NULL;

	UPROPERTY()
		bool bDoOnce_A = true;
	UPROPERTY()
		bool bDoOnce_B = false;
	UPROPERTY()
		class AMyChessRule* ChessRule = NULL;
	UPROPERTY()
		int NumberOfPlayers=0;
	UPROPERTY()
		int controllerNum = 0;
	UPROPERTY()
		int ReadyPlayer = 0;
	UPROPERTY(BlueprintReadWrite)
		int ReadyPlayerForPlay = 0;

	UPROPERTY()
		float ReadyTimer = 30.f;

	UPROPERTY(EditAnywhere)
		TSubclassOf<APawn> CharacterClass;
	UPROPERTY(EditAnywhere)
		TSubclassOf<APawn> ObserverClass;
	UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> ChessRuleClass;

	//ConnectedControllers : 플레이어 + 관전자, ConnectedPlayers : 플레이어
	UPROPERTY(BlueprintReadWrite)
		TArray<class AMyPlayerController*> ConnectedControllers;
	UPROPERTY(BlueprintReadWrite)
		TArray<class AMyPlayerController*> ConnectedPlayers;
	//

	UPROPERTY(BlueprintReadWrite)
		TArray<class AMyPlayerState*> PlayerStates;
	UPROPERTY(BlueprintReadWrite)
		FString servername = "";
	UPROPERTY(BlueprintReadWrite)
		TArray<FString> PlayerName;

	void GameResult(int CurrentTeamColor, bool bPromotion);
	void ReadyButtonOff(class AMyPlayerController* OutPlayer);
	
	UPROPERTY()
		bool bFirstCustomer = true;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,	FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void Logout(AController* Exiting) override;

	void ResetThisGame();

	void ChangeMasterPlayer(FString Originpid, FString Targetpid);

	void UpdatePlayerData(AMyPlayerState* PS, int res, bool bLeaved);
	UPROPERTY()
		FPlayerData PlayerDataArray[2];
	
	UFUNCTION(BlueprintCallable)
		void ExecServer_Beacon(class AMyOnlineBeaconClient* bc, const FString& ports);

	FString APP_ID = RAW_APP_ID;
	UPROPERTY(BlueprintReadWrite)
		FServerData serverdata;
	UPROPERTY()
		TArray<FServerData> ServerPortArray;

	void QuitServer();
	void ServerPortInsert();

	void ServerActivate(bool bActivate);

	UPROPERTY()
		float QuitTimer = 15.f;

	void ServerUpdate_NumberOfPlayers(bool bEnter, bool bSpectator);
	void ServerUpdate_ChangeServerName(FString NewName);

	UPROPERTY()
		class AMyPlayerController* MasterPlayer = NULL;

	UPROPERTY()
		TArray<FPlayerLists> PlayerList;

	UPROPERTY()
		TArray<AMyPlayerState*> PSArray;

	UPROPERTY()
		TArray<FOptions> OptionArray;

	void RefreshTheList();

	void GetPlayers(bool bGet);

	void GetPlayerDataForClient(AMyPlayerState* PS, class AMyOnlineBeaconClient* BC, FString pid, int Friend, FString SendMessage); //Friend 0: 일반 접속, 1: 친구 요청, 2: 친구 요청 받기

	void RenameCheckPlayerForBeaconClient(class AMyOnlineBeaconClient* BC, const FString& name);
	void RenamePlayerForBeaconClient(class AMyOnlineBeaconClient* BC, const FString& name);

	void GetServerPortForBeaconClient(class AMyOnlineBeaconClient* BC, class AMyOnlineBeaconClient* OtherBeacon, int MatchNumber, FString PortNumber);

	void ServerLoaded();
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		class AMyOnlineBeaconClient* Beacons = NULL;
	UPROPERTY()
		bool bWaitProccess = false;
	UPROPERTY()
		bool bWaitProccess_Register = false;
	UPROPERTY()
		bool bWaitProccess_Rename = false;
	UPROPERTY()
		bool bWaitProccess_Login = false;

	UPROPERTY()
		bool bWaitProccess_PlayerNum = false; //게임 서버전용
	void MoveServerPlayer();
	UPROPERTY()
		FTimerHandle movehandle;
	UPROPERTY()
		TArray<class AMyOnlineBeaconClient*> BCArray;

	void FindServerPortForBeaconClient(class AMyOnlineBeaconClient* BC, FString port);

	void OpenSearchedPort(class AMyOnlineBeaconClient* BC, FString Serverdata, FString port);

	void GetUserData(class AMyOnlineBeaconClient* BC, class AMyPlayerState* PS, const FString& id, const FString& pass, int LoginState, bool bTraveling);

	void AddUserData(class AMyOnlineBeaconClient* BC, FString id, const FString& pass);

	void AddUserIDToPlayerData(FString id);

	void UserLogin(class AMyOnlineBeaconClient* BC, class AMyPlayerState* PS, FString id, FString pass, int Login, bool bTravel);

	void FriendRequst(class AMyOnlineBeaconClient* BC, FPlayerData playerData);

	void FriendResponse(class AMyOnlineBeaconClient* BC, FPlayerData playerData);

	void AcceptFriends(class AMyOnlineBeaconClient* BC, FString targetid, FPlayerData playerData, bool bFriend);

	void RejectFriends(class AMyOnlineBeaconClient* BC, FString targetid, FPlayerData playerData, bool bFriend);

	void ReLoginUserData(class AMyOnlineBeaconClient* BC, FString id, FString pass);
	void CheckUserExisted(FString id, FString pass);

	UPROPERTY()
		TArray<FLoginData> ReLoginBeacons;

	UPROPERTY()
		float LoginTempTimer = 10.f;

	TArray<FString> GetSplitStringArray(FString LongString);
	UPROPERTY()
		class AMyOnlineBeaconHostObject* BH = NULL;

	void GetPlayerDataInfo(class AMyOnlineBeaconClient* BC, FString url);
	void CheckPlayer(class AMyPlayerController* controllers);
protected:
	class FHttpModule* Http;
};
