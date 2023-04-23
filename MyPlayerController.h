// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyChess_.h"
/*
#pragma warning(push)
#pragma warning(disable: 4996)
#include "Steam/steam_api.h"
#pragma warning(pop)
*/
#include "MyPlayerController.generated.h"

#define RAW_APP_ID "480"
/**
 * 
 */

UENUM(BlueprintType)
enum class EWidgetState : uint8
{
	NONEWIDGET,
	MENU,
	LOBBY,
	INGAME,
	Spectator,
	LOADING
};

USTRUCT(BlueprintType)
struct FPlayerListsd
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


USTRUCT(BlueprintType)
struct FPlayerDataBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FString pid = "-1";
	UPROPERTY()
		bool isvalid = false;
	UPROPERTY()
		int win = 0;
	UPROPERTY()
		int lose = 0;
	UPROPERTY()
		int draw = 0;
	UPROPERTY()
		FString username = "";
};
USTRUCT(BlueprintType)
struct FServerDataBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FString ServerIP = "172.30.1.71";
	UPROPERTY(BlueprintReadWrite)
		FString Ports = "-1";
	UPROPERTY(BlueprintReadWrite)
		int Players = 0;
	UPROPERTY(BlueprintReadWrite)
		int users = 0;
	UPROPERTY(BlueprintReadWrite)
		FString servername = "";
};



UCLASS()
class MYCHESS__API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AMyPlayerController();
	UPROPERTY(BlueprintReadWrite)
		bool bPromotion = false;
	UPROPERTY(BlueprintReadWrite)
		int MyTeamColor = 1;
	UPROPERTY()
		bool bDoOnce = true;
	
	UPROPERTY(BlueprintReadWrite)
		bool bAndroidController = false;
		
	virtual void BeginPlay()override;
	virtual void Tick(float DeltaTime)override;
	virtual void OnPossess(APawn* InPawn) override;

	void InteractionMode();
	void MovableMode();


	class AMyPlayer* Playersd;
	UPROPERTY()
		class AMyPlayerState* PlayerStates;
	UPROPERTY(BlueprintReadWrite)
		class AMyCharacter* MyPlayer;
	UPROPERTY()
		class AMyChessPiece* ChessPiece;
	UPROPERTY()
		class AMyTile* ChessTile;
	UPROPERTY()
		bool bInteraction = false;
	UPROPERTY(BlueprintReadWrite)
		bool bSpectator = false;

	UPROPERTY()
		class AMyChessRule* ChessRule;

	UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> ChessRuleClass;
	UPROPERTY()
		class AMyChessPiece* SelectedPiece = NULL;
	UPROPERTY()
		class AMyTile* SelectedTiles = NULL;
	void TraceForBlock(const FVector& Start, const FVector& End);

	UFUNCTION(Server, Reliable)
		void CallTileEmphasize_Client(class AMyTile* tiles);
	void CallTileEmphasize_Client_Implementation(class AMyTile* tiles);

	void GetPiece(AMyChessPiece* piece, bool bReset);
	UFUNCTION(Server, Reliable)
		void GetPieces_Client(AMyChessPiece* piece, bool bReset);
	void GetPieces_Client_Implementation(AMyChessPiece* piece, bool bReset);

	void GetTile(AMyTile* tiles);
	
	UFUNCTION(Server, Reliable)
		void GetTiles_Client(AMyTile* tiles);
	void GetTiles_Client_Implementation(AMyTile* tiles);
	UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> ObserverClass;

	UPROPERTY(BlueprintReadWrite)
		class AMyObserver* Observer = NULL;

	UFUNCTION(Server, Reliable)
		void PossessActor(bool bPlayer);
	void PossessActor_Implementation(bool bPlayer);

	UFUNCTION(Server, Reliable)
		void ResetPossessActor();
	void ResetPossessActor_Implementation();

	UPROPERTY()
		UUserWidget* LoadingWidget = NULL;

	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> LoadingWidgetClass;

	UFUNCTION(BlueprintCallable)
		void CreateLoadingWidget();

	FLatentActionInfo latentinfo;

	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> MenuWidgetClass;
	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> IngameWidgetClass;
	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> LobbyWidgetClass;
	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> MenuWidgetClass_M;
	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> IngameWidgetClass_M;
	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> LobbyWidgetClass_M;
	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> SpectatorWidgetClass;
	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> SpectatorWidgetClass_M;

	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> ChatWidgetClass;
	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> ChatLogWidgetClass;
	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> PlayerListWidgetClass;
	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> SearchWidgetClass;
	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> SearchDataWidgetClass;
	UPROPERTY(EditAnywhere, Category = "Widget")
		TSubclassOf<UUserWidget> SystemMessageClass;

	UPROPERTY(EditAnywhere, Category = "TouchSetup")
		UTouchInterface* MenuInterface;
	UPROPERTY(EditAnywhere, Category = "TouchSetup")
		UTouchInterface* LobbyInterface;
	UPROPERTY(EditAnywhere, Category = "TouchSetup")
		UTouchInterface* IngameInterface;
	UPROPERTY(EditAnywhere, Category = "TouchSetup")
		UTouchInterface* SpectatorInterface;

	UPROPERTY(BlueprintReadWrite)
		class UUserWidget* CurrentWidget = NULL;

	UPROPERTY(BlueprintReadWrite)
		class UMyFriendsListWidget* friendsList = NULL;
	UPROPERTY(BlueprintReadWrite)
		class UMyInviteWidget* invitewidget = NULL;
	UPROPERTY(BlueprintReadWrite)
		class UMyMessageListWidget* messagewidget = NULL;
	UPROPERTY(BlueprintReadWrite)
		EWidgetState WidgetState = EWidgetState::LOADING;
	UFUNCTION(Client, Reliable, BlueprintCallable)
		void ChangeWidget();
	void ChangeWidget_Implementation();
	void ApplyWidget(TSubclassOf<class UUserWidget> Widget, bool bShowMouseCursors);

	UPROPERTY(BlueprintReadWrite)
		class UUserWidget* chatWidget = NULL;
	UPROPERTY(BlueprintReadWrite)
		class UMyChatLogWidget* chatlogWidget = NULL;
	UPROPERTY(BlueprintReadWrite)
		class UUserWidget* ListWidget = NULL;
	UPROPERTY(BlueprintReadWrite)
		class UUserWidget* SearchWidget = NULL;
	UPROPERTY(BlueprintReadWrite)
		class UUserWidget* SearchDataWidget = NULL;
	UPROPERTY(BlueprintReadWrite)
		class UMySystemMessageWidget* SystemMessageWidget = NULL;

	UFUNCTION(BlueprintCallable)
		void Chatting(UUserWidget* chattingwidget);
	UFUNCTION(BlueprintCallable, Server, Reliable)
		void SendChat(const FString& message,const FString& name, const FString& playerpid, bool bServer, bool bMaster);
	void SendChat_Implementation(const FString& message, const FString& name, const FString& playerpid, bool bServer, bool bMaster);
	UFUNCTION(BlueprintCallable, Client, Reliable)
		void UpdateChat(const FString& message,const FString& name, const FString& playerpid, bool bSpectators, bool bServer, bool bMaster);
	void UpdateChat_Implementation(const FString& message, const FString& name, const FString& playerpid, bool bSpectators, bool bServer, bool bMaster);

	UFUNCTION(BlueprintCallable)
		void PlayerList(bool bOpen);


	UFUNCTION(BlueprintCallable, Server, Reliable)
		void SendList(bool bSendFromServer);
	void SendList_Implementation(bool bSendFromServer);
	UFUNCTION(BlueprintCallable, Client,Reliable)
		void UpdateList(const TArray<FPlayerListsd>& playerlists);
	void UpdateList_Implementation(const TArray<FPlayerListsd>& playerlists);
	UFUNCTION(BlueprintCallable, Client, Reliable)
		void AddList(FPlayerListsd playerlists);
	void AddList_Implementation(FPlayerListsd playerlists);

	UPROPERTY(BlueprintReadWrite)
		FString PlayText = "";
	UPROPERTY(BlueprintReadWrite)
		FString PlayerName = "";
	UPROPERTY(BlueprintReadWrite)
		bool bQuittingGame = false;

	class UMyGameInstance* GameIns;
	UFUNCTION(Client, Reliable)
		void SetSpectator(bool bSpectators);
	void SetSpectator_Implementation(bool bSpectators);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bReadyButton = false;

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
		void SetReadyButtons(bool bReady);
	void SetReadyButtons_Implementation(bool bReady);
	UFUNCTION(BlueprintCallable, Server, Reliable)
		void SetReadyButton(bool bReady);
	void SetReadyButton_Implementation(bool bReady);

	UFUNCTION(Client, Reliable)
		void HiddenCeiling();
	void HiddenCeiling_Implementation();

	UFUNCTION(Client, Reliable)
		void FadeScreen(int FadeState);
	void FadeScreen_Implementation(int FadeState);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bWidgetVisibility = false;

	UFUNCTION(Client, Reliable)
		void GetPlayers(AMyPlayerState* playerstate0, AMyPlayerState* playerstate1, bool bReset);
	void GetPlayers_Implementation(AMyPlayerState* playerstate0, AMyPlayerState* playerstate1, bool bReset);

	UFUNCTION(Server, Reliable)
		void GerPlayersFromServer();
	void GerPlayersFromServer_Implementation();
	
	UPROPERTY(BlueprintReadWrite)
		FString PlayerPID;
	UPROPERTY(BlueprintReadWrite)
		int PlayerWin = 0;
	UPROPERTY(BlueprintReadWrite)
		int PlayerLose = 0;
	UPROPERTY()
		FServerDataBase serverdata;
	UPROPERTY(BlueprintReadWrite)
		TArray<FServerDataBase> ServerPortArray;

	UFUNCTION(BlueprintCallable, Client, Reliable)
		void GetServerNameCli(const FString& NewName);
	void GetServerNameCli_Implementation(const FString& NewName);

	UFUNCTION(BlueprintCallable)
		void ClearWidget();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString OldServerName = "";

	FString Pstr = "";

	UFUNCTION(Server, Reliable)
		void PlayerControllerSetup(bool Spec, bool bIsLobbyLevel);
	void PlayerControllerSetup_Implementation(bool Spec, bool bIsLobbyLevel);

	UFUNCTION(Client, Reliable)
		void KickPlayer(bool bMainServer);
	void KickPlayer_Implementation(bool bMainServer);

	UPROPERTY()
		FString FindPort = "";
	UPROPERTY()
		TArray<FServerDataBase> ServerDataArray;

	UPROPERTY(BlueprintReadWrite)
		AMyPlayerState* PS0 = NULL;
	UPROPERTY(BlueprintReadWrite)
		AMyPlayerState* PS1 = NULL;

	UFUNCTION(Client, Reliable)
		void MessageDeliv(const FString& m);
	void MessageDeliv_Implementation(const FString& m);

	void LobbyWidgetChangeIndex(int index);

	void LoginWidgetChangeIndex(int index, FString tempID);

	UFUNCTION(Server, Reliable)
		void GetPlayerDataFromServer(class AMyPlayerState* PS, const FString& id, const FString& pass);
	void GetPlayerDataFromServer_Implementation(class AMyPlayerState* PS, const FString& id, const FString& pass);

	UFUNCTION(Client, Reliable)
		void CheckingValidate(const FString& Options);
	void CheckingValidate_Implementation(const FString& Options);

};
