// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyTile.generated.h"

UCLASS()
class MYCHESS__API AMyTile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMyTile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	UPROPERTY()
		bool bDoOnce = true;
	UPROPERTY()
		bool bDoOnce_B = true;
	UPROPERTY()
		class AMyChessPiece* ChessPiece = NULL;
	UPROPERTY()
		class AMyChessRule* ChessRule;
	UPROPERTY(Replicated)
		int32 ColorIndex = 0;
	UPROPERTY(EditAnywhere, Category = "Class")
		TSubclassOf<AActor> RuleClass;
	UPROPERTY(EditAnywhere)
		UStaticMeshComponent* MeshComp;
	UPROPERTY(EditAnywhere, Category = "State")
		bool bDeadZone = false;
	UPROPERTY(EditAnywhere, Category = "State")
		bool bWhiteSquare = true;
	UPROPERTY(EditAnywhere, Category = "Class")
		UClass* SpawnClass_Black[9];
	UPROPERTY(EditAnywhere, Category = "Class")
		UClass* SpawnClass_White[9];
	UPROPERTY(EditAnywhere)
		UMaterialInterface* MaterialColors[5];
	UPROPERTY(EditAnywhere)
		UMaterialInterface* EmphasizeColors[3];
	UPROPERTY(Replicated)
		UMaterialInterface* CurrentMaterial;
	UPROPERTY(Replicated)
		class AMyChessPiece* CurrentPiece = NULL;

	UPROPERTY(Replicated)
		int TileTeamColor = 2;
	UPROPERTY(Replicated)
		int TileNumber = -1;

	int EndLine = 2;
	UPROPERTY(Replicated)
		int TileDeadZoneNumber = 0;
	UPROPERTY(Replicated)
		int TileState = 0;
	void ClickedTile(class AMyPlayerController* Controllers);

	void GetPiece();
	UFUNCTION(NetMulticast, Reliable)
		void GetPieceServer();
	void GetPieceServer_Implementation();

	void SetupMaterial();

	void ChangeMaterialToEmphasize(int index);
	UFUNCTION(NetMulticast, Reliable)
		void TileEmphasize_Server(int index);
	void TileEmphasize_Server_Implementation(int index);

	void ChangeMaterial(int color);
	UFUNCTION(NetMulticast, Reliable)
		void ChangeMaterial_Server(int color);
	void ChangeMaterial_Server_Implementation(int color);

	int ChangeMaterials(int color, int index, bool bPawn);

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const;
};
