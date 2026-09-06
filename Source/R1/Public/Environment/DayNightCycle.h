/// 최초작성 : 2026.09.05
/// 작 성 자 : 우 진
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DayNightCycle.generated.h"

UCLASS()
class R1_API ADayNightCycle : public AActor
{
	GENERATED_BODY()
	
public:	
	ADayNightCycle();

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// 틱 삭제했습니다
	virtual void BeginPlay() override;

public:
	// 0-24시 게임 시간으로 변횐한 현재 시간을 갖고 오는 함수
	UFUNCTION(BlueprintPure, Category = "Day Night")
	float GetCurrentHour() const;

	// 현재 시간이 낮 시간인지? true : 낮
	UFUNCTION(BlueprintPure, Category = "Day Night")
	bool IsDaytime() const;

	// 서버에서 현재 게임 시각을 변경하는 함수
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Day Night")
	void SetTime(float NewHour);

protected:
	// 클라이언트가 서버의 사이클 시작 시각을 받았을 때 언리얼 네트워크 시스템이 자동으로 호출하는 함수
	UFUNCTION()
	void OnRep_CycleStartServerTime(); // CycleStartServerTime

	//! 테스트용 디버그 시간 출력 함수
	void PrintDebugTime();

	// 현재 게임 시각에 맞춰 태양을 회전하는 함수
	void UpdateEnvironmentVisuals();

private:
	// =========================================================================
protected:

	// 시간
	UPROPERTY(EditAnywhere, Category = "Day Night|Time")
	float DayStartHour = 6.f; // 게임에서 낮이 시작되는 시각 : 아침 6시

	UPROPERTY(EditAnywhere, Category = "Day Night|Time")
	float NightStartHour = 18.f; // 밤이 시작되는 시각 : 저녁 18시

	UPROPERTY(EditAnywhere, Category = "Day Night|Time", meta = (ClampMin = "1.0"))
	float DayDurationSeconds = 45.f; //2700.f; // 현실 기준 낮의 길이: 45분

	UPROPERTY(EditAnywhere, Category = "Day Night|Time", meta = (ClampMin = "1.0"))
	float NightDurationSeconds = 15.f; //900.f; // 현실 기준 밤의 길이: 15분

	// 서버의 값이 클라이언트에 도착하면 OnRep_CycleStartServerTime()이 호출됨
	UPROPERTY(ReplicatedUsing = OnRep_CycleStartServerTime)
	double CycleStartServerTime = -1.f; // 밤낮 사이클이 서버에서 언제 시작됐는지.. -1은 아직 서버의 시작 시각을 받지 못했다는 뜻

	// 서버 기준 시각을 사용할 준비가 됐는지 나타냄
	bool bIsTimeSynchronized = false; // 동기화 여부 담당

	FTimerHandle DebugTimerHandle; // 현재 시간을 출력하는 타이머

	UPROPERTY(EditAnywhere, Category = "Day Night")
	bool bEnableDebugLog = true; //! 테스트용 / 에디터에서 디버그 로그 사용 여부를 선택

	UPROPERTY(EditAnywhere, Category = "Day Night", meta = (ClampMin = "0.1"))
	float DebugLogInterval = 1.0f; //! 테스트용 / 로그를 출력할 시간 간격

	// 태양
	UPROPERTY(EditInstanceOnly, Category = "Day Night|Lighting")
	TObjectPtr<class ADirectionalLight> SunLight; // 레벨에서 태양 역할을 하는 Directional Light

	FTimerHandle SunUpdateTimerHandle; // 태양 갱신 타이머

	UPROPERTY(EditAnywhere, Category = "Day Night|Lighting", meta = (ClampMin = "0.02"))
	float SunUpdateInterval = 0.1f; // 태양을 갱신하는 현실!! 시간 간격

	UPROPERTY(EditAnywhere, Category = "Day Night|Lighting")
	float SunYaw = 0.0f; // 태양이 동쪽에서 서쪽으로 움직이는? 지나가는? 뜨고 지는? 방향 (에디터에서 세팅)

	float CachedSunIntensity = 0.0f;  // 에디터에서 설정한 태양의 원래 최대 밝기

	UPROPERTY(EditAnywhere, Category = "Day Night|Lighting")
	FLinearColor DaySunColor = FLinearColor(1.0f, 0.95f, 0.8f); // 태양이 높이 떠 있을 때의 빛 색상

	UPROPERTY(EditAnywhere, Category = "Day Night|Lighting")
	FLinearColor HorizonSunColor = FLinearColor(1.0f, 0.25f, 0.05f); // 일출과 일몰 때 빛 색상
	
	UPROPERTY(EditInstanceOnly, Category = "Day Night|Lighting")
	TObjectPtr<class ASkyLight> SkyLight; // 하늘에서 들어오는 전체적인 환경광

	float CachedSkyLightIntensity = 0.0f; // 에디터에서 설정한 Sky Light의 원래 최대 밝기

	UPROPERTY(EditAnywhere, Category = "Day Night|Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NightSkyLightIntensityMultiplier = 0.05f; // 밤에 유지할 Sky Light 밝기의 비율 / 0.05는 낮 밝기의 5%라는 의미

	UPROPERTY(EditInstanceOnly, Category = "Day Night|Lighting")
	TObjectPtr<ADirectionalLight> MoonLight; // 밤에 빛을 제공하는 달 Directional Light

	UPROPERTY(EditAnywhere, Category = "Day Night|Lighting", meta = (ClampMin = "0.0"))
	float MaxMoonIntensity = 0.005f; // 밤에 사용할 달 Directional Light 최대 밝기

	UPROPERTY(EditAnywhere, Category = "Day Night|Lighting")
	float MoonLightPitch = -45.0f; // 달빛이 지면을 비추는 고정된 높이
};
