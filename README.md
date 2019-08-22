xml2json is a tiny (47 sloc), header only library to convert xml documents into nlohmann::json objects.

### minimal functioning example

```c++

#include "json.hpp"
#include "rapidxml.hpp"
#include "xml2json.hpp"

using json = nlohmann::json;

int main()
{

	json feed = xml2json("<note><to>Tove</to><from>Jani</from><heading>Reminder</heading><body>Don't forget me this weekend!</body></note>");
	std::cout << feed.dump(2) << std::endl;
	std::cout << feed["note"]["from"]["#text"].get<std::string>() << std::endl;

	return 0;

}

```

### dependencies

- rapidxml
- nlohmann::json
