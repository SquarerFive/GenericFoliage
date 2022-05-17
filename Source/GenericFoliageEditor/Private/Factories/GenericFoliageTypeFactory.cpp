// Copyright Aiden. S. All rights reserved


#include "GenericFoliageTypeFactory.h"

#include "GenericFoliage/Public/Foliage/GenericFoliageType.h"

UGenericFoliageTypeFactory::UGenericFoliageTypeFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UGenericFoliageType::StaticClass();
}

UObject* UGenericFoliageTypeFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UGenericFoliageType>(InParent, InClass, InName, Flags | RF_Transactional);
}
