#pragma once

#include <yaml-cpp/yaml.h>
#include <rapidxml.hpp>

#include "json.hpp"

#include <fstream>

/* Adding declarations to make it compatible with gcc 4.7 and greater */
#if __GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ > 40700
namespace rapidxml {
namespace internal {
template <class OutIt, class Ch> inline OutIt print_children(OutIt out, const xml_node<Ch>* node, int flags, int indent);
template <class OutIt, class Ch> inline OutIt print_attributes(OutIt out, const xml_node<Ch>* node, int flags);
template <class OutIt, class Ch> inline OutIt print_data_node(OutIt out, const xml_node<Ch>* node, int flags, int indent);
template <class OutIt, class Ch> inline OutIt print_cdata_node(OutIt out, const xml_node<Ch>* node, int flags, int indent);
template <class OutIt, class Ch> inline OutIt print_element_node(OutIt out, const xml_node<Ch>* node, int flags, int indent);
template <class OutIt, class Ch> inline OutIt print_declaration_node(OutIt out, const xml_node<Ch>* node, int flags, int indent);
template <class OutIt, class Ch> inline OutIt print_comment_node(OutIt out, const xml_node<Ch>* node, int flags, int indent);
template <class OutIt, class Ch> inline OutIt print_doctype_node(OutIt out, const xml_node<Ch>* node, int flags, int indent);
template <class OutIt, class Ch> inline OutIt print_pi_node(OutIt out, const xml_node<Ch>* node, int flags, int indent);
}  // namespace internal
}  // namespace rapidxml
#include "rapidxml_print.hpp"
#endif

#if __has_cpp_attribute(nodiscard)
#define TOJSON_NODISCARD [[nodiscard]]
#else
#define TOJSON_NODISCARD
#endif

namespace tojson {
namespace detail {

/// \todo refactor and pass nlohmann::json down by reference instead of returning it
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
			// toyaml through the attributes
			for (auto *attr = node->first_attribute(); attr; attr = attr->next_attribute())
				n[key][attr->name()] = attr->value();
			j[key].emplace_back(n[key]);
		} else {
			if (node->first_node())
				j[key] = xml2json(node);
			else
				j["@text"] = node->value();
			// toyaml through the attributes
			for (auto *attr = node->first_attribute(); attr; attr = attr->next_attribute())
				j[key][attr->name()] = attr->value();
		}
	}

	return j;
}

inline nlohmann::json parse_scalar(const YAML::Node &node) {
	int i;
	double d;
	bool b;
	std::string s;

	if (YAML::convert<int>::decode(node, i)) return i;
	if (YAML::convert<double>::decode(node, d)) return d;
	if (YAML::convert<bool>::decode(node, b)) return b;
	if (YAML::convert<std::string>::decode(node, s)) return s;

	return nullptr;
}

/// \todo refactor and pass nlohmann::json down by reference instead of returning it
inline nlohmann::json yaml2json(const YAML::Node &root) {
	nlohmann::json j{};

	switch (root.Type()) {
	case YAML::NodeType::Null: break;
	case YAML::NodeType::Scalar: return parse_scalar(root);
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

/// \todo handle @text entries better
inline void toyaml(const nlohmann::json &j, YAML::Emitter &e) {
	for (auto it = j.begin(); it != j.end(); ++it) {
		if (it->is_object()) {
			e << YAML::Key << it.key() << YAML::Value << YAML::BeginMap;
			toyaml(*it, e);
			e << YAML::EndMap;
		} else if (it->is_array()) {
			e << YAML::Key << it.key() << YAML::Value << YAML::BeginSeq;
			toyaml(it.value(), e);
			e << YAML::EndSeq;
		} else {
			if (it.key() == "@text") {
				e << YAML::Value << it.value().get<std::string>();
			} else {
				e << YAML::Key << it.key() << YAML::Value;
				if (it->type() == nlohmann::json::value_t::string)
					e << it.value().get<std::string>();
				else
					e << it->dump();
			}
		}
	}
}

// Forward declaration required here for circular dipedency.
inline void toxml(const nlohmann::json &j, rapidxml::xml_document<> &doc, rapidxml::xml_node<> *parent);

inline std::string repr(const nlohmann::json &j) {
	if (j.is_number()) return std::to_string(j.get<int>());
	if (j.is_boolean()) return j.get<bool>() ? "true" : "false";
	if (j.is_number_float()) return std::to_string(j.get<double>());
	if (j.is_string()) return j.get<std::string>();
	std::runtime_error("invalid type");
	return "";
}

/// \todo handle @text entries better
inline void toxml(const nlohmann::json &j,
                  rapidxml::xml_document<> &doc,
                  rapidxml::xml_node<> *parent,
                  const std::string &key) {
  // Not the prettiest of designs, but it works fine.
	for (auto it = j.begin(); it != j.end(); ++it) {
		if (it->is_object()) {
			auto *node = doc.allocate_node(rapidxml::node_element, doc.allocate_string(it.key().data()));
			detail::toxml(*it, doc, node);
			parent->append_node(node);
		} else if (it->is_array()) {
			detail::toxml(*it, doc, parent, key);
		} else {
			auto *node = doc.allocate_node(rapidxml::node_element, doc.allocate_string(key.data()));
			node->value(doc.allocate_string(repr(it.value()).data()));
			parent->append_node(node);
		}
	}
}

/// \todo handle @text entries better
inline void toxml(const nlohmann::json &j, rapidxml::xml_document<> &doc, rapidxml::xml_node<> *parent) {
	for (auto it = j.begin(); it != j.end(); ++it) {
		if (it->is_object()) {
			auto *node = doc.allocate_node(rapidxml::node_element, doc.allocate_string(it.key().data()));
			detail::toxml(*it, doc, node);
			parent->append_node(node);
		} else if (it->is_array()) {
			detail::toxml(*it, doc, parent, it.key());
		} else {
			auto *node = doc.allocate_node(rapidxml::node_element, doc.allocate_string(it.key().data()));
			node->value(doc.allocate_string(repr(it.value()).data()));
			parent->append_node(node);
		}
	}
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

namespace emitters {

/// \brief Generate string representation of json as an YAML document.
TOJSON_NODISCARD inline std::string toyaml(const nlohmann::json &j) {
	YAML::Emitter e;
	e << YAML::BeginDoc;
	if (j.is_object()) {
		e << YAML::BeginMap;
		detail::toyaml(j, e);
		e << YAML::EndMap;
	} else if (j.is_array()) {
		e << YAML::BeginSeq;
		detail::toyaml(j, e);
		e << YAML::EndSeq;
	}
	e << YAML::EndDoc;
	return e.c_str();
}

/// \brief Generate string representation of json as an XML document.
/// \param j Json object to convert, must have a single root
/// \throws std::runtime_error if the object have more than one root
TOJSON_NODISCARD inline std::string toxml(const nlohmann::json &j) {
	rapidxml::xml_document<> doc;
	auto *decl = doc.allocate_node(rapidxml::node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
	doc.append_node(decl);

	if (j.is_object() && j.size() == 1) {
		auto *root = doc.allocate_node(rapidxml::node_element, doc.allocate_string(j.begin().key().data()));
		detail::toxml(j.begin().value(), doc, root);
		doc.append_node(root);
	} else {
		throw std::runtime_error("json must have a single root node");
	}

	std::string xml_as_string;
	rapidxml::print(std::back_inserter(xml_as_string), doc);
	return xml_as_string;
}

}  // namespace emitters
}  // namespace tojson
