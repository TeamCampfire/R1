// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/HarvestableComponent.h"

// Sets default values for this component's properties
UHarvestableComponent::UHarvestableComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

FHarvestRes UHarvestableComponent::OnHitted_Implementation(AActionCharacter* InCharacter)
{
	//TODO 세부 로직 구현
	//TODO ItemData 기반으로 연동
	FHarvestRes Res;

	// 0. 캐릭터의 현재 무기가 이 액터를 공격할 수 있는 타입인지 확인
	
	// 1. 무기의 데이터를 기반으로 체력 감소
	CurrentHp -= 50.f;
	
	// 2. 피격 데칼 생성
	UE_LOG(LogTemp, Display, TEXT("데칼 생성"));

	// 3. 자원 액터의 데이터를 통해서 결과 구조체 생성
	// Res.Count = Res.ItemData.Cnt 
	Res.Count = 1;
	Res.HarvesResult = true;
	
	// 3-0. 스위트 스팟에 맞은 경우 개수에 배율을 곱해서 반환
	// Res.Count *= BounusRate;

	UE_LOG(LogTemp, Display, TEXT("자원 [%s]를 %d개 획득!"), *ItemData, Res.Count);

	// 4. 만약 자원 액터의 체력이 0보다 작아지면 OnHarvestEnd() 호출
	if (CurrentHp <= 0)
	{
		CurrentHp = 0.f;
		IHarvestable::Execute_OnHarvestEnd(this);
	}


	return Res;
}

void UHarvestableComponent::OnHarvestEnd_Implementation()
{
	UE_LOG(LogTemp, Display, TEXT("자원 액터의 자원 고갈"));
	// 0. 파괴 연출 재생
	UE_LOG(LogTemp, Display, TEXT("자원 액터의 파괴 애니메이션"));
	// 1. 연출 재생이 끝나면 Destroy
	UE_LOG(LogTemp, Display, TEXT("자원 액터의 소멸"));
}

// Called when the game starts
void UHarvestableComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UHarvestableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

