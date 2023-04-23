// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyChessPiece.h"
#include "MyChessRule.generated.h"

USTRUCT()
struct FChessColumn
{
	GENERATED_BODY()
public:

	UPROPERTY()
		TArray<int> ChessPieces;
	
	void setdata(int PieceNumber, int Arrayvalue)
	{
		if (PieceNumber < 32)
		{
			ChessPieces[PieceNumber] = Arrayvalue;
		}
	};

	int getdata(int PieceNumber)
	{
		if (PieceNumber < 32)
		{
			return ChessPieces[PieceNumber];
		}
		else
		{
			return -1;
		}
	};

	FChessColumn()
	{
		ChessPieces.SetNum(32);
	};
};


UCLASS()
class MYCHESS__API AMyChessRule : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMyChessRule();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintReadWrite, Replicated)
		TArray<FString> ConnectedPlayer;
	UPROPERTY(BlueprintReadWrite, Replicated)
		TArray<float>TimeLimit;
	UPROPERTY(BlueprintReadWrite, Replicated)
		bool bGameOvered = false;
	UPROPERTY(BlueprintReadWrite, Replicated)
		FString GameEndingText = "None";
	UPROPERTY(BlueprintReadWrite, Replicated)
		bool bDisconnectedPlayer = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bVSAI = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bIsMultiplayRule = true;
	UPROPERTY(Replicated)
		bool bWaitGame = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
		bool bCanStartGame = false;
	UPROPERTY()
		bool bDoOnce = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
		bool bDoOnce_S = true;
	UPROPERTY(Replicated)
		float TimerDelay = 5.f;
	UPROPERTY(Replicated)
		TArray<int> Turn;
		//int Turn[2] = { 0,0 };
	UPROPERTY()
		class AMyChessPiece* SelectedPiece;
	UPROPERTY()
		class AMyTile* SelectedTiles;
	UPROPERTY(Replicated)
		int PieceNumber = 0;
	UPROPERTY(Replicated)
		class AMyChessPiece* BlackKing = NULL;
	UPROPERTY(Replicated)
		class AMyChessPiece* WhiteKing = NULL;
	UPROPERTY(Replicated)
		int RemainingWhite = 16;
	UPROPERTY(Replicated)
		int RemainingBlack = 16;
	UPROPERTY()
		TArray<int> bEnpassantB;
	UPROPERTY()
		TArray<int> bEnpassantW;
	UPROPERTY()
		TArray<int> ThreefoldRepetitionCountW;
	UPROPERTY()
		TArray<int> ThreefoldRepetitionCountB;

	UPROPERTY()
		TArray<FChessColumn> ThreefoldRepetitionTableW;
	UPROPERTY()
		TArray<FChessColumn> ThreefoldRepetitionTableB;
	UPROPERTY(EditAnywhere, Category = "Tiles")
		UClass* TileWhite;
	UPROPERTY(EditAnywhere, Category = "Tiles")
		UClass* TileBlack;
	UPROPERTY(EditAnywhere, Category = "Tiles")
		UClass* TileWhite_DeadZone;
	UPROPERTY(EditAnywhere, Category = "Tiles")
		UClass* TileBlack_DeadZone;
	UPROPERTY(EditAnywhere)
		TArray<UStaticMesh*> Meshes;
	UPROPERTY(Replicated)
		UStaticMesh* CurrentMesh;
	UPROPERTY()
		class AMyReferee* Referee;
	UPROPERTY(EditAnywhere)
		TSubclassOf<class AMyReferee>RefereeClass;

	UPROPERTY(Replicated)
		TArray<class AMyTile*> TileArray;
	UPROPERTY(Replicated)
		TArray<class AMyTile*> DeadZoneArray;
	UPROPERTY(Replicated)
		TArray<class AMyChessPiece*> PieceArray;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
		bool bStartGame = false;
	UPROPERTY(BlueprintReadWrite,Replicated)
		bool bPromotion = false;
	UPROPERTY(BlueprintReadWrite, Replicated)
		int TeamColor = 0;
	UPROPERTY()
		int CountTiles = 0;
	
	UFUNCTION(NetMulticast, Reliable)
		void SpawnGameboard();
	void SpawnGameboard_Implementation();

	UPROPERTY(Replicated)
		int FiftyMoveRule = 50;
	UPROPERTY(BlueprintReadWrite, Replicated)
		bool bChecked = false;
	UPROPERTY(Replicated)
		class AMyChessPiece* tempPiece = NULL;
	UPROPERTY()
		class AMyTile* tempTileK = NULL;
	UPROPERTY()
		class AMyTile* tempTileQ = NULL;
	void Capture(class AMyChessPiece* piece);
	bool CheckMate(int teamColor, int location, class AMyTile* tiles);
	void Check(class AMyChessPiece* Piece, class AMyTile* Tile, bool bPawn, bool bEnpassant, bool bKing);
	bool CheckMateAndMovement(class AMyChessPiece* piece, class AMyTile* tile, int location, bool movement);

	void ChessPromotion(int index);
	
	void ThreefoldRepetition(class AMyChessPiece* Piece, bool bCaptured);
	void EnPassant();
	void RefreshPieces(class AMyChessPiece* RefreshPiece, bool bCastling, bool bPromotions);
	
	UFUNCTION(NetMulticast, Reliable)
		void GameResult(bool bThreefoldRepititions, bool FiftyMoveRules, bool bInsufficientMaterial, bool bTimeOut, bool bPromotions);
	void GameResult_Implementation(bool bThreefoldRepititions, bool FiftyMoveRules, bool bInsufficientMaterial, bool bTimeOut, bool bPromotions);

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const;
};
