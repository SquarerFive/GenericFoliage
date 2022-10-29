// Copyright Aiden. Soedjarwo. All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Factories/Factory.h"
#include "GenericFoliageCollectionFactory.generated.h"

UCLASS()
class GENERICFOLIAGEEDITOR_API UGenericFoliageCollectionFactory : public UFactory
{
	GENERATED_BODY()
public:
	UGenericFoliageCollectionFactory();
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
