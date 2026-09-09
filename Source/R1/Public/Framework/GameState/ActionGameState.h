#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ActionGameState.generated.h"

// 새로운 채팅 메시지가 하나 도착했을 때 실행되는 델리게이트
// ChatWidget쪽에서 이 델리게이트를 구독할거예요
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGlobalChatMessageReceived, const FGlobalChatMessage&);

// 채팅 하나를 표현하는 데이터
USTRUCT(BlueprintType)
struct FGlobalChatMessage
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	int32 MessageID = 0; // 서버가 채팅 하나 마다 부여하는 고유 번호

	UPROPERTY(BlueprintReadOnly)
	FString SenderName; // 채팅을 보낸 플레이어 이름

	UPROPERTY(BlueprintReadOnly)
	FString Message; // 채팅 내용
};

// 전체 채팅 기록 모아둘 곳
// GameState는 서버와 모든 클라에 존재 -> 그래서 여기에 두어요
UCLASS()
class R1_API AActionGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 서버 채팅 기록에 새로운 채팅을 추가하는 함수
	void AddGlobalChatMessage(const APlayerState* SenderPlayerState, const FString& Message);

	const TArray<FGlobalChatMessage>& GetGlobalChatHistroy() { return GlobalChatHistory; }

protected:
	UFUNCTION()
	void OnRep_GlobalChatHistory(); // 서버에서 복제된 채팅 기록들을 클라가 받았을 떄 호출됨

private:
	// 아직 알리지 않은 새로운 채팅(들)을 OnGlobalChatMessageReceive 델리게이트로 전달하는 함수
	void BroadcastNewGlobalChatMessages();

public:
	// 서버, 클라 GameState에 새로운 채팅이 도착하면 호출!!
	FOnGlobalChatMessageReceived OnGlobalChatMessageReceive;

protected:
	// 서버가 보관하는 전체 채팅 기록
	UPROPERTY(ReplicatedUsing = OnRep_GlobalChatHistory, BlueprintReadOnly)
	TArray<FGlobalChatMessage> GlobalChatHistory;

private:
	int NextGlobalChatMessageID = 1.f; // 메시지에 부여할 번호(ID) (서버만 이 값을 증가시킬 수 있음)

	static constexpr int32 MaxGlobalChatHistoryCount = 100; // 서버에 보관할 최대 채팅 개수. 가장 오래된 것부터 제거됨

	int32 LastBroadcastGlobalChatMessageID = 0; // 제일 마지막으로 브로드캐스트한 채팅 메시지 ID
	// 이건 복제하지 않고 각 서버, 클라가 독립적으로 GameState에서 저장하는 값
};
