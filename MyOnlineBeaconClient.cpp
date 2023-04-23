// Fill out your copyright notice in the Description pge of Project Settings.


#include "MyOnlineBeaconClient.h"
#include "MyPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MyChess_GameModeBase.h"
#include "JsonObjectConverter.h"
#include "MyGameInstance.h"
#include "MyOnlineBeaconHostObject.h"
#include "Net/UnrealNetwork.h"
#include "MyServerSearchWidget.h"
#include "MyServerSearchDataWidget.h"
#include "Engine/LevelStreaming.h"
#include "MyMenuLoginWidget.h"

AMyOnlineBeaconClient::AMyOnlineBeaconClient()
{
	bReplicates = true;
}

bool AMyOnlineBeaconClient::ConnectToServer(const FString& address)
{

	FURL Destination = FURL(nullptr, *address, ETravelType::TRAVEL_Absolute);
	Destination.Port = 7787;
	return InitClient(Destination);
	
}

void AMyOnlineBeaconClient::OpenLevelClient_Implementation(const FString& port, const FString& id, const FString& pass)
{
	FString Optionstr = TEXT("Id=" + id + "?Pass=" + pass);
	if (gameins) gameins->OpenLevel(port, Optionstr);
	DestroyBeacon();
}

void AMyOnlineBeaconClient::OnFailure()
{
	UE_LOG(LogTemp, Warning, TEXT("Connected Fail"));
	
	Destroy();
}

void AMyOnlineBeaconClient::OnConnected()
{
	UE_LOG(LogTemp, Warning, TEXT("Connected Success"));
	SetLevelName(LevelName, PortNum);

	PC = Cast<AMyPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (UGameplayStatics::GetCurrentLevelName(GetWorld(), true) != FString("LoadingMap"))
	{
		if (IsRunningDedicatedServer())
		{
			
			if (UGameplayStatics::GetCurrentLevelName(GetWorld(), true) == "MainMap")
			{
				OpenSubDedicatedCheck();
			}
			
		}
		else
		{
			if (PC)
			{
				PS = Cast<AMyPlayerState>(PC->PlayerState);

				if (gameins)
				{
					if (Userid != "" && Userpass != "") LoginAttempt(Userid, Userpass, 1, bTraveling, gameins->GetPrimaryPlayerUniqueIdRepl()->ToString());
				}
			}
		}
	}
}

void AMyOnlineBeaconClient::OpenLevelWithPlayerData_Implementation(const FString& url)
{
	if (IsRunningDedicatedServer())
	{
		AMyChess_GameModeBase* gamemode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
		if (gamemode)
		{
			gamemode->GetPlayerDataInfo(this, url);
		}
	}
	else
	{
		OpenLevelClient(url, "", "");
	}
}

void AMyOnlineBeaconClient::RenamePlayer_Server_Implementation(const FString& name)
{
	AMyChess_GameModeBase* gamemode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	
	if (gamemode) gamemode->RenameCheckPlayerForBeaconClient(this, name);
}

void AMyOnlineBeaconClient::RenamePlayer_Client_Implementation(const FString& name)
{
	if (gameins)
	{
		gameins->PlayerName = name;
		if (gameins->MyPC) gameins->MyPC->LobbyWidgetChangeIndex(1);
	}
}

void AMyOnlineBeaconClient::GetServerPort_Server_Implementation(int MatchNumber)
{
	AMyChess_GameModeBase* gamemode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (gamemode) gamemode->GetServerPortForBeaconClient(this, NULL, MatchNumber, "");
}

void AMyOnlineBeaconClient::ServerSearchClient_Implementation(const FString& response)
{
	
	TArray<FServersData> serverdata;
	FJsonObjectConverter::JsonArrayStringToUStruct(response, &serverdata, 0, 0);

	if (PC->SearchWidget)
	{
		UMyServerSearchWidget* ServerSearchWidget = Cast<UMyServerSearchWidget>(PC->SearchWidget);
		if (ServerSearchWidget)
		{
			for (int i = 0; i < serverdata.Num(); i++)
			{
				if (serverdata[i].Ports != "7777")
				{
					PC->SearchDataWidget = CreateWidget<UUserWidget>(GetWorld(), PC->SearchDataWidgetClass);
					//SearchDataWidget = Get
					UMyServerSearchDataWidget* ServerSearchDataWidget = Cast<UMyServerSearchDataWidget>(PC->SearchDataWidget);
					ServerSearchDataWidget->ServerData = serverdata[i];
					ServerSearchWidget->ScrollBox->AddChild(ServerSearchDataWidget);
				}
			}
		}
	}
	
	
}

void AMyOnlineBeaconClient::FindServerPortFromServer_Implementation(const FString& port)
{
	AMyChess_GameModeBase* GameModeBase = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GameModeBase) GameModeBase->FindServerPortForBeaconClient(this, port);
}

void AMyOnlineBeaconClient::LoadingStreamLevel_Implementation(bool bLoad)
{
	PC = Cast<AMyPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (bLoad == false)
	{
		if (gameins) GetWorld()->GetTimerManager().ClearTimer(gameins->h_timer);
		if (PC) PC->FadeScreen(1);
	}
	else
	{
		if (gameins) GetWorld()->GetTimerManager().ClearTimer(gameins->h_timer);
		if (PC) PC->FadeScreen(2);
	}
}

void AMyOnlineBeaconClient::RejectedLogin_Implementation(bool bLogin, int RejectedType)
{
	PC = Cast<AMyPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (bLogin == true)
	{
		if (PC)
		{
			UMyMenuLoginWidget* menu = Cast<UMyMenuLoginWidget>(PC->CurrentWidget);
			if (menu)
			{
				menu->Button_Login->bIsEnabled = true;
				menu->Button_LoginBack->bIsEnabled = true;
				menu->Button_Register->bIsEnabled = true;
				menu->Button_RegisterBack->bIsEnabled = true;
			}
		}
	}
	else
	{
		/*
		if (PC)
		{
			UMyMenuLoginWidget* menu = Cast<UMyMenuLoginWidget>(PC->CurrentWidget);
			if (menu)
			{
				menu->Button_Login->bIsEnabled = true;
				menu->Button_LoginBack->bIsEnabled = true;
				menu->Button_Register->bIsEnabled = true;
				menu->Button_RegisterBack->bIsEnabled = true;
			}
		}
		*/
	}
	gameins = Cast<UMyGameInstance>(GetGameInstance());
	if (gameins)
	{
		switch (RejectedType)
		{
		case 0:
		{
			gameins->MessageDelivary("Can't connect to server.");
			break;
		}
		case 1:
		{
			gameins->MessageDelivary("ID already exists.");
			break;
		}
		case 2:
		{
			gameins->MessageDelivary("ID is already logged in.");
			break;
		}
		case 3:
		{
			gameins->MessageDelivary("ID does not exist.");
			break;
		}
		case 4:
		{
			gameins->MessageDelivary("nickname already exists.");
			break;
		}
		default:
			break;
		}
	}
}

void AMyOnlineBeaconClient::GetAccount_Implementation(const FString& id, const FString& pass)
{
	AMyChess_GameModeBase* gamemode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (gamemode)
	{
		gamemode->GetUserData(this, NULL, id, pass, 3, bTraveling);
	}
	
}

void AMyOnlineBeaconClient::CreateAccount_Implementation(const FString& id, const FString& Pass)
{
	if (gameins)
	{
		gameins->UserId = id;
		AccountRegistration(id, Pass);
	}
}

void AMyOnlineBeaconClient::AccountRegistration_Implementation(const FString& id, const FString& pass)
{
	AMyChess_GameModeBase* gamemode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (gamemode)
	{
		gamemode->AddUserData(this, id, pass);
	}
}



void AMyOnlineBeaconClient::LoginAttempt_Implementation(const FString& id, const FString& pass, int LoginState,  bool bTravel, const FString& uid)
{
	AMyChess_GameModeBase* gamemode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (gamemode)
	{
		InstanceUID = uid;
		gamemode->GetUserData(this, NULL, id, pass, LoginState, false);
	}
}

void AMyOnlineBeaconClient::Login_C_Implementation(const FString& id, const FString& pass, int LoginState)
{
	if (gameins)
	{
		gameins->bTraveling = true;
		gameins->UserId = id;
		gameins->UserPass = pass;
		Userid = id;
		Userpass = pass;
		if (UGameplayStatics::GetCurrentLevelName(GetWorld(), true) == FString("LobbyLevel"))
		{
			if (UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport())
			{
				ViewportClient->RemoveAllViewportWidgets();
			}

			if (PC)
			{
				PC->WidgetState = EWidgetState::LOBBY;
				PC->ChangeWidget();
			}
			else
			{
				PC = Cast<AMyPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
				if (PC)
				{
					PC->WidgetState = EWidgetState::LOBBY;
					PC->ChangeWidget();
				}
			}
		}
	}
}

void AMyOnlineBeaconClient::RegisterComplete_Implementation(const FString& tempid)
{
	if (gameins)
	{
		gameins->MyPC->LoginWidgetChangeIndex(3, tempid);
	}
}

void AMyOnlineBeaconClient::SetPlayerData_Implementation(const FString& name, int win, int draw, int lose, const FString& id, const FString& Strfriends, const FString& Strfriendsres, const FString& Strfriendsreq, bool bOnlyFriendsData)
{
	if (gameins)
	{
		if (bOnlyFriendsData == false)
		{
			gameins->PlayerName = name;
			gameins->PlayerWin = win;
			gameins->PlayerDraw = draw;
			gameins->PlayerLose = lose;
			gameins->UserId = id;
			gameins->Friends = Strfriends;
			gameins->FriendsRes = Strfriendsres;
			gameins->FriendsReq = Strfriendsreq;
		}
		else
		{
			gameins->Friends = Strfriends;
			gameins->FriendsRes = Strfriendsres;
			gameins->FriendsReq = Strfriendsreq;
			GetFriendsList_Server(gameins->UserId, gameins->Friends, gameins->FriendsRes, gameins->FriendsReq);
		}
	}
}

void AMyOnlineBeaconClient::LobbyWidgetChangerIndex_Implementation(int index)
{
	if (gameins)
	{
		if (gameins->MyPC) gameins->MyPC->LobbyWidgetChangeIndex(index);
	}
}

void AMyOnlineBeaconClient::AddMessageDataWidget_Implementation(const FString& id, const FString& name, bool bInvite, const FString& message, bool bSender)
{
	if (gameins)
	{
		if (id != "")
		{
			
			if (bInvite == true)
			{
				FString str = "";
				FString str2 = TEXT("초대를 받음");
				str = TEXT("(" + id + ") " + name + " " + str2);
				if (messages.Num() == 0)
				{
					FMessageStruct mess;
					mess.MESSAGE = str;
					mess.ACTIVE = bInvite;
					messages.Add(mess);
				}
				else
				{
					for (int i = 0; i < messages.Num(); i++)
					{
						if (messages[i].ACTIVE == true)
						{
							if (messages[i].MESSAGE == str)
							{
								messages.RemoveAt(i);

								break;
							}
						}
					}
					FMessageStruct mess;
					mess.MESSAGE = str;
					mess.ACTIVE = bInvite;
					messages.Add(mess);
				}
			}
			else
			{
				FString str = "";
				if (message != "")
				{
					if (bSender == true)
					{
						FString str2 = TEXT("보냄");
						str = TEXT("" + str2 + " (" + id + ") " + name + ": " + message);
					}
					else
					{
						FString str2 = TEXT("받음");
						str = TEXT("" + str2 + " (" + id + ") " + name + ": " + message);
					}
					
					FMessageStruct mess;
					mess.MESSAGE = str;
					mess.ACTIVE = bInvite;

					messages.Add(mess);
				}
				else
				{
					FString str2 = TEXT("초대를 보냄");
					str = TEXT("(" + id + ") " + name + " " + str2);
					
					if (messages.Num() > 0)
					{
						for (int i = 0; i < messages.Num(); i++)
						{
							if (messages[i].MESSAGE == str)
							{
								messages.RemoveAt(i);
								break;
							}
						}
						FMessageStruct mess;
						mess.MESSAGE = str;
						mess.ACTIVE = bInvite;

						messages.Add(mess);
					}
					else
					{
						FMessageStruct mess;
						mess.MESSAGE = str;
						mess.ACTIVE = bInvite;

						messages.Add(mess);
					}
				}
	
				
			}
		}
		
		gameins->AddMessage(messages);
	}
}

void AMyOnlineBeaconClient::GetFriendsList_Implementation(const TArray<FString>& idarray, const TArray<FString>& namearray)
{
	if (gameins)
	{

		gameins->UpdateFriendsList(idarray, namearray); //임시로 초대기능으로도 사용
	}
}

void AMyOnlineBeaconClient::GetFriendsList_Server_Implementation(const FString& id, const FString& StrFriends, const FString& StrFriendsres, const FString& StrFriendsreq)
{
	Userid = id;
	Friends = StrFriends;
	FriendsRes = StrFriendsres;
	FriendsReq = StrFriendsreq;
	AMyChess_GameModeBase* gamemode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (gamemode)
	{
		gamemode->GetPlayerDataForClient(NULL, this, Userid, 4,"");
	}
}

void AMyOnlineBeaconClient::UpdateFriendListData_Implementation(const FString& Targetid, const FString& id, const FString& name, const FString& StrFriends, const FString& StrFriendsres, const FString& StrFriendsreq, int IFriend, const FString& Messages)
{
	Userid = id;
	PlayerName = name;
	Friends = StrFriends;
	FriendsRes = StrFriendsres;
	FriendsReq = StrFriendsreq;
	AMyChess_GameModeBase* gamemode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (gamemode)
	{
		gamemode->GetPlayerDataForClient(NULL, this, Targetid, IFriend, Messages);
	}
}

void AMyOnlineBeaconClient::OpenSubDedicatedCheck_Implementation()
{
	bGameServerBeacon = true;
	AMyChess_GameModeBase* Gamemode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	if (Gamemode) Gamemode->ServerLoaded();
}

void AMyOnlineBeaconClient::ReLoginBeacon_Implementation(const FString& url, const FString& id, const FString& pass)
{
	AMyChess_GameModeBase* Gamemode = Cast<AMyChess_GameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));

	if (Gamemode) Gamemode->ReLoginUserData(this, Userid, Userpass);
	OpenLevelWithPlayerData(url);
}

void AMyOnlineBeaconClient::SetLevelName_Implementation(const FString& name, const FString& port)
{
	LevelName = name;
	PortNum = port;
}
