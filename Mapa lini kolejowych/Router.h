#pragma once
#include "Data Types/Railnode.hpp"
#include <nlohmann/json.hpp>
#include <string_view>
#include "utilities.h"

class Routing
{
	struct Node
	{
		const Railnode* node;
		Node* back_node;
		double distance;

		Node(const Railnode* node, Node* back_node, double distance)
			: node(node), back_node(back_node), distance(distance) {}

		[[nodiscard]]
		double distanceTo(const Railnode& other) const;

		bool operator<(const Node& other) const
		{
			return distance < other.distance;
		}
	};

	struct Heuristic
	{
		Node* goal;
		double distanceToGoal(const Node& node) const;

		bool operator()(const Node* left, const Node* right);
	};

public:
	Routing();

	bool route(const Railnode& start, const Railnode& end);
	uint32_t getDistance() const;
	uint64_t getTimePassed() const;
	uint64_t getTraveledNodes() const;

	[[nodiscard]]
	nlohmann::json toGeoJson(std::string_view start_name = "START_NAME", std::string_view end_name = "END_NAME");

private:
	SQLite::Database SpatiaLite{ ":memory:" };
	std::map<int64_t, Node> nodes{};
	Node* start_node{};
	Node* end_node{};
	uint64_t milisecounds_duration{};

	void find_path();

	[[nodiscard]]
	std::string createMultiLineString() const;
};

