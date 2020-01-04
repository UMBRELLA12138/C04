#include "analyser.h"
#include <algorithm>
#include <vector>
#include <fstream>
#include <climits>
#include <string>
#include <sstream>
#include <iostream>
#include <cstring>
namespace miniplc0 {
	int32_t returnCount = 0;
	int32_t loopLevel = 0;//循环的层数，用于判定break和continue的位置是否正确，由于没有实现switch，它们只能出现在循环语句中
	bool isText = false;
	std::pair<std::vector<Instruction>, std::optional<CompilationError>> Analyser::Analyse(std::ostream& output,bool isBinary) {
		globalTable.init();
		partTable.init();
		auto err = analyseProgram();
		if (err.has_value())
			return std::make_pair(std::vector<Instruction>(), err);
		else {
			if (isBinary) {
				printBinary(output);
			}
			else {
				printTextFile(output);
			}
			return std::make_pair(_instructions, std::optional<CompilationError>());
		}
	}
	// <程序> ::= <变量定义><函数定义>
	std::optional<CompilationError> Analyser::analyseProgram() {
		//std::cout << "<程序>" << std::endl;
		auto err = analyseVariableDeclaration();
		if (err.has_value()) {
		//	std::cout << err.value().GetCode() <<" at (" << err.value().GetPos().first << "," << err.value().GetPos().second << ")" << std::endl;
			return err;
		}
		err = analyseFunctionDeclaration();
		if (err.has_value())
		//	std::cout << err.value().GetCode() << " at (" << err.value().GetPos().first << "," << err.value().GetPos().second << ")" << std::endl;
			return err;
		return {};
	}
	// <变量定义> ::= {<变量声明语句>}
	// <变量声明语句> ::= ['const']('int'|'void')<变量声名序列>';'
	std::optional<CompilationError> Analyser::analyseVariableDeclaration() {
	//	std::cout << "<变量定义>" << std::endl;
		while (true) {
			auto constFlag = false;//这个flag用来判断是常量声明还是变量声明
			auto next = nextToken();
			if (!next.has_value())
				return {};
			if (next.value().GetType() == TokenType::CONST) {
				constFlag = true;
				next = nextToken();
			}
			if (next.value().GetType() == TokenType::INT) {
				next = nextToken();
				if (!next.has_value()) {
	//				std::cout << "Err pos 1" << std::endl;
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidIdentifier);
				}
				if (next.value().GetType() == IDENTIFIER) {
					next = nextToken();
					if (!next.has_value()) {
	//					std::cout << "Err pos 2" << std::endl;
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidVariableDeclaration);
					}
					if (next.value().GetType() == TokenType::LEFT_BRACKET) {
						unreadToken();
						unreadToken();
						unreadToken();
						if (constFlag) {
							unreadToken();
						}
						return {};
					}
					else {
						unreadToken();
						unreadToken();
						auto err = analyseVarDeclareList(constFlag);
						if (err.has_value()) {
							return err;
						}
						next = nextToken();
						if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
	//						std::cout << "Err pos 3" << std::endl;
							return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
						}
					}
				}
				else {
		//			std::cout << "Err pos 4" << std::endl;
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidIdentifier);
				}
			}
			else if(next.value().GetType() == TokenType::VOID) {
				if (constFlag) {
			//		std::cout << "Err pos 5" << std::endl;
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::declareVoidConst);
				}
				else {
					auto next = nextToken();
					if (!next.has_value()) {
				//		std::cout << "Err pos 6" << std::endl;
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidIdentifier);
					}
					if (next.value().GetType() == TokenType::IDENTIFIER) {
						auto next = nextToken();
						if (!next.has_value()) {
				//			std::cout << "Err pos 7" << std::endl;
							return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidIdentifier);
						}
						if (next.value().GetType() == TokenType::LEFT_BRACKET) {
							unreadToken();
							unreadToken();
							unreadToken();
							return {};
						}
						else {
				//			std::cout << "Err pos 8" << std::endl;
							return std::make_optional<CompilationError>(_current_pos, ErrorCode::declareVoidVar);
						}
					}
					else {
				//		std::cout << "Err pos 9" << std::endl;
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidIdentifier);
					}
				}
			}
			else {
				if (!constFlag) {
					unreadToken();
					return {};
				}
				else {
			//		std::cout << "Err pos 10" << std::endl;
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstantNeedValue);
				}
			}
		}
		return {};
	}
	//<变量声名序列>::=<声名>{','<声名>}
	std::optional<CompilationError> Analyser::analyseVarDeclareList(bool constFlag) {
	//	std::cout << "<变量声名序列>" << std::endl;
		auto err = analyzeDeclare(constFlag);
		if (err.has_value()) {
			return err;
		}
		while (true) {
			auto next = nextToken();
			if (!next.has_value()) {
			//	std::cout << "Err pos 11" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			}
			if (next.value().GetType() == TokenType::COMMA) {
				auto err = analyzeDeclare(constFlag);
				if (err.has_value()) {
					return err;
				}
			}
			else {
				unreadToken();
				return {};
			}
		}
		return {};
	}
	//<声名>::=<标识符>['='<表达式>]
	std::optional<CompilationError> Analyser::analyzeDeclare(bool constFlag) {
	//	std::cout << "<声名>" << std::endl;
		auto next = nextToken();
		if (!next.has_value()) {
		//	std::cout << "Err pos 12" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidIdentifier);
		}
		if (next.value().GetType() == TokenType::IDENTIFIER) {
			auto tk = next;
			if (constFlag) {
				next = nextToken();
				if (!next.has_value() || next.value().GetType() != TokenType::EQUAL_SIGN) {
			//		std::cout << "Err pos 13" << std::endl;
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstantNeedValue);
				}
				else {
					//把有赋值的const标识符存进符号表
					if (!insertTable(tk.value(), true, true)) {
			//			std::cout << "ReDeclare1" << std::endl;
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
					}
					auto err = analyseExpression();
					if (err.has_value())
						return err;
					return {};
				}
			}
			else {
				next = nextToken();
				if (!next.has_value()) {
					//std::cout << "Err pos 14" << std::endl;
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidIdentifier);
				}
				if (next.value().GetType() == TokenType::EQUAL_SIGN) {
					//有赋值的int型存入符号表
					if (!insertTable(tk.value(), false, true)) {
			//			std::cout << "ReDeclare2" << std::endl;
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
					}
					auto err = analyseExpression();
					if (err.has_value()) {
						return err;
					}
				}
				else {
					//没有赋值的int存入符号表
					if (!insertTable(tk.value(), false,false)) {
			//			std::cout << "ReDeclare3" << std::endl;
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
					}
					addInstruction(Instruction(ipush, 0));
					unreadToken();
				}
			}
			return {};
		}
		else {
		//	std::cout << "Err pos 57" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidIdentifier);
		}
		return {};
	}
	// <函数定义> ::= {<函数内容>}
	//<函数内容>::=('void'|'int')<标识符>'('[<参数列表>]')'<复合语句>
	std::optional<CompilationError> Analyser::analyseFunctionDeclaration() {
		//std::cout << "<函数定义>" << std::endl;
		while (true) {
			//将解析状态切换为局部，此时优先选用局部表
			isGlobal = false;
			partTable.init();
			bool needReturn = true;
			returnCount = 0;
			//类型
			auto next = nextToken();
			if (!next.has_value()) {
				return {};
			}
			if (next.value().GetType() == TokenType::VOID) {
				needReturn = false;
			}
			else if (next.value().GetType() == TokenType::INT) {
				needReturn = true;
			}
			else {
			//	std::cout << "Err pos 15" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::functionNeedType);
			}
			//标识符
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER) {
			//	std::cout << "Err pos 16" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidIdentifier);
			}
			Token tk = next.value();
			if (!needReturn) {
				addVoid(tk);
			}
			//左括号
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
			//	std::cout << "Err pos 17" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
			}
			next = nextToken(); 
			if (next.has_value() && next.value().GetType() == TokenType::RIGHT_BRACKET) {
				//说明没有参数，直接把函数放进函数表，参数个数为0
				if (!insertFunction(tk, 0)) {
			//		std::cout << "FuntionReDeclare1" << std::endl;
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
				}
				unreadToken();
			}
			else {
				unreadToken();
				auto err = analyseParams(tk);
				if (err.has_value()) {
					return err;
				}
			}
			//右括号
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
			//	std::cout << "Err pos 18" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
			}
			//复合语句
			auto err = analyseCompoundStatement();
			if (err.has_value()) {
				return err;
			}
			if (needReturn && returnCount == 0) {
			//	std::cout << "Err pos 19" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossReturnValue);
			}
			if (!needReturn && returnCount > 0) {
			//	std::cout << "Err pos 20" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::doNotNeedReturnValue);
			}
			if (needReturn) {
				addInstruction(Instruction(ipush, 0));
				addInstruction(Instruction(iret));
			}
			else
				addInstruction(Instruction(ret));
			//释放局部表，状态回归为全局
			isGlobal = true;
			partTable.init();
		}
		return {};
	}
	//<参数序列>::=<参数>{','<参数>}
	std::optional<CompilationError> Analyser::analyseParams(Token& tk) {
	//	std::cout << "<参数序列>" << std::endl;
		int32_t countParam = 1;
		auto err = analyseParam();
		if (err.has_value()) {
			return err;
		}
		while (true) {
			auto next = nextToken();
			if (!next.has_value()) {
			//	std::cout << "Err pos 21" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
			}
			if (next.value().GetType() == COMMA) {
				err = analyseParam();
				if (err.has_value()) {
					return err;
				}
				countParam++;
			}
			else {
				unreadToken();
				//解析出来之前把函数写入函数表
				if (!insertFunction(tk, countParam)) {
			//		std::cout << "FuntionReDeclare2" << std::endl;
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
				}
				return {};
			}
		}
		return {};
	}
	//<参数>::=[const](int|void)<标识符>
	std::optional<CompilationError> Analyser::analyseParam() {
	//	std::cout << "<参数>" << std::endl;
		bool constFlag = false;
		auto next = nextToken();
		if (!next.has_value()) {
		//	std::cout << "Err pos 22" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::paramNeedType);
		}
		//[const]
		if (next.value().GetType() == TokenType::CONST) {
			constFlag = true;
			next = nextToken();
			if (!next.has_value()) {
		//		std::cout << "Err pos 23" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::paramNeedType);
			}
		}
		//(int|void)
		if (next.value().GetType() == TokenType::INT) {

		}
		else if (next.value().GetType() == TokenType::VOID) {
		//	std::cout << "Err pos 24" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::voidParam);
		}
		else {
		//	std::cout << "Err pos 25" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::invalidParamDeclare);
		}
		//<标识符>
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER) {
		//	std::cout << "Err pos 26" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::invalidParamDeclare);
		}
		//参数要写入局部向量表,由于调用有参函数必定会传参，所以参数为初始化过的变量
		if (insertTable(next.value(), constFlag, true));
		return {};
	}
	//<复合语句>::='{'<变量定义><语句>'}'
	std::optional<CompilationError> Analyser::analyseCompoundStatement() {
		//std::cout << "<复合语句>" << std::endl;
		auto next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRECES) {
		//	std::cout << "Err pos 27" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBraces);
		}
		auto err = analyseVariableDeclaration();
		if (err.has_value()) {
			return err;
		}
		err = analyseStatement();
		if (err.has_value()) {
			return err;
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRECES) {
		//	std::cout << "Err pos 28" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBraces);
		}
		return{};
	}
	// <语句> ::= '{'{<语句>}'}'
	// <语句> :: = <赋值语句> | <输出语句> | <空语句>
	// <赋值语句> :: = <标识符>'='<表达式>';'
	//<输入语句> ::= 'scan' '(' <表达式> ')' ';'
	// <输出语句> :: = 'print' '(' <表达式> ')' ';'
	//<函数调用语句> ::= <标识符>'('<表达式>')'';'
	// <空语句> :: = ';'
	std::optional<CompilationError> Analyser::analyseStatement() {
	//	std::cout << "<语句>" << std::endl;
		bool hasReturn = false;
		while (true) {
			auto next = nextToken();
			if (!next.has_value())
				return {};
			unreadToken();
			if (next.value().GetType() != TokenType::IDENTIFIER &&
				next.value().GetType() != TokenType::LEFT_BRECES &&
				next.value().GetType() != TokenType::PRINT &&
				next.value().GetType() != TokenType::SCAN &&
				next.value().GetType() != TokenType::WHILE &&
				next.value().GetType() != TokenType::RETURN &&
				next.value().GetType() != TokenType::IF &&
				next.value().GetType() != TokenType::DO &&
				next.value().GetType() != TokenType::FOR &&
				next.value().GetType() != TokenType::BREAK &&
				next.value().GetType() != TokenType::CONTINUE &&
				next.value().GetType() != TokenType::SEMICOLON) {
				return {};
			}
			auto err = analyseSingleStatement();
			if (err.has_value()) {
				return err;
			}
		}
		return {};
	}
	//<单句>
	std::optional<CompilationError> Analyser::analyseSingleStatement() {
		auto next = nextToken();
		if (!next.has_value())
			return {};
		unreadToken();
		if (next.value().GetType() != TokenType::IDENTIFIER &&
			next.value().GetType() != TokenType::LEFT_BRECES &&
			next.value().GetType() != TokenType::PRINT &&
			next.value().GetType() != TokenType::SCAN &&
			next.value().GetType() != TokenType::WHILE &&
			next.value().GetType() != TokenType::RETURN &&
			next.value().GetType() != TokenType::IF &&
			next.value().GetType() != TokenType::DO &&
			next.value().GetType() != TokenType::FOR &&
			next.value().GetType() != TokenType::BREAK &&
			next.value().GetType() != TokenType::CONTINUE &&
			next.value().GetType() != TokenType::SEMICOLON) {
			return {};
		}
		std::optional<CompilationError> err;
		switch (next.value().GetType()) {
			// 这里需要你针对不同的预读结果来调用不同的子程序
		case TokenType::IDENTIFIER: {
			auto judge = nextToken();
			judge = nextToken();
			unreadToken();
			unreadToken();
			if (judge.has_value() && judge.value().GetType() == TokenType::LEFT_BRACKET) {
				auto err = analyseFunctionCall();
				if (err.has_value())
					return err;
			}
			else {
				auto err = analyseAssignmentStatement();
				if (err.has_value())
					return err;
			}
			next = nextToken();
			if (next.value().GetType() != SEMICOLON) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			}
			break;
		}
		case TokenType::LEFT_BRECES: {
			auto next = nextToken();
			auto err = analyseStatement();
			if (err.has_value())
				return err;
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRECES) {
			//		std::cout << "Err pos 29" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBraces);
			}
			break;
		}
		case TokenType::PRINT: {
			auto err = analyseOutputStatement();
			if (err.has_value())
				return err;
			break;
		}
		case TokenType::SCAN: {
			auto err = analyseScanStatement();
			if (err.has_value())
				return err;
			break;
		}
		case TokenType::WHILE: {
			auto err = analyseLoopStatement();
			if (err.has_value())
				return err;
			break;
		}
		case TokenType::RETURN: {
			auto err = analyseReturnStatement();
			if (err.has_value())
				return err;
			break;
		}
		case TokenType::IF: {
			auto err = analyseConditionStatement();
			if (err.has_value())
				return err;
			break;
		}
		case TokenType::DO: {
			auto err = analyseDoStatement();
			if (err.has_value())
				return err;
			break;
		}
		case TokenType::FOR: {
			auto err = analyseForStatement();
			if (err.has_value())
				return err;
			break;
		}
		case TokenType::BREAK: {
		//	std::cout << "break" << std::endl;
			auto next = nextToken();
			if (loopLevel <= 0) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::breakOutOfLoop);
			}
			addInstruction(Instruction(jmp, -1));//标记一下，-1代表需要break
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			}
		//	std::cout << "endbreak" << std::endl;
			break;
		}
		case TokenType::CONTINUE: {
		//	std::cout << "continue" << std::endl;
			auto next = nextToken();
			if (loopLevel <= 0) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::continueOutOfLoop);
			}
			addInstruction(Instruction(jmp, -2));//标记一下，-2代表需要continue
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			}
			break;
		}
		case TokenType::SEMICOLON: {
			nextToken();
			break;
		}
		default:
			break;
		}
		return {};
	}
	//<循环语句>::= 'while' '(' <逻辑表达式> ')' <语句序列>
	std::optional<CompilationError> Analyser::analyseLoopStatement() {
	//	std::cout << "<循环语句>" << std::endl;
		loopLevel++;
		//如果while没有问题，下一个应该读左括号
		auto next = nextToken();
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
		//	std::cout << "Err pos 30" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
		}
		pushJump(_functions.back().instructions.size());
		auto err = analyseCondition();
		if (err.has_value()) {
			return err;
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
		//	std::cout << "Err pos 31" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
		}
		err = analyseSingleStatement();
		if (err.has_value()) {
			return err;
		}
		_functions.back().instructions[popJump()].SetX(_functions.back().instructions.size() + 1);
		int32_t start = popJump();
		addInstruction(Instruction(jmp, start));
		addInstruction(Instruction(nop));
		int32_t breakPoint = _functions.back().instructions.size() - 1;
		int32_t continuePoint = _functions.back().instructions.size() - 2;
		for (int32_t i = start;i < continuePoint;i++) {
			if (_functions.back().instructions[i].GetOperation() == jmp && _functions.back().instructions[i].GetX() == -1) {
				_functions.back().instructions[i].SetX(breakPoint);
			}
			if (_functions.back().instructions[i].GetOperation() == jmp && _functions.back().instructions[i].GetX() == -2) {
				_functions.back().instructions[i].SetX(continuePoint);
			}
		}
		loopLevel--;
		return {};
	}
	//<Do-while循环>
	std::optional<CompilationError> Analyser::analyseDoStatement() {
		loopLevel++;
		//如果前面没有错误，这里第一个token一定是do
		auto next = nextToken();
		addInstruction(Instruction(nop));
		pushJump(_functions.back().instructions.size() - 1);
		auto err = analyseSingleStatement();
		if (err.has_value()) {
			return err;
		}
		addInstruction(Instruction(nop));
		int32_t continuePoint = _functions.back().instructions.size() - 1;
		//while
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::WHILE) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossWhile);
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
		}
		analyseCondition();
		int32_t jumpPoint = popJump();
		if (_functions.back().instructions[jumpPoint].GetOperation() == je) {
			_functions.back().instructions[jumpPoint].SetOperation(jne);
		}
		else if(_functions.back().instructions[jumpPoint].GetOperation() == jne){
			_functions.back().instructions[jumpPoint].SetOperation(je);
		}
		else if (_functions.back().instructions[jumpPoint].GetOperation() == jl) {
			_functions.back().instructions[jumpPoint].SetOperation(jge);
		}
		else if (_functions.back().instructions[jumpPoint].GetOperation() == jge) {
			_functions.back().instructions[jumpPoint].SetOperation(jl);
		}
		else if (_functions.back().instructions[jumpPoint].GetOperation() == jg) {
			_functions.back().instructions[jumpPoint].SetOperation(jle);
		}
		else if (_functions.back().instructions[jumpPoint].GetOperation() == jle) {
			_functions.back().instructions[jumpPoint].SetOperation(jg);
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
		}
		loopLevel--;
		int32_t start = popJump();
		_functions.back().instructions[jumpPoint].SetX(start);
		addInstruction(Instruction(nop));
		int32_t breakPoint = _functions.back().instructions.size() - 1;
		for (int32_t i = start;i < continuePoint;i++) {
			if (_functions.back().instructions[i].GetOperation() == jmp && _functions.back().instructions[i].GetX() == -1) {
				_functions.back().instructions[i].SetX(breakPoint);
			}
			if (_functions.back().instructions[i].GetOperation() == jmp && _functions.back().instructions[i].GetX() == -2) {
				_functions.back().instructions[i].SetX(continuePoint);
			}
		}
		return {};
	}
	//<for循环>
	std::optional<CompilationError> Analyser::analyseForStatement() {
		loopLevel++;
		bool hasCondition = true;
		//如果前面没有错误，这里第一个token一定是for
		auto next = nextToken();
		//左括号
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
		}
		//赋值语句队列
		while (true) {
			next = nextToken();
			if (!next.has_value()) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
			}
			if (next.value().GetType() == TokenType::SEMICOLON) {
				break;
			}
			else {
				unreadToken();
				auto err = analyseAssignmentStatement();
				if (err.has_value()) {
					return err;
				}
				next = nextToken();
				if (next.value().GetType() != TokenType::COMMA || !next.has_value()) {
					unreadToken();
					continue;
				}
			}
		}
		//判断句
		next = nextToken();
		unreadToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
		}
		pushJump(_functions.back().instructions.size());
		if (next.value().GetType() != TokenType::SEMICOLON) {
			auto err = analyseCondition();
			if (err.has_value()) {
				return err;
			}
		}
		else {
			hasCondition = false;
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
		}
		//保存update部分命令的首地址
		int32_t startUpdate = _functions.back().instructions.size();
		//每一层for循环都需要一个栈来存储updatestatement中的指令
		std::vector<Instruction> _for_item;
		_for_item.clear();
		_for_instructions.push_back(_for_item);
		//对update部分的分析
		while (true) {
			next = nextToken();
			if (!next.has_value()) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
			}
			if (next.value().GetType() == TokenType::RIGHT_BRACKET) {
				break;
			}
			else {
				next = nextToken();
	//			std::cout << next.value().GetValueString() << std::endl;
				unreadToken();
				unreadToken();
				if (!next.has_value()) {
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
				}
				if (next.value().GetType() != TokenType::EQUAL_SIGN && next.value().GetType() != LEFT_BRACKET) {
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::InvalidForStatement);
				}
				if (next.value().GetType() == TokenType::EQUAL_SIGN) {
					auto err = analyseAssignmentStatement();
					if (err.has_value()) {
						return err;
					}
				}
				else {
					auto err = analyseFunctionCall();
					if (err.has_value()) {
						return err;
					}
				}
				next = nextToken();
				if (next.value().GetType() != TokenType::COMMA || !next.has_value()) {
					unreadToken();
					continue;
				}
			}
		}
		moveInstructions(startUpdate);
		auto err = analyseSingleStatement();
		if (err.has_value()) {
			return err;
		}
		recoverFromForStack();
		//弹出本层for循环的_for_stack
		_for_instructions.pop_back();
		if(hasCondition)
			_functions.back().instructions[popJump()].SetX(_functions.back().instructions.size() + 1);
		int32_t start = popJump();
		addInstruction(Instruction(jmp, start));
		addInstruction(Instruction(nop));
		int32_t breakPoint = _functions.back().instructions.size() - 1;
		int32_t continuePoint = _functions.back().instructions.size() - 2;
		for (int32_t i = start;i < continuePoint;i++) {
			if (_functions.back().instructions[i].GetOperation() == jmp && _functions.back().instructions[i].GetX() == -1) {
				_functions.back().instructions[i].SetX(breakPoint);
			}
			if (_functions.back().instructions[i].GetOperation() == jmp && _functions.back().instructions[i].GetX() == -2) {
				_functions.back().instructions[i].SetX(continuePoint);
			}
		}
		loopLevel--;
		return {};
	}
	//<返回语句>::= 'return' [<表达式>] ';'
	std::optional<CompilationError> Analyser::analyseReturnStatement() {
	//	std::cout << "<返回语句>" << std::endl;
		//如果前面没有问题，这里应该已经度过return 了
		auto next = nextToken();
		next = nextToken();
		if (!next.has_value()) {
	//		std::cout << "Err pos 32" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
		}
		if (next.value().GetType() != TokenType::SEMICOLON) {
			unreadToken();
			auto err = analyseExpression();
			if (err.has_value())
				return err;
			next = nextToken();
			if (!next.has_value()||next.value().GetType() != TokenType::SEMICOLON) {
	//			std::cout << "Err pos 33" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			}
			addInstruction(Instruction(iret));
			returnCount++;
			return{};
		}
		addInstruction(Instruction(ret));
		return {};
	}
	//<判断语句>;;='if''('<条件>')'<语句序列>['else'<语句序列>]
	std::optional<CompilationError> Analyser::analyseConditionStatement() {
	//	std::cout << "<判断语句>" << std::endl;
		//如果前面没有问题，这里是if
		auto next = nextToken();
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
	//		std::cout << "Err pos 34" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
		}
		auto err = analyseCondition();
		if (err.has_value()) {
			return err;
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
	//		std::cout << "Err pos 35" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
		}
		err = analyseSingleStatement();
		if (err.has_value()) {
			return err;
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::ELSE) {
			unreadToken();
			//增加之前的逻辑判断生成的指令的操作数，设置为if段结束的首地址
			_functions.back().instructions[popJump()].SetX(_functions.back().instructions.size());
			addInstruction(Instruction(nop));
			return {};
		}
		else {
			_functions.back().instructions[popJump()].SetX(_functions.back().instructions.size()+1);
			addInstruction(Instruction(jmp));
			int32_t offset = _functions.back().instructions.size() - 1;//记住jmp指令的地址
			err = analyseSingleStatement();
			if (err.has_value()) {
				return err;
			}
			_functions.back().instructions[offset].SetX(_functions.back().instructions.size());
		}
		addInstruction(Instruction(nop));
		return {};
	}
	//<逻辑表达式>::=<表达式>[<关系运算符><表达式>] 
	std::optional<CompilationError> Analyser::analyseCondition() {
	//	std::cout << "<逻辑表达式>" << std::endl;
		auto err = analyseExpression();
		if (err.has_value()) {
			return err;
		}
		auto next = nextToken();
		Token tk = next.value();
		if (!next.has_value()) {
	//		std::cout << "Err pos 36" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
		}
		if (next.value().GetType() == TokenType::DOUBLE_EQAUL_SIGN ||
			next.value().GetType() == TokenType::NOT_EQUAL_SIGN ||
			next.value().GetType() == TokenType::LESS_EQUAL ||
			next.value().GetType() == TokenType::LESS_THAN ||
			next.value().GetType() == TokenType::MORE_EQUAL ||
			next.value().GetType() == TokenType::MORE_THAN
			) {}
		else if (next.value().GetType() == TokenType::RIGHT_BRACKET) {
			addInstruction(Instruction(je));
		//	std::cout << "push" << std::endl;
			pushJump(_functions.back().instructions.size() - 1);
			unreadToken();
			return {};
		}
		else {
		//	std::cout << "Err pos 37" << std::endl;
		//	std::cout << next.value().GetValueString() << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::InvalidCondition);
		}
		err = analyseExpression();
		if (err.has_value()) {
			return err;
		}
		addInstruction(Instruction(icmp));
		if (tk.GetType() == TokenType::DOUBLE_EQAUL_SIGN) {
			addInstruction(Instruction(jne));
		}
		else if (tk.GetType() == TokenType::MORE_EQUAL) {
			addInstruction(Instruction(jl));
		}
		else if (tk.GetType() == TokenType::MORE_THAN) {
			addInstruction(Instruction(jle));
		}
		else if (tk.GetType() == TokenType::LESS_EQUAL) {
			addInstruction(Instruction(jg));
		}
		else if (tk.GetType() == TokenType::LESS_THAN) {
			addInstruction(Instruction(jge));
		}
		else if (tk.GetType() == TokenType::NOT_EQUAL_SIGN) {
			addInstruction(Instruction(je));
		}
	//	std::cout << "push" << std::endl;
		pushJump(_functions.back().instructions.size() - 1);
		return {};
	}
	// <表达式> ::= <项>{<加法型运算符><项>}
	std::optional<CompilationError> Analyser::analyseExpression() {
	//	std::cout << "<表达式>" << std::endl;
		// <项>
		auto err = analyseItem();
		if (err.has_value())
			return err;
		// {<加法型运算符><项>}
		while (true) {
			auto next = nextToken();
			if (!next.has_value())
				return {};
			auto type = next.value().GetType();
			if (type != TokenType::PLUS_SIGN && type != TokenType::MINUS_SIGN) {
				
				unreadToken();
				return {};
			}
			// <项>
			err = analyseItem();
			if (err.has_value())
				return err;
			if (type == TokenType::PLUS_SIGN)
				addInstruction(Instruction(iadd));
			else if (type == TokenType::MINUS_SIGN)
				addInstruction(Instruction(isub));
		}
		return {};
	}
	// <赋值语句> ::= <标识符>'='<表达式>
	// 需要补全
	std::optional<CompilationError> Analyser::analyseAssignmentStatement() {
	//	std::cout << "<赋值语句>" << std::endl;
		// 这里除了语法分析以外还要留意
		auto next = nextToken();
		Token tk = next.value();
		int32_t offset;
		offset = getSymbleIndex(tk);
		switch (getLevel(tk)) {
			//是局部变量
		case 1: {
			//	std::cout << next.value().GetValueString() << "is part!" << std::endl;
			
	//		std::cout << offset << "find" << std::endl;
			addInstruction(Instruction(loada, 0, offset));
			break;
		}
			  //是全局变量
		case 0: {
			//	std::cout << next.value().GetValueString() << " is gloable!" << std::endl;
			
		//	std::cout << offset << "find" << std::endl;
			addInstruction(Instruction(loada, 1, offset));
			break;
		}
			  //使用未定义变量
		case -1: {
			//	std::cout << next.value().GetValueString() << "no declared" << std::endl;
		//	std::cout << "notDeclared3" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
		}
		default:
		//	std::cout << "getLevelErr" << std::endl;
			break;
		}
		if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER) {
		//	std::cout << "noId3" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
		}
		// 标识符声明过吗？
		if (getLevel(next.value())==-1) {
		//	std::cout << "notDeclare12" << std::endl;
			std::cout << next.value().GetValueString() << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
		}
		// 标识符是常量吗？
		if (isConst(next.value())) {
		//	std::cout << "isConst" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);
		}
		//标识符赋过值了？
		if(!isInitailized(next.value())) {
			assign(next.value());
		}
		// '='
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::EQUAL_SIGN) {
		//	std::cout << "Err pos 41" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidAssignment);
		}
		//表达式
		auto err = analyseExpression();
		if (err.has_value())
			return err;
		// 需要生成指令吗？
		addInstruction(Instruction(istore));
		return {};
	}
	//<输入语句>::='scan' '(' <标识符> ')' ';'
	std::optional<CompilationError> Analyser::analyseScanStatement() {
	//	std::cout << "<输入语句>" << std::endl;
		auto next = nextToken();
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
	//		std::cout << "Err pos 43" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidScan);
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER) {
	//		std::cout << "Err pos 44" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidScan);
		}
		int32_t offset;
		offset = getSymbleIndex(next.value());
		if (getLevel(next.value()) == -1) {
	//		std::cout << "Err pos 44" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
		}
		else if (getLevel(next.value()) == 1) {
	//		std::cout << offset << "find" << std::endl;
			addInstruction(Instruction(loada, 0, offset));
		}
		else {
	//		std::cout << offset << "find" << std::endl;
			addInstruction(Instruction(loada, 1, offset));
		}
		assign(next.value());
		addInstruction(Instruction(iscan));
		addInstruction(Instruction(istore));
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
	//		std::cout << "Err pos 45" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidScan);
		}
		return {};
	}
	// <输出语句> ::= 'print' '(' [<输出序列>] ')' ';'
	std::optional<CompilationError> Analyser::analyseOutputStatement() {
	//	std::cout << "<输出语句>" << std::endl;
		// 如果之前 <语句> 的实现正确，这里第一个 next 一定是 TokenType::PRINT
		auto next = nextToken();
		// '('
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
	//		std::cout << "Err pos 47" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrint);
		}
		next = nextToken();
		if (next.has_value() && next.value().GetType() == TokenType::RIGHT_BRACKET) {
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
	//			std::cout << "Err pos 65" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			}
			addInstruction(Instruction(printl));
			return {};
		}
		unreadToken();
		// <表达式>
		auto err = analysePrintList();
		if (err.has_value())
			return err;
		// ')'
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
	//		std::cout << "Err pos 48" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrint);
		}
		// ';'
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
	//		std::cout << "Err pos 49" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
		}
		addInstruction(Instruction(printl));
		return {};
	}
	//<输出序列>::=<表达式>{','<表达式>}
	std::optional<CompilationError> Analyser::analysePrintList() {
	//	std::cout << "<输出序列>" << std::endl;
		auto err = analyseExpression();
		if (err.has_value()) {
			return err;
		}
		addInstruction(Instruction(iprint));
		while (true) {
			auto next = nextToken();
			if (!next.has_value()) {
	//			std::cout << "Err pos 50" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
			}
			if (next.value().GetType() != TokenType::COMMA) {
				unreadToken();
				return {};
			}
			auto err = analyseExpression();
			if (err.has_value()) {
				return err;
			}
			addInstruction(Instruction(bipush, 32));
			addInstruction(Instruction(cprint));
			addInstruction(Instruction(iprint));
		}
		return {};
	}
	// <项> :: = <因子>{ <乘法型运算符><因子> }
	std::optional<CompilationError> Analyser::analyseItem() {
	//	std::cout << "<项>" << std::endl;
		// 可以参考 <表达式> 实现
		auto err = analyseFactor();
		if (err.has_value())
			return err;
		// {<乘法型运算符><因子>}
		while (true) {
			// 预读
			auto next = nextToken();
			if (!next.has_value())
				return {};
			auto type = next.value().GetType();
			if (type != TokenType::MULTIPLICATION_SIGN && type != TokenType::DIVISION_SIGN) {
				unreadToken();
				return {};
			}
			// <因子>
			err = analyseFactor();
			if (err.has_value())
				return err;
			if (type == TokenType::MULTIPLICATION_SIGN)
				addInstruction(Instruction(imul));
			else if (type == TokenType::DIVISION_SIGN)
				addInstruction(Instruction(idiv));
		}
		return {};
	}
	// <因子> ::= [<符号>]( <标识符> | <无符号整数> | '('<表达式>')' | <函数调用>)
	std::optional<CompilationError> Analyser::analyseFactor() {
	//	std::cout << "<因子>" << std::endl;
		// [<符号>]
		auto next = nextToken();
		auto prefix = 1;
		if (!next.has_value()) {
	//		std::cout << "Err pos 52" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		if (next.value().GetType() == TokenType::PLUS_SIGN)
			prefix = 1;
		else if (next.value().GetType() == TokenType::MINUS_SIGN) {
			prefix = -1;
		}
		else
			unreadToken();
		next = nextToken();
		//std::cout << next.value().GetValueString() << std::endl;
		if (!next.has_value()) {
	//		std::cout << "Err pos 53" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		switch (next.value().GetType()) {
			// 这里和 <语句序列> 类似，需要根据预读结果调用不同的子程序
		case TokenType::IDENTIFIER: {
			if (isVoid(next.value())) {
		//		std::cout << "void expression" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::voidExpresion);
			}
			auto judge = nextToken();
			unreadToken();
			if (judge.has_value() && judge.value().GetType() == TokenType::LEFT_BRACKET) {
				unreadToken();
				auto err = analyseFunctionCall();
				if (err.has_value()) {
					return err;
				}
			}
			else {
				int32_t offset;
				offset = getSymbleIndex(next.value());
				switch (getLevel(next.value())){
					//是局部变量
				case 1: {
				//	std::cout << next.value().GetValueString() << "is part!" << std::endl;
					if (!isInitailized(next.value())) {
			//			std::cout << "notDeclared1" << std::endl;
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotInitialized);
					}
			//		std::cout << offset << "find" << std::endl;
					addInstruction(Instruction(loada, 0, offset));
					break;
				}
					  //是全局变量
				case 0: {
				//	std::cout << next.value().GetValueString() << " is gloable!" << std::endl;
					if (!isInitailized(next.value())) {
			//			std::cout << "notDeclared2" << std::endl;
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotInitialized);
					}
			//		std::cout << offset << "find" << std::endl;
					int32_t diff;
					if (isGlobal) {
						diff = 0;
					}
					else {
						diff = 1;
					}
					addInstruction(Instruction(loada, diff, offset));
					break;
				}
					  //使用未定义变量
				case -1: {
				//	std::cout << next.value().GetValueString() << "no declared" << std::endl;
			//		std::cout << "notDeclared3" << std::endl;
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
				}
				default:
			//		std::cout << "getLevelErr" << std::endl;
					break;
				}
				addInstruction(Instruction(iload));
			}
			break;
		}
		case TokenType::UNSIGNED_INTEGER: {
			addInstruction(Instruction(ipush, std::stol(next.value().GetValueString())));
			break;
		}
		case TokenType::LEFT_BRACKET: {
			auto err = analyseExpression();
			if (err.has_value())
				return err;
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
			//	std::cout << "Err pos 54" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
			}
			break;
		}
		default: {
		//	std::cout << "Err pos 55" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		}
		// 取负
		if (prefix == -1)
			addInstruction(Instruction(ineg));
		return {};
	}
	//<函数调用>::=<标识符>'('[<表达式列表>]')'
	std::optional<CompilationError> Analyser::analyseFunctionCall() {
		//如果前置解析没有问题，那么此处前两个必定是标识符和'('
	//	std::cout << "<函数调用>" << std::endl;
		auto next = nextToken();
		int32_t offset = getFunctionIndex(next.value());
		if (offset == -1) {
	//		std::cout << "Funciton undifined" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
		}
		Token tk = next.value();
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
	//		std::cout << "Err pos 60" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
		}
		next = nextToken();
		if (!next.has_value()) {
		//	std::cout << "Err pos 61" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
		}
		if (next.value().GetType() == TokenType::RIGHT_BRACKET) {
			if (getParamCount(tk) > 0) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::tooFewArgs);
			}
			addInstruction(Instruction(call, offset));
			return {};
		}
		unreadToken();
		auto err = analyseExpressionList(getParamCount(tk));
		if (err.has_value()) {
			return err;
		}
		//')'
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
		//	std::cout << "Err pos 56" << std::endl;
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
		}
		addInstruction(Instruction(call,offset));
		return {};
	}
	//<表达式列表>::=<表达式>{','<表达式>}
	std::optional<CompilationError> Analyser::analyseExpressionList(int32_t param) {
	//	std::cout << "<表达式列表>" << std::endl;
		int32_t count = 1;
		auto err = analyseExpression();
		if (err.has_value()) {
			return err;
		}
		while (true) {
			auto next = nextToken();
			if (!next.has_value()) {
			//	std::cout << "Err pos 70" << std::endl;
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::lossBrakect);
			}
			if (next.value().GetType() == COMMA) {
				count++;
				err = analyseExpression();
				if (err.has_value()) {
					return err;
				}
			}
			else {
				unreadToken();
				if (count > param) {
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::tooManyArgs);
				}
				if (count < param) {
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::tooFewArgs);
				}
				return {};
			}
		}
		return {};
	}

	std::optional<Token> Analyser::nextToken() {
		if (_offset == _tokens.size())
			return {};
		// 考虑到 _tokens[0..._offset-1] 已经被分析过了
		// 所以我们选择 _tokens[0..._offset-1] 的 EndPos 作为当前位置
		_current_pos = _tokens[_offset].GetEndPos();
		return _tokens[_offset++];
	}

	void Analyser::unreadToken() {
		if (_offset == 0)
			DieAndPrint("analyser unreads token from the begining.");
		_current_pos = _tokens[_offset - 1].GetEndPos();
		_offset--;
	}

	//判断标识符是否是函数
	bool Analyser::isFunction(Token& tk) {
		for (int i = 0;i < _functions.size();i++) {
			if(_functions[i].name == tk.GetValueString())
			return true;
		}
		return false;
	}

	//加入void型，如果函数位void型则不能解析为表达式
	void Analyser::addVoid(Token& tk) {
		_void_func.push_back(tk.GetValueString());
	}
	//标识符是否是void类型
	bool Analyser::isVoid(Token& tk) {
		for (int32_t i = 0;i < _void_func.size();i++) {
			if (tk.GetValueString() == _void_func[i])
				return true;
		}
		return false;
	}

	//是否在当前层级存在
	bool Analyser::ReDeclare(Token& tk) {
		if (isGlobal) {
			if (globalTable.getOffset(tk.GetValueString()) != -1) {
				return true;
			}
		}
		else {
			if (partTable.getOffset(tk.GetValueString()) != -1) {
				return true;
			}
		}
		return false;
	}
	//向对应的符号表中插入符号，函数例外，如果符号重复声明则返回false
	bool Analyser::insertTable(Token& tk,bool isConst,bool initailized) {
		if (ReDeclare(tk)) {
			return false;
		}
		if (isGlobal) {
			globalTable.insert(tk.GetValueString(), tk.GetType(), isConst, initailized);
		}
		else {
			partTable.insert(tk.GetValueString(), tk.GetType(), isConst, initailized);
		}
		return true;
	}
	//向函数表中函数，如果符号重复声明则返回false
	bool Analyser::insertFunction(Token& tk, int32_t param) {
		if (isFunction(tk)) {
			return false;
		}
		//由于不允许函数嵌套，所以函数必定在外层
		if (globalTable.getOffset(tk.GetValueString()) != -1) {
			return false;
		}
		function newFun;
		newFun.instructions.clear();
		newFun.name = tk.GetValueString();
		newFun.param_count = param;
		_functions.push_back(newFun);
		return true;
	}
	//查找函数的参数个数
	int32_t Analyser::getParamCount(Token& tk) {
		for (int32_t i = 0;i < _functions.size();i++) {
			if (tk.GetValueString() == _functions[i].name) {
				return _functions[i].param_count;
			}
		}
		return -1;
	}
	//返回函数的索引
	int32_t Analyser::getFunctionIndex(Token& tk) {
		for (int32_t i = 0;i < _functions.size();i++) {
			if (tk.GetValueString() == _functions[i].name) {
				return i;
			}
		}
		return -1;
	}
	//获取符号在相应符号表中的索引
	int32_t Analyser::getSymbleIndex(Token& tk) {
		if (getLevel(tk) == 1) {
			return partTable.getOffset(tk.GetValueString());
		}
		else if (getLevel(tk) == 0) {
			return globalTable.getOffset(tk.GetValueString());
		}
		else {
			return -1;
		}
	}
	//查找标识符所在的层级，如果是局部变量则返回1，全局变量则返回0，没有找到则返回-1
	int32_t Analyser::getLevel(Token& tk) {
		if (!isGlobal) {
			if (partTable.getOffset(tk.GetValueString()) != -1) {
				return 1;
			}
		}
		if (globalTable.getOffset(tk.GetValueString()) != -1) {
			return 0;
		}
		return -1;
	}
	//压入对应的指令，如果是全局状态直接压入_instruction,如果是局部状态则压入当前函数的最后一条指令
	bool Analyser::addInstruction(Instruction instruction) {
		if (isGlobal) {
			_instructions.push_back(instruction);
		}
		else {
			if (_functions.size() <= 0) {
		//		std::cout << "function out of range" << std::endl;
				return false;
			}
			else {
				_functions.back().instructions.push_back(instruction);
			}
		}
		return true;
	}
	//给变量赋值,改变initailize属性
	bool Analyser::assign(Token& tk) {
		int32_t index = getSymbleIndex(tk);
		if (getLevel(tk) == 1) {
			partTable.symbles[index].initailized = true;
			return true;
		}
		else if (getLevel(tk) == 0) {
			globalTable.symbles[index].initailized = true;
			return true;
		}
		else {
			return false;
		}
	}
	//判断符号是不是常量
	bool Analyser::isConst(Token& tk) {
		int32_t index = getSymbleIndex(tk);
		if (getLevel(tk) == 1) {
			return partTable.symbles[index].isConst;
		}
		else if (getLevel(tk) == 0) {
			return globalTable.symbles[index].isConst;
		}
		else {
			return false;
		}
	}
	//判断符号是否初始化
	bool Analyser::isInitailized(Token& tk) {
		int32_t index = getSymbleIndex(tk);
		if (getLevel(tk) == 1) {
			return partTable.symbles[index].initailized;
		}
		else if (getLevel(tk) == 0) {
			return globalTable.symbles[index].initailized;
		}
		else {
			return false;
		}
	}

	//向跳转指令栈中添加跳转指令的地址
	void Analyser::pushJump(int32_t jump) {
		_jump_stack.push_back(jump);
	}

	//弹出跳转指令栈栈顶的值并返回
	int32_t Analyser::popJump() {
		int32_t result = _jump_stack.back();
		_jump_stack.pop_back();
		return result;
	}

	//把第n条以后的指令全部从指令栈里转移到_for_stack中，用于for语句的处理
	void Analyser::moveInstructions(int32_t n) {
		while (_functions.back().instructions.size() > n) {
			Instruction tmp = _functions.back().instructions.back();
			_functions.back().instructions.pop_back();
			_for_instructions.back().push_back(tmp);
		}
	}

	//用于从_for_stack中取出应有的指令，附加在语句块生成的指令之后
	void Analyser::recoverFromForStack() {
		while (_for_instructions.back().size() > 0) {
			Instruction tmp = _for_instructions.back().back();
			_for_instructions.back().pop_back();
			_functions.back().instructions.push_back(tmp);
		}
	}

	//输出文本文件
	void Analyser::printTextFile(std::ostream& output) {
		output << ".constants:" << std::endl;
		for (int32_t i = 0;i < _functions.size();i++) {
			output << i << " " << 'S' << " \"" << _functions[i].name << "\"" << std::endl;
		}
		output << "." << "start" << ":" << std::endl;
		for (int32_t i = 0; i < _instructions.size(); i++) {
			output << i << " ";
			printTextInstruction(_instructions[i], output);
		}
		output << ".functions:" << std::endl;
		for (int32_t i = 0; i < _functions.size(); i++) {
			output << i << " " << i << " " << _functions[i].param_count << " " << "1" << std::endl;
		}
		for (int32_t i = 0; i < _functions.size(); i++) {
			output << "." << "F" << i << ":" << std::endl;
			for (int32_t j = 0; j < _functions[i].instructions.size(); j++) {
				output << j << " ";
				printTextInstruction(_functions[i].instructions[j], output);
			}
		}
	}

	//输出文本指令
	void Analyser::printTextInstruction(Instruction instruction, std::ostream& output) {
		std::string name;
		switch (instruction.GetOperation())
		{
		case miniplc0::bipush: {
			name = "bipush";
			break;
		}
		case miniplc0::ipush: {
			name = "ipush";
			break;
		}
		case miniplc0::pop: {
			name = "pop";
			break;
		}
		case miniplc0::loadc: {
			name = "loadc";
			break;
		}
		case miniplc0::loada: {
			name = "loada";
		//	std::cout << "Y=" << instruction.GetY() << std::endl;
			break;
		}
		case miniplc0::iload: {
			name = "iload";
			break;
		}
		case miniplc0::aload: {
			name = "aload";
			break;
		}
		case miniplc0::iaload: {
			name = "iaload";
			break;
		}
		case miniplc0::istore: {
			name = "istore";
			break;
		}
		case miniplc0::iastore: {
			name = "iastore";
			break;
		}
		case miniplc0::iadd: {
			name = "iadd";
			break;
		}
		case miniplc0::isub: {
			name = "isub";
			break;
		}
		case miniplc0::imul: {
			name = "imul";
			break;
		}
		case miniplc0::idiv: {
			name = "idiv";
			break;
		}
		case miniplc0::ineg: {
			name = "ineg";
			break;
		}
		case miniplc0::icmp: {
			name = "icmp";
			break;
		}
		case miniplc0::jmp: {
			name = "jmp";
			break;
		}
		case miniplc0::je: {
			name = "je";
			break;
		}
		case miniplc0::jne: {
			name = "jne";
			break;
		}
		case miniplc0::jl: {
			name = "jl";
			break;
		}
		case miniplc0::jle: {
			name = "jle";
			break;
		}
		case miniplc0::jg: {
			name = "jg";
			break;
		}
		case miniplc0::jge: {
			name = "jge";
			break;
		}
		case miniplc0::call: {
			name = "call";
			break;
		}
		case miniplc0::ret: {
			name = "ret";
			break;
		}
		case miniplc0::iret: {
			name = "iret";
			break;
		}
		case miniplc0::iprint: {
			name = "iprint";
			break;
		}
		case miniplc0::cprint: {
			name = "cprint";
			break;
		}
		case miniplc0::printl: {
			name = "printl";
			break;
		}
		case miniplc0::iscan: {
			name = "iscan";
			break;
		}
		case miniplc0::nop: {
			name = "nop";
			break;
		}
		default:
			name = "error";
			break;
		}
		output << name;
		if (instruction.GetX() >= 0)
			output << " " << instruction.GetX();
		if (instruction.GetY() >= 0)
			output << ", " << instruction.GetY();
		output << std::endl;
	}

	//大端法转换四字节
	void Analyser::binary4byte(int number, std::ostream& output) {
		char buffer[4];
		buffer[0] = ((number & 0xff000000) >> 24);
		buffer[1] = ((number & 0x00ff0000) >> 16);
		buffer[2] = ((number & 0x0000ff00) >> 8);
		buffer[3] = ((number & 0x000000ff));
		output.write(buffer, sizeof(buffer));
	}
	//大端法转换双字节
	void Analyser::binary2byte(int number, std::ostream& output) {
		char buffer[2];
		buffer[0] = ((number & 0x0000ff00) >> 8);
		buffer[1] = ((number & 0x000000ff));
		output.write(buffer, sizeof(buffer));
	}
	//输出二进制文件
	void Analyser::printBinary(std::ostream& output) {
		//首先书写固定字段magic和version
		char magic[4];
		magic[0] = 0x43;
		magic[1] = 0x30;
		magic[2] = 0x3a;
		magic[3] = 0x29;
		char version[4];
		version[0] = 0;
		version[1] = 0;
		version[2] = 0;
		version[3] = 1;
		output.write(magic, sizeof(magic));
		output.write(version, sizeof(version));
		int const_size = (int)_functions.size();
		binary2byte(const_size,output);
		for (int i = 0;i < const_size;i++) {
			char buffer[1];
			buffer[0] = 0x00;
			output.write(buffer, sizeof(char));
			int len = _functions[i].name.length();
			binary2byte(len,output);
			for (int j = 0; j < len; j++) {
				buffer[0] = _functions[i].name[j];
				output.write(buffer, sizeof(char));
			}
		}
		binary2byte(_instructions.size(), output);
		for (int i = 0; i < _instructions.size(); i++) {
			printBinaryInstruction(_instructions[i], output);
		}
		binary2byte(_functions.size() , output);
		for (int i = 0; i < _functions.size(); i++) {
			binary2byte(i, output);
			binary2byte(_functions[i].param_count, output);
			binary2byte(1, output);
			binary2byte(_functions[i].instructions.size(), output);
			for (int j = 0; j < _functions[i].instructions.size(); j++) {
				printBinaryInstruction(_functions[i].instructions[j], output);
			}
		}
	}
	//输出二进制指令
	void Analyser::printBinaryInstruction(Instruction instruction, std::ostream& output) {
		char buffer[1];
		switch (instruction.GetOperation()) {
		case nop:
			buffer[0] = 0x00;
			output.write(buffer, sizeof(char));
			break;
		case bipush:
			buffer[0] = 0x01;
			output.write(buffer, sizeof(char));
			buffer[0] = (instruction.GetX() & 0xff);
			output.write(buffer, sizeof(char));
			break;
		case ipush:
			buffer[0] = 0x02;
			output.write(buffer, sizeof(char));
			binary4byte(instruction.GetX(), output);
			break;
		case pop:
			buffer[0] = 0x04;
			output.write(buffer, sizeof(char));
			break;
		case loadc:
			buffer[0] = 0x09;
			output.write(buffer, sizeof(char));
			binary2byte(instruction.GetX(), output);
			break;
		case loada:
			buffer[0] = 0x0a;
			output.write(buffer, sizeof(char));
			binary2byte(instruction.GetX(), output);
			binary4byte(instruction.GetY(), output);
			break;
		case iload:
			buffer[0] = 0x10;
			output.write(buffer, sizeof(char));
			break;
		case istore:
			buffer[0] = 0x20;
			output.write(buffer, sizeof(char));
			break;
		case iadd:
			buffer[0] = 0x30;
			output.write(buffer, sizeof(char));
			break;
		case isub:
			buffer[0] = 0x34;
			output.write(buffer, sizeof(char));
			break;
		case imul:
			buffer[0] = 0x38;
			output.write(buffer, sizeof(char));
			break;
		case idiv:
			buffer[0] = 0x3c;
			output.write(buffer, sizeof(char));
			break;
		case ineg:
			buffer[0] = 0x40;
			output.write(buffer, sizeof(char));
			break;
		case icmp:
			buffer[0] = 0x44;
			output.write(buffer, sizeof(char));
			break;
		case jmp:
			buffer[0] = 0x70;
			output.write(buffer, sizeof(char));
			binary2byte(instruction.GetX(), output);
			break;
		case je:
			buffer[0] = 0x71;
			output.write(buffer, sizeof(char));
			binary2byte(instruction.GetX(), output);
			break;
		case jne:
			buffer[0] = 0x72;
			output.write(buffer, sizeof(char));
			binary2byte(instruction.GetX(), output);
			break;
		case jl:
			buffer[0] = 0x73;
			output.write(buffer, sizeof(char));
			binary2byte(instruction.GetX(), output);
			break;
		case jge:
			buffer[0] = 0x74;
			output.write(buffer, sizeof(char));
			binary2byte(instruction.GetX(), output);
			break;
		case jg:
			buffer[0] = 0x75;
			output.write(buffer, sizeof(char));
			binary2byte(instruction.GetX(), output);
			break;
		case jle:
			buffer[0] = 0x76;
			output.write(buffer, sizeof(char));
			binary2byte(instruction.GetX(), output);
			break;
		case call:
			buffer[0] = 0x80;
			output.write(buffer, sizeof(char));
			binary2byte(instruction.GetX(), output);
			break;
		case ret:
			buffer[0] = 0x88;
			output.write(buffer, sizeof(char));
			break;
		case iret:
			buffer[0] = 0x89;
			output.write(buffer, sizeof(char));
			break;
		case iprint:
			buffer[0] = 0xa0;
			output.write(buffer, sizeof(char));
			break;
		case cprint:
			buffer[0] = 0xa2;
			output.write(buffer, sizeof(char));
			break;
		case printl:
			buffer[0] = 0xaf;
			output.write(buffer, sizeof(char));
			break;
		case iscan:
			buffer[0] = 0xb0;
			output.write(buffer, sizeof(char));
			break;
		default: {
			break;
		}
		}
	}
}