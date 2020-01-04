#include "catch2/catch.hpp"
#include "tokenizer/tokenizer.h"
#include "fmt/core.h"

#include <sstream>
#include <vector>

TEST_CASE("invalid_characters", "[invalid]") {
	std::string input = 
		"int cnt = 0;\n"
		"void move(int id, int from, int to) {\n"
		"cnt = cnt + 1;\n"
		"print(cnt, id, from, to);\n"
	"}\n"
	"/*abced*/"
	"void hanoi(int n, int x, int y, int z) {\n"
		"if (n == 0)\n"
			"return;\n"
		"hanoi(n - 1, x, z, y);\n"
		"move(n, x, z);\n"
		"hanoi(n - 1, y, x, z);\n"
	"}\n"
	"int main(){\n"
		"int n;\n"
		"cnt = 0;\n"
		"scan(n);\n"
		"hanoi(n, 1, 2, 3);\n"
		"return 0;\n"
	"}"
		"---";
	std::stringstream ss;
	ss.str(input);
	miniplc0::Tokenizer tkz(ss);
	auto result = tkz.AllTokens();
	REQUIRE(!(result.second.has_value()));
}

