// Fill out your copyright notice in the Description page of Project Settings.


#include "SpatialLibrary.h"

#include "GenericFoliage.h"
#include "JsonObjectWrapper.h"
#include "Dom/JsonObject.h"
// #include "GeometryScript/MeshPrimitiveFunctions.h"
// #include "GeometryScript/MeshSpatialFunctions.h"

TArray<FSpatialFeature> USpatialLibrary::ParseGeoJSON(const FString& InGeoJSON)
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

		FPolygon2d Poly;

		if (GeometryJsonObject->GetStringField("type") == TEXT("Polygon"))
		{
			TArray<FVector2d> Coordinates;

			for (auto CoordinateValues: GeometryJsonObject->GetArrayField("coordinates")[0]->AsArray())
			{
				auto CoordinateValue = CoordinateValues->AsArray();
				Coordinates.Add(FVector2d{
					CoordinateValue[0]->AsNumber(),
					CoordinateValue[1]->AsNumber()
					});
			}
			Poly = FPolygon2d(Coordinates);

			/*

			Mesh = MeshPool->RequestMesh();
			Mesh->Reset();
			
			UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendSimpleExtrudePolygon(
				Mesh,
				FGeometryScriptPrimitiveOptions{},
				FTransform(),
				Coordinates,
				400.0f
			);
			*/
		}

		/*
		FGeometryScriptDynamicMeshBVH BVH;
		UGeometryScriptLibrary_MeshSpatial::BuildBVHForMesh(Mesh, BVH);
		*/
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
			Poly,
			Properties,
			PropertiesJsonObject->HasField("type") ? PropertiesJsonObject->GetIntegerField("type") : 0,
			Id
		});
		Id ++;
	}

	return Features;
}

float USpatialLibrary::HaversineDistance(const FVector2D& PointA, const FVector2D& PointB, float Radius)
{
	const FVector2D PointARadians = FMath::DegreesToRadians(PointA);
	const FVector2D PointBRadians = FMath::DegreesToRadians(PointB);

	const FVector2D Diff = PointB - PointA;
	const float a = FMath::Pow(FMath::Sin(Diff.Y / 2.0), 2.0) + FMath::Cos(PointA.Y) * FMath::Cos(PointB.Y) * FMath::Pow(FMath::Sin(Diff.X/2.0), 2.0);
	const float c = 2.0 * FMath::Asin(FMath::Sqrt(a));

	return c * Radius;
}

double USpatialLibrary::HaversineDistance(const FVector2d& PointA, const FVector2d& PointB, double Radius)
{
	const FVector2d PointARadians = FMath::DegreesToRadians(PointA);
	const FVector2d PointBRadians = FMath::DegreesToRadians(PointB);

	const FVector2d Diff = PointB - PointA;
	const double a = FMath::Pow(FMath::Sin(Diff.Y / 2.0), 2.0) + FMath::Cos(PointA.Y) * FMath::Cos(PointB.Y) * FMath::Pow(FMath::Sin(Diff.X / 2.0), 2.0);
	const double c = 2.0 * FMath::Asin(FMath::Sqrt(a));

	return c * Radius;
}

FVector2D USpatialLibrary::HaversineDeltaDegrees(const FVector2D& Origin, const float& Distance, float Radius)
{
	// https://math.stackexchange.com/questions/474602/reverse-use-of-haversine-formula
	const float DeltaLatitude = Distance / Radius;
	const float DeltaLongitude = 2 * FMath::Asin(FMath::Sqrt(
		(FMath::Pow(FMath::Sin((Distance / Radius) / 2.0), 2.0)) / FMath::Cos(FMath::DegreesToRadians(Origin.Y))
	));

	return FVector2D(
	DeltaLongitude,
	DeltaLatitude
	);
}

FVector2d USpatialLibrary::HaversineDeltaDegrees(const FVector2d& Origin, const double& Distance, double Radius)
{
	// https://math.stackexchange.com/questions/474602/reverse-use-of-haversine-formula
	const double DeltaLatitude = Distance / Radius;
	const double DeltaLongitude = 2 * FMath::Asin(FMath::Sqrt(
		(FMath::Pow(FMath::Sin((Distance / Radius) / 2.0), 2.0)) / FMath::Cos(FMath::DegreesToRadians(Origin.Y))
	));

	return FVector2d(
		DeltaLongitude,
		DeltaLatitude
	);
}
