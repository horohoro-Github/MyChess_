// Fill out your copyright notice in the Description page of Project Settings.


#include "MyOnlineBeaconHostObject.h"
#include "MyOnlineBeaconClient.h"
#include "MyChess_GameModeBase.h"
#include "Http.h"
#include "JsonObjectConverter.h"
#include "Kismet/GameplayStatics.h"

AMyOnlineBeaconHostObject::AMyOnlineBeaconHostObject()
{
	ClientBeaconActorClass = AMyOnlineBeaconClient::StaticClass();
	BeaconTypeName = ClientBeaconActorClass->GetName();

	Http = &FHttpModule::Get();
	bReplicates = true;
	
}

void AMyOnlineBeaconHostObject::OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection)
{
	Super::OnClientConnected(NewClientActor, ClientConnection);

}

void AMyOnlineBeaconHostObject::NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor)
{
	Super::NotifyClientDisconnected(LeavingClientActor);

	AMyOnlineBeaconClient* BC = Cast<AMyOnlineBeaconClient>(LeavingClientActor);

	AMyChess_GameModeBase* gamemode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (BC->Userid != "")
	{
		UE_LOG(LogTemp, Warning, TEXT("Notify Disconnected %s"), *BC->Userid);
		if (gamemode) gamemode->GetUserData(BC, NULL, BC->Userid, BC->Userpass, 0, true);
	}
	
}

AMyOnlineBeaconClient* AMyOnlineBeaconHostObject::GetBeaconsClient(FString bcid)
{
	for (int i = 0; i < ClientActors.Num(); i++)
	{
		AMyOnlineBeaconClient* BeaconClinet = Cast<AMyOnlineBeaconClient>(ClientActors[i]);

		if (BeaconClinet->Userid == bcid)
		{
			return BeaconClinet;
		}
	}
	return nullptr;
}