#include "argparse.hpp"
#include "fmt/core.h"
#include "tokenizer/tokenizer.h"
#include "analyser/analyser.h"
#include "fmts.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

bool isBinary = false;

std::vector<miniplc0::Token> _tokenize(std::istream& input) {
	miniplc0::Tokenizer tkz(input);
	auto p = tkz.AllTokens();
	if (p.second.has_value()) {
		fmt::print(stderr, "Tokenization error: {}\n", p.second.value());
		exit(2);
	}
	return p.first;
}

void Tokenize(std::istream& input, std::ostream& output) {
	auto v = _tokenize(input);
	for (auto& it : v)
		output << fmt::format("{}\n", it);
	return;
}

void Analyse(std::istream& input, std::ostream& output){
	auto tks = _tokenize(input);
	miniplc0::Analyser analyser(tks);
	auto p = analyser.Analyse(output,isBinary);
	if (p.second.has_value()) {
		fmt::print(stderr, "Syntactic analysis error: {}\n", p.second.value());
		exit(2);
	}
	return;
}

int main(int argc, char** argv) {
	argparse::ArgumentParser program("cc0");
	program.add_argument("input")
		.help("speicify the file to be compiled.");
	program.add_argument("-s")
		.default_value(false)
		.implicit_value(true)
		.help("perform text instructions for the input file.");
	program.add_argument("-c")
		.default_value(false)
		.implicit_value(true)
		.help("perform binary files for the input file.");
	program.add_argument("-o", "--output")
		.required()
		.default_value(std::string("-"))
		.help("specify the output file.");

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {
		fmt::print(stderr, "{}\n\n", err.what());
		program.print_help();
		exit(2);
	}

	auto input_file = program.get<std::string>("input");
	auto output_file = program.get<std::string>("--output");
	std::istream* input;
	std::ostream* output;
	std::ifstream inf;
	std::ofstream outf;
	if (input_file != "-") {
		inf.open(input_file, std::ios::in);
		if (!inf) {
			fmt::print(stderr, "Fail to open {} for reading.\n", input_file);
			exit(2);
		}
		input = &inf;
	}
	else
		input = &std::cin;

	if (program["-s"] == true && program["-c"] == true) {
		fmt::print(stderr, "You can only perform text instructiongs or binary file.");
		exit(2);
	}
	if (program["-s"] == true) {
		std::ofstream outFile;
		std::string name = input_file;
		name.pop_back();
		name.pop_back();
		name.pop_back();
		name.append(".s0");
		if (output_file != "-") {
			name = output_file;
		}
		isBinary = false;
		outFile.open(name);
		output = &outFile;
		Analyse(*input, *output);
	}
	else if (program["-c"] == true) {
		std::ofstream outFile;
		std::string name = input_file;
		name.pop_back();
		name.pop_back();
		name.pop_back();
		name.append(".o0");
		if (output_file != "-") {
			name = output_file;
		}
		isBinary = true;
		outFile.open(name,std::ios::binary);
		output = &outFile;
		Analyse(*input, *output);
	}
	else {
		fmt::print(stderr, "You must choose text instructions or binary file.");
		exit(2);
	}
	return 0;
}