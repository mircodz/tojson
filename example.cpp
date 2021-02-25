#include <iostream>

#include "tojson.hpp"

int main() {
	nlohmann::json a = tojson::loadxml("./example.xml");
	nlohmann::json b = tojson::loadyaml("./example.yml");

	std::cout << a.dump(4) << std::endl;
	std::cout << b.dump(4) << std::endl;

	std::cout << tojson::emitters::toyaml(a) << std::endl;
	std::cout << tojson::emitters::toxml(b) << std::endl;

	return 0;
}
