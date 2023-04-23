// Fill out your copyright notice in the Description page of Project Settings.


#include "MyChessPiece.h"
#include "Kismet/GameplayStatics.h"
#include "MyChessRule.h"
#include "MyPlayerController.h"
#include "MyTile.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"
#include "MyPlayerState.h"
// Sets default values
AMyChessPiece::AMyChessPiece()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Piece"));
	RootComponent = MeshComp;
	bReplicates = true;
}

// Called when the game starts or when spawned
void AMyChessPiece::BeginPlay()
{
	Super::BeginPlay();

	

	ChessRule = Cast<AMyChessRule>(UGameplayStatics::GetActorOfClass(GetWorld(), ChessRuleClass));

	if (HasAuthority())
	{
		if (ChessType == 6)
		{
			if (TeamColor == 0) ChessRule->WhiteKing = this; else ChessRule->BlackKing = this;
		}
		if (ChessType == 3)
		{
			//UE_LOG(LogTemp, Warning, TEXT("%d"), CurrentPosition);
			//bWhiteSquareBishop = ChessRule->TileArray[CurrentPosition]->bWhiteSquare;
			//bWhiteSquareBishop = ChessRule->TileArray[CurrentPosition]->bWhiteSquare;
		}
	}
	
}

// Called every frame
void AMyChessPiece::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (ChessRule->TeamColor != TeamColor || ChessRule->bPromotion == true)
	MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECollisionResponse::ECR_Ignore);
	else
	MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECollisionResponse::ECR_Block); 
	
	if (bMove == true && bDead == false)
	{
		if (GetActorLocation() != newLocation)
		{
			recloc = FMath::VInterpTo(GetActorLocation(), newLocation, GetWorld()->GetDeltaSeconds(), 5.f);
			SetActorLocation(recloc);
			if (GetActorLocation() == newLocation) bMove = false;
		}
	}
	
}

void AMyChessPiece::ClickedPiece(class AMyPlayerController* Controllers)
{
	Controllers->GetPiece(this,false);
}

void AMyChessPiece::MoveLocation(FVector location)
{
	newLocation = location;
	bMove = true;
}

void AMyChessPiece::Highlight_Implementation(bool bHighlight)
{
	MeshComp->SetRenderCustomDepth(bHighlight);
	if (bHighlight == true)
	{
		MeshComp->SetCustomDepthStencilValue(2);
	}
	else
	{
		MeshComp->SetCustomDepthStencilValue(0);
	}

}

void AMyChessPiece::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyChessPiece, CurrentPosition);
	DOREPLIFETIME(AMyChessPiece, bMove);
	DOREPLIFETIME(AMyChessPiece, bFirstAction);
	DOREPLIFETIME(AMyChessPiece, bDead);
	DOREPLIFETIME(AMyChessPiece, bCanMove);
	DOREPLIFETIME(AMyChessPiece, PieceNumbering);
	DOREPLIFETIME(AMyChessPiece, TeamColor);
	DOREPLIFETIME(AMyChessPiece, ChessType);
	DOREPLIFETIME(AMyChessPiece, bWhiteSquareBishop);
	DOREPLIFETIME(AMyChessPiece, recloc);
	DOREPLIFETIME(AMyChessPiece, newLocation);
}