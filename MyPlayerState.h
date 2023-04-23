// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
//#include "Http.h"

/*
#pragma warning(push)
#pragma warning(disable: 4996)
#include "Steam/steam_api.h"
#pragma warning(pop)
*/
#include "MyPlayerState.generated.h"

#define RAW_APP_ID "480"
/**
 * 
 */
USTRUCT(BlueprintType)
struct FUsersData
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FString userid = "";
	UPROPERTY()
		FString userpassword = "";
};

USTRUCT(BlueprintType)
struct FPlayerDB
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
};

UCLASS()
class MYCHESS__API AMyPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	
	AMyPlayerState();
	virtual void BeginPlay()override;
	virtual void Tick(float DeltaTime)override;

	UPROPERTY(BlueprintReadWrite, Replicated)
		bool bPromotion = false;
	UPROPERTY(BlueprintReadWrite)
		FString PlayerName = "";
	UPROPERTY(BlueprintReadWrite)
		bool bDoOnce = true;
	UPROPERTY()
		class AMyChessPiece* tempPiece;
	UPROPERTY()
		class AMyChessRule* ChessRule;
	UPROPERTY()
		class UMyGameInstance* Gameins = NULL;

	UPROPERTY(EditAnywhere, Category = "Class")
		TSubclassOf<AActor>ChessRuleClass;
	UPROPERTY()
		class AMyPlayerController* PlayerControllers = NULL;
	UPROPERTY(BlueprintReadWrite, Replicated)
		bool bSpectator = false;

	UFUNCTION(BlueprintCallable)
		void ChessPromotion(int index);
	UFUNCTION(BlueprintCallable, Server, Reliable)
		void ChessPromotion_Client(class AMyChessRule* Rule,int index);
	void ChessPromotion_Client_Implementation(class AMyChessRule* Rule, int index);

	UFUNCTION(BlueprintCallable, Server, Reliable)
		void Promotion_Client(bool bPromotions);
	void Promotion_Client_Implementation(bool bPromotions);

	UFUNCTION(BlueprintCallable, Server, Reliable)
		void StartGame_Client(class AMyChessRule* Rule, bool bReady);
	void StartGame_Client_Implementation(class AMyChessRule* Rule, bool bReady);

	UFUNCTION(BlueprintCallable, Server, Reliable)
		void SurrenderGame_Client(class AMyChessRule* Rule, int teamcolor);
	void SurrenderGame_Client_Implementation(class AMyChessRule* Rule, int teamcolor);

	UFUNCTION(BlueprintCallable, Server, Reliable)
		void ServerMessage_Server(class AMyPlayerController* PC, const FString& message);
	void ServerMessage_Server_Implementation(class AMyPlayerController* PC, const FString& message);

	UFUNCTION(Client, Reliable)
		void ApplyPlayerListCli();
	void ApplyPlayerListCli_Implementation();
	UFUNCTION(Server, Reliable)
		void ApplyPlayerList();
	void ApplyPlayerList_Implementation();

	UPROPERTY(BlueprintReadWrite, Replicated)
		bool bMasterPlayer = false;

	UFUNCTION(Server, Reliable)
		void ChangeMasterPlayer(const FString& Originpid, const FString& Targetpid);
	void ChangeMasterPlayer_Implementation(const FString& Originpid, const FString& Targetpid);

	UFUNCTION(Client, Reliable)
		void SetMasterPlayer(bool btrue);
	void SetMasterPlayer_Implementation(bool btrue);

	UFUNCTION(Server, Reliable)
		void SetWhitePlayer();
	void SetWhitePlayer_Implementation();

	UFUNCTION(NetMulticast, Reliable)
		void SetMasterPlayerServ(bool btrue);
	void SetMasterPlayerServ_Implementation(bool btrue);


	UFUNCTION(NetMulticast, Reliable)
		void SetSpectatorPlayerState(bool spec);
	void SetSpectatorPlayerState_Implementation(bool spec);

	UFUNCTION(Server, Reliable)
		void UpdatePlayInfo(int res);
	void UpdatePlayInfo_Implementation(int res);
	UFUNCTION(Client, Reliable)
		void UpdatePlayInfoCli(int res);
	void UpdatePlayInfoCli_Implementation(int res);

	UFUNCTION(BlueprintCallable, Server, Reliable)
		void ChangeServerName(const FString& NewName);
	void ChangeServerName_Implementation(const FString& NewName);

	UPROPERTY(BlueprintReadWrite, Replicated)
		FString MyPID;
	
	UPROPERTY(BlueprintReadWrite, Replicated)
		int PlayerWin = 0;
	UPROPERTY(BlueprintReadWrite, Replicated)
		int PlayerLose = 0;
	UPROPERTY(BlueprintReadWrite, Replicated)
		int PlayerDraw = 0;
	UPROPERTY(BlueprintReadWrite, Replicated)
		int MyTeamColor = 1;
	UPROPERTY(BlueprintReadWrite, Replicated)
		FString NickName = "";
	UPROPERTY(BlueprintReadWrite)
		FString PlayerPass = "";
	
	UFUNCTION(Client, Reliable)
		void SetPlayerData(const FString& name, int win, int draw, int lose, const FString& id, int IMasterPlayer, int ISpectator);
	void SetPlayerData_Implementation(const FString& name, int win, int draw, int lose, const FString& id, int IMasterPlayer, int ISpectator);

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const;
};
