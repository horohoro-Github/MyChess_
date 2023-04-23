// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MyChessPiece.h"
#include "MyTile.h"
#include "MyChessRule.h"
#include "MyCharacter.h"
#include "MyPlayerState.h"
//#include "GameFramework/Controller.h"
#include "Blueprint/UserWidget.h"
#include "MyObserver.h"
#include "MyChess_GameModeBase.h"
#include "MyGameInstance.h"
#include "MyChatObject.h"
#include "MyChatLogWidget.h"
#include "MyChatBoxWidget.h"
#include "Components/EditableTextBox.h"
#include "MyPlayerState.h"
#include "MyServerSearchWidget.h"
#include "MyServerSearchDataWidget.h"
#include "MyPlayerListWidget.h"
#include "Components/ListView.h"
#include "MyTransparencyActor.h"
//#include "GameFramework/SpringArmComponent.h"
//#include "Net/UnrealNetwork.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/LevelStreaming.h"
#include "MyLobbyWidget.h"
#include "MyMenuLoginWidget.h"
#include "Components/Border.h"
#include "Kismet/KismetStringLibrary.h"
#include "MyOnlineBeaconClient.h"
#include "MySystemMessageWidget.h"

AMyPlayerController::AMyPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UGameplayStatics::GetPlatformName() == "Android")
	{
		bAndroidController = true;
	}
	GameIns = Cast<UMyGameInstance>(GetGameInstance());

	FadeScreen(1);
}

void AMyPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (PlayerStates == NULL) PlayerStates = Cast<AMyPlayerState>(PlayerState);
	if (ChessRule == NULL) ChessRule = Cast<AMyChessRule>(UGameplayStatics::GetActorOfClass(GetWorld(), ChessRuleClass));

	if (ChessRule != NULL)
	{
		if (ChessRule->bIsMultiplayRule == false) MyTeamColor = ChessRule->TeamColor;

		if (bAndroidController == false)
		{
			if (bSpectator == false && bInteraction == true)
			{
				FVector Start, Dir, End;
				DeprojectMousePositionToWorld(Start, Dir);
				End = Start + (Dir * 8000.f);
				TraceForBlock(Start, End);
			}
		}
	}
}

void AMyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (InPawn == Observer)
	{
		if (Observer) Observer->SetCameraRotation();
	}
	
}

void AMyPlayerController::InteractionMode()
{
	if (bSpectator == false)
	{
		if (bInteraction == false)
		{
			bInteraction = true;
			bShowMouseCursor = true;
			bEnableClickEvents = true;
			bEnableTouchEvents = true;
			FInputModeGameAndUI GameUI;
			GameUI.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
			SetInputMode(GameUI);
		}
		else
		{
			if (ChessRule)
			{
				if (ChessRule->bStartGame == true)
				{
					if (ChessPiece) ChessPiece->ClickedPiece(this);
					if (ChessTile) ChessTile->ClickedTile(this);
				}
			}
		}
	}
}

void AMyPlayerController::MovableMode()
{
	if (ChessRule)
	{
		if (bSpectator == false)
		{
			if (ChessRule->bIsMultiplayRule == true)
			{
				if (PlayerStates->MyTeamColor == ChessRule->TeamColor)
				{
					ChessPiece = NULL;
					ChessTile = NULL;
					CallTileEmphasize_Client(NULL);
				}
			}
			else
			{
				ChessPiece = NULL;
				ChessTile = NULL;
				CallTileEmphasize_Client(NULL);
			}

			bInteraction = false;
			bShowMouseCursor = false;
			FInputModeGameOnly GameMode;
			SetInputMode(GameMode);
		}
	}
}

void AMyPlayerController::TraceForBlock(const FVector& Start, const FVector& End)
{
	FHitResult hitresult;
	GetWorld()->LineTraceSingleByChannel(hitresult, Start, End, ECC_Visibility);
	if (hitresult.GetActor()!=nullptr)
	{
		AMyChessPiece* chesspiece = Cast<AMyChessPiece>(hitresult.GetActor());
		AMyTile* chesstile = Cast<AMyTile>(hitresult.GetActor());
		
		if (chesspiece != ChessPiece) ChessPiece = chesspiece;
		if (chesstile != ChessTile)
		{
			ChessTile = chesstile;
			CallTileEmphasize_Client(ChessTile);
		}
	}
	else
	{
		ChessPiece = nullptr;
		ChessTile = nullptr;
	}
}

void AMyPlayerController::CallTileEmphasize_Client_Implementation(AMyTile* tiles)
{
	if (PlayerStates != NULL)
	{
		if (ChessRule->TeamColor == PlayerStates->MyTeamColor && ChessRule->bGameOvered == false)
		{
			for (int i = 0; i < 64; i++)
			{
				if (ChessRule->TileArray[i]->TileState != 0)
				{
					if (ChessRule->TileArray[i]->TileState >= 4)
					{
						ChessRule->TileArray[i]->ChangeMaterial(4);
					}
					else
					{
						ChessRule->TileArray[i]->ChangeMaterial(ChessRule->TileArray[i]->TileState);
					}
				}
			}
			if (tiles != NULL)
			{
				if (tiles->TileState != 0)
				{
					tiles->ChangeMaterialToEmphasize(tiles->TileState);
				}
			}
		}
	}
}
void AMyPlayerController::GetPiece(AMyChessPiece* piece, bool bReset)
{
	if (ChessRule)
	{
		if (MyTeamColor == ChessRule->TeamColor && ChessRule->bGameOvered == false)
		{
			GetPieces_Client(piece, bReset);
		}
	}
}

void AMyPlayerController::GetPieces_Client_Implementation(AMyChessPiece* piece, bool bReset)
{
	if (ChessRule)
	{
		if (bReset == false)
		{
			if (SelectedPiece == NULL)
			{
				SelectedPiece = Cast<AMyChessPiece>(piece);
				SelectedPiece->Highlight(true);
				ChessRule->TileArray[SelectedPiece->CurrentPosition]->TileTeamColor = 2;

				for (int i = 0; i < 64; i++)
				{
					ChessRule->CheckMateAndMovement(SelectedPiece, ChessRule->TileArray[i], ChessRule->TileArray[i]->TileNumber, true);
				}

			}
			else
			{
				ChessRule->TileArray[SelectedPiece->CurrentPosition]->TileTeamColor = SelectedPiece->TeamColor;
				SelectedPiece->Highlight(false);
				SelectedPiece = NULL;
				for (int i = 0; i < 64; i++)
				{
					ChessRule->TileArray[i]->ChangeMaterial(0);
					ChessRule->TileArray[i]->TileState = 0;
				}
			}
		}
		else
		{
			if (SelectedPiece != NULL)
			{
				ChessRule->TileArray[SelectedPiece->CurrentPosition]->TileTeamColor = SelectedPiece->TeamColor;
				SelectedPiece->Highlight(false);
				SelectedPiece = NULL;
				for (int i = 0; i < 64; i++)
				{
					ChessRule->TileArray[i]->ChangeMaterial(0);
					ChessRule->TileArray[i]->TileState = 0;
				}
			}
		}
	}
}

void AMyPlayerController::GetTile(AMyTile* tiles)
{
	if (ChessRule)
	{
		if (MyTeamColor == ChessRule->TeamColor && ChessRule->bGameOvered == false)
		{
			GetTiles_Client(tiles);
		}
	}
}

void AMyPlayerController::GetTiles_Client_Implementation(AMyTile* tiles)
{
	if (SelectedPiece != NULL)
	{
		SelectedPiece->Highlight(false);
		SelectedTiles = Cast<AMyTile>(tiles);
		if (SelectedTiles != NULL)
		{
			//캐슬링
			if (SelectedTiles->TileState >= 4)
			{
				switch (SelectedTiles->TileState)
				{
				case 4:
				{
					ChessRule->PieceArray[0]->bFirstAction = false;
					ChessRule->TileArray[ChessRule->PieceArray[0]->CurrentPosition]->TileTeamColor = 2;
					ChessRule->TileArray[0]->CurrentPiece = NULL;
					ChessRule->TileArray[3]->CurrentPiece = ChessRule->PieceArray[0];
					ChessRule->PieceArray[0]->MoveLocation(ChessRule->tempTileQ->GetActorLocation() + FVector(0.f, 0.f, 50.f));
					ChessRule->PieceArray[0]->CurrentPosition = ChessRule->tempTileQ->TileNumber;
					ChessRule->TileArray[3]->TileTeamColor = SelectedPiece->TeamColor;
					break;
				}
				case 5:
				{
					ChessRule->PieceArray[7]->bFirstAction = false;
					ChessRule->TileArray[ChessRule->PieceArray[7]->CurrentPosition]->TileTeamColor = 2;
					ChessRule->TileArray[7]->CurrentPiece = NULL;
					ChessRule->TileArray[5]->CurrentPiece = ChessRule->PieceArray[7];
					ChessRule->PieceArray[7]->MoveLocation(ChessRule->tempTileK->GetActorLocation() + FVector(0.f, 0.f, 50.f));
					ChessRule->PieceArray[7]->CurrentPosition = ChessRule->tempTileK->TileNumber;
					ChessRule->TileArray[5]->TileTeamColor = SelectedPiece->TeamColor;
					break;
				}
				case 6:
				{
					ChessRule->PieceArray[24]->bFirstAction = false;
					ChessRule->TileArray[ChessRule->PieceArray[24]->CurrentPosition]->TileTeamColor = 2;
					ChessRule->TileArray[56]->CurrentPiece = NULL;
					ChessRule->TileArray[59]->CurrentPiece = ChessRule->PieceArray[24];
					ChessRule->PieceArray[24]->MoveLocation(ChessRule->tempTileQ->GetActorLocation() + FVector(0.f, 0.f, 50.f));
					ChessRule->PieceArray[24]->CurrentPosition = ChessRule->tempTileQ->TileNumber;
					ChessRule->TileArray[59]->TileTeamColor = SelectedPiece->TeamColor;
					break;
				}
				case 7:
				{
					ChessRule->PieceArray[31]->bFirstAction = false;
					ChessRule->TileArray[ChessRule->PieceArray[31]->CurrentPosition]->TileTeamColor = 2;
					ChessRule->TileArray[63]->CurrentPiece = NULL;
					ChessRule->TileArray[61]->CurrentPiece = ChessRule->PieceArray[31];
					ChessRule->PieceArray[31]->MoveLocation(ChessRule->tempTileK->GetActorLocation() + FVector(0.f, 0.f, 50.f));
					ChessRule->PieceArray[31]->CurrentPosition = ChessRule->tempTileK->TileNumber;
					ChessRule->TileArray[61]->TileTeamColor = SelectedPiece->TeamColor;
					break;
				}
				}
				SelectedPiece->bFirstAction = false;
				ChessRule->TileArray[SelectedPiece->CurrentPosition]->TileTeamColor = 2;
				ChessRule->TileArray[SelectedPiece->CurrentPosition]->CurrentPiece = NULL;
				SelectedPiece->MoveLocation(SelectedTiles->GetActorLocation() + FVector(0.f, 0.f, 50.f));
				SelectedPiece->CurrentPosition = SelectedTiles->TileNumber;
				SelectedTiles->CurrentPiece = SelectedPiece;
				ChessRule->TileArray[SelectedTiles->TileNumber]->TileTeamColor = SelectedPiece->TeamColor;
				ChessRule->ThreefoldRepetition(SelectedPiece, false);
				ChessRule->RefreshPieces(SelectedPiece, true, false);
			} // 
			else if (SelectedTiles->TileState == 2 || SelectedTiles->TileState == 1)
			{
				if (SelectedPiece->ChessType == 1)	//앙파상
				{
					if (SelectedPiece->CurrentPosition + 16 == SelectedTiles->TileNumber)
					{
						ChessRule->bEnpassantW[SelectedPiece->CurrentPosition + 8] = 1;
						ChessRule->tempPiece = SelectedPiece;
					}
					else if (SelectedPiece->CurrentPosition - 16 == SelectedTiles->TileNumber)
					{
						ChessRule->bEnpassantB[SelectedPiece->CurrentPosition - 8] = 1;
						ChessRule->tempPiece = SelectedPiece;
					}
					if (SelectedPiece->TeamColor == 0)
					{
						if (ChessRule->bEnpassantB[SelectedTiles->TileNumber] == 1)
						{
							ChessRule->TileArray[ChessRule->tempPiece->CurrentPosition]->TileTeamColor = 2;
							ChessRule->TileArray[ChessRule->tempPiece->CurrentPosition]->CurrentPiece = NULL;
							ChessRule->Capture(ChessRule->tempPiece);
						}
					}
					else
					{
						if (ChessRule->bEnpassantW[SelectedTiles->TileNumber] == 1)
						{
							ChessRule->TileArray[ChessRule->tempPiece->CurrentPosition]->TileTeamColor = 2;
							ChessRule->TileArray[ChessRule->tempPiece->CurrentPosition]->CurrentPiece = NULL;
							ChessRule->Capture(ChessRule->tempPiece);
						}
					}
					//
				}
				//앙파상 혹은 그 외
				int templocation = SelectedPiece->CurrentPosition;
				SelectedPiece->bFirstAction = false;
				ChessRule->TileArray[SelectedPiece->CurrentPosition]->TileTeamColor = 2;
				ChessRule->TileArray[SelectedPiece->CurrentPosition]->CurrentPiece = NULL;
				SelectedPiece->MoveLocation(SelectedTiles->GetActorLocation() + FVector(0.f, 0.f, 50.f));
				SelectedPiece->CurrentPosition = SelectedTiles->TileNumber;
				if (SelectedTiles->CurrentPiece != NULL)
				{
					ChessRule->Capture(SelectedTiles->CurrentPiece);
					ChessRule->ThreefoldRepetition(SelectedPiece, true);
					ChessRule->FiftyMoveRule = ChessRule->Turn[0] + ChessRule->Turn[1] + 50;
				}
				else
				{
					ChessRule->ThreefoldRepetition(SelectedPiece, false);
					if (SelectedPiece->ChessType == 1)
					{
						ChessRule->FiftyMoveRule = ChessRule->Turn[0] + ChessRule->Turn[1] + 50;
					}
				}
				SelectedTiles->CurrentPiece = SelectedPiece;
				ChessRule->TileArray[SelectedTiles->TileNumber]->TileTeamColor = SelectedPiece->TeamColor;
				if (SelectedPiece->ChessType == 1)	//프로모션
				{
					ChessRule->EnPassant();
					if (SelectedPiece->TeamColor == 0)
					{
						if (SelectedTiles->EndLine == 1)
						{
							ChessRule->tempPiece = SelectedPiece;
							PlayerStates->Promotion_Client(true);
						}
					}
					else
					{
						if (SelectedTiles->EndLine == 0)
						{
							ChessRule->tempPiece = SelectedPiece;
							PlayerStates->Promotion_Client(true);
						}
					}
				}
				ChessRule->RefreshPieces(SelectedPiece, false, false);
			}
			else
			{
				ChessRule->TileArray[SelectedPiece->CurrentPosition]->TileTeamColor = SelectedPiece->TeamColor;
			}
			SelectedPiece = NULL;
			for (int i = 0; i < 64; i++)
			{
				ChessRule->TileArray[i]->ColorIndex = 0;
				ChessRule->TileArray[i]->ChangeMaterial(0);
				ChessRule->TileArray[i]->TileState = 0;
			}
		}
	}
}

void AMyPlayerController::PossessActor_Implementation(bool bPlayer)
{
	if (bPlayer == true)
	{
		if (bSpectator == false)
		{
			UnPossess();
			if (ChessRule->bIsMultiplayRule == true)
			{
				if (PlayerStates->MyTeamColor == 0)
				{
					Observer->bWhite = true;
				}
				else
				{
					Observer->bWhite = false;
				}
			}
			Possess(Observer);
		}
		else
		{
			FActorSpawnParameters spawnparam;
			spawnparam.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Observer = Cast<AMyObserver>(GetWorld()->SpawnActor<AActor>(ObserverClass, FVector(-350.f, 350.f, 350.f), FRotator(0.f, 0.f, 0.f), spawnparam));
			Observer->SetActorRotation(FRotator(0.f, 90.f, 0.f));
			Observer->Player = MyPlayer;
			Possess(Observer);
		}
	}
	else
	{
		UnPossess();
		Possess(MyPlayer);
	}
}

void AMyPlayerController::ResetPossessActor_Implementation()
{
	UnPossess();
	Possess(MyPlayer);
}

void AMyPlayerController::CreateLoadingWidget()
{
	WidgetState = EWidgetState::LOADING;
	ChangeWidget();
}

void AMyPlayerController::ChangeWidget_Implementation()
{
	if (CurrentWidget != NULL) CurrentWidget->RemoveFromParent();
	FInputModeGameAndUI GameUIMode;
	EMouseLockMode lock = EMouseLockMode::LockAlways;
	GameUIMode.SetLockMouseToViewportBehavior(lock);

	switch (WidgetState)
	{
	case EWidgetState::NONEWIDGET:
	{
		break;
	}
	case EWidgetState::MENU:
	{
		if (UGameplayStatics::GetPlatformName() == "Windows")
		{
			ApplyWidget(MenuWidgetClass, true);
			SetInputMode(GameUIMode);
		}
		else if (UGameplayStatics::GetPlatformName() == "Android")
		{
			ApplyWidget(MenuWidgetClass_M, true);
			SetInputMode(GameUIMode);
			ActivateTouchInterface(MenuInterface);
		}
		break;
	}
	case EWidgetState::LOBBY:
	{
		if (UGameplayStatics::GetPlatformName() == "Windows")
		{
			ApplyWidget(LobbyWidgetClass, true);
			SetInputMode(GameUIMode);
		}
		else if (UGameplayStatics::GetPlatformName() == "Android")
		{
			ApplyWidget(LobbyWidgetClass_M, true);
			SetInputMode(GameUIMode);
			ActivateTouchInterface(LobbyInterface);
		}
		break;
	}
	case EWidgetState::INGAME:
	{
		if (UGameplayStatics::GetPlatformName() == "Windows")
		{
			ApplyWidget(IngameWidgetClass, false);
			FInputModeGameOnly gameonly;
			gameonly.SetConsumeCaptureMouseDown(true);
			SetInputMode(gameonly);
		}
		else if (UGameplayStatics::GetPlatformName() == "Android")
		{
			ApplyWidget(IngameWidgetClass_M, false);
			FInputModeGameOnly gameonly;
			gameonly.SetConsumeCaptureMouseDown(true);
			SetInputMode(gameonly);
			ActivateTouchInterface(IngameInterface);
		}
		break;
	}
	case EWidgetState::Spectator:
	{
		if (UGameplayStatics::GetPlatformName() == "Windows")
		{
			ApplyWidget(SpectatorWidgetClass, true);
			SetInputMode(GameUIMode);
		}
		else if (UGameplayStatics::GetPlatformName() == "Android")
		{
			ApplyWidget(SpectatorWidgetClass_M, true);
			SetInputMode(GameUIMode);

			ActivateTouchInterface(SpectatorInterface);
		}
		break;
	}
	case EWidgetState::LOADING:
	{
		ApplyWidget(LoadingWidgetClass, true);
		FInputModeUIOnly LOADINGMode;
		LOADINGMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		SetInputMode(LOADINGMode);

		break;
	}
	}
}

void AMyPlayerController::ApplyWidget(TSubclassOf<class UUserWidget> Widget, bool bShowMouseCursors)
{
	if (Widget != nullptr)
	{
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), Widget);
		bShowMouseCursor = bShowMouseCursors;
		CurrentWidget->AddToViewport();
		
		if (UGameplayStatics::GetCurrentLevelName(GetWorld(), true) == FString("MainMap"))
		{
			if (chatlogWidget == NULL)
			{
				if (UGameplayStatics::GetPlatformName() != "Android")
				{
					chatlogWidget = CreateWidget<UMyChatLogWidget>(GetWorld(), ChatLogWidgetClass);
					chatlogWidget->AddToViewport();
				}
			}
		}

		SystemMessageWidget = CreateWidget<UMySystemMessageWidget>(GetWorld(), SystemMessageClass);
		SystemMessageWidget->AddToViewport(1);
	}
}

void AMyPlayerController::Chatting(UUserWidget* chattingwidget)
{
	if (chatlogWidget != NULL)
	{
		if (chatWidget == NULL)
		{
			chatWidget = chattingwidget;
		}
		UMyChatBoxWidget* chatbox = Cast<UMyChatBoxWidget>(chatWidget);
		if (chatbox->GetVisibility() == ESlateVisibility::Hidden)
		{
			chatbox->SetVisibility(ESlateVisibility::Visible);
			chatbox->chatBox->SetKeyboardFocus();
		}
		else
		{
			if (chatbox->chatBox->GetText().ToString() != FString(""))
			{
				chatbox->OnChatCommitted(chatbox->chatBox->GetText());
			}
			else
			{
				chatbox->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void AMyPlayerController::SendChat_Implementation(const FString& message, const FString& name, const FString& playerpid, bool bServer, bool bMaster)
{
	AMyChess_GameModeBase* GameModeBase = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (PlayerStates == NULL) PlayerStates = Cast<AMyPlayerState>(PlayerState);
	if (bServer == false)
	{
		for (int i = 0; i < GameModeBase->ConnectedControllers.Num(); i++)
		{
			AMyPlayerController* PC = Cast<AMyPlayerController>(GameModeBase->ConnectedControllers[i]);
			if (PC)
			{
				PC->UpdateChat(message, name, PlayerStates->MyPID, PlayerStates->bSpectator, bServer, bMaster);
			}
		}
	}
	else
	{
		for (int i = 0; i < GameModeBase->ConnectedControllers.Num(); i++)
		{
			AMyPlayerController* PC = Cast<AMyPlayerController>(GameModeBase->ConnectedControllers[i]);
			if (PC)
			{
				if (PC != this)PC->UpdateChat(message, name, PlayerStates->MyPID, PlayerStates->bSpectator, bServer, bMaster);
			}
		}
	}
}

void AMyPlayerController::UpdateChat_Implementation(const FString& message, const FString& name, const FString& playerpid, bool bSpectators, bool bServer, bool bMaster)
{
	if(chatlogWidget)
	{
		FChatData chat = { message, name, playerpid, bSpectators, bServer, bMaster };
		chatlogWidget->UpdateLog(chat);
	}
}

void AMyPlayerController::PlayerList(bool bOpen)
{
		if (ListWidget == NULL)
		{
			ListWidget = CreateWidget<UUserWidget>(GetWorld(), PlayerListWidgetClass);
			ListWidget->AddToViewport();
			UMyPlayerListWidget* Lists = Cast<UMyPlayerListWidget>(ListWidget);
			TArray<FPlayerListsd> BlankList;
			if (Lists) Lists->UpdateLists(false,  BlankList);
		}
		else
		{
			ListWidget->RemoveFromParent();
			ListWidget = NULL;
		}
}

void AMyPlayerController::SendList_Implementation(bool bSendFromServer)
{
	AMyChess_GameModeBase* GameModeBase = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	TArray<FPlayerListsd> Listarray;
	FPlayerListsd TempList;
	if (GameModeBase)
	{
		for (int i = 0; i < GameModeBase->PlayerList.Num(); i++)
		{	
			if (GameModeBase->PlayerList[i].id != "" && GameModeBase->PlayerList[i].id != "0" && GameModeBase->PlayerList[i].id != "-1")
			{ 
				TempList.id = GameModeBase->PlayerList[i].id;
				TempList.PlayerName = GameModeBase->PlayerList[i].PlayerName;
				TempList.Win = GameModeBase->PlayerList[i].Win;
				TempList.Draw = GameModeBase->PlayerList[i].Draw;
				TempList.Lose = GameModeBase->PlayerList[i].Lose;
				TempList.MasterPlayer = GameModeBase->PlayerList[i].MasterPlayer;
				TempList.Spectator = GameModeBase->PlayerList[i].Spectator;

				Listarray.Add(TempList);
			}
		}
	}
}

void AMyPlayerController::UpdateList_Implementation(const TArray<FPlayerListsd>& playerlists)
{
	UMyPlayerListWidget* Lists = Cast<UMyPlayerListWidget>(ListWidget);
	//if (Lists) Lists->UpdateLists(true, playerlists);
	if (Lists) Lists->UpdateLists(true, playerlists);
}

void AMyPlayerController::AddList_Implementation(FPlayerListsd playerlists)
{
	UMyPlayerListWidget* Lists = Cast<UMyPlayerListWidget>(ListWidget);
	if (Lists) Lists->AddLists(playerlists);
}

void AMyPlayerController::SetSpectator_Implementation(bool bSpectators)
{
	bSpectator = bSpectators;
}

void AMyPlayerController::SetReadyButtons_Implementation(bool bReady)
{
	bReadyButton = bReady;
}

void AMyPlayerController::SetReadyButton_Implementation(bool bReady)
{
	bReadyButton = bReady;
}

void AMyPlayerController::HiddenCeiling_Implementation()
{
	UMyGameInstance* gameins = Cast<UMyGameInstance>(GetGameInstance());
	if (gameins)
	{
		if (gameins->wall != NULL) gameins->wall->SetActorHiddenInGame(false);
	}
}

void AMyPlayerController::FadeScreen_Implementation(int FadeState) // 0: 초기 설정, 1: 원래 화면, 2: 검은 화면  
{
	APlayerCameraManager* cameramanage = Cast<APlayerCameraManager>(UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0));
	if (cameramanage)
	{
		if (FadeState == 0)
		{
			cameramanage->SetManualCameraFade(1.f, FLinearColor::Black, false);
		}
		if (FadeState == 1)
		{
			bWidgetVisibility = true;
			cameramanage->StartCameraFade(1.f, 0.f, 0.5f, FLinearColor::Black, false, true);
			if (UGameplayStatics::GetStreamingLevel(GetWorld(), FName("LoadingMap"))->IsLevelLoaded() == true)
			{
				UGameplayStatics::UnloadStreamLevel(GetWorld(), FName("LoadingMap"), latentinfo, false);
			}
		}
		if (FadeState == 2)
		{
			bWidgetVisibility = false;
			cameramanage->StartCameraFade(0.f, 1.f, 0.5f, FLinearColor::Black, false,true);
			UMyGameInstance* gameins = Cast<UMyGameInstance>(GetGameInstance());
			GetWorld()->GetTimerManager().SetTimer(gameins->h_timer, FTimerDelegate::CreateLambda([this]()
				{
					UGameplayStatics::LoadStreamLevel(this, FName("LoadingMap"), true,false, latentinfo);
				}), 0.5f, false);
				
		}
	}
}

void AMyPlayerController::GetPlayers_Implementation(AMyPlayerState* playerstate0, AMyPlayerState* playerstate1, bool bReset)
{
	if (bSpectator == false)
	{
		if (playerstate0 == NULL && playerstate1 == NULL)
		{
			PS0 = NULL;
			PS1 = NULL;
		}
		else if (playerstate0 == NULL)
		{
			AMyPlayerState* MyPlayerState = Cast<AMyPlayerState>(PlayerState);
			PS0 = MyPlayerState;
			PS1 = playerstate1;
		}
		else
		{
			if (playerstate0 != NULL && playerstate1 != NULL) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("GetPlayers %s %s"), *playerstate0->NickName, *playerstate1->NickName));
			PS0 = playerstate0;
			PS1 = playerstate1;
		}
	}
	else
	{
		if (bReset == false)
		{
			if (playerstate0 != NULL && playerstate1 != NULL)
			{
				if (playerstate0 != NULL && playerstate1 != NULL) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("GetPlayers %s %s"), *playerstate0->NickName, *playerstate1->NickName));
				PS0 = playerstate0;
				PS1 = playerstate1;
			}
			else
			{
				FTimerHandle h_GetPlayers;
				FTimerDelegate GetPlayersDelegate = FTimerDelegate::CreateLambda([&]()
					{
						GerPlayersFromServer();
					});
				GetWorld()->GetTimerManager().SetTimer(h_GetPlayers, GetPlayersDelegate, 0.2f, false);
			}
		}
		else
		{
			PS0 = NULL;
			PS1 = NULL;
		}
	}
}

void AMyPlayerController::GerPlayersFromServer_Implementation()
{
	AMyChess_GameModeBase* gamemode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (gamemode)
	{
		if (gamemode->ChessRule)
		{
			if (gamemode->ChessRule->bStartGame == true)
			{
				if (gamemode->ConnectedPlayers.Num() == 2)
				{
					AMyPlayerState* PS00 = Cast<AMyPlayerState>(gamemode->ConnectedPlayers[0]->PlayerState);
					AMyPlayerState* PS01 = Cast<AMyPlayerState>(gamemode->ConnectedPlayers[1]->PlayerState);
					GetPlayers(PS00, PS01, false);
				}
			}
		}
	}
}

void AMyPlayerController::PlayerControllerSetup_Implementation(bool Spec, bool bIsLobbyLevel)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (bIsLobbyLevel == false)
	{
		if (AMyChess_GameModeBase* GameMode = GetWorld()->GetAuthGameMode<AMyChess_GameModeBase>())
		{
			FVector SpawnLoc = FVector(-1400.f, 300.f, 100.f);
			SpawnLocation.Z = 100.f;

			if (Spec == false)
			{
				if (MyPlayer == NULL)
				{
					if (APawn* PlayerPawn = GetWorld()->SpawnActor<APawn>(GameMode->CharacterClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams))
					{
						MyPlayer = Cast<AMyCharacter>(PlayerPawn);
						SetSpectator(Spec);
						Possess(MyPlayer);
					}
				}
				if (APawn* Observ = GetWorld()->SpawnActor<APawn>(GameMode->ObserverClass, FVector(-350.f, 350.f, 350.f), FRotator(0.f, 90.f, 0.f), SpawnParams))
				{
					Observer = Cast<AMyObserver>(Observ);
					Observer->Player = MyPlayer;

				}
			}
			else
			{
				if (APawn* Observ = GetWorld()->SpawnActor<APawn>(GameMode->ObserverClass, FVector(-350.f, 350.f, 350.f), FRotator(0.f, 90.f, 0.f), SpawnParams))
				{
					Observer = Cast<AMyObserver>(Observ);
					Observer->SetActorRelativeRotation(FRotator(0.f, 90.f, 0.f));
					Observer->TopView = true;
					SetSpectator(Spec);
					Possess(Observer);
				}
			}
		}
	}
	else
	{
		if (APawn* nonec = GetWorld()->SpawnActor<APawn>(APawn::StaticClass(), FVector(0.f, 0.f, 0.f), FRotator(0.f, 0.f, 0.f), SpawnParams))
		{
			Possess(nonec);
		}
	}
}

void AMyPlayerController::GetServerNameCli_Implementation(const FString& NewName)
{
	OldServerName = NewName;
	if (ListWidget!=NULL)
	{
		UMyPlayerListWidget* listw = Cast<UMyPlayerListWidget>(ListWidget);
		listw->OldServerName = NewName;
		listw->CurrentServerName = NewName;
	}
}

void AMyPlayerController::ClearWidget()
{
	if (CurrentWidget != NULL)
	{
		WidgetState = EWidgetState::NONEWIDGET;
		CurrentWidget->RemoveFromParent();
		CurrentWidget = NULL;
	}
}

void AMyPlayerController::KickPlayer_Implementation(bool bMainServer)
{
	UMyGameInstance* gameins = Cast<UMyGameInstance>(GetGameInstance());
	if (gameins)
	{
		gameins->ReLogin("LobbyLevel");
	}
}

void AMyPlayerController::MessageDeliv_Implementation(const FString& m)
{
	GameIns = Cast<UMyGameInstance>(GetGameInstance());
	if (GameIns)
	{
		GameIns->MessageDelivary(m);
	}
}

void AMyPlayerController::LobbyWidgetChangeIndex(int index)
{
	if (CurrentWidget != NULL)
	{
		UMyLobbyWidget* lobby = Cast<UMyLobbyWidget>(CurrentWidget);
		if (lobby)
		{
			lobby->WidgetSwitcher->SetActiveWidgetIndex(index);
			FString temp = "";
			lobby->EditableTextBox_Name->SetText(FText::FromString(temp));
		}
	}
}

void AMyPlayerController::LoginWidgetChangeIndex(int index, FString tempID)
{
	if (CurrentWidget != NULL)
	{
		UMyMenuLoginWidget* menu = Cast<UMyMenuLoginWidget>(CurrentWidget);
		menu->TempID = tempID;
		menu->WidgetSwitcher->SetActiveWidgetIndex(index);
	}
}

void AMyPlayerController::GetPlayerDataFromServer_Implementation(class AMyPlayerState* PS, const FString& id, const FString& pass)
{
	AMyChess_GameModeBase* gamemode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (gamemode)
	{	
		AMyPlayerState* MyPS = Cast<AMyPlayerState>(PlayerState);
		gamemode->GetPlayerDataForClient(MyPS, NULL, id, 0, "");
	}
}

void AMyPlayerController::CheckingValidate_Implementation(const FString& Options)
{
	GameIns = Cast<UMyGameInstance>(GetGameInstance());
	if (GameIns)
	{

		TArray<FString> strarray;

		FString SourceStr = Options;
		FString substr = "?Id=";
		FString substr2 = "?Pass=";
		FString temp;
		FString temp2;
		FString id = "";
		FString pass = "";

		UKismetStringLibrary::Split(SourceStr, substr, temp, temp2, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		SourceStr = temp2;
		UKismetStringLibrary::Split(SourceStr, "?Pass=", temp, temp2, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		id = temp;
		SourceStr = temp2;
		UKismetStringLibrary::Split(SourceStr, "?", temp, temp2, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		pass = temp;
		if (GameIns->UserId != "")
		{
			if (id != "" && pass != "")
			{
				if ((GameIns->UserId != id || GameIns->UserPass != pass))
				{
					GameIns->UserId = "";
					GameIns->UserPass = "";
					GameIns->bTraveling = false;
					GameIns->OpenLevel("LobbyLevel", "");
				}
			}
			else
			{
				GameIns->UserId = "";
				GameIns->UserPass = "";
				GameIns->bTraveling = false;
				GameIns->OpenLevel("LobbyLevel", "");
			}
		}
	}
}