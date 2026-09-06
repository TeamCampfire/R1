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
#include "Curves/CurveFloat.h"

ADayNightCycle::ADayNightCycle()
{
	// 시간과 환경은 저주기 타이머로 갱신하므로 매 프레임 Tick을 사용하지 않아요
	PrimaryActorTick.bCanEverTick = false;

	// 서버 관련 변수 세팅
	bReplicates = true; // 서버에서 클라로 복제하겠어요
	bAlwaysRelevant = true; // 일반 액터들은 서버랑 너무 멀어지면 네트워크 복제 대상에서 제외될 수 있다고 함. 그 설정을 끄는 변수
	SetReplicateMovement(false); // 액터 트랜스폼은 복제 안 하겠다는 세팅
	SetNetUpdateFrequency(1.0f); // 네트워크 갱신 빈도를 초당 1회로 제한
}

void ADayNightCycle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADayNightCycle, CycleStartServerTime);
}

void ADayNightCycle::BeginPlay()
{
	Super::BeginPlay();

	if (true == HasAuthority())
	{
		// 서버만 사이클 기준 시각을 결정.
		CycleStartServerTime = GetWorld()->GetTimeSeconds();
		bIsTimeSynchronized = true;

		ForceNetUpdate();
	}

	if (true == bEnableDebugLog)
	{
		// (이후 타이머를 직접 제어하지 않으므로 핸들은 이 함수 안에서만 사용)
		FTimerHandle DebugTimerHandle;
		GetWorldTimerManager().SetTimer(DebugTimerHandle, this,
			&ADayNightCycle::PrintDebugTime, DebugLogInterval, true);
	}

	// 	// 데디테이티드 전용 서버는 화면을 렌더링하지 않으므로 태양을 움직이지 않아요
	if (NM_DedicatedServer != GetNetMode() &&
		(true == IsValid(SunLight) || true == IsValid(MoonLight) || true == IsValid(SkyLight)))
	{
		// Curve 출력에 곱할 낮 환경의 기준 밝기를 보관
		if(true == IsValid(SunLight))
		{
			if (ULightComponent* LightComponent = SunLight->GetLightComponent())
				CachedSunIntensity = LightComponent->Intensity;
		}

		if (true == IsValid(SkyLight))
		{
			if (USkyLightComponent* SkyLightComponent = SkyLight->GetLightComponent())
				CachedSkyLightIntensity = SkyLightComponent->Intensity;
		}

		// 타이머의 첫 호출을 기다리지 않고 현재 시각의 환경을 즉시 적용
		UpdateEnvironmentVisuals();

		// 태양 이동과 조명 Curve를 설정된 SunUpdateInterval 간격으로 갱신해요
		FTimerHandle EnvironmentUpdateTimerHandle;
		GetWorldTimerManager().SetTimer(
			EnvironmentUpdateTimerHandle,
			this,
			&ADayNightCycle::UpdateEnvironmentVisuals,
			SunUpdateInterval,
			true
		);
	}
}

float ADayNightCycle::GetCurrentHour() const
{
	if (false == bIsTimeSynchronized) return DayStartHour;

	// 현실 시간 기준의 전체 하루 길이
	const double CycleDuration = DayDurationSeconds + NightDurationSeconds;

	if (CycleDuration <= 0.0) return DayStartHour;

	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (nullptr == GameState) return DayStartHour;

	// 각 클라이언트의 접속 시점과 무관하도록 동기화된 서버 시간을 사용
	const double ElapsedSeconds = GameState->GetServerWorldTimeSeconds() - CycleStartServerTime;

	// 여러 날이 지나도 현재 하루 안의 경과 시간만 얻도록 순환시키는 식
	const double ElapsedInCycle = FMath::Fmod(FMath::Max(ElapsedSeconds, 0.0), CycleDuration);

	// 낮 시간
	if (ElapsedInCycle < DayDurationSeconds)
	{
		// 낮 구간의 진행률을 게임 시각으로 보간해요
		const float DayAlpha = static_cast<float>(ElapsedInCycle / DayDurationSeconds);
		return FMath::Lerp(DayStartHour, NightStartHour, DayAlpha);
	}

	// 밤은 자정을 지나 다음 날 DayStartHour까지 진행됩니다
	const double NightElapsed = ElapsedInCycle - DayDurationSeconds;
	const float NightAlpha = static_cast<float>(NightElapsed / NightDurationSeconds);
	const float NightGameHours = (24.0f - NightStartHour) + DayStartHour;

	return FMath::Fmod(NightStartHour + NightGameHours * NightAlpha, 24.0f);
}

bool ADayNightCycle::IsDaytime() const
{
	const float CurrentHour = GetCurrentHour();

	// 일반 구간과 자정을 통과하는 특수 구간을 모두 지원
	// 예: 낮 시작 시각 20시, 밤 시작 시각 6시 같은...
	if (DayStartHour <= NightStartHour)
		return CurrentHour >= DayStartHour && CurrentHour < NightStartHour;

	return CurrentHour >= DayStartHour || CurrentHour < NightStartHour;
}

void ADayNightCycle::SetTime(float NewHour)
{
	// 클라이언트가 로컬에서 전체 월드 시간을 변경하지 못하도록 서버에서만 실행함
	if (false == HasAuthority()) return;

	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (nullptr == GameState) return;

	// 입력값을 0 이상 24 미만으로 정규화
	// 예: 25시 → 1시, -1시 → 23시
	float NormalizedHour = FMath::Fmod(NewHour, 24.0f);

	if (NormalizedHour < 0.0f)
		NormalizedHour += 24.0f;

	double DesiredElapsedSeconds = 0.0;

	if (NormalizedHour >= DayStartHour && NormalizedHour < NightStartHour)
	{
		// 원하는 낮 시각을 사이클 시작 후 경과 초로 변환
		float DayGameHours = NightStartHour - DayStartHour;
		float DayAlpha = (NormalizedHour - DayStartHour) / DayGameHours;

		DesiredElapsedSeconds = DayAlpha * DayDurationSeconds;
	}
	else
	{
		// 자정 이후 시각은 24를 더해 18시 이후의 연속된 값으로 계산
		float AdjustedHour = NormalizedHour < DayStartHour ? NormalizedHour + 24.0f : NormalizedHour;

		float NightGameHours = (24.0f - NightStartHour) + DayStartHour;
		float NightAlpha = (AdjustedHour - NightStartHour) / NightGameHours;

		DesiredElapsedSeconds = DayDurationSeconds + NightAlpha * NightDurationSeconds;
	}

	const double CurrentServerTime = GameState->GetServerWorldTimeSeconds();

	// 현재 서버 시각에서 원하는 진행량을 빼 새 사이클 기준 시각을 역산한다.
	CycleStartServerTime = CurrentServerTime - DesiredElapsedSeconds;
	ForceNetUpdate();
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
	// 오전 6시를 지평선(0도)으로 두고 시간당 15도(360/24)씩 이동시킴
	float SunOrbitDegrees = (CurrentHour - 6.0f) * 15.0f;

	// 일출/일몰 0, 정오 1, 자정 -1인 높이값. 현재는 태양 색상 fallback에 사용
	float SunHeight = FMath::Sin(FMath::DegreesToRadians(SunOrbitDegrees));

	// Curve가 없으면 태양 높이를 이용한 기본 밝기 계산으로 대체해요
	const float SunIntensityAlpha = IsValid(SunIntensityCurve) ?
		FMath::Clamp(SunIntensityCurve->GetFloatValue(CurrentHour), 0.0f, 1.0f)
		: FMath::SmoothStep(0.0f, 0.25f,SunHeight);

	// 태양의 위치, 직접광 밝기, 색상을 갱신해요
	if (true == IsValid(SunLight))
	{
		if (ULightComponent* LightComponent = SunLight->GetLightComponent())
		{
			// Directional Light가 하늘에서 지면을 향하도록 궤도 각도의 부호를 반전한다.
			float SunPitch = -SunOrbitDegrees;
			FRotator SunRotation(SunPitch, SunYaw, 0.0f);
			SunLight->SetActorRotation(SunRotation);

			LightComponent->SetIntensity(CachedSunIntensity * SunIntensityAlpha);

			// 지평선에서는 석양색, 높이 올라가면 낮 색상으로 보간해요
			float SunColorAlpha = FMath::SmoothStep(0.0f, 0.5f, SunHeight);

			const FLinearColor CurrentSunColor = FLinearColor::LerpUsingHSV(HorizonSunColor, DaySunColor, SunColorAlpha);
			LightComponent->SetLightColor(CurrentSunColor);
		}
	}

	// 밤 보조광(달)은 실제 달 궤도와 분리해 항상 지면을 비추는 고정 각도를 사용
	if (true == IsValid(MoonLight))
	{
		if (ULightComponent* MoonLightComponent = MoonLight->GetLightComponent())
		{
			MoonLight->SetActorRotation(FRotator(MoonLightPitch, SunYaw + 180.0f, 0.0f));

			// Curve가 없으면 낮에는 끄고 밤에는 최대 밝기를 사용하는 단순 fallback을 적용
			const float NightFillAlpha = IsValid(NightFillIntensityCurve) ?
				FMath::Clamp(NightFillIntensityCurve->GetFloatValue(CurrentHour), 0.0f, 1.0f) : (IsDaytime() ? 0.0f : 1.0f);
			MoonLightComponent->SetIntensity(MaxNightFillIntensity * NightFillAlpha);
		}
	}

	// Sky Light는 시간 Curve로 전체 환경광과 반사의 밝기를 조절
	if (true == IsValid(SkyLight))
	{
		if (USkyLightComponent* SkyLightComponent = SkyLight->GetLightComponent())
		{
			// Curve가 없으면 에디터에서 설정한 기준 밝기를 그대로 유지
			const float SkyLightIntensityAlpha = IsValid(SkyLightIntensityCurve)?
				FMath::Clamp(SkyLightIntensityCurve->GetFloatValue(CurrentHour), 0.0f, 1.0f) : 1.0f;

			// Intensity는 빛 밝기를 만들어주는 게 아님 !!!!! 화면에 캡쳐된 하늘 조명색에 곱하는 값인 것.. (자체적인 빛 생성이 없어요)
			SkyLightComponent->SetIntensity(CachedSkyLightIntensity * SkyLightIntensityAlpha);
		}
	}
}
