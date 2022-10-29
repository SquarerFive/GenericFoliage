#include "GenericFoliageCollectionFactory.h"

#include "Foliage/GenericFoliageCollection.h"

UGenericFoliageCollectionFactory::UGenericFoliageCollectionFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UGenericFoliageCollection::StaticClass();
}

UObject* UGenericFoliageCollectionFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UGenericFoliageCollection>(InParent, InClass, InName, Flags | RF_Transactional);
}
