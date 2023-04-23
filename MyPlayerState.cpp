// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "MyChessRule.h"
#include "MyTile.h"
#include "MyChessPiece.h"
#include "MyChess_GameModeBase.h"
#include "MyPlayerController.h"
#include "MyGameInstance.h"
#include "MyOnlineBeaconClient.h"
#include "MyPlayerListObject.h"
#include "MyGameStateBase.h"
#include "Kismet/KismetStringLibrary.h"

AMyPlayerState::AMyPlayerState()
{
	PrimaryActorTick.bCanEverTick = true;
//	Http = &FHttpModule::Get();
}

void AMyPlayerState::BeginPlay()
{
	Super::BeginPlay();
	Gameins = Cast<UMyGameInstance>(GetGameInstance());
	PlayerControllers = Cast<AMyPlayerController>(GetPlayerController());
}

void AMyPlayerState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ChessRule == NULL) ChessRule = Cast<AMyChessRule>(UGameplayStatics::GetActorOfClass(GetWorld(), ChessRuleClass));

	if (ChessRule != NULL)
	{
		if (ChessRule->bIsMultiplayRule == false) MyTeamColor = ChessRule->TeamColor;
	}
}

void AMyPlayerState::ChessPromotion(int index)
{
	if (ChessRule)
	{
		AMyChessPiece* temp = ChessRule->PieceArray[tempPiece->CurrentPosition];
		temp->MeshComp->SetStaticMesh(ChessRule->Meshes[index]);
	}
}

void AMyPlayerState::ChessPromotion_Client_Implementation(class AMyChessRule* Rule, int index)
{
	Rule->ChessPromotion(index);
	Promotion_Client(false);
}

void AMyPlayerState::Promotion_Client_Implementation(bool bPromotions)
{
	if (ChessRule)
	{
		TArray<AActor*> actors;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("State"), actors);
		for (int i = 0; i < actors.Num(); i++)
		{
			AMyPlayerState* state = Cast<AMyPlayerState>(actors[i]);
			state->bPromotion = bPromotions;
		}
		ChessRule->bPromotion = bPromotions;
	}
}

void AMyPlayerState::StartGame_Client_Implementation(AMyChessRule* Rule, bool bReady)
{
	AMyChess_GameModeBase* GameModeBase = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GameModeBase)
	{
		if (bReady == true)
		{
			GameModeBase->ReadyPlayerForPlay++;
			if (GameModeBase->ReadyPlayerForPlay == 1)
			{
				for (int i = 0; i < 2; i++)
				{
					if (GameModeBase->ConnectedPlayers[i] != PlayerControllers && GameModeBase->ConnectedPlayers[i] != NULL)
					{
						FString msg = TEXT("" + NickName + "is ready. You must be ready within 30 seconds.");
						GameModeBase->ConnectedControllers[i]->MessageDeliv(msg);
					}
					else if (GameModeBase->ConnectedPlayers[i] == NULL)
					{
						GameModeBase->ReadyPlayerForPlay--;
						GameModeBase->ReadyButtonOff(NULL);
						break;
					}
				}
			}
			if (GameModeBase->ReadyPlayerForPlay == 2)
			{
				GameModeBase->ReadyButtonOff(NULL);
				Rule->bCanStartGame = true;
			}
		}
		else
		{
			GameModeBase->ReadyPlayerForPlay--;

			for (int i = 0; i < 2; i++)
			{
				if (GameModeBase->ConnectedPlayers[i] == NULL)
				{
					GameModeBase->ReadyPlayerForPlay = 0;
					break;
				}
			}
		}

		GameModeBase->ReadyTimer = 30.f;
	}
}

void AMyPlayerState::SurrenderGame_Client_Implementation(class AMyChessRule* Rule, int teamcolor)
{
	if (bSpectator == false)
	{
		if (Rule->bGameOvered == false)
		{
			Rule->bGameOvered = true;

			if (teamcolor == 0) Rule->GameEndingText = "White surrender"; else Rule->GameEndingText = "Black surrender";

			AMyChess_GameModeBase* GameMode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
			if (GameMode) GameMode->GameResult(teamcolor, Rule->bPromotion);
		}
	}
}

void AMyPlayerState::ServerMessage_Server_Implementation(class AMyPlayerController* PC,const FString& message)
{
	UMyGameInstance* GameIns = Cast<UMyGameInstance>(GetGameInstance());
	PC->SendChat(message, GetPlayerName(), MyPID, true, false);
}

void AMyPlayerState::ApplyPlayerListCli_Implementation()
{
	if (!HasAuthority()) ApplyPlayerList();
}

void AMyPlayerState::ApplyPlayerList_Implementation()
{
	if (PlayerControllers) PlayerControllers->SendList(false);
}

void AMyPlayerState::ChangeMasterPlayer_Implementation(const FString& Originpid, const FString& Targetpid)
{
	AMyChess_GameModeBase* gamemode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (gamemode) gamemode->ChangeMasterPlayer(Originpid, Targetpid);
}

void AMyPlayerState::SetMasterPlayer_Implementation(bool btrue)
{
	bMasterPlayer = btrue;
}

void AMyPlayerState::SetWhitePlayer_Implementation()
{
	MyTeamColor = 0;
}

void AMyPlayerState::SetMasterPlayerServ_Implementation(bool btrue)
{
	bMasterPlayer = btrue;
}

void AMyPlayerState::SetSpectatorPlayerState_Implementation(bool spec)
{
	bSpectator = spec;
}

void AMyPlayerState::UpdatePlayInfo_Implementation(int res)
{
	if (res == 0) PlayerLose++; else if (res == 1) PlayerWin++; else PlayerDraw++;
}

void AMyPlayerState::UpdatePlayInfoCli_Implementation(int res)
{
	if (res == 0) PlayerLose++; else if (res == 1) PlayerWin++; else PlayerDraw++;
}

void AMyPlayerState::ChangeServerName_Implementation(const FString& NewName)
{
	AMyChess_GameModeBase* GameModeBase = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GameModeBase) GameModeBase->ServerUpdate_ChangeServerName(NewName);
}

void AMyPlayerState::SetPlayerData_Implementation(const FString& name, int win, int draw, int lose, const FString& id, int IMasterPlayer, int ISpectator)
{
	NickName = name;
	PlayerWin = win;
	PlayerDraw = draw;
	PlayerLose = lose;
	MyPID = id;

	if (IMasterPlayer == 1)
	{
		bMasterPlayer = true;
	}
	else if (IMasterPlayer == 0)
	{
		bMasterPlayer = false;
	}

	if (ISpectator == 1)
	{
		bSpectator = true;
	}
	else if (ISpectator == 0)
	{
		bSpectator = false;
	}

	Gameins = Cast<UMyGameInstance>(GetGameInstance());
	if (Gameins)
	{
		Gameins->PlayerWin = win;
		Gameins->PlayerDraw = draw;
		Gameins->PlayerLose = lose;
	}
}

void AMyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AMyPlayerState, bPromotion);
	DOREPLIFETIME(AMyPlayerState, bSpectator);
	DOREPLIFETIME(AMyPlayerState, MyPID);
	DOREPLIFETIME(AMyPlayerState, PlayerWin);
	DOREPLIFETIME(AMyPlayerState, PlayerLose);
	DOREPLIFETIME(AMyPlayerState, PlayerDraw);
	DOREPLIFETIME(AMyPlayerState, MyTeamColor);
	DOREPLIFETIME(AMyPlayerState, NickName);
	DOREPLIFETIME(AMyPlayerState, bMasterPlayer);
}