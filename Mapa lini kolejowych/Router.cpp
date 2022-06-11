#include "Router.h"
#include <fmt/core.h>
#include <cmath>
#include <numbers>
#include <map>
#include <queue>
#include <chrono>
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
	nodes.clear();
	nodes.try_emplace(start.ID, &start, nullptr, 0);
	nodes.try_emplace(end.ID, &end, nullptr, std::numeric_limits<double>::max());

	start_node = &nodes.at(start.ID);
	end_node = &nodes.at(end.ID);

	auto time_start = std::chrono::high_resolution_clock::now();
	find_path();
	auto time_end = std::chrono::high_resolution_clock::now();
	milisecounds_duration = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();

	if (end_node->distance == std::numeric_limits<double>::max())
		return false;

	return true;
}

uint32_t Routing::getDistance() const
{
	return static_cast<uint32_t>(end_node->distance);
}

uint64_t Routing::getTimePassed() const
{
	return milisecounds_duration;
}

uint64_t Routing::getTraveledNodes() const
{
	return nodes.size();
}

nlohmann::json Routing::toGeoJson(std::string_view start_name, std::string_view end_name)
{
	auto json = GeoJSON::createFeature();
	auto& properties = json["properties"];

	auto geometry = createMultiLineString();
	json["geometry"] = nlohmann::json::parse(utilities::asGeoJSON(SpatiaLite, geometry));

	auto boundry = utilities::getGeometryBoundry(SpatiaLite, geometry);
	properties["bounds"] = nlohmann::json::parse(utilities::asGeoJSON(SpatiaLite, std::move(boundry)));

	properties["distance"] = getDistance();

	properties["start_name"] = start_name;
	properties["end_name"] = end_name;

	return json;
}

void Routing::find_path()
{
	Heuristic heuristic;
	heuristic.goal = start_node;

	//std::priority_queue<Node*, std::vector<Node*>, Heuristic> queue(heuristic);
	std::priority_queue<Node*> queue;

	const auto& end_distance = end_node->distance;

	queue.push(start_node);

	while (!queue.empty())
	{
		auto step = queue.top();
		queue.pop();

		for (const auto n : step->node->neighbours)
		{
			auto new_distance = step->distanceTo(*n);
			new_distance += step->distance;

			if (end_distance < new_distance)
				continue;

			if (auto it = nodes.find(n->ID); it != nodes.end())
			{
				auto& distance = it->second;
				if (distance.distance <= new_distance)
				{
					continue;
				}
				distance.distance = new_distance;
				distance.back_node = step;
			}

			auto [emplacing, emplacing_bool] = nodes.try_emplace(n->ID, n, &nodes.at(step->node->ID), new_distance);
			queue.push(&emplacing->second);
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

double Routing::Heuristic::distanceToGoal(const Node& node) const
{
	auto lat = std::abs(goal->node->lat - node.node->lat);
	auto lon = std::abs(goal->node->lon - node.node->lon);

	return lat + lon;
}

bool Routing::Heuristic::operator()(const Node* left, const Node* right)
{
	auto left_distance = left->distance + distanceToGoal(*left);
	auto right_distance = right->distance + distanceToGoal(*right);

	return left_distance < right_distance;
}
