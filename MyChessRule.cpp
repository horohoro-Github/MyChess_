// Fill out your copyright notice in the Description page of Project Settings.


#include "MyChessRule.h"
#include "MyTile.h"
#include "Kismet/GameplayStatics.h"
#include "MyReferee.h"
#include "Net/UnrealNetwork.h"
#include "MyChess_GameModeBase.h"
#include "MyPlayerController.h"
#include "MyPlayerState.h"
// Sets default values
AMyChessRule::AMyChessRule()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Turn.SetNum(2);
	Meshes.SetNum(8);
	TimeLimit.SetNum(2);
	TileArray.SetNum(64);
	PieceArray.SetNum(32);
	DeadZoneArray.SetNum(32);
	bEnpassantB.SetNum(64);
	bEnpassantW.SetNum(64);
	ThreefoldRepetitionCountB.SetNum(25);
	ThreefoldRepetitionCountW.SetNum(25);
	ThreefoldRepetitionTableW.SetNum(25);
	ThreefoldRepetitionTableB.SetNum(25);
}

// Called when the game starts or when spawned
void AMyChessRule::BeginPlay()
{
	Super::BeginPlay();
	
	TimeLimit[0] = 600.f;
	TimeLimit[1] = 600.f;

	ConnectedPlayer.SetNum(10);

	TileArray.Init(NULL, 64);
	DeadZoneArray.Init(NULL, 32);
	PieceArray.Init(NULL, 32);

	Referee = Cast<AMyReferee>(UGameplayStatics::GetActorOfClass(GetWorld(), RefereeClass));
}

// Called every frame
void AMyChessRule::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bDoOnce == true)
	{
		if(HasAuthority()) SpawnGameboard();
		else PrimaryActorTick.SetTickFunctionEnable(false);
	}
	if (bStartGame == true && bGameOvered == false)
	{
		if (TimerDelay <= 0.f)
		{
			if (bPromotion == false)
			{
				if (TeamColor == 0)
				{
					if (TimeLimit[0] >= 0.f)
					{
						TimeLimit[0] -= 1.f * DeltaTime;
						if (TimeLimit[0] <= 0.f) GameResult(false, false, false, true, false);
					}
				}
				else
				{
					if (TimeLimit[1] >= 0.f)
					{
						TimeLimit[1] -= 1.f * DeltaTime;
						if (TimeLimit[1] <= 0.f) GameResult(false, false, false, true, false);
					}
				}
			}
			else
			{
				if (TeamColor == 1)
				{
					if (TimeLimit[0] >= 0.f)
					{
						TimeLimit[0] -= 1.f * DeltaTime;
						if (TimeLimit[0] <= 0.f) GameResult(false, false, false, true, true);
					}
				}
				else
				{
					if (TimeLimit[1] >= 0.f)
					{
						TimeLimit[1] -= 1.f * DeltaTime;
						if (TimeLimit[1] <= 0.f) GameResult(false, false, false, true,true);
					}
				}
			}
		}
		if (bIsMultiplayRule == true)
		{
			if (TimerDelay > 0.f) TimerDelay -= 1.f * DeltaTime;
		}
	}
}

void AMyChessRule::SpawnGameboard_Implementation()
{
	bDoOnce = false;
	FActorSpawnParameters FieldSpawnParams;
	float vertical = 0.f, horizontal = 0.f;
	bool bOdd = true;
	for (int i = 0; i < 8; i++)
	{
		horizontal = 0.f;
		for (int j = 0; j < 8; j++)
		{
			AMyTile* Tile = NULL;
			if (bOdd == true)
			{
				Tile = Cast<AMyTile>(GetWorld()->SpawnActor<AActor>(TileBlack, FVector(horizontal, vertical, 0.f), FRotator(0.f, 0.f, 0.f), FieldSpawnParams));
				bOdd = false;
			}
			else
			{
				Tile = Cast<AMyTile>(GetWorld()->SpawnActor<AActor>(TileWhite, FVector(horizontal, vertical, 0.f), FRotator(0.f, 0.f, 0.f), FieldSpawnParams));
				bOdd = true;
			}
			if (Tile != NULL)
			{
				Tile->TileNumber = CountTiles;
				TileArray[CountTiles] = Tile;
				CountTiles++;
				if (i == 0) Tile->EndLine = 0;
					
				if (i == 7) Tile->EndLine = 1;
				}
				horizontal -= 100.f;
			}
			if (bOdd == true)bOdd = false; else bOdd = true;
			vertical += 100.f;
		}

	vertical = -1000.f;

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 2; j++)
		{

		}
	}
}

void AMyChessRule::Capture(class AMyChessPiece* piece)
{
	if (TeamColor == 0)
	{
		piece->CurrentPosition = 100;
		piece->bFirstAction = false;
		piece->bDead = true;
		piece->SetActorLocation(FVector((RemainingBlack > 8 ? (16 - RemainingBlack) * -100.f : (8 - RemainingBlack) * -100.f) , (RemainingBlack > 8 ? 1700.f : 1600.f), 50.f));
		RemainingBlack--;
	}
	else
	{
		piece->CurrentPosition = 100;
		piece->bDead = true;
		piece->bFirstAction = false;
		piece->SetActorLocation(FVector((RemainingWhite > 8 ? (16 - RemainingWhite) * -100.f : (8 - RemainingWhite) * -100.f),(RemainingWhite > 8 ? -1000.f : -900.f), 50.f));
		RemainingWhite--;
	}
}

bool AMyChessRule::CheckMate(int teamColor, int location, AMyTile* tiles)
{
	if (teamColor == 0)
	{
		for (int i = 16; i < 32; i++)
		{
			if (PieceArray[i]->bDead == false)
			{
				if (CheckMateAndMovement(PieceArray[i], tiles, location, false) == true)
				{
					return true;
				}
			}

		}
	}
	else if (teamColor == 1)
	{
		for (int i = 0; i < 16; i++)
		{
			if (PieceArray[i]->bDead == false)
			{

				if (CheckMateAndMovement(PieceArray[i], tiles, location, false) == true)
				{
					return true;
				}
			}

		}
	}

	return false;
}

void AMyChessRule::Check(AMyChessPiece* Piece, AMyTile* Tile, bool bPawn, bool bEnpassant, bool bKing)
{
	if (Piece->bDead == false)
	{
		int KingPosition;
		int KingPositionBeta = 0;
		if (Piece->TeamColor == 0)
		{
			if (bKing == false)
			{
				KingPosition = WhiteKing->CurrentPosition;
			}
			else
			{
				KingPosition = Tile->TileNumber;
				KingPositionBeta = WhiteKing->CurrentPosition;
			}
		}
		else
		{
			if (bKing == false)
			{
				KingPosition = BlackKing->CurrentPosition;
			}
			else
			{
				KingPosition = Tile->TileNumber;
				KingPositionBeta = BlackKing->CurrentPosition;
			}
		}
		int t, k;
		if (bEnpassant == true)
		{
			if (Piece->TeamColor == 0)
			{
				if (CheckMate(Piece->TeamColor, KingPosition, Tile) == true)
				{
					if (TileArray[Tile->TileNumber - 8]->CurrentPiece != NULL)
					{
						t = TileArray[Tile->TileNumber - 8]->CurrentPiece->ChessType;
						TileArray[Tile->TileNumber - 8]->CurrentPiece->ChessType = 0;
						if (CheckMate(Piece->TeamColor, KingPosition, Tile) == true)
						{
							Tile->ChangeMaterial(3);
							Tile->TileState = 3;
						}
						else
						{
							Tile->ChangeMaterial(2);
							Tile->TileState = 2;
							Piece->bCanMove = true;
						}
						TileArray[Tile->TileNumber - 8]->CurrentPiece->ChessType = t;
					}
				}
				else
				{
					Tile->ChangeMaterial(2);
					Tile->TileState = 2;
					Piece->bCanMove = true;
				}
			}
			else
			{
				if (CheckMate(Piece->TeamColor, KingPosition, Tile) == true)
				{
					if (TileArray[Tile->TileNumber + 8]->CurrentPiece != NULL)
					{
						t = TileArray[Tile->TileNumber + 8]->CurrentPiece->ChessType;
						TileArray[Tile->TileNumber + 8]->CurrentPiece->ChessType = 0;
						if (CheckMate(Piece->TeamColor, KingPosition, Tile) == true)
						{
							Tile->ChangeMaterial(3);
							Tile->TileState = 3;
						}
						else
						{
							Tile->ChangeMaterial(2);
							Tile->TileState = 2;
							Piece->bCanMove = true;
						}
						TileArray[Tile->TileNumber + 8]->CurrentPiece->ChessType = t;
					}
				}
				else
				{
					Tile->ChangeMaterial(2);
					Tile->TileState = 2;
					Piece->bCanMove = true;
				}
			}
		}
		else
		{
			if (bPawn == false)
			{
				if (bKing == true)
				{
					TileArray[KingPositionBeta]->TileTeamColor = 2;
				}
				if (Tile->ChangeMaterials(2, Piece->TeamColor, false) == 2)
				{
					t = Tile->CurrentPiece->ChessType;
					Tile->CurrentPiece->ChessType = 0;
					k = Tile->TileTeamColor;
					if (CheckMate(Piece->TeamColor, KingPosition, Tile) == true)
					{
						Tile->ChangeMaterial(3);
						Tile->TileState = 3;
					}
					else
					{
						Tile->ChangeMaterials(0, Piece->TeamColor, false);
						Piece->bCanMove = true;
					}
					if (Tile->CurrentPiece != NULL) Tile->CurrentPiece->ChessType = t;
					TileArray[Tile->TileNumber]->TileTeamColor = k;
				}
				else if (Tile->ChangeMaterials(1, Piece->TeamColor, false) == 1)
				{
					k = Tile->TileTeamColor;
					TileArray[Tile->TileNumber]->TileTeamColor = Piece->TeamColor;
					if (CheckMate(Piece->TeamColor, KingPosition, Tile) == true)
					{
						Tile->ChangeMaterial(3);
						Tile->TileState = 3;

					}
					else
					{
						Piece->bCanMove = true;
					}
					Tile->TileTeamColor = k;
				}
			}
			else
			{
				if (CheckMate(Piece->TeamColor, KingPosition, Tile) == true)
				{
					if (Tile->ChangeMaterials(1, Piece->TeamColor, true) == 1)
					{
						t = Tile->TileTeamColor;
						TileArray[Tile->TileNumber]->TileTeamColor = Piece->TeamColor;
						if (CheckMate(Piece->TeamColor, KingPosition, Tile) == true)
						{
							Tile->ChangeMaterial(3);
							Tile->TileState = 3;
						}
						else
						{
							Piece->bCanMove = true;
						}
						Tile->TileTeamColor = t;
					}
				}
				else
				{
					Tile->ChangeMaterials(1, Piece->TeamColor, true);
				}
			}
		}
	}
}

bool AMyChessRule::CheckMateAndMovement(AMyChessPiece* piece, AMyTile* tile, int location, bool movement)
{
	AMyChessPiece* Piece = piece;
	AMyTile* Tile = tile;
	if (Piece)
	{
		switch (Piece->ChessType)
		{
		case 0:	//없음
		{
			break;
		}
		case 1:	//폰
		{
			if (Piece->TeamColor == 0)
			{
				if (Piece->bFirstAction == true)
				{
					if (Piece->CurrentPosition + 8 == location || Piece->CurrentPosition + 16 == location)
					{
						if (TileArray[Piece->CurrentPosition + 8]->TileTeamColor == 2 || Piece->CurrentPosition + 8 == location)
						{
							if (movement == true) Check(Piece, Tile, true, false, false);
						}
					}
				}
				else
				{
					if (Piece->CurrentPosition + 8 == location)
					{
						if (movement == true) Check(Piece, Tile, true, false, false);
					}
				}
				switch (Piece->CurrentPosition % 8)
				{
				case 0:
				{
					if (movement == false)
					{
						if (Piece->CurrentPosition + 9 == location) return true;
					}
					if ((Piece->TeamColor != TileArray[location]->TileTeamColor && TileArray[location]->TileTeamColor != 2) && (Piece->CurrentPosition + 9 == location))
					{
						if (movement == true) Check(Piece, Tile, false, false, false); else return true;
					}

					if (Piece->CurrentPosition + 9 == location)
					{
						if (bEnpassantB[Piece->CurrentPosition + 9] == 1)
						{
							if (movement == true) Check(Piece, Tile, false, true, false); else return true;
						}
					}
					/*
					if ((bEnpassantB[Piece->CurrentPosition + 9] == 1 && Piece->CurrentPosition + 9 == location))
					{
						if (movement == true) Check(Piece, Tile, false, true, false); else return true;
					}
					*/
					break;
				}
				case 7:
				{
					if (movement == false)
					{
						if (Piece->CurrentPosition + 7 == location) return true;
					}
					if ((Piece->TeamColor != TileArray[location]->TileTeamColor && TileArray[location]->TileTeamColor != 2) && (Piece->CurrentPosition + 7 == location))
					{
						if (movement == true) Check(Piece, Tile, false, false, false); else return true;
					}

					if (Piece->CurrentPosition + 7 == location)
					{
						if (bEnpassantB[Piece->CurrentPosition + 7] == 1)
						{
							if (movement == true) Check(Piece, Tile, false, true, false); else return true;
						}
					}
					/*
					if ((bEnpassantB[Piece->CurrentPosition + 7] == 1 && Piece->CurrentPosition + 7 == location))
					{
						if (movement == true) Check(Piece, Tile, false, true, false); else return true;
					}
					*/
					
					break;
				}
				default:
				{
					if (movement == false)
					{
						if (Piece->CurrentPosition + 7 == location || Piece->CurrentPosition + 9 == location) return true;
					}
					if ((Piece->TeamColor != TileArray[location]->TileTeamColor && TileArray[location]->TileTeamColor != 2) && (Piece->CurrentPosition + 7 == location || Piece->CurrentPosition + 9 == location))
					{
						if (movement == true) Check(Piece, Tile, false, false, false); else return true;
					}
					if (Piece->CurrentPosition + 7 == location)
					{
						if (bEnpassantB[Piece->CurrentPosition + 7] == 1)
						{
							if (movement == true) Check(Piece, Tile, false, true, false); else return true;
						}
					}
					else if (Piece->CurrentPosition + 9 == location)
					{
						if (bEnpassantB[Piece->CurrentPosition + 9] == 1)
						{
							if (movement == true) Check(Piece, Tile, false, true, false); else return true;
						}
					}
					/*
					if ((bEnpassantB[Piece->CurrentPosition + 7] == 1 && Piece->CurrentPosition + 7 == location) || (bEnpassantB[Piece->CurrentPosition + 9] == 1 && Piece->CurrentPosition + 9 == location))
					{
						if (movement == true) Check(Piece, Tile, false, true, false); else return true;
					}
					*/
				}
				break;
				}
			}
			else
			{
				if (Piece->bFirstAction == true)
				{
					if (Piece->CurrentPosition - 8 == location || Piece->CurrentPosition - 16 == location)
					{
						if (TileArray[Piece->CurrentPosition - 8]->TileTeamColor == 2 || Piece->CurrentPosition - 8 == location)
						{
							if (movement == true) Check(Piece, Tile, true, false, false);
						}
					}
				}
				else
				{
					if (Piece->CurrentPosition - 8 == location)
					{
						if (movement == true) Check(Piece, Tile, true, false, false);
					}
				}
				switch (Piece->CurrentPosition % 8)
				{
				case 0:
				{
					if (movement == false)
					{
						if (Piece->CurrentPosition - 7 == location) return true;
					}
					if ((Piece->TeamColor != TileArray[location]->TileTeamColor && TileArray[location]->TileTeamColor != 2) && (Piece->CurrentPosition - 7 == location))
					{
						if (movement == true) Check(Piece, Tile, false, false, false); else return true;
					}
					if (Piece->CurrentPosition - 7 == location)
					{
						if (bEnpassantW[Piece->CurrentPosition - 7] == 1)
						{
							if (movement == true) Check(Piece, Tile, false, true, false); return true;
						}
					}
					/*
					if ((bEnpassantW[Piece->CurrentPosition - 7] == 1 && Piece->CurrentPosition - 7 == location))
					{
						if (movement == true) Check(Piece, Tile, false, true, false); return true;
					}
					*/
					break;
				}
				case 7:
				{
					if (movement == false)
					{
						if (Piece->CurrentPosition - 9 == location) return true;
					}
					if ((Piece->TeamColor != TileArray[location]->TileTeamColor && TileArray[location]->TileTeamColor != 2) && (Piece->CurrentPosition - 9 == location))
					{
						if (movement == true) Check(Piece, Tile, false, false, false); else return true;
					}
					if (Piece->CurrentPosition - 9 == location)
					{
						if (bEnpassantW[Piece->CurrentPosition - 9] == 1)
						{
							if (movement == true) Check(Piece, Tile, false, true, false); else return true;
						}
					}

					/*
					if ((bEnpassantW[Piece->CurrentPosition - 9] == 1 && Piece->CurrentPosition - 9 == location))
					{
						if (movement == true) Check(Piece, Tile, false, true, false); else return true;
					}
					*/
					break;
				}
				default:
				{
					if (movement == false)
					{
						if (Piece->CurrentPosition - 7 == location || Piece->CurrentPosition - 9 == location) return true;
					}
					if ((Piece->TeamColor != TileArray[location]->TileTeamColor && TileArray[location]->TileTeamColor != 2) && (Piece->CurrentPosition - 7 == location || Piece->CurrentPosition - 9 == location))
					{
						if (movement == true) Check(Piece, Tile, false, false, false); else return true;
					}
					if (Piece->CurrentPosition - 7 == location)
					{
						if (bEnpassantW[Piece->CurrentPosition - 7] == 1)
						{
							if (movement == true) Check(Piece, Tile, false, true, false); else return true;
						}
					}
					else if(Piece->CurrentPosition - 9 == location)
					{
						if (bEnpassantW[Piece->CurrentPosition - 9] == 1)
						{
							if (movement == true) Check(Piece, Tile, false, true, false); else return true;
						}
					}
					/*
					if ((bEnpassantW[Piece->CurrentPosition - 7] == 1 && Piece->CurrentPosition - 7 == location) || (bEnpassantW[Piece->CurrentPosition - 9] == 1 && Piece->CurrentPosition - 9 == location))
					{
						if (movement == true) Check(Piece, Tile, false, true, false); else return true;
					}
					*/
				}
				break;
				}
			}
			break;
		}
		case 2:	//나이트
		{
			switch (Piece->CurrentPosition % 8)
			{
			case 0:
			{
				if (Piece->CurrentPosition + 10 == location || Piece->CurrentPosition - 6 == location || Piece->CurrentPosition + 17 == location || Piece->CurrentPosition - 15 == location)
				{
					if (movement == true) Check(Piece, Tile, false, false, false); else return true;
				}
				break;
			}
			case 1:
			{
				if (Piece->CurrentPosition + 17 == location || Piece->CurrentPosition + 15 == location || Piece->CurrentPosition + 10 == location || Piece->CurrentPosition - 6 == location || Piece->CurrentPosition - 15 == location || Piece->CurrentPosition - 17 == location)
				{
					if (movement == true) Check(Piece, Tile, false, false, false); else return true;
				}
				break;
			}
			case 6:
			{
				if (Piece->CurrentPosition - 10 == location || Piece->CurrentPosition + 15 == location || Piece->CurrentPosition + 17 == location || Piece->CurrentPosition - 17 == location || Piece->CurrentPosition - 15 == location || Piece->CurrentPosition + 6 == location)
				{
					if (movement == true) Check(Piece, Tile, false, false, false); else return true;
				}
				break;
			}
			case 7:
			{
				if (Piece->CurrentPosition - 10 == location || Piece->CurrentPosition - 17 == location || Piece->CurrentPosition + 6 == location || Piece->CurrentPosition + 15 == location)
				{
					if (movement == true) Check(Piece, Tile, false, false, false); else return true;
				}
				break;
			}
			default:
			{
				if (Piece->CurrentPosition + 15 == location || Piece->CurrentPosition + 17 == location || Piece->CurrentPosition + 10 == location || Piece->CurrentPosition - 6 == location || Piece->CurrentPosition - 15 == location || Piece->CurrentPosition - 17 == location || Piece->CurrentPosition - 10 == location || Piece->CurrentPosition + 6 == location)
				{
					if (movement == true) Check(Piece, Tile, false, false, false); else return true;
				}
			}
			}
			break;
		}
		case 3:	//비숍
		{
			for (int i = 1; i <= Piece->CurrentPosition % 8; i++)
			{
				if (location == Piece->CurrentPosition + i * 7 || location == Piece->CurrentPosition - i * 9)
				{
					if (location == Piece->CurrentPosition + i * 7)
					{
						for (int j = 1; j <= i; j++)
						{
							if (i == j)
							{
								if (movement == true) Check(Piece, Tile, false, false, false); else return true;
							}
							if (TileArray[Piece->CurrentPosition + j * 7]->TileTeamColor != 2) break;
						}
					}
					else
					{
						for (int j = 1; j <= i; j++)
						{
							if (i == j)
							{
								if (movement == true) Check(Piece, Tile, false, false, false); else return true;
							}
							if (TileArray[Piece->CurrentPosition - j * 9]->TileTeamColor != 2) break;
						}
					}
				}
			}
			for (int i = 1; i <= 7 - Piece->CurrentPosition % 8; i++)
			{
				if (location == Piece->CurrentPosition + i * 9 || location == Piece->CurrentPosition - i * 7)
				{
					if (location == Piece->CurrentPosition + i * 9)
					{
						for (int j = 1; j <= i; j++)
						{
							if (i == j)
							{
								if (movement == true) Check(Piece, Tile, false, false, false); else return true;
							}
							if (TileArray[Piece->CurrentPosition + j * 9]->TileTeamColor != 2) break;
						}
					}
					else
					{
						for (int j = 1; j <= i; j++)
						{
							if (i == j)
							{
								if (movement == true) Check(Piece, Tile, false, false, false); else return true;
							}
							if (TileArray[Piece->CurrentPosition - j * 7]->TileTeamColor != 2) break;
						}
					}
				}
			}
			break;
		}
		case 4:	//룩
		{
			for (int i = 1; i <= Piece->CurrentPosition % 8; i++)
			{
				if (Piece->CurrentPosition - 1 * i == location)
				{
					for (int j = 1; j <= i; j++)
					{
						if (i == j)
						{
							if (movement == true) Check(Piece, Tile, false, false, false); else return true;
						}
						if (TileArray[Piece->CurrentPosition - 1 * j]->TileTeamColor != 2) break;
					}
				}
			}
			for (int i = 1; i <= 7 - Piece->CurrentPosition % 8; i++)
			{
				if (Piece->CurrentPosition + 1 * i == location)
				{
					for (int j = 1; j <= i; j++)
					{
						if (i == j)
						{
							if (movement == true) Check(Piece, Tile, false, false, false); else return true;
						}
						if (TileArray[Piece->CurrentPosition + 1 * j]->TileTeamColor != 2) break;
					}
				}
			}
			for (int i = 1; i <= 7; i++)
			{
				if (Piece->CurrentPosition + 8 * i <= 63)
				{
					if (Piece->CurrentPosition + 8 * i == location)
					{
						for (int j = 1; j <= i; j++)
						{
							if (i == j)
							{
								if (movement == true) Check(Piece, Tile, false, false, false); else return true;
							}
							if (TileArray[Piece->CurrentPosition + 8 * j]->TileTeamColor != 2) break;
						}
					}
				}
				if (Piece->CurrentPosition - 8 * i >= 0)
				{
					if (Piece->CurrentPosition - 8 * i == location)
					{
						for (int j = 1; j <= i; j++)
						{
							if (i == j)
							{
								if (movement == true) Check(Piece, Tile, false, false, false); else return true;
							}
							if (TileArray[Piece->CurrentPosition - 8 * j]->TileTeamColor != 2) break;
						}
					}
				}
			}
			break;
		}
		case 5:	//퀸
		{
			for (int i = 1; i <= Piece->CurrentPosition % 8; i++)
			{
				if (Piece->CurrentPosition - 1 * i == location)
				{
					for (int j = 1; j <= i; j++)
					{
						if (i == j)
						{
							if (movement == true) Check(Piece, Tile, false, false, false); else return true;
						}
						if (TileArray[Piece->CurrentPosition - 1 * j]->TileTeamColor != 2) break;
					}
				}
				if (Piece->CurrentPosition + 7 * i == location)
				{
					for (int j = 1; j <= i; j++)
					{
						if (i == j)
						{
							if (movement == true) Check(Piece, Tile, false, false, false); else return true;
						} 
						if (TileArray[Piece->CurrentPosition + 7 * j]->TileTeamColor != 2) break;
					}
				}
				if (Piece->CurrentPosition - 9 * i == location)
				{
					for (int j = 1; j <= i; j++)
					{
						if (i == j)
						{
							if (movement == true) Check(Piece, Tile, false, false, false); else return true;
						}
						if (TileArray[Piece->CurrentPosition - 9 * j]->TileTeamColor != 2) break;
					}
				}
			}
			for (int i = 1; i <= 7 - Piece->CurrentPosition % 8; i++)
			{
				if (Piece->CurrentPosition + 1 * i == location)
				{
					for (int j = 1; j <= i; j++)
					{
						if (i == j)
						{
							if (movement == true) Check(Piece, Tile, false, false, false); else return true;
						}
						if (TileArray[Piece->CurrentPosition + 1 * j]->TileTeamColor != 2) break;
					}
				}
				if (Piece->CurrentPosition - 7 * i == location)
				{
					for (int j = 1; j <= i; j++)
					{
						if (i == j)
						{
							if (movement == true) Check(Piece, Tile, false, false, false); else return true;
						}
						if (TileArray[Piece->CurrentPosition - 7 * j]->TileTeamColor != 2) break;
					}
				}
				if (Piece->CurrentPosition + 9 * i == location)
				{
					for (int j = 1; j <= i; j++)
					{
						if (i == j)
						{
							if (movement == true) Check(Piece, Tile, false, false, false); else return true;
						}
						if (TileArray[Piece->CurrentPosition + 9 * j]->TileTeamColor != 2) break;
					}
				}
			}
			for (int i = 1; i <= 7; i++)
			{
				if (Piece->CurrentPosition + 8 * i == location)
				{
					for (int j = 1; j <= i; j++)
					{
						if (i == j)
						{
							if (movement == true) Check(Piece, Tile, false, false, false); else return true;
						}
						if (TileArray[Piece->CurrentPosition + 8 * j]->TileTeamColor != 2) break;
					}
				}
				if (Piece->CurrentPosition - 8 * i == location)
				{
					for (int j = 1; j <= i; j++)
					{
						if (i == j)
						{
							if (movement == true) Check(Piece, Tile, false, false, false); else return true;
						}
						if (TileArray[Piece->CurrentPosition - 8 * j]->TileTeamColor != 2) break;
					}
				}
			}
			break;
		}
		case 6:	//킹
		{
			//캐슬링
			if (Piece->TeamColor == 0)
			{
				if (TileArray[5]->TileTeamColor == 2 && TileArray[6]->TileTeamColor == 2 && (location == 6 || location == 5))
				{
					if (Piece->bFirstAction == true && PieceArray[7]->bFirstAction == true && CheckMate(Piece->TeamColor, Piece->CurrentPosition, Tile) == false && CheckMate(Piece->TeamColor, TileArray[6]->TileNumber, TileArray[6]) == false)
					{
						if (movement == true)
						{
							Tile->TileState = 5;
							//Tile->ChangeMaterial(4, 3, 0);
							Tile->ChangeMaterial(4);
							if (location == 5) tempTileK = Tile;
						}
					}
				}
				else if (TileArray[1]->TileTeamColor == 2 && TileArray[2]->TileTeamColor == 2 && TileArray[3]->TileTeamColor == 2 && (location == 2 || location == 3))
				{
					if (Piece->bFirstAction == true && PieceArray[0]->bFirstAction == true && CheckMate(Piece->TeamColor, Piece->CurrentPosition, Tile) == false && CheckMate(Piece->TeamColor, TileArray[2]->TileNumber, TileArray[2]) == false)
					{
						if (movement == true)
						{
							Tile->TileState = 4;
							//Tile->ChangeMaterial(4, 3, 0);
							Tile->ChangeMaterial(4);
							if (location == 3) tempTileQ = Tile;
						}
					}
				}
			}
			else
			{
				if (TileArray[61]->TileTeamColor == 2 && TileArray[62]->TileTeamColor == 2 && (location == 62 || location == 61))
				{
					if (Piece->bFirstAction == true && PieceArray[31]->bFirstAction == true && CheckMate(Piece->TeamColor, Piece->CurrentPosition, Tile) == false && CheckMate(Piece->TeamColor, TileArray[62]->TileNumber, TileArray[62]) == false)
					{
						if (movement == true)
						{
							Tile->TileState = 7;
							//Tile->ChangeMaterial(4, 3, 0);
							Tile->ChangeMaterial(4);
							if (location == 61) tempTileK = Tile;
						}
					}
				}
				else if (TileArray[57]->TileTeamColor == 2 && TileArray[58]->TileTeamColor == 2 && TileArray[59]->TileTeamColor == 2 && (location == 58 || location == 59))
				{
					if (Piece->bFirstAction == true && PieceArray[24]->bFirstAction == true && CheckMate(Piece->TeamColor, Piece->CurrentPosition, Tile) == false && CheckMate(Piece->TeamColor, TileArray[58]->TileNumber, TileArray[58]) == false)
					{
						if (movement == true)
						{
							Tile->TileState = 6;
							//Tile->ChangeMaterial(4, 3, 0);
							Tile->ChangeMaterial(4);
							if (location == 59) tempTileQ = Tile;
						}
					}
				}
			}
			//이동 및 체크
			switch (Piece->CurrentPosition % 8)
			{
			case 0:
			{
				if (Piece->CurrentPosition + 1 == location || Piece->CurrentPosition + 9 == location || Piece->CurrentPosition - 7 == location)
				{
					if (movement == true) Check(Piece, Tile, false, false, true); else return true;
				}
				break;
			}
			case 7:
			{
				if (Piece->CurrentPosition + 7 == location || Piece->CurrentPosition - 1 == location || Piece->CurrentPosition - 9 == location)
				{
					if (movement == true) Check(Piece, Tile, false, false, true); else return true;
				}
				break;
			}
			default:
			{
				if (Piece->CurrentPosition + 1 == location || Piece->CurrentPosition + 9 == location || Piece->CurrentPosition - 7 == location || Piece->CurrentPosition + 7 == location || Piece->CurrentPosition - 1 == location || Piece->CurrentPosition - 9 == location)
				{
					if (movement == true) Check(Piece, Tile, false, false, true); else return true;
				}
			}
			}

			if (Piece->CurrentPosition + 8 == location || Piece->CurrentPosition - 8 == location)
			{
				if (movement == true) Check(Piece, Tile, false, false, true); else return true;
			}
			break;
		}
		}
	}
	return false;
}

void AMyChessRule::ChessPromotion(int index)
{
	if (tempPiece)
	{
		if (index == 4)
		{
			tempPiece = NULL;
		}
		else
		{
			switch (index)
			{
			case 0:
			{
				tempPiece->ChessType = 2;
				tempPiece->bMajorPieceOrPawn = false;
				break;
			}
			case 1:
			{
				tempPiece->ChessType = 3;
				tempPiece->bMajorPieceOrPawn = false;
				tempPiece->bWhiteSquareBishop = TileArray[tempPiece->CurrentPosition]->bWhiteSquare;
				break;
			}
			case 2:
			{
				tempPiece->ChessType = 4;
				break;

			}
			case 3:
			{
				tempPiece->ChessType = 5;
				break;
			}
			}

			if (TeamColor == 1)
			{
				CurrentMesh = Meshes[index];
				tempPiece->MeshComp->SetStaticMesh(CurrentMesh);
			}
			else
			{
				CurrentMesh = Meshes[index + 4];
				tempPiece->MeshComp->SetStaticMesh(CurrentMesh);
			}

			for (int i = 0; i < 64; i++)
			{
				TileArray[i]->ColorIndex = 0;
				TileArray[i]->ChangeMaterial(0);
				TileArray[i]->TileState = 0;
				if (TileArray[i]->CurrentPiece) TileArray[i]->TileTeamColor = TileArray[i]->CurrentPiece->TeamColor; else TileArray[i]->TileTeamColor = 2;
			}
			RefreshPieces(tempPiece, false, true);
			tempPiece = NULL;
		}
		TimerDelay = 5.f;
	}
}

void AMyChessRule::ThreefoldRepetition(AMyChessPiece* Piece, bool bCaptured)
{
	int temparray[32] = { 0, };
	int temp[32] = { 0, };
	for (int i = 0; i < 32; i++)
	{
		temparray[i] = PieceArray[i]->CurrentPosition;
		if (bCaptured == true || Piece->ChessType == 1)
		{
			for (int j = 0; j < 25; j++)
			{
				ThreefoldRepetitionTableW[j].setdata(i, 0);
				ThreefoldRepetitionTableB[j].setdata(i, 0);
			}
			if (i < 25)
			{
				ThreefoldRepetitionCountW[i] = 0;
				ThreefoldRepetitionCountB[i] = 0;
			}
		}
	}
	if (bCaptured == true || Piece->ChessType == 1) FiftyMoveRule = Turn[0] + Turn[1] + 50;

	if (Piece->TeamColor == 0)
	{
		for (int i = 0; i < 25; i++)
		{
			for (int k = 0; k < 32; k++)
			{
				if (ThreefoldRepetitionTableW[i].getdata(k) != temparray[k])
				{
					if (ThreefoldRepetitionTableW[i].getdata(0) == 0 && ThreefoldRepetitionTableW[i].getdata(1) == 0)
					{
						for (int j = 0; j < 32; j++)
						{
							ThreefoldRepetitionTableW[i].setdata(j, PieceArray[j]->CurrentPosition);
						}
						ThreefoldRepetitionCountW[i]++;
						i = 25;
					}
					break;
				}
				
				if (k == 31) ThreefoldRepetitionCountW[i]++;
			}
		}
	}
	else
	{
		for (int i = 0; i < 25; i++)
		{
			for (int k = 0; k < 32; k++)
			{
				if (ThreefoldRepetitionTableB[i].getdata(k) != temparray[k])
				{
					if (ThreefoldRepetitionTableB[i].getdata(0) == 0 && ThreefoldRepetitionTableB[i].getdata(1) == 0)
					{
						for (int j = 0; j < 32; j++)
						{
							ThreefoldRepetitionTableB[i].setdata(j, PieceArray[j]->CurrentPosition);
						}
						ThreefoldRepetitionCountB[i]++;
						i = 25;
					}
					break;
				}
				if (k == 31) ThreefoldRepetitionCountB[i]++;
			}
		}
	}

	bool bThreefoldRepetitionWhite = false;
	bool bThreefoldRepetitionBlack = false;

	for (int i = 0; i < 25; i++)
	{
		if (ThreefoldRepetitionCountW[i] >= 3) bThreefoldRepetitionWhite = true;
		if (ThreefoldRepetitionCountB[i] >= 3) bThreefoldRepetitionBlack = true;
	
		if (bThreefoldRepetitionWhite == true || bThreefoldRepetitionBlack == true)
		{
			GameResult(true, false,false,false, false);
			break;
		}
	}
}

void AMyChessRule::EnPassant()
{
	if (TeamColor == 0)
	{
		for (int i = 0; i < 64; i++)
		{
			if (bEnpassantB[i] == 1) bEnpassantB[i] = 0;
		}
	}
	else
	{
		for (int i = 0; i < 64; i++)
		{
			if (bEnpassantW[i] == 1) bEnpassantW[i] = 0;
		}
	}
}

void AMyChessRule::RefreshPieces(AMyChessPiece* RefreshPiece, bool bCastling, bool bPromotions)
{
	if (bCastling == false)
	{
		if (TileArray[RefreshPiece->CurrentPosition] != NULL) TileArray[RefreshPiece->CurrentPosition]->TileTeamColor = RefreshPiece->TeamColor;
		int k = 0;
		if (bPromotions == false)
		{
			if (TeamColor == 0)
			{
				TimerDelay = 5.f;
				Turn[TeamColor]++;
				TeamColor = 1;
				if (CheckMate(TeamColor, BlackKing->CurrentPosition, TileArray[RefreshPiece->CurrentPosition]) == true) bChecked = true;
				else bChecked = false;
	
				for (int i = 16; i < 32; i++)
				{
					if (PieceArray[i]->bDead == false)
					{
						for (int j = 0; j < 64; j++)
						{
							CheckMateAndMovement(PieceArray[i], TileArray[j], j, true);
						}
					}
					if (PieceArray[i]->bCanMove == true) break;
					
					if (i == 31) GameResult(false, false, false, false, false);
				}
			}
			else if (TeamColor == 1)
			{
				TimerDelay = 5.f;
				Turn[TeamColor]++;
				TeamColor = 0;
				if (CheckMate(TeamColor, WhiteKing->CurrentPosition, TileArray[RefreshPiece->CurrentPosition]) == true) bChecked = true;
				else bChecked = false;

				for (int i = 0; i < 16; i++)
				{
					if (PieceArray[i]->bDead == false)
					{
						for (int j = 0; j < 64; j++)
						{
							CheckMateAndMovement(PieceArray[i], TileArray[j], j, true);
						}
					}
					if (PieceArray[i]->bCanMove == true) break;
					
					if (i == 15) GameResult(false, false, false, false, false);
				}
			}
			if(Referee) Referee->HandUp_Server(TeamColor);
		}
		else
		{
			if (TeamColor == 0)
			{
				if (CheckMateAndMovement(RefreshPiece, TileArray[WhiteKing->CurrentPosition], WhiteKing->CurrentPosition, false) == true) bChecked = true;
				else bChecked = false;
		
				for (int i = 0; i < 16; i++)
				{
					if (PieceArray[i]->bDead == false)
					{
						for (int j = 0; j < 64; j++)
						{
							CheckMateAndMovement(PieceArray[i], TileArray[j], j, true);
						}
					}
					if (PieceArray[i]->bCanMove == true) break;
		
					if (i == 15) GameResult(false, false, false, false, true);
				}
			}
			else
			{
				if (CheckMateAndMovement(RefreshPiece, TileArray[BlackKing->CurrentPosition], BlackKing->CurrentPosition, false) == true) bChecked = true;
				else bChecked = false;
				
				for (int i = 16; i < 32; i++)
				{
					if (PieceArray[i]->bDead == false)
					{
						for (int j = 0; j < 64; j++)
						{
							CheckMateAndMovement(PieceArray[i], TileArray[j], j, true);
						}
					}
					if (PieceArray[i]->bCanMove == true) break;
			
					if (i == 31) GameResult(false, false, false, false, true);
				}
			}
		}
		int BS = 0, WS = 0;
		for (int i = 0; i < 32; i++)
		{
			if (PieceArray[i]->bDead == false)
			{
				if (PieceArray[i]->bMajorPieceOrPawn == true)
				{
					k += 2;
					break;
				}
				else
				{
					if (PieceArray[i]->ChessType != 6)
					{
						if (PieceArray[i]->ChessType == 2)
						{
							k += 1;
						}
						else if (PieceArray[i]->ChessType == 3)
						{
							if (PieceArray[i]->bWhiteSquareBishop == true)
							{
								k += 1;
								WS += 1;
							}
							else
							{
								k += 1;
								BS += 1;
							}
						}
					}
				}
			}
		}

		if (k < 2) GameResult(false, false, true, false, false);
		else if (k == WS || k == BS) GameResult(false, false, true, false, false);

		for (int i = 0; i < 32; i++)
		{
			PieceArray[i]->bCanMove = false;
		}
	}
	else
	{
		if (TeamColor == 0)
		{
			TimerDelay = 5.f;
			Turn[TeamColor]++;
			TeamColor = 1;
			if (Referee) Referee->HandUp_Server(TeamColor);

			if (CheckMate(TeamColor, BlackKing->CurrentPosition, TileArray[RefreshPiece->CurrentPosition]) == true) bChecked = true;
			else bChecked = false;
		
			for (int i = 16; i < 32; i++)
			{
				if (PieceArray[i]->bDead == false)
				{
					for (int j = 0; j < 64; j++)
					{
						CheckMateAndMovement(PieceArray[i], TileArray[j], j, true);
					}
				}
				if (PieceArray[i]->bCanMove == true) break;

				if (i == 31) GameResult(false, false, false, false, false);
			}
		}
		else
		{
			TimerDelay = 5.f;
			Turn[TeamColor]++;
			TeamColor = 0;
			if (Referee) Referee->HandUp_Server(TeamColor);

			if (CheckMate(TeamColor, WhiteKing->CurrentPosition, TileArray[RefreshPiece->CurrentPosition]) == true) bChecked = true;
			else bChecked = false;

			for (int i = 0; i < 16; i++)
			{
				if (PieceArray[i]->bDead == false)
				{
					for (int j = 0; j < 64; j++)
					{
						CheckMateAndMovement(PieceArray[i], TileArray[j], j, true);
					}
				}
				if (PieceArray[i]->bCanMove == true) break;
				
				if (i == 15) GameResult(false, false, false, false, false);
			}
		}
		
		for (int i = 0; i < 32; i++)
		{
			PieceArray[i]->bCanMove = false;
		}
	}

	for (int i = 0; i < 64; i++)
	{
		TileArray[i]->ColorIndex = 0;
		TileArray[i]->ChangeMaterial(0);
		TileArray[i]->TileState = 0;

		if (TileArray[i]->CurrentPiece) TileArray[i]->TileTeamColor = TileArray[i]->CurrentPiece->TeamColor;
		else  TileArray[i]->TileTeamColor = 2;
	}

	if (bPromotion == false)
	{
		if (FiftyMoveRule <= (Turn[0] + Turn[1])) GameResult(false, true, false, false, false);
	}
}

void AMyChessRule::GameResult_Implementation(bool bThreefoldRepititions, bool FiftyMoveRules, bool bInsufficientMaterial, bool bTimeOut, bool bPromotions)
{
	AMyChess_GameModeBase* GameMode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	
	if (bThreefoldRepititions == true)
	{
		GameEndingText = "Threefold Repetition";
		if(GameMode) GameMode->GameResult(2, bPromotions);
	}
	else if (FiftyMoveRules == true)
	{
		GameEndingText = "Fifty Move Rule";
		if (GameMode) GameMode->GameResult(2, bPromotions);
	}
	else if (bInsufficientMaterial == true)
	{
		GameEndingText = "InsufficientMaterial";
		if (GameMode) GameMode->GameResult(2, bPromotions);
	}
	else if (bTimeOut == true)
	{
		GameEndingText = "TimeOut";
		if (GameMode) GameMode->GameResult(TeamColor, bPromotions);
	}
	else
	{
		if (bChecked == true)
		{
			GameEndingText = "CheckMate";
			if (GameMode) GameMode->GameResult(TeamColor, bPromotions);
		}
		else
		{
			GameEndingText = "StaleMate";
			if (GameMode) GameMode->GameResult(2, bPromotions);
		}
	}
	bGameOvered = true;
}

void AMyChessRule::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyChessRule, ConnectedPlayer);
	DOREPLIFETIME(AMyChessRule, bDisconnectedPlayer);
	DOREPLIFETIME(AMyChessRule, RemainingBlack);
	DOREPLIFETIME(AMyChessRule, RemainingWhite);
	DOREPLIFETIME(AMyChessRule, TimeLimit);
	DOREPLIFETIME(AMyChessRule, Turn);
	DOREPLIFETIME(AMyChessRule, bStartGame);
	DOREPLIFETIME(AMyChessRule, TimerDelay);
	DOREPLIFETIME(AMyChessRule, bGameOvered);
	DOREPLIFETIME(AMyChessRule, GameEndingText);
	DOREPLIFETIME(AMyChessRule, bCanStartGame);
	DOREPLIFETIME(AMyChessRule, bDoOnce_S);
	DOREPLIFETIME(AMyChessRule, PieceNumber);
	DOREPLIFETIME(AMyChessRule, TileArray);
	DOREPLIFETIME(AMyChessRule, DeadZoneArray);
	DOREPLIFETIME(AMyChessRule, PieceArray);
	DOREPLIFETIME(AMyChessRule, TeamColor);
	DOREPLIFETIME(AMyChessRule, FiftyMoveRule);
	DOREPLIFETIME(AMyChessRule, BlackKing);
	DOREPLIFETIME(AMyChessRule, WhiteKing);
	DOREPLIFETIME(AMyChessRule, bChecked);
	DOREPLIFETIME(AMyChessRule, bPromotion);
	DOREPLIFETIME(AMyChessRule, tempPiece);
	DOREPLIFETIME(AMyChessRule, CurrentMesh);
	DOREPLIFETIME(AMyChessRule, bWaitGame);
}