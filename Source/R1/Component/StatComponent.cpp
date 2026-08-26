// Fill out your copyright notice in the Description page of Project Settings.

#include "R1/Component/StatComponent.h"

UStatComponent::UStatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

float UStatComponent::GetCurrentStamina_Implementation() const
{
	return 0.0f;
}

float UStatComponent::GetMaxStamina_Implementation() const
{
	return 0.0f;
}

bool UStatComponent::ConsumeStamina_Implementation(float InAmount)
{
	return false;
}

void UStatComponent::RecoverStamina_Implementation(float InAmount)
{
}

void UStatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UStatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
