#pragma once
#include "Data Types/Railnode.hpp"
#include <nlohmann/json.hpp>
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


public:
	Routing();

	bool route(const Railnode& start, const Railnode& end);
	uint32_t getDistance() const;

	[[nodiscard]]
	nlohmann::json toGeoJson();

private:
	SQLite::Database SpatiaLite{ ":memory:" };
	std::map<int64_t, Node> nodes;
	Node* start_node;
	Node* end_node;

	void find_path();

	[[nodiscard]]
	std::string createMultiLineString() const;
};

