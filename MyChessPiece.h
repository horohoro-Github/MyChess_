// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyChessPiece.generated.h"

UCLASS()
class MYCHESS__API AMyChessPiece : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMyChessPiece();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(Replicated)
		FVector newLocation;
	UPROPERTY(Replicated)
		FVector recloc;
	UPROPERTY(Replicated)
		bool bMove = false;
	UPROPERTY(Replicated)
		bool bFirstAction = true;
	UPROPERTY(Replicated)
		bool bDead = false;
	UPROPERTY(Replicated)
		bool bCanMove = false;
	UPROPERTY(Replicated)
		bool bWhiteSquareBishop = true;
	UPROPERTY(Replicated)
		int PieceNumbering = 0;
	UPROPERTY(EditAnywhere)
		UStaticMeshComponent* MeshComp;
	UPROPERTY(EditAnywhere)
		bool bMajorPieceOrPawn = false;
	UPROPERTY(EditAnywhere, Category = "Class")
		TSubclassOf<AActor> ChessRuleClass;
	class AMyChessRule* ChessRule;
	UPROPERTY(EditAnywhere, Category = "State", Replicated)
		int TeamColor = 0;
	UPROPERTY(EditAnywhere, Category = "State", Replicated)
		uint8 ChessType;
	UPROPERTY(EditAnywhere, Category = "State")
		UStaticMesh* OriginMesh;
	UPROPERTY(EditAnywhere, Category = "State")
		uint8 OriginChessType;
	
	UPROPERTY(Replicated)
		int CurrentPosition = -1;
	
	void ClickedPiece(class AMyPlayerController* Controllers);
	void MoveLocation(FVector location);

	UFUNCTION(NetMulticast, Reliable)
		void Highlight(bool bHighlight);
	void Highlight_Implementation(bool bHighlight);

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const;
};
