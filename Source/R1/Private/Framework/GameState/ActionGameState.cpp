
#include "Framework/GameState/ActionGameState.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

void AActionGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AActionGameState, GlobalChatHistory);
}

void AActionGameState::AddGlobalChatMessage(const APlayerState* SenderPlayerState, const FString& Message)
{
	//  채팅 기록은 서버만 건들 수 있도록 함
	if (false == HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Chat][GameState] 클라이언트에서 채팅 기록 변경을 시도했습니다."));
		return;
	}

	if (nullptr == SenderPlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Chat][GameState] SenderPlayerState가 없습니다."));
		return;
	}

	FString ValidMsg = Message.TrimStartAndEnd();
	if (true == ValidMsg.IsEmpty()) return; // 빈 메시지는 의미가 없다

	FGlobalChatMessage NewMsg;

	NewMsg.MessageID = NextGlobalChatMessageID++; // ID
	NewMsg.SenderName = SenderPlayerState->GetPlayerName(); // PlayerName
	NewMsg.Message = ValidMsg;
	NewMsg.Timestamp = FDateTime::Now().ToString(TEXT("[%H:%M:%S]"));

	GlobalChatHistory.Add(NewMsg); // 서버는 최신 채팅을 들고 있게 됨

	while (GlobalChatHistory.Num() > MaxGlobalChatHistoryCount )
	{
		GlobalChatHistory.RemoveAt(0);
	}

	// 서버 플레이어쪽은 OnRep이 호출되지 않기 때문에
	// 직접 로컬 UI에게 브로드캐스트해요
	BroadcastNewGlobalChatMessages();

	ForceNetUpdate();
}

void AActionGameState::OnRep_GlobalChatHistory()
{
	if (true == GlobalChatHistory.IsEmpty()) return; // 넘길 게 없잖아요

	BroadcastNewGlobalChatMessages(); // 구독자들에게 브로드캐스트..

	// 베열의 마지막 요소가 가장 최신 채팅
	const FGlobalChatMessage& LatestMessage = GlobalChatHistory.Last();
}

void AActionGameState::BroadcastNewGlobalChatMessages()
{
	for (const FGlobalChatMessage& Chat : GlobalChatHistory)
	{
		if (LastBroadcastGlobalChatMessageID >= Chat.MessageID)
			continue; // 이미 델리게이트 쏜 채팅은 다시 전달하지 않아요

		OnGlobalChatMessageReceive.Broadcast(Chat);
		LastBroadcastGlobalChatMessageID = Chat.MessageID; // 마지막으로 보낸 값으로 저장
	}
}
