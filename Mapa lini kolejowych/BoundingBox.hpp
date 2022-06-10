#pragma once
#include <string>
#include <fmt/core.h>

struct BoundingBox
{
	double min_lon;
	double min_lat;
	double max_lon;
	double max_lat;

	BoundingBox(double min_lon, double min_lat, double max_lon, double max_lat)
		: min_lon(min_lon), min_lat(min_lat), max_lon(max_lon), max_lat(max_lat) {}

	std::string getPolygon() const
	{
		return fmt::format("POLYGON(({0} {1}, {2} {1}, {2} {3}, {0} {3}))", min_lat, min_lon, max_lat, max_lon);
	}
};
