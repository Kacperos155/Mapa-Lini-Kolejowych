#include "Router.h"
#include <fmt/core.h>
#include <cmath>
#include <numbers>
#include <map>
#include <queue>
#include "GeoJSON_conversion.h"


double Routing::Node::distanceTo(const Railnode& other) const
{
	constexpr auto Earth_radius = 6'371'000;
	constexpr auto PI_radian = std::numbers::pi / 180;

	auto lat1 = node->lat * PI_radian;
	auto lat2 = other.lat * PI_radian;

	auto y1 = (other.lat - node->lat) * PI_radian;
	auto y2 = (other.lon - node->lon) * PI_radian;

	auto y1_sin = std::sin(y1 / 2);
	y1_sin *= y1_sin;
	auto y2_sin = std::sin(y2 / 2);
	y2_sin *= y2_sin;

	auto x_cos = std::cos(lat1) * std::cos(lat2);

	auto a = y1_sin + x_cos * y2_sin;
	auto c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));

	return Earth_radius * c;
}

Routing::Routing()
{
	utilities::loadSpatiaLite(SpatiaLite);
}

bool Routing::route(const Railnode& start, const Railnode& end)
{
	nodes.try_emplace(start.ID, &start, nullptr, 0);
	nodes.try_emplace(end.ID, &end, nullptr, std::numeric_limits<double>::max());

	start_node = &nodes.at(start.ID);
	end_node = &nodes.at(end.ID);

	find_path();

	if (end_node->distance == std::numeric_limits<double>::max())
		return false;

	return true;
}

uint32_t Routing::getDistance() const
{
	return static_cast<uint32_t>(end_node->distance);
}

nlohmann::json Routing::toGeoJson()
{
	auto json = GeoJSON::createFeature();
	auto& properties = json["properties"];

	json["geometry"] = createMultiLineString();
	properties["boundry"] = utilities::getGeometryBoundry(SpatiaLite, json["geometry"].get_ref<std::string&>());

	properties["distance"] = getDistance();

	properties["start_name"] = "START_NAME";
	properties["end_name"] = "END_NAME";

	return json;
}

void Routing::find_path()
{
	std::priority_queue<Node> queue;

	const auto& end_distance = end_node->distance;

	queue.push(*start_node);

	while (!queue.empty())
	{
		auto step = queue.top();
		queue.pop();

		if (end_distance < step.distance)
			continue;

		for (const auto n : step.node->neighbours)
		{
			auto new_distance = step.distanceTo(*n);
			new_distance += step.distance;

			if (nodes.contains(n->ID))
			{
				auto& distance = nodes.at(n->ID);
				if (distance.distance <= new_distance)
				{
					continue;
				}
				distance.distance = new_distance;
				distance.back_node = &nodes.at(step.node->ID);
			}

			auto new_distance_obj = Node(n, &nodes.at(step.node->ID), new_distance);
			nodes.try_emplace(n->ID, new_distance_obj);
			queue.emplace(new_distance_obj);
		}
	}
}

std::string Routing::createMultiLineString() const
{
	std::string line;
	line.reserve(16'000'000);
	line = "MULTILINESTRING ((";

	auto end_it = end_node;
	while (end_it != nullptr)
	{
		line += fmt::format("{} {}, ", end_it->node->lon, end_it->node->lat);
		end_it = end_it->back_node;
	}
	line.pop_back();
	line.pop_back();
	line += "))";
	return line;
}
