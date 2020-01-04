#include "tokenizer/tokenizer.h"
#include<string>
#include <cctype>
#include <sstream>
#include<iostream>
#include <cstring>
using namespace std;
namespace miniplc0 {

	bool isHex(char input) {
		char *hexsym = "0123456789abcdefABCDEF";
		for (int i = 0; i < 22; i++) {
			if (hexsym[i] == input)	return true;
		}
		return false;
	}
	//由16进制字符串返回10进制的值，如果非法返回-1，溢出返回-2
	long long Hex2Digit(char hexstr[]) {
		long long digit = 0;
		int len = strlen(hexstr);
		if (len == 1 && hexstr[0] == '0') return 0;
		if (len == 2) return -1;
		if (hexstr[0] != '0' || hexstr[1] != 'x')
			return -1;
		for (int i = 2; i < strlen(hexstr); i++) {
			if (hexstr[i] != '0')	break;
			len--;
		}
		if (len > 10)	return -2;
		for (int i = 2; i < strlen(hexstr); i++) {
			if (!miniplc0::isHex(hexstr[i])) return -1;
			if (miniplc0::islower(hexstr[i])) {
				digit = digit * 16 + (hexstr[i] + 10 - 'a');
			}
			else if (miniplc0::isupper(hexstr[i])) {
				digit = digit * 16 + (hexstr[i] + 10 - 'A');
			}
			else {
				digit = digit * 16 + (hexstr[i] - '0');
			}
		}
		if (digit > 2147483648) return -2;
		return digit;
	}

	std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::NextToken() {
		if (!_initialized)
			readAll();
		if (_rdr.bad())
			return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrorCode::ErrStreamError));
		if (isEOF())
			return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrorCode::ErrEOF));
		auto p = nextToken();
		if (p.second.has_value())
			return std::make_pair(p.first, p.second);
		auto err = checkToken(p.first.value());
		if (err.has_value())
			return std::make_pair(p.first, err.value());
		return std::make_pair(p.first, std::optional<CompilationError>());
	}

	std::pair<std::vector<Token>, std::optional<CompilationError>> Tokenizer::AllTokens() {
		std::vector<Token> result;
		while (true) {
			auto p = NextToken();
			if (p.second.has_value()) {
				if (p.second.value().GetCode() == ErrorCode::ErrEOF)
					return std::make_pair(result, std::optional<CompilationError>());
				else
					return std::make_pair(std::vector<Token>(), p.second);
			}
			if(p.first.has_value()&&p.first.value().GetType() != TokenType::NULL_TOKEN)
				result.emplace_back(p.first.value());
		}
	}

	// 注意：这里的返回值中 Token 和 CompilationError 只能返回一个，不能同时返回。
	std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::nextToken() {
		// 用于存储已经读到的组成当前token字符
		std::stringstream ss;
		// 分析token的结果，作为此函数的返回值
		std::pair<std::optional<Token>, std::optional<CompilationError>> result;
		// <行号，列号>，表示当前token的第一个字符在源代码中的位置
		std::pair<int64_t, int64_t> pos;
		// 记录当前自动机的状态，进入此函数时是初始状态
		DFAState current_state = DFAState::INITIAL_STATE;
		// 这是一个死循环，除非主动跳出
		// 每一次执行while内的代码，都可能导致状态的变更
		//用来检验注释末尾的星号
		bool end_star = false;
		while (true) {
			// 读一个字符，请注意auto推导得出的类型是std::optional<char>
			// 这里其实有两种写法
			// 1. 每次循环前立即读入一个 char
			// 2. 只有在可能会转移的状态读入一个 char
			// 因为我们实现了 unread，为了省事我们选择第一种
			auto current_char = nextChar();
			// 针对当前的状态进行不同的操作
			switch (current_state) {

				// 初始状态
				// 这个 case 我们给出了核心逻辑，但是后面的 case 不用照搬。
			case INITIAL_STATE: {
				// 已经读到了文件尾
				if (!current_char.has_value())
					// 返回一个空的token，和编译错误ErrEOF：遇到了文件尾
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrEOF));

				// 获取读到的字符的值，注意auto推导出的类型是char
				auto ch = current_char.value();
				// 标记是否读到了不合法的字符，初始化为否
				auto invalid = false;

				// 使用了自己封装的判断字符类型的函数，定义于 tokenizer/utils.hpp
				// see https://en.cppreference.com/w/cpp/string/byte/isblank
				 // 读到的字符是空白字符（空格、换行、制表符等）
				if (miniplc0::isspace(ch)) {
					current_state = DFAState::INITIAL_STATE; // 保留当前状态为初始状态，此处直接break也是可以的
				}
				else if (!miniplc0::isprint(ch)) // control codes and backspace
					invalid = true;
				else if (miniplc0::isdigit(ch) && ch != '0') // 读到的字符是数字
					current_state = DFAState::UNSIGNED_INTEGER_STATE; // 切换到无符号整数的状态
				else if (ch == '0')
					current_state = DFAState::HEX_STATE;
				else if (miniplc0::isalpha(ch)) // 读到的字符是英文字母
					current_state = DFAState::IDENTIFIER_STATE; // 切换到标识符的状态
				else {
					switch (ch) {
					case '=': // 如果读到的字符是`=`，则切换到等于号的状态
						current_state = DFAState::EQUAL_SIGN_STATE;
						break;
					case '>': 
						current_state = DFAState::MORE_THAN_STATE;
						break;
					case '<': 
						current_state = DFAState::LESS_THAN_STATE;
						break;
					case '-':
						current_state = DFAState::MINUS_SIGN_STATE;
						break;
						// 请填空：切换到减号的状态
					case '+':
						current_state = DFAState::PLUS_SIGN_STATE;
						break;
						// 请填空：切换到加号的状态
					case '*':
						current_state = DFAState::MULTIPLICATION_SIGN_STATE;
						break;
						// 请填空：切换状态
					case '/':
						current_state = DFAState::DIVISION_SIGN_STATE;
						break;
						// 请填空：切换状态

					///// 请填空：
					///// 对于其他的可接受字符
					///// 切换到对应的状态
					case '(':
						current_state = DFAState::LEFTBRACKET_STATE;
						break;
					case ')':
						current_state = DFAState::RIGHTBRACKET_STATE;
						break;
					case ';':
						current_state = DFAState::SEMICOLON_STATE;
						break;
					case ',':
						current_state = DFAState::COMMA_STATE;
						break;
					case '{':
						current_state = DFAState::LEFTBRACES_STATE;
						break;
					case '}':
						current_state = DFAState::RIGHTBACES_STATE;
						break;
					case '!':
						current_state = DFAState::NOT_STATE;
						break;
					// 不接受的字符导致的不合法的状态
					default:
						invalid = true;
						break;
					}
				}
				// 如果读到的字符导致了状态的转移，说明它是一个token的第一个字符
				if (current_state != DFAState::INITIAL_STATE)
					pos = previousPos(); // 记录该字符的的位置为token的开始位置
				// 读到了不合法的字符
				if (invalid) {
					// 回退这个字符
					unreadLast();
					// 返回编译错误：非法的输入
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
				}
				// 如果读到的字符导致了状态的转移，说明它是一个token的第一个字符
				if (current_state != DFAState::INITIAL_STATE) // ignore white spaces
					ss << ch; // 存储读到的字符
				break;
			}

			// 当前状态是无符号整数
			case UNSIGNED_INTEGER_STATE: {
				// 请填空：
				// 如果当前已经读到了文件尾，则解析已经读到的字符串为整数
				//     解析成功则返回无符号整数类型的token，否则返回编译错误
				if (!current_char.has_value()) {
					int len = ss.str().length();
					for (int i = 0; i < ss.str().length(); i++) {
						if (ss.str()[i] != '0') break;
						len--;
					}
					if (len > 10)
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrorCode::ErrIntegerOverflow));
					else {
						long long num = 0;
						ss >> num;
						if (num > 2147483648)
							return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrorCode::ErrIntegerOverflow));
						else
							return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, (int32_t)num, pos, currentPos()), std::optional<CompilationError>());
					}
				}
				// 如果读到的字符是数字，则存储读到的字符
				else if (miniplc0::isdigit(current_char.value())) {
					ss << current_char.value();
				}
				// 如果读到的是字母，则存储读到的字符，并切换状态到标识符
				else if (miniplc0::isalpha(current_char.value())) {
					ss << current_char.value();
					current_state = DFAState::IDENTIFIER_STATE;
				}
				// 如果读到的字符不是上述情况之一，则回退读到的字符，并解析已经读到的字符串为整数
				//     解析成功则返回无符号整数类型的token，否则返回编译错误
				else{
					unreadLast();
					int len = ss.str().length();
					for (int i = 0; i < ss.str().length(); i++) {
						if (ss.str()[i] != '0') break;
						len--;
					}
					if (len > 10)
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0,0, ErrorCode::ErrIntegerOverflow));
					else {
						long long num = 0;
						ss >> num;
						if (num > 2147483648)
							return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0,0, ErrorCode::ErrIntegerOverflow));
						else
							return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, (int32_t)num, pos, currentPos()), std::optional<CompilationError>());
					}
				}
				break;
			}
			case HEX_STATE: {
				if (!current_char.has_value()) {
					long long value = 0;
					value = miniplc0::Hex2Digit((char*)(ss.str().c_str()));
					if (value == -1) {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidIdentifier));
					}
					else if (value == -2) {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrorCode::ErrIntegerOverflow));
					}
					else {
						return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, (int32_t)value, pos, currentPos()), std::optional<CompilationError>());
					}
				}
				// 如果读到的字符是数字，则存储读到的字符
				else if (miniplc0::isHex(current_char.value())) {
					ss << current_char.value();
				}
				// 如果读到的是字母，则存储读到的字符，并切换状态到标识符
				else if (miniplc0::isalpha(current_char.value())) {
					ss << current_char.value();
				}
				// 如果读到的字符不是上述情况之一，则回退读到的字符，并解析已经读到的字符串为整数
				//     解析成功则返回无符号整数类型的token，否则返回编译错误
				else {
					unreadLast();
					long long value = 0;
					value = miniplc0::Hex2Digit((char*)(ss.str().c_str()));
					if (value == -1) {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidIdentifier));
					}
					else if (value == -2) {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrorCode::ErrIntegerOverflow));
					}
					else {
						return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, (int32_t)value, pos, currentPos()), std::optional<CompilationError>());
					}
				}
				break;
			}
			case IDENTIFIER_STATE: {
				// 请填空：
				// 如果当前已经读到了文件尾，则解析已经读到的字符串
				//     如果解析结果是关键字，那么返回对应关键字的token，否则返回标识符的token
				if (!current_char.has_value()) {
					string str = ss.str();
					if (str == "void") {
						return std::make_pair(std::make_optional<Token>(TokenType::VOID, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "int") {
						return std::make_pair(std::make_optional<Token>(TokenType::INT, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "char") {
						return std::make_pair(std::make_optional<Token>(TokenType::CHAR, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "double") {
						return std::make_pair(std::make_optional<Token>(TokenType::DOUBLE, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "const") {
						return std::make_pair(std::make_optional<Token>(TokenType::CONST, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "print") {
						return std::make_pair(std::make_optional<Token>(TokenType::PRINT, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "scan") {
						return std::make_pair(std::make_optional<Token>(TokenType::SCAN, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "return") {
						return std::make_pair(std::make_optional<Token>(TokenType::RETURN, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "break") {
						return std::make_pair(std::make_optional<Token>(TokenType::BREAK, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "continue") {
						return std::make_pair(std::make_optional<Token>(TokenType::CONTINUE, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "while") {
						return std::make_pair(std::make_optional<Token>(TokenType::WHILE, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "for") {
						return std::make_pair(std::make_optional<Token>(TokenType::FOR, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "do") {
						return std::make_pair(std::make_optional<Token>(TokenType::DO, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "switch") {
						return std::make_pair(std::make_optional<Token>(TokenType::SWITCH, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "case") {
						return std::make_pair(std::make_optional<Token>(TokenType::CASE, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "default") {
						return std::make_pair(std::make_optional<Token>(TokenType::DEFAULT, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "if") {
						return std::make_pair(std::make_optional<Token>(TokenType::IF, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "else") {
						return std::make_pair(std::make_optional<Token>(TokenType::ELSE, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "struct") {
						return std::make_pair(std::make_optional<Token>(TokenType::STRUCT, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (miniplc0::isdigit(str.at(0))) {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidIdentifier));
					}
					else {
						return std::make_pair(std::make_optional<Token>(TokenType::IDENTIFIER, str, pos, currentPos()), std::optional<CompilationError>());
					}
				}
				// 如果读到的是字符或字母，则存储读到的字符
				if (miniplc0::isalpha(current_char.value()) || miniplc0::isdigit(current_char.value())) {
					ss << current_char.value();
				}
				// 如果读到的字符不是上述情况之一，则回退读到的字符，并解析已经读到的字符串
				else {
					unreadLast();
					string str = ss.str();
					if (str == "void") {
						return std::make_pair(std::make_optional<Token>(TokenType::VOID, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "int") {
						return std::make_pair(std::make_optional<Token>(TokenType::INT, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "char") {
						return std::make_pair(std::make_optional<Token>(TokenType::CHAR, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "double") {
						return std::make_pair(std::make_optional<Token>(TokenType::DOUBLE, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "const") {
						return std::make_pair(std::make_optional<Token>(TokenType::CONST, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "print") {
						return std::make_pair(std::make_optional<Token>(TokenType::PRINT, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "scan") {
						return std::make_pair(std::make_optional<Token>(TokenType::SCAN, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "return") {
						return std::make_pair(std::make_optional<Token>(TokenType::RETURN, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "break") {
						return std::make_pair(std::make_optional<Token>(TokenType::BREAK, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "continue") {
						return std::make_pair(std::make_optional<Token>(TokenType::CONTINUE, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "while") {
						return std::make_pair(std::make_optional<Token>(TokenType::WHILE, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "for") {
						return std::make_pair(std::make_optional<Token>(TokenType::FOR, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "do") {
						return std::make_pair(std::make_optional<Token>(TokenType::DO, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "switch") {
						return std::make_pair(std::make_optional<Token>(TokenType::SWITCH, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "case") {
						return std::make_pair(std::make_optional<Token>(TokenType::CASE, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "default") {
						return std::make_pair(std::make_optional<Token>(TokenType::DEFAULT, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "if") {
						return std::make_pair(std::make_optional<Token>(TokenType::IF, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "else") {
						return std::make_pair(std::make_optional<Token>(TokenType::ELSE, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (str == "struct") {
						return std::make_pair(std::make_optional<Token>(TokenType::STRUCT, str, pos, currentPos()), std::optional<CompilationError>());
					}
					else if (miniplc0::isdigit(str.at(0))) {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidIdentifier));
					}
					else {
						return std::make_pair(std::make_optional<Token>(TokenType::IDENTIFIER, str, pos, currentPos()), std::optional<CompilationError>());
					}
				}
				//     如果解析结果是关键字，那么返回对应关键字的token，否则返回标识符的token
				break;
			}
			// 如果当前状态是加号
			case PLUS_SIGN_STATE: {
				// 请思考这里为什么要回退，在其他地方会不会需要
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::PLUS_SIGN, '+', pos, currentPos()), std::optional<CompilationError>());
			}
								  // 当前状态为减号的状态
			case MINUS_SIGN_STATE: {
				// 请填空：回退，并返回减号token
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::MINUS_SIGN, '-', pos, currentPos()), std::optional<CompilationError>());
			}

								   // 请填空：
								   // 对于其他的合法状态，进行合适的操作
								   // 比如进行解析、返回token、返回编译错误
			case MULTIPLICATION_SIGN_STATE: {
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::MULTIPLICATION_SIGN, '*', pos, currentPos()), std::optional<CompilationError>());
			}

			case DIVISION_SIGN_STATE: {
				if (current_char.value() == '*') {
					current_state = DFAState::DENY_STATE;
				}
				else if (current_char.value() == '/') {
					current_state = DFAState::DENY_LINE_STATE;
				}
				else {
					unreadLast(); // Yes, we unread last char even if it's an EOF.
					return std::make_pair(std::make_optional<Token>(TokenType::DIVISION_SIGN, '/', pos, currentPos()), std::optional<CompilationError>());
				}
				break;
			}

			case LEFTBRACKET_STATE: {
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::LEFT_BRACKET, '(', pos, currentPos()), std::optional<CompilationError>());
			}

			case RIGHTBRACKET_STATE: {
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::RIGHT_BRACKET, ')', pos, currentPos()), std::optional<CompilationError>());
			}

			case EQUAL_SIGN_STATE: {
				if (current_char == '=') {
					current_state == DFAState::INITIAL_STATE;
					//cout << "get double equal" << endl;
					return std::make_pair(std::make_optional<Token>(TokenType::DOUBLE_EQAUL_SIGN, 'E', pos, currentPos()), std::optional<CompilationError>());
				}
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				//cout << "get equal" << endl;
				return std::make_pair(std::make_optional<Token>(TokenType::EQUAL_SIGN, '=', pos, currentPos()), std::optional<CompilationError>());
			}
			case NOT_STATE: {
				if (current_char == '=') {
					current_state == DFAState::INITIAL_STATE;
					//cout << "get not equal" << endl;
					return std::make_pair(std::make_optional<Token>(TokenType::NOT_EQUAL_SIGN ,'N', pos, currentPos()), std::optional<CompilationError>());
				}
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				//cout << "get not" << endl;
				return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::InputNot));
				//return std::make_pair(std::make_optional<Token>(TokenType::NOT_SIGN, '!', pos, currentPos()), std::optional<CompilationError>());
			}
			case MORE_THAN_STATE: {
				if (current_char == '=') {
					current_state == DFAState::INITIAL_STATE;
					//cout << "get not less" << endl;
					return std::make_pair(std::make_optional<Token>(TokenType::MORE_EQUAL, 'M', pos, currentPos()), std::optional<CompilationError>());
				}
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				//cout << "get more" << endl;
				return std::make_pair(std::make_optional<Token>(TokenType::MORE_THAN, '>', pos, currentPos()), std::optional<CompilationError>());
			}
			case LESS_THAN_STATE: {
				if (current_char == '=') {
					current_state == DFAState::INITIAL_STATE;
					//cout << "get not more" << endl;
					return std::make_pair(std::make_optional<Token>(TokenType::LESS_EQUAL, 'L', pos, currentPos()), std::optional<CompilationError>());
				}
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				//cout << "get less" << endl;
				return std::make_pair(std::make_optional<Token>(TokenType::LESS_THAN, '<', pos, currentPos()), std::optional<CompilationError>());
			}
			case SEMICOLON_STATE: {
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::SEMICOLON, ';', pos, currentPos()), std::optional<CompilationError>());
			}
			case COMMA_STATE: {
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::COMMA, ',', pos, currentPos()), std::optional<CompilationError>());
			}
			case LEFTBRACES_STATE: {
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::LEFT_BRECES, '{', pos, currentPos()), std::optional<CompilationError>());
			}
			case RIGHTBACES_STATE: {
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::RIGHT_BRECES, '}', pos, currentPos()), std::optional<CompilationError>());
			}
			case DENY_LINE_STATE: {
				if (current_char.value() == '\n' || current_char.value() == '\r')
					return std::make_pair(std::make_optional<Token>(TokenType::NULL_TOKEN, ' ', pos, currentPos()), std::optional<CompilationError>());
				break;
			}
			case DENY_STATE: {
				if (current_char.value() == '*')
					end_star = true;
				if (end_star) {
					if (current_char.value() == '/') {
						end_star = false;
						return std::make_pair(std::make_optional<Token>(TokenType::NULL_TOKEN, ' ', pos, currentPos()), std::optional<CompilationError>());
					}
					else if (current_char.value() != '*') {
						end_star = false;
					}
				}
				break;
			}
								   // 预料之外的状态，如果执行到了这里，说明程序异常
			default:
				DieAndPrint("unhandled state.");
				break;
			}
		}
		// 预料之外的状态，如果执行到了这里，说明程序异常
		return std::make_pair(std::optional<Token>(), std::optional<CompilationError>());
	}

	std::optional<CompilationError> Tokenizer::checkToken(const Token& t) {
		switch (t.GetType()) {
			case IDENTIFIER: {
				auto val = t.GetValueString();
				if (miniplc0::isdigit(val[0]))
					return std::make_optional<CompilationError>(t.GetStartPos().first, t.GetStartPos().second, ErrorCode::ErrInvalidIdentifier);
				break;
			}
		default:
			break;
		}
		return {};
	}

	void Tokenizer::readAll() {
		if (_initialized)
			return;
		for (std::string tp; std::getline(_rdr, tp);)
			_lines_buffer.emplace_back(std::move(tp + "\n"));
		_initialized = true;
		_ptr = std::make_pair<int64_t, int64_t>(0, 0);
		return;
	}

	// Note: We allow this function to return a postion which is out of bound according to the design like std::vector::end().
	std::pair<uint64_t, uint64_t> Tokenizer::nextPos() {
		if (_ptr.first >= _lines_buffer.size())
			DieAndPrint("advance after EOF");
		if (_ptr.second == _lines_buffer[_ptr.first].size() - 1)
			return std::make_pair(_ptr.first + 1, 0);
		else
			return std::make_pair(_ptr.first, _ptr.second + 1);
	}

	std::pair<uint64_t, uint64_t> Tokenizer::currentPos() {
		return _ptr;
	}

	std::pair<uint64_t, uint64_t> Tokenizer::previousPos() {
		if (_ptr.first == 0 && _ptr.second == 0)
			DieAndPrint("previous position from beginning");
		if (_ptr.second == 0)
			return std::make_pair(_ptr.first - 1, _lines_buffer[_ptr.first - 1].size() - 1);
		else
			return std::make_pair(_ptr.first, _ptr.second - 1);
	}

	std::optional<char> Tokenizer::nextChar() {
		if (isEOF())
			return {}; // EOF
		auto result = _lines_buffer[_ptr.first][_ptr.second];
		_ptr = nextPos();
		return result;
	}

	bool Tokenizer::isEOF() {
		return _ptr.first >= _lines_buffer.size();
	}

	// Note: Is it evil to unread a buffer?
	void Tokenizer::unreadLast() {
		_ptr = previousPos();
	}
}