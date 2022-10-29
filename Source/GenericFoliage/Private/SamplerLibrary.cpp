// Fill out your copyright notice in the Description page of Project Settings.


#include "SamplerLibrary.h"
#include "SpatialLibrary.h"
#include "GenericFoliage.h"

#define MAX_GRID_SIZE 67108864

bool IsValidCandidate(const FVector2D& Candidate, const FVector2D& RegionSize, const double& CellSize,
                      const double& Radius, const TArray<FVector2D>& Points, const int32* Grid, const int32& GridSizeX,
                      const int32& GridSizeY, const FPoissonDiscSamplingSettings& Settings)
{
	if (Candidate.X >= 0.0 && Candidate.X < RegionSize.X &&
		Candidate.Y >= 0.0 && Candidate.Y < RegionSize.Y
	)
	{
		const int32 CellX = static_cast<int32>(Candidate.X / CellSize);
		const int32 CellY = static_cast<int32>(Candidate.Y / CellSize);

		const int32 SearchStartX = FMath::Max<int32>(CellX - 2, 0);
		const int32 SearchEndX = FMath::Min<int32>(CellX + 2, GridSizeX - 1);

		const int32 SearchStartY = FMath::Max<int32>(CellY - 2, 0);
		const int32 SearchEndY = FMath::Min<int32>(CellY + 2, GridSizeY - 1);

		for (int32 x = SearchStartX; x < SearchEndX + 1; ++x)
		{
			for (int32 y = SearchStartY; y < SearchEndY + 1; ++y)
			{
				const int PointIndex = Grid[GridSizeX * y + x] - 1;

				if (PointIndex != -1)
				{
					if (Settings.bUseGeographicCoordinates)
					{
						const double Distance = USpatialLibrary::HaversineDistance(
							Candidate + Settings.Origin,
							Points[PointIndex] + Settings.Origin
						);
						
						if (Distance < Settings.Radius)
						{
							return false;
						}
					} else
					{
						const double Distance = FVector2d::DistSquared(
							Candidate,
							Points[PointIndex]
						);

						if (Distance < Radius * Radius)
						{
							return false;
						}
					}
				}
			}
		}

		return true;
	}

	return false;
}

TArray<FVector2D> USamplerLibrary::PoissonDiscSampling(const double Radius,
                                                       const FVector2D RegionSize,
                                                       const int32 RejectionThreshold,
                                                       const FPoissonDiscSamplingSettings Settings)
{
	const double CellSize = Radius / FMath::Sqrt(2.0);
	const int32 GridSizeX = static_cast<int32>(ceil(RegionSize.X / CellSize));
	const int32 GridSizeY = static_cast<int32>(ceil(RegionSize.Y / CellSize));

	// Limit max grid size so we don't crash the game

	if ((GridSizeX * GridSizeY) > MAX_GRID_SIZE)
	{
		UE_LOG(LogGenericFoliage, Error, TEXT("Grid Size (%i %i) exceeds MAX_GRID_SIZE!"));
		return {};
	}

	TArray<FVector2D> Points;
	TArray<FVector2D> ActivePoints = {RegionSize / 2.0};

	int32* Grid = new int32[GridSizeX * GridSizeY]{0};

	while (ActivePoints.Num() > 0)
	{
		// Sample a point
		const int32 ActiveIndex = FMath::RandRange(0, ActivePoints.Num() - 1);
		const FVector2D& ActivePoint = ActivePoints[
			ActiveIndex
		];

		bool bAcceptedCandidate = false;

		for (int i = 0; i < RejectionThreshold; ++i)
		{
			const double Angle = FMath::FRand() * PI * 2.0;
			const FVector2D Direction = FVector2d(sin(Angle), cos(Angle));
			const double Length = FMath::FRandRange(Radius, Radius * 2.0);

			const FVector2D Candidate = ActivePoint + (Direction * Length);

			if (IsValidCandidate(Candidate, RegionSize, CellSize, Radius, Points, Grid, GridSizeX, GridSizeY, Settings))
			{
				Points.Add(Candidate);
				ActivePoints.Add(Candidate);

				const int32 CellX = static_cast<int32>(Candidate.X / CellSize);
				const int32 CellY = static_cast<int32>(Candidate.Y / CellSize);

				Grid[
					GridSizeX * CellY + CellX
				] = Points.Num();
				bAcceptedCandidate = true;
				break;
			}
		}

		if (!bAcceptedCandidate)
		{
			ActivePoints.RemoveAt(ActiveIndex);
		}
	}

	delete[] Grid;

	return Points;
}

TArray<FVector2D> USamplerLibrary::K2_PoissonDiscSampling2d(const double Radius, const FVector2D RegionSize,
                                                            const int32 RejectionThreshold)
{
	return PoissonDiscSampling(Radius, RegionSize, RejectionThreshold);
}

#undef MAX_GRID_SIZE
