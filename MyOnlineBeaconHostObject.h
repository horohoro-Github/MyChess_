// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconHostObject.h"
#include "MyOnlineBeaconHostObject.generated.h"

/**
 * 
 */
UCLASS()
class MYCHESS__API AMyOnlineBeaconHostObject : public AOnlineBeaconHostObject
{
	GENERATED_BODY()
	
public:

	AMyOnlineBeaconHostObject();

	virtual void OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection) override;

	virtual void NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor) override;


	class  AMyOnlineBeaconClient* GetBeaconsClient(FString bcid);

	class FHttpModule* Http;
};
