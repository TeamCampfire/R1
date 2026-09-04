/// 최초작성 : 2026.08.26
/// 작 성 자 : 강 진 구
/// 캐릭터의 생존 스탯(체력, 허기, 갈증) 전반 담당하는 컴포넌트
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interface/HealthInterface.h"
#include "Interface/CaloriesInterface.h"
//#include "Interface/StaminaInterface.h"
#include "Interface/StatusEffectInterface.h"
#include "Interface/TemperatureInterface.h"
#include "Interface/HydrationInterface.h"
#include "Character/ActionCharacter.h"
#include "StatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatEmpty);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatusEffectChange);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatChange, float, Current, float, Max);

enum class EParameterType : uint8
{
	Health,
	Hydration,
	HydrationDropRate,
	Calories,
	CaloriesDropRate,
	Temperature
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class R1_API UStatComponent : public UActorComponent, public IHealthInterface, public ICaloriesInterface, public ITemperatureInterface, public IHydrationInterface, public IStatusEffectInterface
{
	GENERATED_BODY()

public:
	UStatComponent();

	void InitializeStat();
	// 체력
	virtual float GetCurrentHealth_Implementation() const override;
	virtual float GetMaxHealth_Implementation() const override;
	virtual void InflictDamage_Implementation(float InAmount) override;
	virtual void Heal_Implementation(float InAmount) override;
	virtual bool IsAlive() const override;

	// 스태미나
	//virtual float GetCurrentStamina_Implementation() const override;
	//virtual float GetMaxStamina_Implementation() const override;
	//virtual bool ConsumeStamina_Implementation(float InAmount) override;
	//virtual void RecoverStamina_Implementation(float InAmount) override;

	// 칼로리
	virtual float GetCurrentCalories_Implementation() const override;
	virtual float GetMaxCalories_Implementation() const override;
	virtual bool DecreaseCalories_Implementation(float InAmount) override;
	virtual void RecoverCalories_Implementation(float InAmount) override;
	virtual float GetCaloriesDropRate_Implementation() const override;
	virtual void SetCaloriesDropRate_Implementation(float InDropRate) override;
	virtual void ResetCaloriesDropRate_Implementation() override;
	// 수분
	virtual float GetCurrentHydration_Implementation() const override;
	virtual float GetMaxHydration_Implementation() const override;
	virtual bool DecreaseHydration_Implementation(float InAmount) override;
	virtual void RecoverHydration_Implementation(float InAmount) override;
	virtual float GetHydrationDropRate_Implementation() const override;
	virtual void SetHydrationDropRate_Implementation(float InDropRate) override;
	virtual void ResetHydrationDropRate_Implementation() override;
	// 체온
	virtual float GetCurrentTemperature_Implementation() const override;
	virtual bool IncreaseTemperature_Implementation(float InAmount) override;
	virtual void DecreaseTemperature_Implementation(float InAmount) override;
	// 상태이상
	virtual EStatusEffect GetCurrentStatusEffect_Implementation() override;
	virtual void RemoveStatusEffect_Implementation(EStatusEffect InStatusEffectType) override;
	virtual void SetStatusEffect_Implementation(EStatusEffect InStatusEffectType) override;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// 파라미터 감소 내부처리 함수
	void IncreaseParameter(EParameterType InEParameterType, float InAmount);
	// 파라미터 증가 내부처리 함수
	void DecreaseParameter(EParameterType inEParameterType, float inAmount);
	// 파라미터 설정 내부처리용 함수
	void SetParameter(EParameterType inEParameterType, float inAmount);
	// 틱마다 허기, 수분 감소
	void DrainSurvivalStats();
	// 굶주림, 갈증 데미지 효과 적용 함수
	void UpdateStatusEffectDamage();

	void ApplyStarvationDamage();
	void ApplyDehydrationDamage();

	// Owner캐릭터 캐시용
	UPROPERTY()
	TObjectPtr<AActionCharacter> CachedCharacter = nullptr;

	// 현재 체력 리플리케이션 함수
	UFUNCTION()
	void OnRep_CurrentHealth();
	// 현재 수분 리플리케이션 함수
	UFUNCTION()
	void OnRep_CurrentHydration();
	// 현재 허기 리플리케이션 함수
	UFUNCTION()
	void OnRep_CurrentCalories();
	// 현재 상태이상 리플리케이션 함수
	UFUNCTION()
	void OnRep_PlayerStatusEffects();
	UFUNCTION()
	void OnRep_bAlive();
protected:
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float CurrentStamina = 100.0f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float MaxStamina = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, EditAnywhere, BlueprintReadWrite)
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHydration, EditAnywhere, BlueprintReadWrite)
	float CurrentHydration = 250.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHydration = 250.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentCalories, EditAnywhere, BlueprintReadWrite)
	float CurrentCalories = 500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxCalories = 500.0f;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerStatusEffects, EditAnywhere, BlueprintReadWrite)
	EStatusEffect PlayerStatusEffects = EStatusEffect::None;

	// 굶주림 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StarvationDamage = 1.0f;
	// 탈수 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DehydrationDamage = 1.0f;

	// 초당 칼로리 감소율(기본)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefaultCaloryDropRate = 0.016f;
	// 초당 수분 감소율(기본)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefaultHydrationDropRate = 0.0032f;
	// 칼로리 감소율
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CaloryDropRate = 0.016f;
	// 수분 감소율
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float HydrationDropRate = 0.0032f;


	// 목마름 시작 수치
	// 목마름: 달리기 불가, 체력 자동회복 off
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 ThirstThreshold = 50;

	// 탈수 시작 수치
	// 탈수: 초당 데미지 발생
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 DehydrationThreshold = 25;

	// 배고픔 시작 수치
	// 배고픔: 체력 자동회복 off
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 HungerThreshold = 100;

	// 굶주림 시작 수치
	// 굶주림: 초당 데미지 발생
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 StarvationThreshold = 40;

	FTimerHandle SurvivalStatTimerHandle;
	FTimerHandle HealthRegenTimerHandle;
	FTimerHandle StarvationDamageTimerHandle;
	FTimerHandle DehydrationDamageTimerHandle;

	float SurvivalStatUpdateInterval = 1.0f;


	// Debug
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DebugDamage = 50.0f;

	UPROPERTY(ReplicatedUsing = OnRep_bAlive)
	bool bAlive = false;
private:

public:
	//UPROPERTY(BlueprintAssignable, Category = "Stat|Stamina")
	//FOnStatEmpty OnStaminaEmpty;
	//UPROPERTY(BlueprintAssignable, Category = "Stat|Stamina")
	//FOnStatChange OnStaminaChange;
	UPROPERTY(BlueprintAssignable, Category = "Stat|Health")
	FOnStatEmpty OnDeath;
	UPROPERTY(BlueprintAssignable, Category = "Stat|Health")
	FOnStatChange OnHealthChange;
	UPROPERTY(BlueprintAssignable, Category = "Stat|Hydration")
	FOnStatChange OnHydrationChange;
	UPROPERTY(BlueprintAssignable, Category = "Stat|Calories")
	FOnStatEmpty OnHydrationDepletion;
	UPROPERTY(BlueprintAssignable, Category = "Stat|Calories")
	FOnStatChange OnCaloryChange;
	UPROPERTY(BlueprintAssignable, Category = "Stat|Calories")
	FOnStatEmpty OnCaloryDepletion;

	UPROPERTY(BlueprintAssignable, Category = "StatusEffect")
	FOnStatusEffectChange OnStatusEffectChange;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
public:
	UFUNCTION(CallInEditor, Category = "Debug|Health")
	void TestInflictDamage();
	UFUNCTION(CallInEditor, Category = "Debug|Hydration")
	void TestDecreaseHydration();
	UFUNCTION(CallInEditor, Category = "Debug|Calories")
	void TestDecreaseCalories();
	UFUNCTION(CallInEditor, Category = "Debug|Hydration")
	void TestIncreaseHydration();
	UFUNCTION(CallInEditor, Category = "Debug|Calories")
	void TestIncreaseCalories();
	UFUNCTION(CallInEditor, Category = "Debug|StatusEffect")
	void TestAddHungry();
	UFUNCTION(CallInEditor, Category = "Debug|StatusEffect")
	void TestAddStarving();

};
