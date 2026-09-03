#include "Framework/MultiplayerMenuGameMode.h"

AMultiplayerMenuGameMode::AMultiplayerMenuGameMode()
{
	// 메뉴 맵에는 조종할 Pawn이 필요하지 않으므로 자동 스폰을 비활성화
	DefaultPawnClass = nullptr;
}
