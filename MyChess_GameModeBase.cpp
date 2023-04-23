// Copyright Epic Games, Inc. All Rights Reserved.


#include "MyChess_GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "MyCharacter.h"
#include "MyChessRule.h"
#include "MyTile.h"
#include "MyChessPiece.h"
#include "MyReferee.h"
#include "MyObserver.h"
#include "MyGameInstance.h"
#include "MyPlayerState.h"
#include "MyCharacter.h"
#include "JsonObjectConverter.h"
#include "GameFramework/GameSession.h"
#include "Engine/NetDriver.h"
#include "MyOnlineBeaconClient.h"
#include "OnlineBeaconHost.h"
#include "MyOnlineBeaconHostObject.h"
#include "MyGameStateBase.h"
#include "Containers/UnrealString.h"
#include "Kismet/KismetStringLibrary.h"

class ULevelStreaming* mainstreaming = NULL;

AMyChess_GameModeBase::AMyChess_GameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;

	ConnectedControllers.Init(NULL, 4);
	ConnectedPlayers.Init(NULL, 2);
	PlayerStates.SetNum(10);
	PlayerName.SetNum(10);
	Http = &FHttpModule::Get();
}

void AMyChess_GameModeBase::BeginPlay()
{
	Super::BeginPlay();

	int a = GetWorld()->URL.Port;
	FString s = FString::FromInt(a);
	
	UE_LOG(LogTemp, Warning, TEXT("%s"), *s);
	serverdata.Ports = s;
	serverdata.servername = s;
	if (s != "7777" && s!="0") ServerPortInsert();
	else if(s == "7777")
	{
		if (IsRunningDedicatedServer())
		{
			if (UGameplayStatics::GetCurrentLevelName(GetWorld(), true) == FString("LobbyLevel"))
			{
				serverdata.servername = "Activated";
				ServerActivate(true);
				CreateHostBeacon();
			}
		}	
	}
	if (IsRunningDedicatedServer())
	{
		if (UGameplayStatics::GetCurrentLevelName(GetWorld(), true) == FString("MainMap"))
		{
			if (Beacons == NULL)
			{
				FActorSpawnParameters spawnbeacon;
				Beacons = GetWorld()->SpawnActor<AMyOnlineBeaconClient>(AMyOnlineBeaconClient::StaticClass(), FTransform(FRotator(0.f, 0.f, 0.f), FVector(0.f, 0.f, 0.f)), spawnbeacon);
				Beacons->ConnectToServer(SERVER_IPADDR);
			}
		}
	}
	FPlayerLists FS;
	PlayerList.Add(FS);

}

void AMyChess_GameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if(serverdata.servername=="Activated") ServerActivate(false);
	
}

void AMyChess_GameModeBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (bDoOnce_A == true)
	{
		if (UGameplayStatics::GetCurrentLevelName(GetWorld(), true) == FString("MainMap"))
		{
			if (ChessRule == NULL)
			{
				ChessRule = Cast<AMyChessRule>(UGameplayStatics::GetActorOfClass(GetWorld(),ChessRuleClass));
				if (ChessRule != NULL)
				{
					bDoOnce_A = false;
					bDoOnce_B = true;
				}
			}
		}
		if (UGameplayStatics::GetCurrentLevelName(GetWorld(), true) == FString("OffLineMap"))
		{
			if (ChessRule == NULL)
			{
				ChessRule = Cast<AMyChessRule>(UGameplayStatics::GetActorOfClass(GetWorld(), ChessRuleClass));
				if (ChessRule != NULL) 	bDoOnce_A = false;
			}
		}
	}
	if (bDoOnce_B == true)
	{
		if (GetNumPlayers() == 0)
		{
			QuitTimer -= 1.f * DeltaTime;
			if (QuitTimer <= 0.f)
			{
				QuitServer();
				bDoOnce_B = false;
			}
		}

		if (ReadyPlayerForPlay == 1)
		{
			if (ConnectedPlayers[0] != NULL && ConnectedPlayers[1] != NULL)
			{
				ReadyTimer -= 1.f * DeltaTime;

				if (ReadyTimer <= 0.f)
				{
					if (ConnectedPlayers[0]->bReadyButton == false)
					{
						ConnectedPlayers[0]->KickPlayer(false);
					}
					if (ConnectedPlayers[1]->bReadyButton == false)
					{
						ConnectedPlayers[1]->KickPlayer(false);
					}
					ReadyTimer = 30.f;
				}
			}
		}
	}
}

bool AMyChess_GameModeBase::CreateHostBeacon()
{
	if (AOnlineBeaconHost* Host = GetWorld()->SpawnActor<AOnlineBeaconHost>(AOnlineBeaconHost::StaticClass()))
	{
		if (Host->InitHost())
		{
			Host->PauseBeaconRequests(false);
			HostObject = GetWorld()->SpawnActor<AMyOnlineBeaconHostObject>(AMyOnlineBeaconHostObject::StaticClass());
			if (HostObject)
			{
				Host->RegisterHost(HostObject);
				return true;
			}
		}
	}
	return false;
}

void AMyChess_GameModeBase::GameResult(int CurrentTeamColor, bool bPromotion)
{
	ChessRule->bCanStartGame = false;

	FTimerHandle handle;
	GetWorld()->GetTimerManager().SetTimer(handle, FTimerDelegate::CreateLambda([&]()
		{
			GetPlayers(false);
			ResetThisGame();
		}), 5.f, false);

	AMyPlayerState* PS1 = NULL;
	AMyPlayerState* PS2 = NULL;

	if(ConnectedPlayers[0] != NULL) PS1 = Cast<AMyPlayerState>(ConnectedPlayers[0]->PlayerState);
	if(ConnectedPlayers[1] != NULL) PS2 = Cast<AMyPlayerState>(ConnectedPlayers[1]->PlayerState);

	if (CurrentTeamColor == 2)
	{
		if (PS1) UpdatePlayerData(PS1, 2, false);
	
		if (PS2) UpdatePlayerData(PS2, 2, false);
	}
	else
	{
		if (PS1)
		{
			if (PS1->MyTeamColor != CurrentTeamColor) UpdatePlayerData(PS1, 1, false); else UpdatePlayerData(PS1, 0, false);
		}
		if (PS2)
		{
			if (PS2->MyTeamColor != CurrentTeamColor) UpdatePlayerData(PS2, 1, false); else UpdatePlayerData(PS2, 0, false);
		}
	}
}

void AMyChess_GameModeBase::ReadyButtonOff(class AMyPlayerController* OutPlayer)
{
	ReadyTimer = 30.f;
	if (ConnectedPlayers[0] != NULL && ConnectedPlayers[0] != OutPlayer) ConnectedPlayers[0]->SetReadyButtons(false);
	
	if (ConnectedPlayers[1] != NULL && ConnectedPlayers[1] != OutPlayer) ConnectedPlayers[1]->SetReadyButtons(false);
}

void AMyChess_GameModeBase::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	//OptionsString = Options;

	FOptions op = { Options, UniqueId };
	OptionArray.Add(op);

	FTimerHandle h_PreLogin;
	FTimerDelegate PreLoginDelegate = FTimerDelegate::CreateLambda([UniqueId, this]()
		{
			for (int i = 0; i < this->OptionArray.Num(); i++)
			{
				if (this->OptionArray[i].uniqueid == UniqueId)
				{
					this->OptionArray.RemoveAt(i);
					break;
				}
			}
		});
	GetWorld()->GetTimerManager().SetTimer(h_PreLogin, PreLoginDelegate, 15.f, false);
}

void AMyChess_GameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AMyPlayerController* PC = Cast<AMyPlayerController>(NewPlayer);

	if (PC)
	{
		PC->FadeScreen(0);
		
		if (UGameplayStatics::GetCurrentLevelName(GetWorld(), true) == FString("OffLineMap"))
		{
			PC->PlayerControllerSetup(false, false);
		}
		if (UGameplayStatics::GetCurrentLevelName(GetWorld(), true) != FString("MainMap"))
		{
			PC->CheckingValidate(OptionsString);
		}
		
		if (UGameplayStatics::GetCurrentLevelName(GetWorld(), true) == FString("MainMap"))
		{
			AMyPlayerState* states = Cast<AMyPlayerState>(PC->PlayerState);
			
			if (states)
			{
				for (int i = 0; i < OptionArray.Num(); i++)
				{
					if (OptionArray[i].uniqueid == states->GetUniqueId())
					{
						PC->CheckingValidate(OptionArray[i].Options);
						OptionArray.RemoveAt(i);
						break;
					}
					if (OptionArray.Num() - 1 == i)
					{
						PC->KickPlayer(true);
					}
				}
			}
			QuitTimer = 15.f;
			
			for (int i = 0; i < ConnectedControllers.Num(); i++)
			{
				if (ConnectedControllers[i] == NULL)
				{
					ConnectedControllers[i] = PC;
					break;
				}
			}
			
			//PSArray.Add(states);

			if (NumberOfPlayers == 2)
			{
				ServerUpdate_NumberOfPlayers(true, true);
				PC->PlayerControllerSetup(true, false);
				PC->SetSpectator(true);
				states->SetSpectatorPlayerState(true);
			}
			else
			{
				if (NumberOfPlayers == 0)
				{
					if (MasterPlayer == NULL)
					{
						MasterPlayer = PC;
						states->SetMasterPlayerServ(true);
					}
				}

				PC->PlayerControllerSetup(false, false);

				for (int i = 0; i < ConnectedPlayers.Num(); i++)
				{
					if (ConnectedPlayers[i] == NULL)
					{
						ConnectedPlayers[i] = PC;
						break;
					}
				}
				NumberOfPlayers++;
				ServerUpdate_NumberOfPlayers(true, false);
			}
		}
		if (UGameplayStatics::GetCurrentLevelName(GetWorld(), true) == FString("LobbyLevel") || UGameplayStatics::GetCurrentLevelName(GetWorld(), true) == FString("MenuLevel"))
		{
			PC->PlayerControllerSetup(false, true);
		}
	}
}

void AMyChess_GameModeBase::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	AMyPlayerController* ExitingController = Cast<AMyPlayerController>(Exiting);
	AMyPlayerState* states = Cast<AMyPlayerState>(ExitingController->PlayerState);

	if (IsRunningDedicatedServer())
	{
		if (UGameplayStatics::GetCurrentLevelName(GetWorld(), true) == FString("MainMap"))
		{
			ExitingController->SendChat(FString("Leaves"), states->NickName, states->MyPID, true, false);
		
			for (int i = 0; i < PlayerList.Num(); i++)
			{
				if (states->MyPID == PlayerList[i].PID)
				{
					PlayerList.RemoveAt(i);

					break;
				}
			}

			if (ChessRule)
			{
				if (states->bSpectator == false)
				{
					ReadyButtonOff(ExitingController);
					ReadyPlayer = 0;
					ReadyPlayerForPlay = 0;
					NumberOfPlayers--;
					ServerUpdate_NumberOfPlayers(false, false);
					if (ChessRule->bCanStartGame == true && ChessRule->bIsMultiplayRule == true)
					{
						for (int i = 0; i < ConnectedPlayers.Num(); i++)
						{
							if (ConnectedPlayers[i] == NULL)
							{
								break;
							}
							if (i == 1)
							{
								ChessRule->bGameOvered = true;
								ChessRule->bCanStartGame = false;
								ChessRule->bDisconnectedPlayer = true;
								if (ConnectedPlayers[0] == ExitingController)
								{
									AMyPlayerState* PS0 = Cast<AMyPlayerState>(ConnectedPlayers[0]->PlayerState);
									AMyPlayerState* PS1 = Cast<AMyPlayerState>(ConnectedPlayers[1]->PlayerState);
									UpdatePlayerData(PS0, 0, true);
									UpdatePlayerData(PS1, 1, false);
								}
								if (ConnectedPlayers[1] == ExitingController)
								{
									AMyPlayerState* PS0 = Cast<AMyPlayerState>(ConnectedPlayers[0]->PlayerState);
									AMyPlayerState* PS1 = Cast<AMyPlayerState>(ConnectedPlayers[1]->PlayerState);
									UpdatePlayerData(PS1, 0, true);
									UpdatePlayerData(PS0, 1, false);
								}

								ChessRule->GameEndingText = "Opposite player disconnected";
								FTimerHandle handle;
								GetWorld()->GetTimerManager().SetTimer(handle, FTimerDelegate::CreateLambda([&]()
									{
										GetPlayers(false);
										ResetThisGame();
									}), 5.f, false);
							}
						}
					}
					else
					{
						ExitingController->KickPlayer(false);
					}
				}
				else
				{
					ExitingController->KickPlayer(false);
					ServerUpdate_NumberOfPlayers(false, true);
				}
			}
			for (int i = 0; i < ConnectedPlayers.Num(); i++)
			{
				if (ExitingController == ConnectedPlayers[i])
				{

					ConnectedPlayers[i] = NULL;
					break;
				}
			}
			TArray<FPlayerListsd> parray;
			for (int i = 0; i < ConnectedControllers.Num(); i++)
			{
				if (ExitingController == ConnectedControllers[i])
				{
					ConnectedControllers[i] = NULL;
					break;
				}
			}

			for (int i = 0; i < PSArray.Num(); i++)
			{
				if (PSArray[i] == states)
				{
					PSArray.RemoveAt(i);
					break;
				}
			}

			if (ExitingController == MasterPlayer)
			{
				for (int i = 0; i < ConnectedControllers.Num(); i++)
				{
					if (ConnectedControllers[i] != NULL && ConnectedControllers[i] != ExitingController)
					{
						MasterPlayer = ConnectedControllers[i];
						AMyPlayerState* PS = Cast<AMyPlayerState>(ConnectedControllers[i]->PlayerState);
						if (PS)
						{
							PS->SetMasterPlayerServ(true);

							for (int j = 0; j < PSArray.Num(); j++)
							{
								if (PS == PSArray[j])
								{
									PS->bMasterPlayer = true;
									break;
								}
							}
						}
						break;
					}
				}
			}

			TArray<FPlayerListsd> listdatas;

			for (int i = 0; i < ConnectedControllers.Num(); i++)
			{
				if (ConnectedControllers[i] != ExitingController && ConnectedControllers[i])
				{
					AMyPlayerState* exitstate = Cast<AMyPlayerState>(ConnectedControllers[i]->PlayerState);

					if (exitstate)
					{
						FPlayerListsd listdata;
						listdata.PID = exitstate->MyPID;
						listdata.PlayerName = exitstate->NickName;
						listdata.MasterPlayer = exitstate->bMasterPlayer;
						listdata.Spectator = exitstate->bSpectator;

						listdatas.Add(listdata);
					}
				}
			}
			for (int i = 0; i < ConnectedControllers.Num(); i++)
			{
				if (ConnectedControllers[i] != ExitingController && ConnectedControllers[i])
				{
					ConnectedControllers[i]->UpdateList(listdatas);
				}
			}

			if (ExitingController->MyPlayer) ExitingController->MyPlayer->Destroy();
			if (ExitingController->Observer) ExitingController->Observer->Destroy();
			RefreshTheList();
		}
	}
}

void AMyChess_GameModeBase::ResetThisGame()	
{
	ReadyPlayer = 0;
	ReadyPlayerForPlay = 0;
	ChessRule->bDisconnectedPlayer = false;
	ChessRule->bGameOvered = false;
	ChessRule->bCanStartGame = false;
	ChessRule->GameEndingText = "None";
	ChessRule->bChecked = false;
	ChessRule->Turn[0] = 0;
	ChessRule->Turn[1] = 0;
	ChessRule->FiftyMoveRule = 50;
	ChessRule->TimerDelay = 5.f;
	ChessRule->TimeLimit[0] = 600.f;
	ChessRule->TimeLimit[1] = 600.f;
	ChessRule->RemainingBlack = 16;
	ChessRule->RemainingWhite = 16;
	ChessRule->TeamColor = 0;

	for (int i = 0; i < 64; i++)
	{
		ChessRule->TileArray[i]->CurrentPiece = NULL;
		if (i < 16)
		{
			ChessRule->TileArray[i]->TileTeamColor = 0;
			ChessRule->PieceArray[i]->Highlight(false);
			ChessRule->PieceArray[i]->bMove = false;
			ChessRule->PieceArray[i]->SetActorLocation(ChessRule->TileArray[i]->GetActorLocation() + FVector(0.f, 0.f, 50.f));
			ChessRule->PieceArray[i]->CurrentPosition = ChessRule->TileArray[i]->TileNumber;
			ChessRule->PieceArray[i]->bDead = false;
			ChessRule->PieceArray[i]->bFirstAction = true;
			ChessRule->PieceArray[i]->ChessType = ChessRule->PieceArray[i]->OriginChessType;
			ChessRule->PieceArray[i]->MeshComp->SetStaticMesh(ChessRule->PieceArray[i]->OriginMesh);
			ChessRule->TileArray[i]->CurrentPiece = ChessRule->PieceArray[i];

			switch (ChessRule->PieceArray[i]->ChessType)
			{
			case 0:
			{
				break;
			}
			case 1:
			{
				ChessRule->PieceArray[i]->bMajorPieceOrPawn = true;
				break;
			}
			case 2:
			{
				ChessRule->PieceArray[i]->bMajorPieceOrPawn = false;
				break;
			}
			case 3:
			{
				ChessRule->PieceArray[i]->bMajorPieceOrPawn = false;
				ChessRule->PieceArray[i]->bWhiteSquareBishop = ChessRule->TileArray[i]->bWhiteSquare;
				break;
			}
			case 4:
			{
				ChessRule->PieceArray[i]->bMajorPieceOrPawn = true;
				break;
			}
			case 5:
			{
				ChessRule->PieceArray[i]->bMajorPieceOrPawn = true;
				break;
			}
			case 6:
			{
				ChessRule->PieceArray[i]->bMajorPieceOrPawn = false;
				break;
			}
			}
			ChessRule->TileArray[i]->CurrentPiece = ChessRule->PieceArray[i];
		}
		else if (i > 47)
		{
			ChessRule->TileArray[i]->TileTeamColor = 1;
			ChessRule->PieceArray[i-32]->Highlight(false);
			ChessRule->PieceArray[i-32]->bMove = false;
			ChessRule->PieceArray[i-32]->SetActorLocation(ChessRule->TileArray[i]->GetActorLocation() + FVector(0.f, 0.f, 50.f));
			ChessRule->PieceArray[i-32]->CurrentPosition = ChessRule->TileArray[i]->TileNumber;
			ChessRule->PieceArray[i-32]->bDead = false;
			ChessRule->PieceArray[i-32]->bFirstAction = true;
			ChessRule->PieceArray[i-32]->ChessType = ChessRule->PieceArray[i-32]->OriginChessType;
			ChessRule->PieceArray[i-32]->MeshComp->SetStaticMesh(ChessRule->PieceArray[i-32]->OriginMesh);
			ChessRule->TileArray[i]->CurrentPiece = ChessRule->PieceArray[i-32];

			switch (ChessRule->PieceArray[i-32]->ChessType)
			{
			case 0:
			{
				break;
			}
			case 1:
			{
				ChessRule->PieceArray[i-32]->bMajorPieceOrPawn = true;
				break;
			}
			case 2:
			{
				ChessRule->PieceArray[i-32]->bMajorPieceOrPawn = false;
				break;
			}
			case 3:
			{
				ChessRule->PieceArray[i-32]->bMajorPieceOrPawn = false;
				ChessRule->PieceArray[i-32]->bWhiteSquareBishop = ChessRule->TileArray[i]->bWhiteSquare;
				break;
			}
			case 4:
			{
				ChessRule->PieceArray[i-32]->bMajorPieceOrPawn = true;
				break;
			}
			case 5:
			{
				ChessRule->PieceArray[i-32]->bMajorPieceOrPawn = true;
				break;
			}
			case 6:
			{
				ChessRule->PieceArray[i-32]->bMajorPieceOrPawn = false;
				break;
			}
			}

		}
		else
		{
			ChessRule->TileArray[i]->TileTeamColor = 2;
		}

		ChessRule->TileArray[i]->ColorIndex = 0;
		ChessRule->TileArray[i]->ChangeMaterial(0);
		ChessRule->TileArray[i]->TileState = 0;
	}

	ChessRule->bEnpassantB.Init(0, 64);
	ChessRule->bEnpassantW.Init(0, 64);
	for (int i = 0; i < 25; i++)
	{
		ChessRule->ThreefoldRepetitionCountW[i] = 0;
		ChessRule->ThreefoldRepetitionCountB[i] = 0;
		for (int j = 0; j < 32; j++)
		{
			ChessRule->ThreefoldRepetitionTableW[i].setdata(j, 0);
			ChessRule->ThreefoldRepetitionTableB[i].setdata(j, 0);
		}
	}
	ChessRule->bDoOnce_S = true;
	if (ChessRule->bIsMultiplayRule==true)
	{
		ChessRule->bStartGame = false;
		ChessRule->Referee->bDead = false;
		ChessRule->Referee->bDoOnce = true;
		ChessRule->Referee->ReturnToIdle_Server();
	}
	else
	{
		ChessRule->bStartGame = true;
	}

	if (ConnectedPlayers[0] != NULL)
	{
		ConnectedPlayers[0]->ResetPossessActor();
		ConnectedPlayers[0]->HiddenCeiling();
		AMyPlayerState* PS = Cast<AMyPlayerState>(ConnectedPlayers[0]->PlayerState);
		if (PS) PS->MyTeamColor = 1;
	}
	if (ConnectedPlayers[1] != NULL)
	{
		ConnectedPlayers[1]->ResetPossessActor();
		ConnectedPlayers[1]->HiddenCeiling();
		AMyPlayerState* PS = Cast<AMyPlayerState>(ConnectedPlayers[1]->PlayerState);
		if (PS) PS->MyTeamColor = 1;
	}

	for (int i = 0; i < ConnectedControllers.Num(); i++)
	{
		if (ConnectedControllers[i])
		{
			ConnectedControllers[i]->GetPlayers(NULL, NULL, true);
		}
	}
}

void AMyChess_GameModeBase::ChangeMasterPlayer(FString Originpid, FString Targetpid)
{
	UE_LOG(LogTemp, Warning, TEXT("Running On Server"));
	TArray<FPlayerListsd> templists;
	AMyGameStateBase* gamestate = Cast<AMyGameStateBase>(UGameplayStatics::GetGameState(GetWorld()));
	if (gamestate)
	{
		for (int i = 0; i < gamestate->PlayerArray.Num(); i++)
		{
			AMyPlayerState* PS = Cast<AMyPlayerState>(gamestate->PlayerArray[i]);
			if (Originpid == PS->MyPID)
			{
				//방장 넘겨주는 플레이어
				PS->bMasterPlayer = false;
				PS->SetMasterPlayer(false);
			}
			else
			{
				if (Targetpid == PS->MyPID)
				{
					//위임받는 플레이어
					PS->bMasterPlayer = true;
					PS->SetMasterPlayer(true);
					AMyPlayerController* MasterController = Cast<AMyPlayerController>(PS->GetPlayerController());
					MasterPlayer = MasterController;
				}
			}

			FPlayerListsd list;
			list.PID = PS->MyPID;
			list.Win = PS->PlayerWin;
			list.Draw = PS->PlayerDraw;
			list.Lose = PS->PlayerLose;
			list.MasterPlayer = PS->bMasterPlayer;
			list.Spectator = PS->bSpectator;
			list.PlayerName = PS->NickName;


			templists.Add(list);

			if (i == gamestate->PlayerArray.Num() - 1)
			{
				for (int j = 0; j < gamestate->PlayerArray.Num(); j++)
				{
					AMyPlayerState* PSS = Cast<AMyPlayerState>(gamestate->PlayerArray[j]);
					if (PSS)
					{
						AMyPlayerController* PC = Cast<AMyPlayerController>(PSS->GetPlayerController());
						if (PC) PC->UpdateList(templists);
					}
				}
			}
		}
	}
}


void AMyChess_GameModeBase::UpdatePlayerData(AMyPlayerState* PS, int res, bool bLeaved) //res 0 패배, 1 승리, 2 무승부
{
	if (IsRunningDedicatedServer())
	{
		if (res == 1) PS->PlayerWin++; else if (res == 0) PS->PlayerLose++; else PS->PlayerDraw++;

		FPlayerData PlayerDatas;
		PlayerDatas.userid = PS->MyPID;
		PlayerDatas.win = PS->PlayerWin;
		PlayerDatas.lose = PS->PlayerLose;
		PlayerDatas.draw = PS->PlayerDraw;
		PlayerDatas.username = PS->NickName;

		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
		FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();

		Request->SetURL("http://127.0.0.1:8080/api/PlayerDataBase");
		Request->SetVerb("Put");
		Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));

		Delegate.BindLambda([PS, PlayerDatas](FHttpRequestPtr Request, FHttpResponsePtr Response, bool Success)
			{
				FPlayerData playerdb;
				if (!Response->GetContentAsString().Contains("timestamp")) FJsonObjectConverter::JsonObjectStringToUStruct(Response->GetContentAsString(), &playerdb, 0, 0);

				if (PS) PS->SetPlayerData(PlayerDatas.username, PlayerDatas.win, PlayerDatas.draw, PlayerDatas.lose, PlayerDatas.userid, 2, 2);

			});

		FString JsonString;
		FJsonObjectConverter::UStructToJsonObjectString(PlayerDatas, JsonString);
		Request->SetContentAsString(JsonString);
		Request->ProcessRequest();
	}
}


void AMyChess_GameModeBase::ExecServer_Beacon(AMyOnlineBeaconClient* bc, const FString& ports)
{

	UE_LOG(LogTemp, Warning, TEXT("ExecServer %s"), *ports);
	//FString sm = TEXT("C:\\UE5Packaged\\ServerAdd\\WindowsServer\\MyChess_Server.exe");
	FString sm = TEXT("C:\\UE5Packaged\\Shipping\\ServerAdd\\WindowsServer\\MyChess_Server.exe");
	const TCHAR* ssm = *sm;
	//FString op = TEXT("-port=" + ports + " -log");
	FString op = TEXT("-port=" + ports);
	const TCHAR* opt = *op;
	FPlatformProcess::CreateProc(ssm, opt, true, false, false, nullptr, 0, nullptr, nullptr);
}

void AMyChess_GameModeBase::QuitServer()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Requests = Http->CreateRequest();
	FString addr = SERVER_IPADDR;
	Requests->SetURL("http://127.0.0.1:8080/api/ServerDataBase");
	Requests->SetVerb("Delete");
	Requests->SetHeader(TEXT("Content-type"), TEXT("application/json"));
	FString JsonString;
	FServerData servd;
	servd.ServerIP = SERVER_IPADDR;
	servd.Ports = serverdata.Ports;
	FJsonObjectConverter::UStructToJsonObjectString(servd, JsonString);
	Requests->SetContentAsString(JsonString);
	Requests->ProcessRequest();

	FGenericPlatformMisc::RequestExit(false);
}
void AMyChess_GameModeBase::ServerPortInsert()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Requests = Http->CreateRequest();
	FString addr = SERVER_IPADDR;
	Requests->SetURL("http://127.0.0.1:8080/api/ServerDataBase");
	Requests->SetVerb("Post");
	Requests->SetHeader(TEXT("Content-type"), TEXT("application/json"));

	FString JsonString;
	FServerData servd;
	servd.ServerIP = SERVER_IPADDR;
	servd.Ports = serverdata.Ports;
	servd.servername = serverdata.Ports;

	FJsonObjectConverter::UStructToJsonObjectString(servd, JsonString);
	Requests->SetContentAsString(JsonString);
	Requests->ProcessRequest();
}

void AMyChess_GameModeBase::ServerActivate(bool bActivate)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Requests = Http->CreateRequest();
	FString addr = SERVER_IPADDR;
	Requests->SetURL("http://127.0.0.1:8080/api/ServerDataBase");
	Requests->SetVerb("Put");
	Requests->SetHeader(TEXT("Content-type"), TEXT("application/json"));

	FString JsonString;
	FServerData servd;
	servd.ServerIP = SERVER_IPADDR;
	servd.Ports = serverdata.Ports;
	if (bActivate == true) servd.servername = "Activated";
	else servd.servername = "Deactivated";

	FJsonObjectConverter::UStructToJsonObjectString(servd, JsonString);
	Requests->SetContentAsString(JsonString);
	Requests->ProcessRequest();
}

void AMyChess_GameModeBase::ServerUpdate_NumberOfPlayers(bool bEnter, bool bSpectator) //플레이어가 나갔을때 플레이어 수 반영
{
	if (IsRunningDedicatedServer())
	{
		if (bWaitProccess_PlayerNum == false)
		{
			int a = GetWorld()->URL.Port;
			FString s = FString::FromInt(a);
			if (s != "7777" && s != "0" && s != "-1")
			{
				AMyChess_GameModeBase* gamemode = this;
				bWaitProccess_PlayerNum = true;
				TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
				FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();

				Delegate.BindLambda([gamemode, bEnter](FHttpRequestPtr Request, FHttpResponsePtr Response, bool Success)
					{
						if (bEnter == true)
						{
							if (gamemode->bFirstCustomer == true)
							{
								gamemode->bFirstCustomer = false;
							}
							else
							{
								if (gamemode->Beacons)
								{
									gamemode->Beacons->OpenSubDedicatedCheck();
								}
							}
						}
						gamemode->bWaitProccess_PlayerNum = false;
					});

				FString addr = SERVER_IPADDR;
				Request->SetURL("http://127.0.0.1:8080/api/ServerDataBase");
				Request->SetVerb("Put");
				Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));

				FString JsonString;
				FServerData Servd;
				Servd.Ports = s;
				Servd.Players = NumberOfPlayers;
				serverdata.Players = NumberOfPlayers;
				if (bEnter == true)
				{
					Servd.users = GetNumPlayers();
					serverdata.users = GetNumPlayers();
				}
				else
				{
					Servd.users = GetNumPlayers() - 1;
					serverdata.users = GetNumPlayers() - 1;
				}
				Servd.servername = serverdata.servername;

				FJsonObjectConverter::UStructToJsonObjectString(Servd, JsonString);
				Request->SetContentAsString(JsonString);
				Request->ProcessRequest();
			}
		}
		else
		{
			AMyChess_GameModeBase* gamemode = this;
			FTimerHandle h_NOP;
			FTimerDelegate NOPDelegate = FTimerDelegate::CreateLambda([gamemode, bEnter, bSpectator]()
				{
					gamemode->ServerUpdate_NumberOfPlayers(bEnter, bSpectator);
				});
			GetWorld()->GetTimerManager().SetTimer(h_NOP, NOPDelegate, 0.2f, false);
		}
	}
}

void AMyChess_GameModeBase::ServerUpdate_ChangeServerName(FString NewName)
{
	int a = GetWorld()->URL.Port;
	FString s = FString::FromInt(a);
	if (s != "7777" && s != "0" && s != "-1" && NewName != "")
	{
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();

		FString addr = SERVER_IPADDR;
		Request->SetURL("http://127.0.0.1:8080/api/ServerDataBase");
		Request->SetVerb("Put");
		Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));

		FString JsonString;
		FServerData Servd;

		Servd.Ports = serverdata.Ports;
		Servd.Players = serverdata.Players;
		Servd.users = serverdata.users;
		Servd.servername = NewName;
		serverdata.servername = NewName;
		FJsonObjectConverter::UStructToJsonObjectString(Servd, JsonString);
		Request->SetContentAsString(JsonString);
		Request->ProcessRequest();
	}

}

void AMyChess_GameModeBase::RefreshTheList()
{
	for (int i = 0; i < ConnectedControllers.Num(); i++)
	{
		if (ConnectedControllers[i] != NULL)
		{	
			AMyPlayerState* PS = Cast<AMyPlayerState>(ConnectedControllers[i]->PlayerState);
			if (PS) PS->ApplyPlayerListCli();
		}
	}
}

void AMyChess_GameModeBase::GetPlayers(bool bGet)
{
	if (bGet == false)
	{
		for (int i = 0; i < ConnectedControllers.Num(); i++)
		{
			if (ConnectedControllers[i] != NULL)
			{
				ConnectedControllers[i]->GetPlayers(NULL, NULL, true);
			}
		}
	}
	else
	{
		if (ConnectedPlayers.Num() == 2)
		{
			if (ConnectedPlayers[0] != NULL && ConnectedPlayers[1] != NULL)
			{
				AMyPlayerState* PS0 = Cast<AMyPlayerState>(ConnectedPlayers[0]->PlayerState);
				AMyPlayerState* PS1 = Cast<AMyPlayerState>(ConnectedPlayers[1]->PlayerState);
				ConnectedPlayers[0]->GetPlayers(NULL, PS1, false);
				ConnectedPlayers[1]->GetPlayers(NULL, PS0, false);
			
				for (int i = 0; i < ConnectedControllers.Num(); i++)
				{
					if (ConnectedControllers[i] != NULL && ConnectedControllers[i] != ConnectedPlayers[0] && ConnectedControllers[i] != ConnectedPlayers[1])
					{
						ConnectedControllers[i]->GetPlayers(PS0, PS1, false);
					}
				}

			}
		}
	}
}

void AMyChess_GameModeBase::GetPlayerDataForClient(AMyPlayerState* PS, class AMyOnlineBeaconClient* BC, FString pid, int Friend, FString SendMessage)
{
	if (IsRunningDedicatedServer())
	{
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
		FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();
		AMyChess_GameModeBase* gamemode = this;
		if (Request->DoesSharedInstanceExist())
		{
			FString addr = SERVER_IPADDR;

			Delegate.BindLambda([BC, PS, pid, gamemode, Friend, SendMessage](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool Succeeded)
				{
					if (Succeeded)
					{
						TArray<FPlayerData> playerdb;
						if (!HttpResponse->GetContentAsString().Contains("timestamp")) FJsonObjectConverter::JsonArrayStringToUStruct(HttpResponse->GetContentAsString(), &playerdb, 0, 0);

						if (Friend == 4)
						{
							if (BC)
							{
								TArray<FString> friendArray, friendResArray, friendReqArray;
								TArray<FPlayerData> playerdata;
								friendArray = gamemode->GetSplitStringArray(BC->Friends);
								friendResArray = gamemode->GetSplitStringArray(BC->FriendsRes);
								friendReqArray = gamemode->GetSplitStringArray(BC->FriendsReq);

								for (int f = 0; f < friendArray.Num(); f++)
								{
									for (int p = 0; p < playerdb.Num(); p++)
									{
										if (friendArray[f] == playerdb[p].userid)
										{
											FPlayerData pd;
									
											playerdata.Add(playerdb[p]);
											break;
										}
									}
								}
								for (int f = 0; f < friendReqArray.Num(); f++)
								{
									for (int p = 0; p < playerdb.Num(); p++)
									{
										if (friendReqArray[f] == playerdb[p].userid)
										{
											playerdata.Add(playerdb[p]);
											break;
										}
									}
								}
								for (int f = 0; f < friendResArray.Num(); f++)
								{
									for (int p = 0; p < playerdb.Num(); p++)
									{
										if (friendResArray[f] == playerdb[p].userid)
										{
											playerdata.Add(playerdb[p]);
											break;
										}
									}
								}

								TArray<FString> idarray, namearray;
								for (int a = 0; a < playerdata.Num(); a++)
								{
									
									idarray.Add(playerdata[a].userid);
									namearray.Add(playerdata[a].username);
									
								}
								BC->GetFriendsList(idarray, namearray);
							}
						}
						else
						{
							if (playerdb.Num() > 0)
							{
								for (int i = 0; i < playerdb.Num(); i++)
								{
									if (Friend == 7 || Friend == 8 || Friend == 9)
									{
										
										if (playerdb[i].username == pid)
										{
											if(Friend == 9) gamemode->GetPlayerDataForClient(PS, BC, playerdb[i].userid, 5, SendMessage);
											if(Friend == 7) gamemode->GetPlayerDataForClient(PS, BC, playerdb[i].userid, 1, SendMessage);
											if(Friend == 8) gamemode->GetPlayerDataForClient(PS, BC, playerdb[i].userid, 6, SendMessage);
										}
										
									}
									else
									{
										if (playerdb[i].userid == pid)
										{
											//플레이어 데이터 가져오기
											if (BC)
											{
												if (Friend == 0)
												{
													BC->PlayerName = playerdb[i].username;
													BC->PlayerWin = playerdb[i].win;
													BC->PlayerDraw = playerdb[i].draw;
													BC->PlayerLose = playerdb[i].lose;
													BC->Userid = playerdb[i].userid;
													BC->Friends = playerdb[i].friends;
													BC->FriendsReq = playerdb[i].friendsreq;
													BC->FriendsRes = playerdb[i].friendsres;
													BC->SetPlayerData(playerdb[i].username, playerdb[i].win, playerdb[i].draw, playerdb[i].lose, playerdb[i].userid, playerdb[i].friends, playerdb[i].friendsres, playerdb[i].friendsreq, false);
													
													if (PS == NULL)
													{
														
														if (playerdb[i].username == "")
														{
															BC->LobbyWidgetChangerIndex(5);
														}
														else
														{
															BC->LobbyWidgetChangerIndex(1);
														}
													}
													else
													{
													
														PS->NickName = playerdb[i].username;
														PS->PlayerWin = playerdb[i].win;
														PS->PlayerDraw = playerdb[i].draw;
														PS->PlayerLose = playerdb[i].lose;
														PS->MyPID = playerdb[i].userid;
														PS->SetPlayerData(playerdb[i].username, playerdb[i].win, playerdb[i].draw, playerdb[i].lose, playerdb[i].userid, 2, 2);

														gamemode->PSArray.Add(PS);
														
														for (int j = 0; j < gamemode->ConnectedControllers.Num(); j++)
														{
															FPlayerListsd listdata;
															listdata.PID = playerdb[i].userid;
															listdata.PlayerName = playerdb[i].username;
															listdata.Spectator = PS->bSpectator;
															listdata.MasterPlayer = PS->bMasterPlayer;

															if (gamemode->ConnectedControllers[j])
															{
																AMyPlayerState* ThisPS = Cast<AMyPlayerState>(gamemode->ConnectedControllers[j]->PlayerState);
																if (ThisPS != PS)
																{
																	gamemode->ConnectedControllers[j]->UpdateChat("Enters", playerdb[i].username, playerdb[i].userid, PS->bSpectator, true, PS->bMasterPlayer);
																	gamemode->ConnectedControllers[j]->AddList(listdata);
																}
															}
														}

													}
												}
												else
												{
													TArray<FString> strarray;
													TArray<FString> strarray2;
													TArray<FString> strarray3;

													strarray = gamemode->GetSplitStringArray(BC->Friends);
													strarray2 = gamemode->GetSplitStringArray(BC->FriendsRes);
													strarray3 = gamemode->GetSplitStringArray(BC->FriendsReq);

													TArray<FString> resultarray;
													for (int a = 0; a < strarray.Num(); a++)
													{
														resultarray.Add(strarray[a]);
													}
													for (int a = 0; a < strarray2.Num(); a++)
													{
														resultarray.Add(strarray2[a]);
													}
													for (int a = 0; a < strarray3.Num(); a++)
													{
														resultarray.Add(strarray3[a]);
													}

													if (resultarray.Num() == 0)
													{
														if (Friend == 1) gamemode->FriendRequst(BC, playerdb[i]);
													}
													else
													{
														for (int d = 0; d < resultarray.Num(); d++)
														{
															if (pid == resultarray[d])
															{
																if (Friend == 2) gamemode->AcceptFriends(BC, playerdb[i].userid, playerdb[i], false);
																if (Friend == 3) gamemode->RejectFriends(BC, playerdb[i].userid, playerdb[i], false);
																break;
															}

															if (d == resultarray.Num() - 1)
															{
																if (Friend == 1) gamemode->FriendRequst(BC, playerdb[i]);
															}
														}
													}
													if (Friend == 5)
													{
														if (BC->Userid != pid)
														{
															if (gamemode->HostObject)
															{
																AMyOnlineBeaconClient* otherbeacon = gamemode->HostObject->GetBeaconsClient(pid);

																if (otherbeacon)
																{
																	if (SendMessage != "")
																	{

																		otherbeacon->AddMessageDataWidget(BC->Userid, BC->PlayerName, false, SendMessage, false);
																		BC->AddMessageDataWidget(otherbeacon->Userid, otherbeacon->PlayerName, false, SendMessage, true);
																	}
																	else
																	{

																		otherbeacon->AddMessageDataWidget(BC->Userid, BC->PlayerName, true, SendMessage, false);
																		BC->AddMessageDataWidget(otherbeacon->Userid, otherbeacon->PlayerName, false, SendMessage, true);
																	}
																}
															}
														}
													}

													if (Friend == 6)
													{
														if (gamemode->HostObject)
														{
															AMyOnlineBeaconClient* otherbeacon = gamemode->HostObject->GetBeaconsClient(pid);
															if (otherbeacon)
															{
																//상대의 위치 찾기 beacon의 levelname 기준으로 이동
																
																FString levelname = otherbeacon->LevelName;
																FString portnum = otherbeacon->PortNum;
																
																UE_LOG(LogTemp, Warning, TEXT("GetPlayerDataForClient level name %s"), *levelname);
																UE_LOG(LogTemp, Warning, TEXT("GetPlayerDataForClient portnum %s"), *portnum);
																if (levelname == "LobbyLevel")
																{
																	BC->LoadingStreamLevel(true);
																	otherbeacon->LoadingStreamLevel(true);
																	gamemode->GetServerPortForBeaconClient(BC, otherbeacon, 4, "");
																}
																if (levelname == "MainMap")
																{
																	BC->LoadingStreamLevel(true);
																	gamemode->GetServerPortForBeaconClient(BC, NULL, 3, portnum); //초대 보낸 플레이어가 있는 곳으로 이동
																}
															}
														}
													}
												}

											}
											else
											{
												if (PS)
												{
													PS->NickName = playerdb[i].username;
													PS->PlayerWin = playerdb[i].win;
													PS->PlayerDraw = playerdb[i].draw;
													PS->PlayerLose = playerdb[i].lose;
													PS->MyPID = playerdb[i].userid;
													PS->SetPlayerData(playerdb[i].username, playerdb[i].win, playerdb[i].draw, playerdb[i].lose, playerdb[i].userid, 2, 2);

													if (Friend == 0)
													{
														
														gamemode->PSArray.Add(PS);

														for (int j = 0; j < gamemode->ConnectedControllers.Num(); j++)
														{
															FPlayerListsd listdata;
															listdata.PID = playerdb[i].userid;
															listdata.PlayerName = playerdb[i].username;
															listdata.Spectator = PS->bSpectator;
															listdata.MasterPlayer = PS->bMasterPlayer;

															if (gamemode->ConnectedControllers[j])
															{
																AMyPlayerState* ThisPS = Cast<AMyPlayerState>(gamemode->ConnectedControllers[j]->PlayerState);
																if (ThisPS != PS)
																{
																	gamemode->ConnectedControllers[j]->UpdateChat("Enters", playerdb[i].username, playerdb[i].userid, PS->bSpectator, true, PS->bMasterPlayer);
																	gamemode->ConnectedControllers[j]->AddList(listdata);
																}
															}
														}
													}
												}
											}
											break;

										}

										if (i == playerdb.Num() - 1)
										{
											//플레이어 데이터 추가
											if (BC)
											{
												if (Friend == 0)
												{
													if (gamemode)
													{
														if (pid != "")
														{
															gamemode->AddUserIDToPlayerData(pid);
															gamemode->GetUserData(BC, NULL, BC->Userid, BC->Userpass, 2, BC->bTraveling);
															BC->LobbyWidgetChangerIndex(5);
														}
														else
														{
															//잘못된 데이터
														}
													}
												}
											}
											if (PS)
											{

												//잘못된 데이터
											}
											break;
										}
									}
								}
							}
							else
							{
								if (BC)
								{
									if (gamemode)
									{

										gamemode->AddUserIDToPlayerData(pid);
										gamemode->GetUserData(BC, NULL, BC->Userid, BC->Userpass, 2, BC->bTraveling);
										BC->LobbyWidgetChangerIndex(5);
									}
								}
								if (PS)
								{
									//잘못된 데이터
								}
							}
						}
					}
				});
			Request->SetURL("http://127.0.0.1:8080/api/PlayerDataBase");
			Request->SetVerb("Get");
			Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));
			Request->ProcessRequest();
		}
	}
}

void AMyChess_GameModeBase::RenameCheckPlayerForBeaconClient(AMyOnlineBeaconClient* BC, const FString& name)
{
	AMyChess_GameModeBase* gamemode = this;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();
	if (Request->DoesSharedInstanceExist())
	{
		FString addr = SERVER_IPADDR;
		Request->SetURL("http://127.0.0.1:8080/api/PlayerDataBase");
		Request->SetVerb("Get");

		Delegate.BindLambda([BC, name, gamemode](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool Succeeded)
			{
				TArray<FPlayerData> playerdb;
				if (!HttpResponse->GetContentAsString().Contains("timestamp")) FJsonObjectConverter::JsonArrayStringToUStruct(HttpResponse->GetContentAsString(), &playerdb, 0, 0);
				if (playerdb.Num() > 0)
				{
					for (int i = 0; i < playerdb.Num(); i++)
					{
						if (playerdb[i].username == name)
						{
							//닉네임이 이미 존재함
							BC->RejectedLogin(false, 4);

							break;
						}
						if (i == playerdb.Num() - 1)
						{
							
							if (gamemode) gamemode->RenamePlayerForBeaconClient(BC, name);
						}
					}
				}
				else
				{
					if (gamemode) gamemode->RenamePlayerForBeaconClient(BC, name);
					//데이터 없음
				}
			});

		Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));
		Request->ProcessRequest();
	}
}

void AMyChess_GameModeBase::RenamePlayerForBeaconClient(AMyOnlineBeaconClient* BC, const FString& name)
{
	if (bWaitProccess_Rename == false)
	{
		bWaitProccess_Rename = true;
		AMyChess_GameModeBase* gamemode = this;
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
		FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();
		if (Request->DoesSharedInstanceExist())
		{
			FString addr = SERVER_IPADDR;
			Request->SetURL("http://127.0.0.1:8080/api/PlayerDataBase");
			Request->SetVerb("Put");
			Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));


			Delegate.BindLambda([gamemode, BC, name](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool Succeeded)
				{
					if (BC) BC->RenamePlayer_Client(name);
					gamemode->bWaitProccess_Rename = false;
				});

			FPlayerData playerdb;
			FString JsonString;
			playerdb.pid = BC->MyPID;
			playerdb.win = BC->PlayerWin;
			playerdb.lose = BC->PlayerLose;
			playerdb.draw = BC->PlayerDraw;
			playerdb.login = false;
			playerdb.username = name;
			playerdb.userid = BC->Userid;
			FJsonObjectConverter::UStructToJsonObjectString(playerdb, JsonString);
			Request->SetContentAsString(JsonString);
			Request->ProcessRequest();
		}
	}
	else
	{
		AMyChess_GameModeBase* gamemode = this;
		FTimerHandle h_Rename;
		FTimerDelegate RenameDelegate = FTimerDelegate::CreateLambda([gamemode, BC, name]()
			{
				gamemode->RenameCheckPlayerForBeaconClient(BC, name);
			});
		GetWorld()->GetTimerManager().SetTimer(h_Rename, RenameDelegate, 0.5f, false);
	}
}

void AMyChess_GameModeBase::GetServerPortForBeaconClient(AMyOnlineBeaconClient* BC, class AMyOnlineBeaconClient* OtherBeacon, int MatchNumber, FString PortNumber) //MatchNumber 0: 빠른 방찾기, 1: 방 만들기, 2: 모든 방찾기, 3: 초대 받은 방으로 입장
{
	if (IsRunningDedicatedServer())
	{
		if (bWaitProccess == false || MatchNumber == 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("Working"));
			if(MatchNumber != 2) bWaitProccess = true;
			AMyChess_GameModeBase* gamemode = this;
			TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
			FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();

			if (Request->DoesSharedInstanceExist())
			{
				Delegate.BindLambda([BC, MatchNumber, gamemode, OtherBeacon, PortNumber](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool Succeeded)
					{
						if (BC)
						{
							TArray<FServerData> serverdb;
							if (!HttpResponse->GetContentAsString().Contains("timestamp")) FJsonObjectConverter::JsonArrayStringToUStruct(HttpResponse->GetContentAsString(), &serverdb, 0, 0);
							switch (MatchNumber)
							{
							case 0:
							{

								int k = 0;
								for (int i = 1; i < 5000; i++)
								{
									for (int j = 0; j < serverdb.Num(); j++)
									{
										FString PortStr = FString::FromInt(i);

										if (PortStr == serverdb[j].Ports)
										{
											if (serverdb[j].Players == 1)
											{
												FString addr = SERVER_IPADDR;
												FString PStr = TEXT("" + addr + ":" + PortStr);
												k = 1;
												
												//BC->OpenLevelClient(PStr);
												BC->ReLoginBeacon(PStr, BC->Userid, BC->Userpass);
												break;
											}
										}
									}
									if (k == 1)
									{
										break;
									}
									if (i == 4999)
									{
										if (k == 0)
										{
											UE_LOG(LogTemp, Warning, TEXT("No Servers"));
											
											BC->LoadingStreamLevel(false);
											
											gamemode->bWaitProccess = false;
											//BC->LoadingStreamLevel(false);
										}
									}
								}
								break;
							}
							case 1:
							{
								int k = 0;
								for (int i = 1; i < 5000; i++)
								{
									for (int j = 0; j < serverdb.Num(); j++)
									{
										FString PortStr = FString::FromInt(i);

										if (PortStr != serverdb[j].Ports)
										{
											if (j == serverdb.Num() - 1)
											{
												gamemode->ExecServer_Beacon(BC, PortStr);
												FString addr = SERVER_IPADDR;
												FString PStr = TEXT("" + addr + ":" + PortStr);
												k = 1;
												BC->ReLoginBeacon(PStr, BC->Userid, BC->Userpass);
												//BC->OpenLevelClient(PStr);

												break;
											}
										}
										else
										{
											break;
										}
									}

									if (k == 1)
									{
										break;
									}
								}

								if (k == 0)
								{
									gamemode->bWaitProccess = false;
									BC->LoadingStreamLevel(false);
								}
								break;
							}
							case 2:
							{
								BC->ServerSearchClient(HttpResponse->GetContentAsString());
								break;
							}
							case 3:
							{
								for (int i = 0; i < serverdb.Num(); i++)
								{
									if (serverdb[i].Ports == PortNumber)
									{
										if (serverdb[i].users < 4)
										{
											FString addr = SERVER_IPADDR;
											FString PStr = TEXT("" + addr + ":" + PortNumber);
											BC->ReLoginBeacon(PStr, BC->Userid, BC->Userpass);
											//BC->OpenLevelClient(PStr);
											break;
										}
										else
										{
											BC->LoadingStreamLevel(false);
											gamemode->bWaitProccess = false;
											break;
										}
									}
									if (i == serverdb.Num() - 1)
									{
										BC->LoadingStreamLevel(false);
										gamemode->bWaitProccess = false;
										break;
									}
								}
								break;
							}
							case 4:
							{
								if (OtherBeacon != NULL)
								{
									int k = 0;
									for (int i = 1; i < 5000; i++)
									{
										for (int j = 0; j < serverdb.Num(); j++)
										{
											FString PortStr = FString::FromInt(i);

											if (PortStr != serverdb[j].Ports)
											{
												if (j == serverdb.Num() - 1)
												{
													gamemode->ExecServer_Beacon(BC, PortStr);
													FString addr = SERVER_IPADDR;
													FString PStr = TEXT("" + addr + ":" + PortStr);
													k = 1;
													OtherBeacon->ReLoginBeacon(PStr, OtherBeacon->Userid, OtherBeacon->Userpass);
													BC->ReLoginBeacon(PStr, BC->Userid, BC->Userpass);
													break;
												}
											}
											else
											{
												break;
											}
										}

										if (k == 1)
										{
											break;
										}
									}

									if (k == 0)
									{
										BC->LoadingStreamLevel(false);
										OtherBeacon->LoadingStreamLevel(false);
										gamemode->bWaitProccess = false;
									}
									break;
								}
							}
							break;
							}
						}
					});


				FString addr = SERVER_IPADDR;
				Request->SetURL("http://127.0.0.1:8080/api/ServerDataBase");
				Request->SetVerb("Get");
				Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));

				Request->ProcessRequest();
			}
			else
			{
				gamemode->bWaitProccess = false;
			}
		}
		else
		{
			FTimerHandle UniqueHandle;
			FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(this, &AMyChess_GameModeBase::GetServerPortForBeaconClient, BC, OtherBeacon, MatchNumber, PortNumber);
			GetWorldTimerManager().SetTimer(UniqueHandle, RespawnDelegate, 0.5f, false);
		}
	}
	else
	{
		if (BC)
		{
			BC->LoadingStreamLevel(false);
		}
	}
}

void AMyChess_GameModeBase::ServerLoaded()
{
	bWaitProccess = false;
}

void AMyChess_GameModeBase::MoveServerPlayer()
{
	if (BCArray.Num() > 0)
	{
		AMyOnlineBeaconClient* BC = Cast<AMyOnlineBeaconClient>(BCArray[0]);
		for (int i = 0; i < BCArray.Num(); i++)
		{
			if (BCArray[i] == BC)
			{
				BCArray.RemoveAt(i);
			}
		}
		AMyChess_GameModeBase* gamemode = this;
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
		FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();
		Delegate.BindLambda([BC, gamemode](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool Succeeded)
			{
				TArray<FServerData> serverdb;
				if (!HttpResponse->GetContentAsString().Contains("timestamp")) FJsonObjectConverter::JsonArrayStringToUStruct(HttpResponse->GetContentAsString(), &serverdb, 0, 0);
				int k = 0;
				for (int i = 1; i < 5000; i++)
				{
					for (int j = 0; j < serverdb.Num(); j++)
					{
						FString PortStr = FString::FromInt(i);

						if (PortStr != serverdb[j].Ports)
						{
							if (j == serverdb.Num() - 1)
							{
								gamemode->ExecServer_Beacon(BC, PortStr);
								//UGameplayStatics::
								FString addr = SERVER_IPADDR;
								FString PStr = TEXT("" + addr + ":" + PortStr);
								k = 1;
								BC->OpenLevelClient(PStr, BC->Userid, BC->Userpass);

								break;
							}
						}
						else
						{
							break;
						}
					}

					if (k == 1)
					{
						break;
					}
				}

				if (k == 0)
				{
				}
			});
		FString addr = SERVER_IPADDR;
		Request->SetURL("http://127.0.0.1:8080/api/ServerDataBase");
		Request->SetVerb("Get");
		Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));
	}
}

void AMyChess_GameModeBase::FindServerPortForBeaconClient(AMyOnlineBeaconClient* BC, FString port)
{
	if (IsRunningDedicatedServer())
	{
		if (bWaitProccess == false)
		{
			AMyChess_GameModeBase* gamemode = this;
			TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
			FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();

			Delegate.BindLambda([BC, gamemode, port](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool Succeeded)
				{
					if (BC)
					{
						gamemode->GetServerPortForBeaconClient(BC, NULL, 3, port);
					}
				});

			Request->SetURL("http://127.0.0.1:8080/api/ServerDataBase");
			Request->SetVerb("Get");
			Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));

			Request->ProcessRequest();
		}
		else
		{
			AMyChess_GameModeBase* gamemode = this;
			FTimerHandle h_EnterServer;
			FTimerDelegate EnterServerDelegate = FTimerDelegate::CreateLambda([gamemode, BC, port]()
				{
					gamemode->FindServerPortForBeaconClient(BC, port);
				});
			GetWorld()->GetTimerManager().SetTimer(h_EnterServer, EnterServerDelegate, 0.5f, false);
		}
	}
	else
	{
		if (BC) BC->LoadingStreamLevel(false);
	}
}

void AMyChess_GameModeBase::OpenSearchedPort(class AMyOnlineBeaconClient* BC, FString Serverdata, FString port)
{
	TArray<FServerData> serverdb;
	if (!Serverdata.Contains("timestamp")) FJsonObjectConverter::JsonArrayStringToUStruct(Serverdata, &serverdb, 0, 0);

	for (int i = 0; i < serverdb.Num(); i++)
	{
		if (serverdb[i].Ports != "7777" && serverdb[i].Ports != "" && serverdb[i].Ports != "-1" && serverdb[i].Ports == port)
		{
			if (serverdb[i].Players < 4)
			{
				UE_LOG(LogTemp, Warning, TEXT("OpenSearchedPortClient %s"), *serverdb[i].Ports);
				FString ports = serverdb[i].Ports;
				FString addr = SERVER_IPADDR;
				FString Pstr = TEXT("" + addr + ":" + ports);
				bWaitProccess = false;
				if (BC) BC->OpenLevelClient(Pstr, BC->Userid, BC->Userpass);
			}
			break;
		}

		if (i == serverdb.Num() - 1)
		{
			bWaitProccess = false;
			if (BC) BC->LoadingStreamLevel(false);
			break;
		}
	}
	
}

void AMyChess_GameModeBase::GetUserData(AMyOnlineBeaconClient* BC, class AMyPlayerState* PS, const FString& id, const FString& pass, int LoginState, bool bTraveling)
{
	if (IsRunningDedicatedServer())
	{
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
		FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();
		
		AMyChess_GameModeBase* gamemode = this;
		FString addr = SERVER_IPADDR;
		Delegate.BindLambda([BC, PS, id, pass, LoginState, bTraveling, gamemode](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool Succeeded)
			{
				TArray<FUserdData> userdb;
				if (!HttpResponse->GetContentAsString().Contains("timestamp")) 	FJsonObjectConverter::JsonArrayStringToUStruct(HttpResponse->GetContentAsString(), &userdb, 0, 0);

				if (userdb.Num() > 0)
				{
					for (int i = 0; i < userdb.Num(); i++)
					{
						if (userdb[i].userid == id)
						{
							if (LoginState == 0)
							{
								if (bTraveling == true)
								{
									if (gamemode)
									{
										if (gamemode->ReLoginBeacons.Num() > 0)
										{
											for (int j = 0; j < gamemode->ReLoginBeacons.Num(); j++)
											{
												if (gamemode->ReLoginBeacons[j].userid == id)
												{
												//	BC->RejectedLogin(true, );
													break;
												}
												if (j == gamemode->ReLoginBeacons.Num() - 1)
												{
													gamemode->UserLogin(BC, PS, id, userdb[i].userpassword, 0, bTraveling);
												}
											}
										}
										else
										{
											gamemode->UserLogin(BC, PS, id, userdb[i].userpassword, 0, bTraveling);
										}
									}
								}
								else
								{
									if (gamemode)
									{
										if (gamemode->ReLoginBeacons.Num() > 0)
										{
											for (int j = 0; j < gamemode->ReLoginBeacons.Num(); j++)
											{
												if (gamemode->ReLoginBeacons[j].userid == id)
												{
													break;
												}
												if (j == gamemode->ReLoginBeacons.Num() - 1)
												{
													gamemode->UserLogin(BC, PS, id, userdb[i].userpassword, 0, bTraveling);
												}
											}
										}
										else
										{
											gamemode->UserLogin(BC, PS, id, userdb[i].userpassword, 0, bTraveling);
										}
									}
								}
								
							}
							if (LoginState == 1)
							{
								if (userdb[i].userpassword == pass)
								{
									if (bTraveling == true)
									{
										if (gamemode)
										{
											if (gamemode->ReLoginBeacons.Num() > 0)
											{
												for (int j = 0; j < gamemode->ReLoginBeacons.Num(); j++)
												{
													if (gamemode->ReLoginBeacons[j].userid == id)
													{
														gamemode->UserLogin(BC, PS, id, userdb[i].userpassword, LoginState, bTraveling);
														break;
													}
													if (j == gamemode->ReLoginBeacons.Num() - 1)
													{
														if (userdb[i].login == false)
														{
															gamemode->UserLogin(BC, PS, id, userdb[i].userpassword, LoginState, bTraveling);
														}
														else
														{
															BC->RejectedLogin(true, 2);
															break;
														}
													}
												}
											}
											else
											{
												if (userdb[i].login == false)
												{
													gamemode->UserLogin(BC, PS, id, userdb[i].userpassword, LoginState, bTraveling);
												}
												else
												{
													BC->RejectedLogin(true, 2);
													break;
												}
											}
										}
									}
									else
									{
										if (userdb[i].login == false)
										{
											//정상적인 접속
											if (BC)
											{
												if (gamemode) gamemode->UserLogin(BC, PS, id, userdb[i].userpassword, LoginState, bTraveling);
											}
										}
										else
										{
											if (gamemode)
											{
												if (gamemode->ReLoginBeacons.Num() > 0)
												{
													for (int j = 0; j < gamemode->ReLoginBeacons.Num(); j++)
													{
														if (gamemode->ReLoginBeacons[j].userid == id && gamemode->ReLoginBeacons[j].UID == BC->InstanceUID)
														{
															gamemode->UserLogin(BC, PS, id, userdb[i].userpassword, LoginState, true);
															break;
														}
														if (j == gamemode->ReLoginBeacons.Num() - 1)
														{

															BC->RejectedLogin(true, 2);
														}
													}
												}
												else
												{
													BC->RejectedLogin(true, 2);
												}
											}
											
											//추방
										}
									}
								}
								else
								{
									BC->RejectedLogin(true, 2);
									//추방
								}
							}
							if (LoginState == 2)
							{
								
							}
							if (LoginState == 3)
							{
								BC->RejectedLogin(false, 1);
							}
							break;
						}
						if (i == userdb.Num() - 1)
						{
							if (LoginState == 3) gamemode->AddUserData(BC, id, pass);
							
							if (LoginState == 1) BC->RejectedLogin(true, 3);
						}
					}
				}
				else
				{
					if (LoginState == 1) BC->RejectedLogin(true, 3);
					if (LoginState == 3) gamemode->AddUserData(BC, id, pass);
				}
			});
		Request->SetURL("http://127.0.0.1:8080/api/UserDataBase");
		Request->SetVerb("Get");
		Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));
		Request->ProcessRequest();
	}
	else
	{
		BC->RejectedLogin(false, 0);
	}
}

void AMyChess_GameModeBase::AddUserData(AMyOnlineBeaconClient* BC, FString id, const FString& pass)
{
	if (bWaitProccess_Register == false)
	{
		bWaitProccess_Register = true;
		AMyChess_GameModeBase* gamemode = this;
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
		FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();
		
		FString addr = SERVER_IPADDR;
		Request->SetURL("http://127.0.0.1:8080/api/UserDataBase");
		Request->SetVerb("Post");
		Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));

		Delegate.BindLambda([BC, id, pass, gamemode](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool Succeeded)
			{
				if (BC) BC->RegisterComplete(id);
				gamemode->bWaitProccess_Register = false;
			});
		FUserdData userdb;
		userdb.userid = id;
		userdb.userpassword = pass;
		FString JsonString;
		FJsonObjectConverter::UStructToJsonObjectString(userdb, JsonString);
		Request->SetContentAsString(JsonString);
		Request->ProcessRequest();
		
	}
	else
	{
		AMyChess_GameModeBase* gamemode = this;
		AMyOnlineBeaconClient* onlinebeacon = BC;
		AMyPlayerState* psNull = NULL;
		const FString id_ = id;
		const FString pass_ = pass;
		FTimerHandle h_AddUser;
		int logstate = 3;
		bool traveling = true;
		FTimerDelegate AddUserDelegate = FTimerDelegate::CreateLambda([gamemode, onlinebeacon, psNull, id_, pass_, logstate, traveling]()
			{
				gamemode->GetUserData(onlinebeacon, psNull, id_, pass_, logstate, traveling);
			});
		GetWorldTimerManager().SetTimer(h_AddUser, AddUserDelegate, 0.5f, false);
	
	}
}


void AMyChess_GameModeBase::AddUserIDToPlayerData(FString id)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	Request->SetURL("http://127.0.0.1:8080/api/PlayerDataBase");
	Request->SetVerb("Post");
	Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));
	FPlayerData PD;
	PD.userid = id;
	FString JsonString;
	FJsonObjectConverter::UStructToJsonObjectString(PD, JsonString);
	Request->SetContentAsString(JsonString);
	Request->ProcessRequest();
}

void AMyChess_GameModeBase::UserLogin(class AMyOnlineBeaconClient* BC, class AMyPlayerState* PS, FString id, FString pass, int Login, bool bTravel)
{
	if (bWaitProccess_Login == false || bTravel == true)
	{
		if(bTravel==false)bWaitProccess_Login = true;
		AMyChess_GameModeBase* gamemode = this;
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
		FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();
		if (Request->DoesSharedInstanceExist())
		{
			FString addr = SERVER_IPADDR;
			Request->SetURL("http://127.0.0.1:8080/api/UserDataBase");
			Request->SetVerb("Put");
			Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));

			Delegate.BindLambda([gamemode, PS, BC, id, pass, Login, bTravel](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool Succeeded)
				{
					if (Login == 1)
					{
						if (BC)
						{
							BC->Login_C(id, pass, Login);
							BC->bTraveling = true;
							BC->Userid = id;
							BC->Userpass = pass;
							gamemode->GetPlayerDataForClient(PS, BC, id, 0, "");
						}
					}
					if (bTravel == true)
					{
						for (int i = 0; i < gamemode->ReLoginBeacons.Num(); i++)
						{
							if (id == gamemode->ReLoginBeacons[i].userid)
							{
								gamemode->ReLoginBeacons.RemoveAt(i);
								break;
							}
						}
					}
					else
					{
						gamemode->bWaitProccess_Login = false;
					}
				});

			FUserdData userdb;
			FString JsonString;
			userdb.userid = id;
			userdb.userpassword = pass;
			if (Login == 0)
			{
				userdb.login = false;
			}
			else
			{
				userdb.login = true;
			}
			FJsonObjectConverter::UStructToJsonObjectString(userdb, JsonString);
			Request->SetContentAsString(JsonString);
			Request->ProcessRequest();
		}
	}
	else
	{
		AMyChess_GameModeBase* gamemode = this;
		FTimerHandle h_Login;
		FTimerDelegate LoginDelegate = FTimerDelegate::CreateLambda([gamemode, BC, PS, pass, id, Login, bTravel]()
			{
				gamemode->GetUserData(BC, PS, id, pass, Login, bTravel);
			});
		GetWorld()->GetTimerManager().SetTimer(h_Login, LoginDelegate, 0.5f, false);
	}
}

void AMyChess_GameModeBase::FriendRequst(class AMyOnlineBeaconClient* BC, FPlayerData playerData)
{
	AMyChess_GameModeBase* gamemode = this;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();
	if (Request->DoesSharedInstanceExist())
	{
		Request->SetURL("http://127.0.0.1:8080/api/PlayerDataBase");
		Request->SetVerb("Patch");
		Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));

		Delegate.BindLambda([BC, playerData, gamemode](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool Succeeded)
			{
				if (BC)
				{
					BC->SetPlayerData("", 0, 0, 0, "", BC->Friends, BC->FriendsRes, BC->FriendsReq, true);
					gamemode->FriendResponse(BC, playerData);
				}
			});

		if (BC->FriendsReq == "")
		{
			BC->FriendsReq = playerData.userid;
		}
		else
		{
			FString Temp = TEXT("" + BC->FriendsReq + "," + playerData.userid);
			BC->FriendsReq = Temp;

		}
		FPlayerData playerdb;
		playerdb.userid = BC->Userid;
		playerdb.username = BC->PlayerName;
		playerdb.friends = BC->Friends;
		playerdb.friendsreq = BC->FriendsReq;
		playerdb.friendsres = BC->FriendsRes;

		FString JsonString;
		FJsonObjectConverter::UStructToJsonObjectString(playerdb, JsonString);
		Request->SetContentAsString(JsonString);
		Request->ProcessRequest();
	}
}

void AMyChess_GameModeBase::FriendResponse(AMyOnlineBeaconClient* BC, FPlayerData playerData)
{
	AMyChess_GameModeBase* gamemode = this;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();
	if (Request->DoesSharedInstanceExist())
	{
		Request->SetURL("http://127.0.0.1:8080/api/PlayerDataBase");
		Request->SetVerb("Patch");
		Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));

		Delegate.BindLambda([BC, playerData, gamemode](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool Succeeded)
			{
				if (gamemode->HostObject)
				{
					AMyOnlineBeaconClient* otherbeacon = gamemode->HostObject->GetBeaconsClient(playerData.userid);
					if (otherbeacon)
					{
						otherbeacon->SetPlayerData("", 0, 0, 0, "", otherbeacon->Friends, otherbeacon->FriendsRes, otherbeacon->FriendsReq, true);
					}
				}
			});

		if (playerData.friendsres == "")
		{
			playerData.friendsres = BC->Userid;
		}
		else
		{
			FString Temp = TEXT("" + playerData.friendsres + "," + BC->Userid);
			playerData.friendsres = Temp;

		}
		if (gamemode->HostObject)
		{
			AMyOnlineBeaconClient* otherbeacon = gamemode->HostObject->GetBeaconsClient(playerData.userid);
			if (otherbeacon)
			{
				otherbeacon->Friends = playerData.friends;
				otherbeacon->FriendsRes = playerData.friendsres;
				otherbeacon->FriendsReq = playerData.friendsreq;
			}
		}
		FPlayerData playerdb;
		playerdb.userid = playerData.userid;
		playerdb.username = playerData.username;
		playerdb.friends = playerData.friends;
		playerdb.friendsres = playerData.friendsres;
		playerdb.friendsreq = playerData.friendsreq;

		FString JsonString;
		FJsonObjectConverter::UStructToJsonObjectString(playerdb, JsonString);
		Request->SetContentAsString(JsonString);
		Request->ProcessRequest();
	}
}

void AMyChess_GameModeBase::AcceptFriends(AMyOnlineBeaconClient* BC, FString targetid, FPlayerData playerData, bool bFriend)
{
	AMyChess_GameModeBase* gamemode = this;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();
	FString f = "";
	FString fres = "";
	FString freq = "";
	if (Request->DoesSharedInstanceExist())
	{
		Request->SetURL("http://127.0.0.1:8080/api/PlayerDataBase");
		Request->SetVerb("Patch");
		Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));

		Delegate.BindLambda([BC, targetid, playerData, gamemode, bFriend, f, fres, freq](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool Succeeded)
			{
				if (BC)
				{
					if (bFriend == false)
					{
						BC->SetPlayerData("", 0, 0, 0, "", BC->Friends, BC->FriendsRes, BC->FriendsReq, true);
						gamemode->AcceptFriends(BC, targetid, playerData, true);
						
					}
					else
					{
						if (gamemode->HostObject)
						{
							AMyOnlineBeaconClient* otherbeacon = gamemode->HostObject->GetBeaconsClient(playerData.userid);
							if (otherbeacon)
							{
								otherbeacon->SetPlayerData("", 0, 0, 0, "", otherbeacon->Friends, otherbeacon->FriendsRes, otherbeacon->FriendsReq, true);
							}
						}
					}
				}
			});

		FPlayerData playerdb;

		if (bFriend == false)
		{
			TArray<FString> strarrayres;
			strarrayres = GetSplitStringArray(BC->FriendsRes);

			FString strres = "";
			for (int i = 0; i < strarrayres.Num(); i++)
			{
				if (targetid != strarrayres[i])
				{
					if (strres == "")
					{
						strres = strarrayres[i];
					}
					else
					{
						strres = TEXT("" + strres + "," + strarrayres[i]);
					}
					
				}
			}
			if (BC->Friends == "")
			{
				BC->Friends = targetid;
			}
			else
			{
				BC->Friends = TEXT("" + BC->Friends + "," + targetid);
			}
			BC->FriendsRes = strres;
			
			playerdb.userid = BC->Userid;
			playerdb.username = BC->PlayerName;
			playerdb.friends = BC->Friends;
			playerdb.friendsres = BC->FriendsRes;
			playerdb.friendsreq = BC->FriendsReq;
		}
		else
		{
			TArray<FString> strarrayreq;
			strarrayreq = GetSplitStringArray(playerData.friendsreq);

			FString strreq = "";
			for (int i = 0; i < strarrayreq.Num(); i++)
			{
				if (BC->Userid != strarrayreq[i])
				{
					if (strreq == "")
					{
						strreq = strarrayreq[i];
					}
					else
					{
						strreq = TEXT("" + strreq + "," + strarrayreq[i]);
					}
				}
			}
			if (playerData.friends == "")
			{
				playerData.friends = BC->Userid;
			}
			else
			{
				playerData.friends = TEXT("" + playerData.friends + "," + BC->Userid);
			}
			playerData.friendsreq = strreq;

			if(gamemode->HostObject)
			{
				AMyOnlineBeaconClient* otherbeacon = gamemode->HostObject->GetBeaconsClient(playerData.userid);
				if (otherbeacon)
				{
					//	UE_LOG(LogTemp, Warning, TEXT("otherbeacon f: %s,"),
					otherbeacon->Friends = playerData.friends;
					otherbeacon->FriendsRes = playerData.friendsres;
					otherbeacon->FriendsReq = playerData.friendsreq;

				}
			}
			f = playerData.friends;
			fres = playerData.friendsres;
			freq = playerData.friendsreq;

			playerdb.userid = playerData.userid;
			playerdb.username = playerData.username;
			playerdb.friends = playerData.friends;
			playerdb.friendsres = playerData.friendsres;
			playerdb.friendsreq = playerData.friendsreq;
		}
	
		FString JsonString;
		FJsonObjectConverter::UStructToJsonObjectString(playerdb, JsonString);
		Request->SetContentAsString(JsonString);
		Request->ProcessRequest();
	}
}

void AMyChess_GameModeBase::RejectFriends(AMyOnlineBeaconClient* BC, FString targetid, FPlayerData playerData, bool bFriend)
{
	AMyChess_GameModeBase* gamemode = this;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	FHttpRequestCompleteDelegate& Delegate = Request->OnProcessRequestComplete();
	if (Request->DoesSharedInstanceExist())
	{
		Request->SetURL("http://127.0.0.1:8080/api/PlayerDataBase");
		Request->SetVerb("Patch");
		Request->SetHeader(TEXT("Content-type"), TEXT("application/json"));

		Delegate.BindLambda([BC, targetid, playerData, gamemode, bFriend](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool Succeeded)
			{
				if (BC)
				{
					if (bFriend == false)
					{
						BC->SetPlayerData("", 0, 0, 0, "", BC->Friends, BC->FriendsRes, BC->FriendsReq, true);
						gamemode->RejectFriends(BC, BC->Userid, playerData, true);
					}
					else
					{
						if (gamemode->HostObject)
						{
							AMyOnlineBeaconClient* otherbeacon = gamemode->HostObject->GetBeaconsClient(playerData.userid);
							if (otherbeacon)
							{
								otherbeacon->SetPlayerData("", 0, 0, 0, "", otherbeacon->Friends, otherbeacon->FriendsRes, otherbeacon->FriendsReq, true);
							}
						}
					}
				}
			});

		FPlayerData playerdb;
		TArray<FString> strarrayres;
		TArray<FString> strarrayreq;
		TArray<FString> strarray;
		if (bFriend == false)
		{
			strarrayres = GetSplitStringArray(BC->FriendsRes);
			strarrayreq = GetSplitStringArray(BC->FriendsReq);
			strarray = GetSplitStringArray(BC->Friends);
		}
		else
		{
			strarrayres = GetSplitStringArray(playerData.friendsres);
			strarrayreq = GetSplitStringArray(playerData.friendsreq);
			strarray = GetSplitStringArray(playerData.friends);
		}
		FString str = "";
		FString strres = "";
		FString strreq = "";
		for (int i = 0; i < strarrayres.Num(); i++)
		{
			if (targetid != strarrayres[i])
			{
				if (strres == "")
				{
					strres = strarrayres[i];
				}
				else
				{
					strres = TEXT("" + strres + "," + strarrayres[i]);
				}
			}
		}
		for (int i = 0; i < strarrayreq.Num(); i++)
		{
			if (targetid != strarrayreq[i])
			{
				if (strreq == "")
				{
					strreq = strarrayreq[i];
				}
				else
				{
					strreq = TEXT("" + strreq + "," + strarrayreq[i]);
				}

			}
		}
		for (int i = 0; i < strarray.Num(); i++)
		{
			if (targetid != strarray[i])
			{
				if (str == "")
				{
					str = strarray[i];
				}
				else
				{
					str = TEXT("" + str + "," + strarray[i]);
				}

			}
		}

		if (bFriend == false)
		{
			BC->Friends = str;
			BC->FriendsRes = strres;
			BC->FriendsReq = strreq;

			playerdb.userid = BC->Userid;
			playerdb.username = BC->PlayerName;
			playerdb.friends = BC->Friends;
			playerdb.friendsres = BC->FriendsRes;
			playerdb.friendsreq = BC->FriendsReq;
		}
		else
		{
			playerData.friends = str;
			playerData.friendsres = strres;
			playerData.friendsreq = strreq;

			if (gamemode->HostObject)
			{
				AMyOnlineBeaconClient* otherbeacon = gamemode->HostObject->GetBeaconsClient(playerData.userid);
				if (otherbeacon)
				{
					otherbeacon->Friends = playerData.friends;
					otherbeacon->FriendsRes = playerData.friendsres;
					otherbeacon->FriendsReq = playerData.friendsreq;
				}
			}

			playerdb.userid = playerData.userid;
			playerdb.username = playerData.username;
			playerdb.friends = playerData.friends;
			playerdb.friendsres = playerData.friendsres;
			playerdb.friendsreq = playerData.friendsreq;

		}
		FString JsonString;
		FJsonObjectConverter::UStructToJsonObjectString(playerdb, JsonString);
		Request->SetContentAsString(JsonString);
		Request->ProcessRequest();
	}
}

void AMyChess_GameModeBase::ReLoginUserData(class AMyOnlineBeaconClient* BC, FString id, FString pass)
{
	AMyChess_GameModeBase* gamemode = this;
	UE_LOG(LogTemp, Warning, TEXT("ReLoginUserData %s %s %s"), *BC->InstanceUID, *BC->Userid, *BC->Userpass);
	FLoginData logindata = { BC->InstanceUID, BC->Userid, BC->Userpass };
	ReLoginBeacons.Add(logindata);
	FTimerHandle h_Login;
	FTimerDelegate LoginDelegate = FTimerDelegate::CreateLambda([gamemode, id, pass, BC]()
		{
			gamemode->CheckUserExisted(BC->Userid, BC->Userpass);
		});
	GetWorld()->GetTimerManager().SetTimer(h_Login, LoginDelegate, 10.f, false);
}

void AMyChess_GameModeBase::CheckUserExisted(FString id, FString pass)
{
	AMyChess_GameModeBase* gamemode = this;
	if (gamemode->HostObject)
	{
		AMyOnlineBeaconClient* otherbeacon = gamemode->HostObject->GetBeaconsClient(id);

		if (otherbeacon != NULL)
		{
			return;
		}
		else
		{
			for (int i = 0; i < ReLoginBeacons.Num(); i++)
			{
				if (id == ReLoginBeacons[i].userid)
				{
					gamemode->UserLogin(NULL, NULL, id, pass, 0, true);
					break;
				}
			}
		}
	}
}

TArray<FString> AMyChess_GameModeBase::GetSplitStringArray(FString LongString)
{
	TArray<FString> strarray;
	while (true)
	{
		FString substr = ",";
		FString temp;
		FString temp2;

		if (UKismetStringLibrary::Split(LongString, substr, temp, temp2, ESearchCase::IgnoreCase, ESearchDir::FromStart) == true)
		{
			strarray.Add(temp);
			LongString = temp2;
		}
		else
		{
			strarray.Add(LongString);
			break;
		}
	}
	return strarray;
}

void AMyChess_GameModeBase::GetPlayerDataInfo(class AMyOnlineBeaconClient* BC, FString url)
{
	if (BC)
	{
		UE_LOG(LogTemp, Warning, TEXT("id %s pass %s"), *BC->Userid, *BC->Userpass);
		BC->OpenLevelClient(url, BC->Userid, BC->Userpass);
		
	}
}
