/// 최초작성 : 2026.09.03
/// 작성자 : 주형진
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/HealthInterface.h"
#include "AnimalCharacter.generated.h"

class UHarvestableComponent;
class UNiagaraSystem;
class USoundBase;

/**
 * 사슴 등 야생 동물 캐릭터 베이스 클래스 (리슨 서버 복제 및 시체 채집 지원)
 */
UCLASS()
class R1_API AAnimalCharacter : public ACharacter, public IHealthInterface
{
	GENERATED_BODY()

public:
	AAnimalCharacter();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//체력 인터페이스
	virtual float GetCurrentHealth_Implementation() const override;
	virtual float GetMaxHealth_Implementation() const override;
	virtual void InflictDamage_Implementation(float InAmount) override;
	virtual void Heal_Implementation(float InAmount) override;
	virtual bool IsAlive() const override;

	// 사망 처리 (서버 권한)
	void Die();

	// 모든 클라이언트에 랙돌 및 사망 상태 복제
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Die();

	UFUNCTION(BlueprintPure, Category = "Animal|State")
	FORCEINLINE bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "Animal|Stats")
	FORCEINLINE float GetCurrentHp() const { return CurrentHp; }

	UFUNCTION(BlueprintPure, Category = "Animal|Stats")
	FORCEINLINE float GetMaxHp() const { return MaxHp; }

protected:
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayHitMontage();

protected:
	// 시체 채집 컴포넌트 (사망 시 가죽, 천, 고기 드랍 처리)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHarvestableComponent> HarvestableComponent;

	// 동물 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animal|Stats")
	float MaxHp = 100.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Animal|Stats")
	float CurrentHp = 100.0f;

	// 사망 여부
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Animal|State")
	bool bIsDead = false;

	// 평소 이동 속도 (배회 걷기)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animal|Movement")
	float WalkSpeed = 150.0f;

	// 도망 시 이동 속도 (전력 질주)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animal|Movement")
	float RunSpeed = 600.0f;

/*--------------------------------
*			AM 변수
--------------------------------*/
#pragma region Anim Montage
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> AM_Hitted;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> AM_Death;
#pragma endregion
};
