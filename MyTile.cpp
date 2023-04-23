// Fill out your copyright notice in the Description page of Project Settings.


#include "MyTile.h"
#include "Kismet/GameplayStatics.h"
#include "MyChessPiece.h"
#include "MyChessRule.h"
#include "MyPlayerController.h"
#include "Net/UnrealNetwork.h"
// Sets default values
AMyTile::AMyTile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tile"));
	RootComponent = MeshComp;
	bReplicates = true;
}

// Called when the game starts or when spawned
void AMyTile::BeginPlay()
{
	Super::BeginPlay();
	
	ChessRule = Cast<AMyChessRule>(UGameplayStatics::GetActorOfClass(GetWorld(), RuleClass));
}

// Called every frame
void AMyTile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		if (bDoOnce) GetPiece(); else PrimaryActorTick.SetTickFunctionEnable(false);
	}
	else
	{
		PrimaryActorTick.SetTickFunctionEnable(false);
	}
	

}

void AMyTile::ClickedTile(class AMyPlayerController* Controllers)
{
	Controllers->GetTile(this);
}

void AMyTile::GetPiece()
{
	if (HasAuthority()) GetPieceServer();
}

void AMyTile::GetPieceServer_Implementation()
{
	if (bDoOnce == true)
	{
		bDoOnce = false;
		if (ChessRule && bDeadZone == false)
		{
			FActorSpawnParameters spawnparams;
			spawnparams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (TileNumber < 16)
			{
				if (EndLine != 2)
				{
					int temp = TileNumber % 8;
					ChessPiece = Cast<AMyChessPiece>(GetWorld()->SpawnActor<AActor>(SpawnClass_White[temp], GetActorLocation() + FVector(0.f, 0.f, 50.f), FRotator(0.f, 0.f, 0.f), spawnparams));
					ChessPiece->CurrentPosition = TileNumber;
					if (ChessPiece->ChessType == 3) ChessPiece->bWhiteSquareBishop = bWhiteSquare;
					TileTeamColor = ChessPiece->TeamColor;
					if (ChessPiece->ChessType == 6)
					{
						if (ChessPiece->TeamColor == 0) ChessRule->WhiteKing = ChessPiece;
						else ChessRule->BlackKing = ChessPiece;
					}
					ChessRule->PieceArray[TileNumber] = ChessPiece;
					ChessPiece->PieceNumbering = ChessRule->PieceNumber;
					CurrentPiece = ChessPiece;
					ChessRule->PieceNumber++;
				}
				else
				{
					ChessPiece = Cast<AMyChessPiece>(GetWorld()->SpawnActor<AActor>(SpawnClass_White[8], GetActorLocation() + FVector(0.f, 0.f, 50.f), FRotator(0.f, 0.f, 0.f), spawnparams));
					ChessPiece->CurrentPosition = TileNumber;
					TileTeamColor = ChessPiece->TeamColor;
					ChessRule->PieceArray[TileNumber] = ChessPiece;
					ChessPiece->PieceNumbering = ChessRule->PieceNumber;
					CurrentPiece = ChessPiece;
					ChessRule->PieceNumber++;

				}
			}
			if (TileNumber > 47)
			{
					//	TeamColor = 1;
				if (EndLine != 2)
				{
					int temp = TileNumber % 8;
					ChessPiece = Cast<AMyChessPiece>(GetWorld()->SpawnActor<AActor>(SpawnClass_Black[temp], GetActorLocation() + FVector(0.f, 0.f, 50.f), FRotator(0.f, 180.f, 0.f), spawnparams));
					ChessPiece->CurrentPosition = TileNumber;
					if (ChessPiece->ChessType == 3) ChessPiece->bWhiteSquareBishop = bWhiteSquare;
					TileTeamColor = ChessPiece->TeamColor;
					if (ChessPiece->ChessType == 6)
					{
						if (ChessPiece->TeamColor == 0) ChessRule->WhiteKing = ChessPiece;
						else ChessRule->BlackKing = ChessPiece;
					}
					ChessRule->PieceArray[TileNumber - 32] = ChessPiece;
					ChessPiece->PieceNumbering = ChessRule->PieceNumber;
					CurrentPiece = ChessPiece;
					ChessRule->PieceNumber++;
				}
				else
				{
					ChessPiece = Cast<AMyChessPiece>(GetWorld()->SpawnActor<AActor>(SpawnClass_Black[8], GetActorLocation() + FVector(0.f, 0.f, 50.f), FRotator(0.f, 180.f, 0.f), spawnparams));
					ChessPiece->CurrentPosition = TileNumber;
					TileTeamColor = ChessPiece->TeamColor;
					ChessRule->PieceArray[TileNumber - 32] = ChessPiece;
					ChessPiece->PieceNumbering = ChessRule->PieceNumber;
					CurrentPiece = ChessPiece;
					ChessRule->PieceNumber++;
				}
			}
		}
	}
	else
	{
		if (CurrentPiece != NULL)
		{
			ChessRule->PieceArray[ChessRule->PieceNumber] = CurrentPiece;
			ChessRule->PieceNumber = CurrentPiece->PieceNumbering;
			
		}
	}
}

void AMyTile::SetupMaterial()
{
	MeshComp->SetMaterial(0, CurrentMaterial);
}

void AMyTile::ChangeMaterialToEmphasize(int index)
{
	if (HasAuthority())
	{
		TileEmphasize_Server(index);
	}
}

void AMyTile::TileEmphasize_Server_Implementation(int index)
{
	if (index == 1)
	{
		CurrentMaterial = EmphasizeColors[0];
	}
	else if(index==2)
	{
		CurrentMaterial = EmphasizeColors[1];
	}
	else if (index >= 4)
	{
		CurrentMaterial = EmphasizeColors[2];
	}
	SetupMaterial();
}

void AMyTile::ChangeMaterial(int color)
{
	if (HasAuthority()) ChangeMaterial_Server(color);
}

void AMyTile::ChangeMaterial_Server_Implementation(int color)
{
	CurrentMaterial = MaterialColors[color];
	SetupMaterial();
}

int AMyTile::ChangeMaterials(int color, int index, bool bPawn)
{
	if (index == 3)
	{
		ChangeMaterial(color);
		return 4;
	}
	else
	{
		if (bPawn == true)
		{
			if (ChessRule->TileArray[TileNumber]->TileTeamColor == 2)
			{
				ColorIndex = 1;
				TileState = 1;
				ChangeMaterial(TileState);
				return 1;
			}
			else
			{
				ColorIndex = 3;
				TileState = 3;
				ChangeMaterial(TileState);
				return 3;
			}
		}
		else
		{
			if (ChessRule->TileArray[TileNumber]->TileTeamColor == 2)
			{
				ColorIndex = 1;
				TileState = 1;
				ChangeMaterial(TileState);
				return 1;
			}
			else if (ChessRule->TileArray[TileNumber]->TileTeamColor == index)
			{
				ColorIndex = 3;
				TileState = 3;
				ChangeMaterial(TileState);
				return 3;
			}
			else
			{
				ColorIndex = 2;
				TileState = 2;
				ChangeMaterial(TileState);
				return 2;
			}
		}
	}
}

void AMyTile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyTile, ColorIndex);
	DOREPLIFETIME(AMyTile, TileNumber);
	DOREPLIFETIME(AMyTile, TileTeamColor);
	DOREPLIFETIME(AMyTile, CurrentPiece);
	DOREPLIFETIME(AMyTile, CurrentMaterial);
	DOREPLIFETIME(AMyTile, TileDeadZoneNumber);
	DOREPLIFETIME(AMyTile, TileState);
}