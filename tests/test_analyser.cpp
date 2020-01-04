#include "catch2/catch.hpp"

#include "instruction/instruction.h"
#include "tokenizer/tokenizer.h"
#include "analyser/analyser.h"
#include "simple_vm.hpp"

#include <sstream>
#include <vector>

std::ostream& output = std::cout;

TEST_CASE("1", "[valid]") {
	std::string input =
		"const int a = 1+i;"
		;
	std::stringstream ss;
	ss.str(input);
	miniplc0::Tokenizer tkz(ss);
	auto tokens = tkz.AllTokens();
	if (tokens.second.has_value())
		FAIL();
	miniplc0::Analyser analyser(tokens.first);
	auto instructions = analyser.Analyse(output,false);
	REQUIRE((instructions.second.has_value()));
}

TEST_CASE("2", "[valid]") {
	std::string input =
		"int a = 3,b,c,d=1;\n"
		"const int e=1;\n"
		"int return1(){\n"
			"int a = 1;\n"
			"return 1;\n"
		"}\n"
		"void return2(int a,int b,int c){\n"
			"int e;\n"
			"int i=0 ;\n"
			"int result;"
				"{\n"
				"a = 7;b=4;\n"
				"a = d;\n"
				"result = a+b;\n"
					"{{{{{{\n"
						"print(123,a,b);\n"
					"}}}}}}\n"
				"}\n"
			"while(i<=8){\n"
				"i = i+1;\n"
			"}\n"
			"while(123){\n"
				"return ;"
			"}\n"
			"print(234,return1());\n"
			"print();"
			"scan(i);\n"
			"if(i){;;;}"
			"else{i = return1();}\n"
		"}\n"
		"void main(){\n"
		"return2(1,2,3);\n"
		";;;;\n"
		"}\n"
		;
	std::stringstream ss;
	ss.str(input);
	miniplc0::Tokenizer tkz(ss);
	auto tokens = tkz.AllTokens();
	if (tokens.second.has_value())
		FAIL();
	miniplc0::Analyser analyser(tokens.first);
	auto instructions = analyser.Analyse(output, false);
	REQUIRE(!(instructions.second.has_value()));
}

TEST_CASE("3", "[valid]") {
	std::string input =



		"void swap(int x, int y)\n"
	"{\n"
		"int temp;\n"
		"temp = x;\n"
		"x = y;\n"
		"y = temp;\n"
	"}\n"
	"int mod(int x, int y)\n"
	"{\n"
		"return (x - x / y * y);\n"
	"}\n"
	"void main()\n"
	"{\n"
		"int a, b, c, temp1, temp2, result1, result2;\n"
		"print();\n"
		"scan(a);\n"
		"print();\n"
		"scan(b);\n"
		"temp1 = a;\n"
		"temp2 = b;\n"
		"if (temp1 != 0)\n"
		"{\n"
			"print(temp1);\n"
			"print(temp2);\n"
			"swap(temp1, temp2);\n"
			"print(temp1);\n"
			"print(temp2);\n"
			"c = mod(a, b);\n"
			"print(c);\n"
		"}\n"
		"result1 = temp2;\n"
		"result2 = a * b / result1;\n"
		"print(result1);\n"
		"print(result2);\n"
	"}\n"
		;
	std::stringstream ss;
	ss.str(input);
	miniplc0::Tokenizer tkz(ss);
	auto tokens = tkz.AllTokens();
	if (tokens.second.has_value())
		FAIL();
	miniplc0::Analyser analyser(tokens.first);
	auto instructions = analyser.Analyse(output, false);
	REQUIRE(!(instructions.second.has_value()));
}

TEST_CASE("4", "[valid]") {
	std::string input =
	
		"void main()\n"
	"{\n"
		"int a=1, b=2;\n"
		"print(a);\n"
		"scan(a);\n"
		"print(b);\n"
		"scan(b);\n"
		"while (b != 0)\n"
		"{\n"
			"print((a + b)*a);\n"
			"print(a - b);\n"
			"print(a * b);\n"
			"print(a / b);\n"
			"print(a);\n"
			"scan(a);\n"
			"print(b);\n"
			"scan(b);\n"
		"}\n"
	"}\n"
		;
	std::stringstream ss;
	ss.str(input);
	miniplc0::Tokenizer tkz(ss);
	auto tokens = tkz.AllTokens();
	if (tokens.second.has_value())
		FAIL();
	miniplc0::Analyser analyser(tokens.first);
	auto instructions = analyser.Analyse(output, false);
	REQUIRE(!(instructions.second.has_value()));
}

TEST_CASE("5", "[valid]") {
	std::string input =
		"const int PI = 3;\n"
	"int radius=0, area, perimeter;\n"
	"void main(){\n"
		"print(PI);\n"
		"scan(radius);\n"
		"if (radius < 0)\n"
			"print(0);\n"
		"else{\n"
			"print(radius);\n"
			"perimeter = 2 * PI * radius;\n"
			"area = PI * radius * radius;\n"
		"}\n"
		"print(PI);\n"
		"print(area);\n"
	"}\n"
		;
	std::stringstream ss;
	ss.str(input);
	miniplc0::Tokenizer tkz(ss);
	auto tokens = tkz.AllTokens();
	if (tokens.second.has_value())
		FAIL();
	miniplc0::Analyser analyser(tokens.first);
	auto instructions = analyser.Analyse(output, false);
	REQUIRE(!(instructions.second.has_value()));
}
TEST_CASE("6", "[valid]") {
	std::string input =
	"int pi = 3;\n"
	"int N = 0xbabe;\n"
	"int ma;\n"
	"int max;\n"
	"void k(){}\n"
	"int fib(int n) {\n"
		"if (n <= 0) return 0;\n"
		"}"
		;
	std::stringstream ss;
	ss.str(input);
	miniplc0::Tokenizer tkz(ss);
	auto tokens = tkz.AllTokens();
	if (tokens.second.has_value())
		FAIL();
	miniplc0::Analyser analyser(tokens.first);
	auto instructions = analyser.Analyse(output, false);
	REQUIRE(!(instructions.second.has_value()));
}