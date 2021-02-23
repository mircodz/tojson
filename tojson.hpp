#pragma once

#include <yaml-cpp/yaml.h>

#include <fstream>

#include "json.hpp"
#include "rapidxml.hpp"

#if __has_cpp_attribute(nodiscard)
	#define TOJSON_NODISCARD [[nodiscard]]
#else
	#define TOJSON_NODISCARD
#endif

namespace tojson {

namespace detail {

inline nlohmann::json xml2json(const rapidxml::xml_node<> *root) {
	nlohmann::json j{};
	std::unordered_map<std::string, int> occurrence{};

	for (auto *node = root->first_node(); node; node = node->next_sibling()) {
		auto key = node->name();
		occurrence[key] += 1;

		// if seen current node, convert it into a list
		if (occurrence[key] == 2)
			j[key] = {j[key]};

		// if the node is a sequence
		if (occurrence[key] > 1) {
			nlohmann::json n{};
			if (node->first_node())
				n[key] = xml2json(node);
			else
				n["@text"] = node->value();
			// iterate through the attributes
			for (auto *attr = node->first_attribute(); attr; attr = attr->next_attribute())
				n[key][attr->name()] = attr->value();
			j[key].emplace_back(n[key]);
		} else {
			if (node->first_node())
				j[key] = xml2json(node);
			else
				j["@text"] = node->value();
			// iterate through the attributes
			for (auto *attr = node->first_attribute(); attr; attr = attr->next_attribute())
				j[key][attr->name()] = attr->value();
		}
	}

	return j;
}

inline nlohmann::json yaml2json(const YAML::Node &root) {
	nlohmann::json j{};

	switch (root.Type()) {
	case YAML::NodeType::Null:
		break;
	case YAML::NodeType::Scalar:
		return root.as<std::string>();
	case YAML::NodeType::Sequence:
		for (auto &&node : root)
			j.emplace_back(yaml2json(node));
		break;
	case YAML::NodeType::Map:
		for (auto &&it : root)
			j[it.first.as<std::string>()] = yaml2json(it.second);
		break;
	default: break;
	}
	return j;
}

}  // namespace detail

/// \brief Convert XML string to JSON.
TOJSON_NODISCARD inline nlohmann::json xml2json(const std::string &str) {
	nlohmann::json j{};
	rapidxml::xml_document<> doc{};

	doc.parse<0>(const_cast<char *>(str.data()));

	auto *root = doc.first_node();
	if (root)
		j[root->name()] = detail::xml2json(root);

	return j;
}

/// \brief Convert YAML string to JSON.
TOJSON_NODISCARD inline nlohmann::json yaml2json(const std::string &str) {
	YAML::Node root = YAML::Load(str);
	return detail::yaml2json(root);
}

/// \brief Load a YAML file to JSON.
TOJSON_NODISCARD inline nlohmann::json loadyaml(const std::string &filepath) {
	YAML::Node root = YAML::LoadFile(filepath);
	return detail::yaml2json(root);
}

/// \brief Load XML file to JSON.
TOJSON_NODISCARD inline nlohmann::json loadxml(const std::string &filepath) {
	std::ifstream file{filepath.data()};
	std::string str{std::istream_iterator<char>(file), std::istream_iterator<char>()};
	return xml2json(str);
}

}  // namespace tojson
