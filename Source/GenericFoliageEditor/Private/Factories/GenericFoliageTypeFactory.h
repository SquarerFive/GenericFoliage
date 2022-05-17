// Copyright Aiden. S. All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Factories/Factory.h"
#include "GenericFoliageTypeFactory.generated.h"

/**
 * 
 */
UCLASS()
class GENERICFOLIAGEEDITOR_API UGenericFoliageTypeFactory : public UFactory
{
	GENERATED_BODY()

public:
	UGenericFoliageTypeFactory();
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};