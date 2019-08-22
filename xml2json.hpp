#ifndef XML2JSON_H
#define XML2JSON_H

#include <string>
#include <iostream>

#include "rapidxml.hpp"
#include "json.hpp"
using json = nlohmann::json;

inline json xml2json(std::string xml);
inline json _xml2json(rapidxml::xml_node<> *root);

/**
 * Recursively traverses the xml tree and converts it into a nlohmann::json object.
 * TODO remove the #text key if object has no attributes.
 * TODO possibly reduce the calls to node->name()?
 */
json _xml2json(rapidxml::xml_node<> *root)
{
	std::map<std::string, int> occurrence;
	json j;

	for(rapidxml::xml_node<> *node = root->first_node(); node; node = node->next_sibling()) {
		occurrence[node->name()] += 1;

		if (occurrence[node->name()] == 2) /* convert the node to a list */
			j[node->name()] = { j[node->name()] };
		if (occurrence[node->name()] > 1) { /* in case there's a list of nodes */
			json n;
			if(node->first_node())
				n[node->name()] = _xml2json(node);
			else
				n["#text"] = node->value();
			for(rapidxml::xml_attribute<> *attr = node->first_attribute(); attr; attr = attr->next_attribute())
				n[node->name()][attr->name()] = attr->value();
			j[node->name()].push_back(n[node->name()]);
		} else { /* normal xml node */
			if(node->first_node())
				j[node->name()] = _xml2json(node);
			else
				j["#text"] = node->value();
			for(rapidxml::xml_attribute<> *attr = node->first_attribute(); attr; attr = attr->next_attribute())
				j[node->name()][attr->name()] = attr->value();
		}
	}

	return j;
}

/**
 * Wrapper function for the xml to json conversion.
 */
json xml2json(std::string xml)
{
	json j;
	rapidxml::xml_document<> doc;
	try {
		doc.parse<0>((char*)(xml.c_str()));
	} catch(...) {
		return j;
	}
	rapidxml::xml_node<> *root = doc.first_node();
	j[root->name()] = _xml2json(root);
	return j;
}

#endif /* XML2JSON_H */
