// Fill out your copyright notice in the Description page of Project Settings.


#include "SpatialLibrary.h"

#include "GenericFoliage.h"
#include "JsonObjectWrapper.h"
#include "Dom/JsonObject.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshSpatialFunctions.h"

TArray<FSpatialFeature> USpatialLibrary::ParseGeoJSON(const FString& InGeoJSON, UDynamicMeshPool* MeshPool)
{
	FJsonObjectWrapper Wrapper;
	if (!ensure(Wrapper.JsonObjectFromString(InGeoJSON)))
	{
		UE_LOG(LogGenericFoliage, Error, TEXT("Failed to parse GeoJSON"))
		return {};
	}

	auto Object = Wrapper.JsonObject;

	if (!(Object->GetStringField("type") == TEXT("FeatureCollection")))
	{
		UE_LOG(LogGenericFoliage, Error, TEXT("GeoJSON type is not of type 'FeatureCollection'!"))
		return {};
	}

	TArray<FSpatialFeature> Features;
	int32 Id = 0;
	
	for (auto FeatureJsonValue: Object->GetArrayField("features")) {
		auto FeatureJsonObject = FeatureJsonValue->AsObject();
		auto GeometryJsonObject = FeatureJsonObject->GetObjectField("geometry");
		auto PropertiesJsonObject = FeatureJsonObject->GetObjectField("properties");
		
		
		TMap<FString, FString> Properties;
		UDynamicMesh* Mesh = nullptr;

		if (GeometryJsonObject->GetStringField("type") == TEXT("Polygon"))
		{
			TArray<FVector2D> Coordinates;

			for (auto CoordinateValues: GeometryJsonObject->GetArrayField("coordinates")[0]->AsArray())
			{
				auto CoordinateValue = CoordinateValues->AsArray();
				Coordinates.Add(FVector2D(
					CoordinateValue[0]->AsNumber(),
					CoordinateValue[1]->AsNumber()
				));
			}

			Mesh = MeshPool->RequestMesh();
			Mesh->Reset();
			
			UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendSimpleExtrudePolygon(
				Mesh,
				FGeometryScriptPrimitiveOptions{},
				FTransform(),
				Coordinates,
				400.0f
			);
		}

		FGeometryScriptDynamicMeshBVH BVH;
		UGeometryScriptLibrary_MeshSpatial::BuildBVHForMesh(Mesh, BVH);

		for (auto Property: PropertiesJsonObject->Values)
		{
			if (Property.Value->Type == EJson::String)
			{
				Properties.Add(Property.Key, Property.Value->AsString());
			} else if (Property.Value->Type == EJson::Number)
			{
				Properties.Add(Property.Key, FString::SanitizeFloat(Property.Value->AsNumber()));
			} else if (Property.Value->Type == EJson::Boolean)
			{
				Properties.Add(Property.Key, Property.Value->AsBool() ? TEXT("true") : TEXT("false"));
			}
		}

		Features.Emplace(FSpatialFeature {
			MoveTemp(Mesh),
			MoveTemp(BVH),
			Properties,
			PropertiesJsonObject->HasField("type") ? PropertiesJsonObject->GetIntegerField("type") : 0,
			Id
		});
		Id ++;
	}

	return Features;
}
