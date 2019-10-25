#ifndef TOJSON_H
#define TOJSON_H

#include <exception>
#include <fstream>
#include <iostream>

#include <yaml-cpp/yaml.h>

/* header only libraries */
#include "rapidxml.hpp"
#include "json.hpp"
using json = nlohmann::json;

namespace tojson {

class ParseError : public std::exception
{
	virtual const char* what() const throw()
	{
		return "parse error";
	}
};

namespace detail {

/**
 * Recursively traverses the xml tree and converts it into a nlohmann::json
 * object.  Possibly override the [] operator to automatically access the
 * `#text` element when the result is used as a string.
 */
inline json xml2json(rapidxml::xml_node<> *root)
{
	json j;
	std::map<std::string, int> occurrence;

	/* rapidxml::xml_node<> */
	for (auto *node = root->first_node(); node; node = node->next_sibling()) {
		const char *key = node->name();
		occurrence[key] += 1;

		/* if seen current node, convert it into a list */
		if (occurrence[key] == 2)
			j[key] = { j[key] };

		/* if the node is a sequence */
		if (occurrence[key] > 1) {
			json n;
			if(node->first_node())
				n[key] = xml2json(node);
			else
				n["#text"] = node->value();
			/* iterate through the attributes  */
			for(auto *attr = node->first_attribute(); attr; attr = attr->next_attribute())
				n[key][attr->name()] = attr->value();
			j[key].push_back(n[key]);
		} else {
			if (node->first_node()) {
				j[key] = xml2json(node);
			} else {
				j["#text"] = node->value();
			}
			/* iterate through the attributes  */
			for (auto *attr = node->first_attribute(); attr; attr = attr->next_attribute())
				j[key][attr->name()] = attr->value();
		}
	}

	return j;
}

/**
 * Recursively traverses the yaml tree and converts it into a nlohmann::json
 * object.  As yaml and json files are essentially the same we don't have to do
 * anything special in this function.
 */
inline json yaml2json(YAML::Node root)
{
	json j;

	switch (root.Type()) {
	case YAML::NodeType::Null:
		break;
	case YAML::NodeType::Scalar:
		return root.as<std::string>();
	case YAML::NodeType::Sequence:
		for (auto node : root)
			j.push_back(yaml2json(node));
		break;
	case YAML::NodeType::Map:
		for (const auto &it : root) {
			YAML::Node key   = it.first;
			YAML::Node value = it.second;
			j[key.as<std::string>()] = yaml2json(value);
		}
		break;
	default:
		break;
	}
	return j;
}

}

/**
 * Wrapper function for xml to nlohmann::json conversion.
 */
inline json xml2json(std::string str)
{
	json j;
	rapidxml::xml_document<> doc;
	try {
		doc.parse<0>(const_cast<char*>(str.data()));
	} catch (const rapidxml::parse_error &e) {
		std::cerr << "parse error: " << e.what() << "\n";
		throw ParseError();
    } catch (const std::exception &e) {
		std::cerr << "error: " << e.what() << "\n";
		return NULL;
	}
	rapidxml::xml_node<> *root = doc.first_node();
	if(root)
		j[root->name()] = detail::xml2json(root);
	return j;
}


/**
 * Wrapper function for yaml to nlohmann::json conversion.
 */
inline json yaml2json(std::string_view str)
{
    YAML::Node root;
    try {
        root = YAML::Load(std::string(str));
	} catch (const YAML::ParserException &e) {
		std::cerr << "parse error: " << e.what() << "\n";
		throw ParseError();
    } catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << "\n";
        return NULL;
    }
	return detail::yaml2json(root);
}

/**
 * Load a file containing a yaml document into a nlohmann::json object.
 */
inline json loadyaml(std::string_view filepath)
{
    YAML::Node root;
    try {
        root = YAML::LoadFile(std::string(filepath));
	} catch (const YAML::ParserException &e) {
		std::cerr << "parse error: " << e.what() << "\n";
		throw ParseError();
    } catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << "\n";
        return json{};
    }
	return detail::yaml2json(root);
}

/**
 * Load a file containing a xml document into a nlohmann::json object.
 */
inline json loadxml(std::string_view filepath)
{
	std::ifstream file(filepath.data());
	std::string str((std::istream_iterator<char>(file)), std::istream_iterator<char>());
	return xml2json(str);
}

}

#endif /* TOJSON_H */
