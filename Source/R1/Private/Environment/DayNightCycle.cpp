/// 최초작성 : 2026.09.05
/// 작 성 자 : 우 진

#include "Environment/DayNightCycle.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"

ADayNightCycle::ADayNightCycle()
{
 	PrimaryActorTick.bCanEverTick = false; // 틱 꺼버리기

	// 서버 관련 변수 세팅
	bReplicates = true; // 서버에서 클라로 복제하겠어요
	bAlwaysRelevant = true; // 일반 액터들은 서버랑 너무 멀어지면 네트워크 복제 대상에서 제외될 수 있다고 함. 그 설정을 끄는 변수
	SetReplicateMovement(false); // 액터 트랜스폼은 복제 안 하겠다는 세팅
	SetNetUpdateFrequency(1.0f); // 네트워크 갱신 빈도를 초당 1회로 제한
}

void ADayNightCycle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADayNightCycle, CycleStartServerTime); // CycleStartServerTime 복제
}

void ADayNightCycle::BeginPlay()
{
	Super::BeginPlay();

	if (true == HasAuthority())
	{
		CycleStartServerTime = GetWorld()->GetTimeSeconds(); // 월드가 시작된 후 몇 초가 흘렀는지 반환하는 함수예요
		bIsTimeSynchronized = true; // 서버는 OnRep 함수가 호출되지 않으므로 직접 설정

		ForceNetUpdate();
	}

	if (true == bEnableDebugLog)
	{
		GetWorldTimerManager().SetTimer(DebugTimerHandle, this,
			&ADayNightCycle::PrintDebugTime, DebugLogInterval, true);
	}

	// 태양 타이머 스타트
	// 데디테이티드 전용 서버는 화면을 렌더링하지 않으므로 태양을 움직이지 않음
	if (NM_DedicatedServer != GetNetMode() &&
		(true == IsValid(SunLight) || true == IsValid(MoonLight) || true == IsValid(SkyLight)))
	{
		// 에디터에서 설정한 태양 밝기를 최대 밝기로 변수로 저장
		if(true == IsValid(SunLight))
		{
			if (ULightComponent* LightComponent = SunLight->GetLightComponent())
				CachedSunIntensity = LightComponent->Intensity;
		}

		//// 에디터에서 설정한 달 밝기를 최대 밝기로 저장
		//if (true == IsValid(MoonLight))
		//{
		//	if (ULightComponent* MoonLightComponent = MoonLight->GetLightComponent())
		//		CachedMoonIntensity = MoonLightComponent->Intensity;
		//}

		// 에디터에서 설정한 환경광 밝기를 최대 밝기로 변수로 저장
		if (true == IsValid(SkyLight))
		{
			if (USkyLightComponent* SkyLightComponent = SkyLight->GetLightComponent())
				CachedSkyLightIntensity = SkyLightComponent->Intensity;
		}

		// 게임 시작 즉시 태양 위치를 한 번 적용
		UpdateEnvironmentVisuals();

		// 이후 0.1초 간격으로 태양 위치 갱신
		GetWorldTimerManager().SetTimer(
			SunUpdateTimerHandle,
			this,
			&ADayNightCycle::UpdateEnvironmentVisuals,
			SunUpdateInterval,
			true
		);
	}
}

float ADayNightCycle::GetCurrentHour() const
{
	// 클라이언트가 서버 기준 시각을 받기 전에는 안전하게 낮 시작 시각을 반환
	if (false == bIsTimeSynchronized) return DayStartHour;
	//if (CycleStartServerTime < 0.0) return DayStartHour; // 아직 서버의 기준 시각을 받지 못했으면(-1) 낮 시작 시각을 반환

	// 총 하루의 초 (전체 하루 사이클 시간)
	const double CycleDuration = DayDurationSeconds + NightDurationSeconds;

	if (CycleDuration <= 0.0) return DayStartHour;

	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (nullptr == GameState) return DayStartHour;

	// 클라마다 월드에 들어온 시점이 다를 수 있음
	// GetServerWorldTimeSeconds() 함수를 통해 클라이언트에서도 동기화된 서버 기준 시각을 받을 수 있음

	// 동기화된 서버 현재 시각 - 복제받은 사이클 시작 시간
	const double ElapsedSeconds = GameState->GetServerWorldTimeSeconds() - CycleStartServerTime;

	// 현재 사이클 안에서의 위치
	const double ElapsedInCycle = FMath::Fmod(FMath::Max(ElapsedSeconds, 0.0), CycleDuration);

	// 낮 시간
	if (ElapsedInCycle < DayDurationSeconds)
	{
		// 낮을 0-1 로 표현, 0이면 낮 시작, 1이면 낮 종료
		const float DayAlpha = static_cast<float>(ElapsedInCycle / DayDurationSeconds);
		return FMath::Lerp(DayStartHour, NightStartHour, DayAlpha);
	}

	// 밤 시간
	const double NightElapsed = ElapsedInCycle - DayDurationSeconds; // 낮시간을 빼서 밤이 시작된 후 흐른 시간만 구해요
	const float NightAlpha = static_cast<float>(NightElapsed / NightDurationSeconds); // 0-1 사이
	const float NightGameHours = (24.0f - NightStartHour) + DayStartHour;

	return FMath::Fmod(NightStartHour + NightGameHours * NightAlpha, 24.0f);
}

bool ADayNightCycle::IsDaytime() const
{
	const float CurrentHour = GetCurrentHour();

	// 일반적인 설정: 06:00부터 18:00까지 낮
	if (DayStartHour <= NightStartHour)
		return CurrentHour >= DayStartHour && CurrentHour < NightStartHour;

	// 낮 구간이 자정을 통과하는 특수 설정
	// 예: 낮 시작 시각 20시, 밤 시작 시각 6시 같은...
	return CurrentHour >= DayStartHour || CurrentHour < NightStartHour;
}

void ADayNightCycle::SetTime(float NewHour)
{
	if (false == HasAuthority()) return; // 시간 변경 권한은 서버만 가져요

	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (nullptr == GameState) return;

	// 입력값을 0 이상 24 미만으로 정규화
	// 예: 25시 → 1시, -1시 → 23시
	float NormalizedHour = FMath::Fmod(NewHour, 24.0f);

	if (NormalizedHour < 0.0f)
		NormalizedHour += 24.0f;

	double DesiredElapsedSeconds = 0.0; // 원하는 시간

	if (NormalizedHour >= DayStartHour && NormalizedHour < NightStartHour)
	{
		// 낮 구간에서 얼마나 진행됐는지 계산
		float DayGameHours = NightStartHour - DayStartHour;
		float DayAlpha = (NormalizedHour - DayStartHour) / DayGameHours;

		// 원하는 시간에 대한 초를 얻어와요
		DesiredElapsedSeconds = DayAlpha * DayDurationSeconds;
	}
	else
	{
		// 자정 이후 시각은 계산하기 쉽게 24를 더함
		// 예: 새벽 2시 → 26시
		float AdjustedHour = NormalizedHour < DayStartHour ? NormalizedHour + 24.0f : NormalizedHour;

		float NightGameHours = (24.0f - NightStartHour) + DayStartHour;
		float NightAlpha = (AdjustedHour - NightStartHour) / NightGameHours;

		// 원하는 시간에 대한 초를 얻어와요
		DesiredElapsedSeconds = DayDurationSeconds + NightAlpha * NightDurationSeconds;
	}

	const double CurrentServerTime = GameState->GetServerWorldTimeSeconds();

	// 현재 서버 시각에서 원하는 진행 시간을 역산해서 새로운 사이클 시작 시각을 세팅해요
	CycleStartServerTime = CurrentServerTime - DesiredElapsedSeconds;
	ForceNetUpdate(); // 빠르게 복제
}

void ADayNightCycle::OnRep_CycleStartServerTime()
{
	// 서버가 CycleStartServerTime을 변경
	// -> 네트워크 복제!
	// -> 클라이언트가 새 값 수신
	// ---> OnRep_CycleStartServerTime() 자동 호출!
	// 해서 여기에 들어온 거예요
	// == 클라이언트가 서버의 기준 시각을 받았다는 뜻 !
	bIsTimeSynchronized = true;
}

void ADayNightCycle::PrintDebugTime()
{
	const float CurrentHour = GetCurrentHour();

	int32 Hour = FMath::FloorToInt(CurrentHour);
	int32 Minute = FMath::FloorToInt((CurrentHour - Hour) * 60.0f);
	const TCHAR* NetworkSide = HasAuthority() ? TEXT("Server") : TEXT("Client");
	const TCHAR* DayState = IsDaytime() ? TEXT("Day") : TEXT("Night");

	UE_LOG(LogTemp, Log, TEXT("[%s] Game Time %02d:%02d / %s / Synced: %s"),
		NetworkSide, Hour, Minute, DayState, bIsTimeSynchronized ? TEXT("True") : TEXT("False"));

	if (false == IsValid(SkyLight))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkyLight] Reference is invalid"));
		return;
	}

	const USkyLightComponent* SkyLightComponent = SkyLight->GetLightComponent();
	if (false == IsValid(SkyLightComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkyLight] Component is invalid"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[SkyLight] Cached: %.3f / Current: %.3f"),
		CachedSkyLightIntensity, SkyLightComponent->Intensity);
}

void ADayNightCycle::UpdateEnvironmentVisuals()
{
	const float CurrentHour = GetCurrentHour();
	// 6시: 0도 / 12시: -90도 / 18시: -180도 / 0시: 90도

	// 게임 시각을 태양 각도로 변환시켜요
	// (오전 6시를 0도로 옮김) * 시간당 15도(24/360)
	float SunOrbitDegrees = (CurrentHour - 6.0f) * 15.0f;

	// 태양 높이를 -1~1 범위로 계산
	// 일출/일몰: 0, 정오: 1, 자정: -1
	float SunHeight = FMath::Sin(FMath::DegreesToRadians(SunOrbitDegrees));

	// 태양 높이를 0-1 밝기 비율로 전환 (0~0.25 구간에서 밝기를 부드럽게)
	// 그치만 수평으로 비출 때 바닥에 도달하는 빛이 거의 없기 때문에
	// 태양빛이 지평선 아래로 내려간 뒤까지 서서히 빛은 약간 남도록 해야 함 --> 이게 트와일라잇!!
	float DaylightAlpha = FMath::SmoothStep(-0.1f, 0.25f, SunHeight);

	// 태양 밝기 갱신
	if (true == IsValid(SunLight))
	{
		if (ULightComponent* LightComponent = SunLight->GetLightComponent())
		{
			float SunPitch = -SunOrbitDegrees; // 동쪽에서 올라오게 하려고 마이너스 붙였어요

			// SunPitch: 시간에 따른 태양의 높이
			// SunYaw : 태양이 뜨고 지는 동서 방향
			// 0.0f : 태양을 옆으로 기울일 필요가 없어서 Roll은 0
			FRotator SunRotation(SunPitch, SunYaw, 0.0f);
			SunLight->SetActorRotation(SunRotation);

			LightComponent->SetIntensity(CachedSunIntensity * DaylightAlpha);

			// 태양이 지평선에서 멀어질수록 주황색에서 일반적인 낮 색상으로 변경
			float SunColorAlpha = FMath::SmoothStep(0.0f, 0.5f, SunHeight); // 색 혼합

			const FLinearColor CurrentSunColor = FLinearColor::LerpUsingHSV( // 두 색상을 비율에 맞춰 섞는 함수
			HorizonSunColor, DaySunColor, SunColorAlpha);

			LightComponent->SetLightColor(CurrentSunColor);
		}
	}

	// 달 갱신
	if (true == IsValid(MoonLight))
	{
		if (ULightComponent* MoonLightComponent = MoonLight->GetLightComponent())
		{
			// 달은 태양의 반대편에 위치
			//float MoonOrbitDegrees = SunOrbitDegrees + 180.0f;

			//float MoonPitch = -MoonOrbitDegrees;
			//MoonLight->SetActorRotation(FRotator(MoonPitch, SunYaw, 0.0f));

			// 태양이 지면 아래로 내려가면 달은 지면 위로 올라옴
			//float MoonHeight = -SunHeight;

			// 달빛은 항상 지면을 비출 수 있는 각도를 유지
			MoonLight->SetActorRotation(FRotator(MoonLightPitch, SunYaw + 180.0f, 0.0f));

			// 달이 지평선에 도달하기 조금 전부터 달빛을 켬
			// 달이 지평선에 도달하면 최대 밝기!
			// 이러지 않으면.. 밤이 되었을 때 암흑이었다가 밤중에 달 밝기가 변함..
			//const float MoonlightAlpha = FMath::SmoothStep(-0.1f, 0.f, MoonHeight);

			// 태양이 낮아지기 시작하면 달빛이 미리 섞임! 일몰 순간에는 달빛이 이미 대부분 켜져 있음
			const float MoonlightAlpha = 1.0f - FMath::SmoothStep(-0.05f, 0.15f, SunHeight);
			MoonLightComponent->SetIntensity(MaxMoonIntensity * MoonlightAlpha);
		}
	}

	// 환경광 갱신
	if (true == IsValid(SkyLight))
	{
		if (USkyLightComponent* SkyLightComponent = SkyLight->GetLightComponent())
		{
			// 낮 밝기 가쟈와서 Multiplier만큼 밤 밝기에 적용
			const float SkyLightMultiplier = FMath::Lerp(
				NightSkyLightIntensityMultiplier, 1.0f, DaylightAlpha);

			// 밝기를 새로 만들어주는 게 아님
			// 화면에 캡쳐된 하늘 색에 곱하는 값인 것..
			SkyLightComponent->SetIntensity(CachedSkyLightIntensity * SkyLightMultiplier);
		}
	}

	// =-=================================================================================
	//if (false == IsValid(SunLight)) return;

	//ULightComponent* LightComponent = SunLight->GetLightComponent();
	//if (false == IsValid(LightComponent)) return;

	//const float CurrentHour = GetCurrentHour();
	//// 6시: 0도 / 12시: -90도 / 18시: -180도 / 0시: 90도

	//// 게임 시각을 태양 각도로 변환시켜요
	//// (오전 6시를 0도로 옮김) * 시간당 15도(24/360)
	//float SunOrbitDegrees = (CurrentHour - 6.0f) * 15.0f;
	//float SunPitch = -SunOrbitDegrees; // 동쪽에서 올라오게 하려고 마이너스 붙였어요

	//// SunPitch: 시간에 따른 태양의 높이
	//// SunYaw : 태양이 뜨고 지는 동서 방향
	//// 0.0f : 태양을 옆으로 기울일 필요가 없어서 Roll은 0
	//FRotator SunRotation(SunPitch, SunYaw, 0.0f);
	//SunLight->SetActorRotation(SunRotation);

	// // 태양 높이를 -1~1 범위로 계산
	//// 일출/일몰: 0, 정오: 1, 자정: -1
	//float SunHeight = FMath::Sin(FMath::DegreesToRadians(SunOrbitDegrees));

	//// 태양 높이 0~0.25 구간에서 밝기를 부드럽게 전환
	//float DaylightAlpha = FMath::SmoothStep(0.0f, 0.25f, SunHeight);
	//LightComponent->SetIntensity(CachedSunIntensity * DaylightAlpha);

	//// 태양이 지평선에서 멀어질수록 주황색에서 일반적인 낮 색상으로 변경
	//float SunColorAlpha = FMath::SmoothStep(0.0f, 0.5f, SunHeight); // 색 혼합

	//const FLinearColor CurrentSunColor = FLinearColor::LerpUsingHSV( // 두 색상을 비율에 맞춰 섞는 함수
	//		HorizonSunColor, DaySunColor, SunColorAlpha);

	//LightComponent->SetLightColor(CurrentSunColor);
}
