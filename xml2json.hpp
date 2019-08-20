#ifndef XML2JSON_H
#define XML2JSON_H

#include <string>

#include "rapidxml.hpp"
#include "json.hpp"
using json = nlohmann::json;

inline json xml2json(std::string xml);
inline json _xml2json(rapidxml::xml_node<> *root);

json _xml2json(rapidxml::xml_node<> *root)
{

	std::map<std::string, int> occurrence;

	json j;

	for(rapidxml::xml_node<> *node = root->first_node(); node; node = node->next_sibling()) {
		occurrence[node->name()] += 1;

		if (occurrence[node->name()] > 1) { /* list of xml nodes */
			if (occurrence[node->name()] == 2)
				j = { j }; /* convert existing json node to an array */

			json n;

			for(rapidxml::xml_attribute<> *attr = node->first_attribute(); attr; attr = attr->next_attribute())
				n[node->name()][attr->name()] = attr->value();

			if(node->type() == rapidxml::node_data || node->type() == rapidxml::node_cdata)
				n[node->name()]["@val"] = node->value();
			else if(node->type() == rapidxml::node_element)
				n[node->name()]["@val"] = _xml2json(node);

			j.push_back(n);

		} else { /* normal xml node */
			for(rapidxml::xml_attribute<> *attr = node->first_attribute(); attr; attr = attr->next_attribute())
				j[node->name()][attr->name()] = attr->value();

			if(node->type() == rapidxml::node_data || node->type() == rapidxml::node_cdata)
				j[node->name()]["@val"] = node->value();
			else if(node->type() == rapidxml::node_element)
				j[node->name()]["@val"] = _xml2json(node);
		}
	}

	return j;

}

json xml2json(std::string xml)
{
	json j;
	rapidxml::xml_document<> doc;
	doc.parse<0>((char*)(xml.c_str()));
	rapidxml::xml_node<> *root = doc.first_node();
	j[root->name()] = _xml2json(root);
	return j;
}

#endif /* XML2JSON_H */
