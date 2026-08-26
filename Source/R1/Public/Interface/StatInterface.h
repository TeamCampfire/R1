// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StatInterface.generated.h"

class UStatComponent;

UINTERFACE(MinimalAPI)
class UStatInterface : public UInterface
{
	GENERATED_BODY()
};

class R1_API IStatInterface
{
	GENERATED_BODY()

public:
	virtual UStatComponent* GetStatComponent() const = 0;
};
