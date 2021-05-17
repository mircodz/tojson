tojson is a tiny (127 sloc), header only library to convert xml and yaml documents into nlohmann::json objects.

### minimal functioning example

```c++
#include <iostream>

#include "tojson.hpp"

int main() {
	using namespace tojson;
	using namespace tojson::emitters;

	nlohmann::json a = loadxml("./example.xml");
	nlohmann::json b = loadyaml("./example.yml");

	std::cout << a.dump() << std::endl;
	std::cout << b.dump() << std::endl;

	std::cout << toyaml(a) << std::endl;
	std::cout << toxml(b) << std::endl;
}
```

### dependencies

- rapidxml
- yaml-cpp
- nlohmann::json
