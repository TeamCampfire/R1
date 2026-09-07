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
	virtual void BeginPlay() override;

public:
	// 동기화된 서버 시간을 0~24시 범위의 게임 시각으로 변환
	UFUNCTION(BlueprintPure, Category = "Day Night")
	float GetCurrentHour() const;

	// 현재 게임 시각이 설정된 낮 구간에 포함되는지 반환
	UFUNCTION(BlueprintPure, Category = "Day Night")
	bool IsDaytime() const;

	// 서버에서 현재 게임 시각을 변경하고 새 기준 시각을 클라이언트에 복제해요
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Day Night")
	void SetTime(float NewHour);

protected:
	// 클라이언트가 서버의 사이클 기준 시각을 수신했을 때 자동 호출
	UFUNCTION()
	void OnRep_CycleStartServerTime();

	// (테스트용)현재 게임 시각과 동기화 상태를 로그로 출력
	void PrintDebugTime();

	// 현재 게임 시각과 Curve에 따라 태양, 밤 보조광, Sky Light 갱신
	void UpdateEnvironmentVisuals();

protected:
	UPROPERTY(EditAnywhere, Category = "Day Night|Time")
	float DayStartHour = 6.f; // 게임에서 낮이 시작되는 시각

	UPROPERTY(EditAnywhere, Category = "Day Night|Time")
	float NightStartHour = 18.f; // 게임에서 밤이 시작되는 시각

	UPROPERTY(EditAnywhere, Category = "Day Night|Time", meta = (ClampMin = "1.0"))
	float DayDurationSeconds = 45.f; // 낮 구간이 현실에서 지속되는 시간(초). 테스트 기본값은 45초

	UPROPERTY(EditAnywhere, Category = "Day Night|Time", meta = (ClampMin = "1.0"))
	float NightDurationSeconds = 15.f; // 밤 구간이 현실에서 지속되는 시간(초). 테스트 기본값은 15초

	UPROPERTY(ReplicatedUsing = OnRep_CycleStartServerTime)
	double CycleStartServerTime = -1.f; // 서버 월드 시간 기준의 사이클 시작 시각. 변경이 되었을 떄 클라이언트에 복제

	bool bIsTimeSynchronized = false; // 서버는 직접 설정하고 클라이언트는 OnRep에서 설정하는 동기화 완료 여부

	UPROPERTY(EditAnywhere, Category = "Day Night|Debug")
	bool bEnableDebugLog = true;

	UPROPERTY(EditAnywhere, Category = "Day Night|Debug", meta = (ClampMin = "0.1"))
	float DebugLogInterval = 1.0f;

	UPROPERTY(EditInstanceOnly, Category = "Day Night|Lighting")
	TObjectPtr<class ADirectionalLight> SunLight; // 낮의 방향광으로 사용할 레벨의 Directional Light

	UPROPERTY(EditAnywhere, Category = "Day Night|Lighting", meta = (ClampMin = "0.02"))
	float SunUpdateInterval = 0.1f; // 태양을 업뎃하는 주기

	UPROPERTY(EditAnywhere, Category = "Day Night|Lighting")
	float SunYaw = 0.0f; 	// 태양이 이동하는 수직 궤도의 수평 방향

	float CachedSunIntensity = 0.0f; // BeginPlay에서 저장하는 태양의 에디터 설정 밝기

	UPROPERTY(EditAnywhere, Category = "Day Night|Lighting")
	FLinearColor DaySunColor = FLinearColor(1.0f, 0.95f, 0.8f); // 태양이 충분히 높을 때 사용할 색상

	UPROPERTY(EditAnywhere, Category = "Day Night|Curves")
	TObjectPtr<UCurveFloat> SunIntensityCurve; //. X축 게임 시각(0~24), Y축 태양 밝기 비율(0~1)

	UPROPERTY(EditAnywhere, Category = "Day Night|Lighting")
	FLinearColor HorizonSunColor = FLinearColor(1.0f, 0.25f, 0.05f); // 일출과 일몰에 사용할 태양 색상
	
	UPROPERTY(EditAnywhere, Category = "Day Night|Curves")
	TObjectPtr<UCurveFloat> SkyLightIntensityCurve; //. X축 게임 시각(0~24), Y축 Sky Light 밝기 비율(0~1)

	UPROPERTY(EditInstanceOnly, Category = "Day Night|Lighting")
	TObjectPtr<class ASkyLight> SkyLight; 	// 전체 환경광으로 사용할 레벨의 Sky Light

	float CachedSkyLightIntensity = 0.0f; // BeginPlay에서 저장하는 Sky Light의 에디터 설정 밝기

	UPROPERTY(EditInstanceOnly, Category = "Day Night|Lighting")
	TObjectPtr<ADirectionalLight> MoonLight; // 최소 밤에도 약간은 보여야 하니까.. 필요한 고정 방향인 Directional Light

	// 밤 보조광 Curve에 곱할 최대 밝기
	UPROPERTY(EditAnywhere, Category = "Day Night|Lighting", meta = (ClampMin = "0.0"))
	float MaxNightFillIntensity = 0.005f;

	UPROPERTY(EditAnywhere, Category = "Day Night|Lighting")
	float MoonLightPitch = -45.0f; // 밤 보조광이 지면을 안정적으로 비추도록 유지할 고정 Pitch

	UPROPERTY(EditAnywhere, Category = "Day Night|Curves")
	TObjectPtr<UCurveFloat> NightFillIntensityCurve; //. X축 게임 시각(0~24), Y축 밤 보조광 밝기 비율(0~1)

};
