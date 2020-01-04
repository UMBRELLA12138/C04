#pragma once

#include "error/error.h"
#include "instruction/instruction.h"
#include "tokenizer/token.h"
#include <vector>
#include<iostream>
#include <optional>
#include <utility>
#include <map>
#include <cstdint>
#include <cstring>
#include <cstddef> // for std::size_t

namespace miniplc0 {

	class Analyser final {
	private:
		using uint64_t = std::uint64_t;
		using int64_t = std::int64_t;
		using uint32_t = std::uint32_t;
		using int32_t = std::int32_t;
	public:
		//_instructions存放的是.start中的指令
		//_uninitialized_vars存放的是未初始化的变量
		//_vars存放的是已经初始化过的变量
		//_const存放的是常量
		//_functions存放的是函数
		Analyser(std::vector<Token> v)
			: _tokens(std::move(v)), _offset(0), _instructions({}), _current_pos(0, 0), _void_func({}), _jump_stack({}), _for_instructions({}),
			 _functions({}), _nextTokenIndex(0),_functionIndex(0),isGlobal(true) {}
		Analyser(Analyser&&) = delete;
		Analyser(const Analyser&) = delete;
		Analyser& operator=(Analyser) = delete;
		// 唯一接口
		std::pair<std::vector<Instruction>, std::optional<CompilationError>> Analyse(std::ostream& output, bool isBinary);
		//函数的单体结构，函数表中储存的都是这样的数据
		typedef struct function {
			std::string name;
			std::vector<Instruction> instructions;
			int32_t param_count;
		}function;

		//符号表的单体结构，符号表中储存的都是这样的数据
		typedef struct symble {
			std::string name;
			TokenType type;
			bool isConst;
			bool initailized;
		}symble;

		//定义符号表，在写程序的过程中，应该注意有一个全局表和一个局部表
		typedef struct table {
			int32_t index;
			std::vector<symble> symbles;
			//初始化表格，包括清空表格的数据和索引归零
			void init() {
				index = 0;
				symbles.clear();
			}
			//插入symble
			void insert(std::string name , TokenType type,bool isConst,bool initailized) {
				symble newSymble;
				newSymble.name = name;
				newSymble.type = type;
				newSymble.isConst = isConst;
				newSymble.initailized = initailized;
				symbles.push_back(newSymble);
				index++;
			}
			//寻找symble在表中的偏移量,即其所在的索引,不存在则返回-1
			int32_t getOffset(std::string name) {
				for (int32_t i = 0;i < symbles.size();i++) {
					if (symbles[i].name == name) {
						return i;
					}
				}
				return -1;
			}
		}table;

	private:
		// <程序>
		std::optional<CompilationError> analyseProgram();
		// <变量声明>
		std::optional<CompilationError> analyseVariableDeclaration();
		//<变量声名序列>
		std::optional<CompilationError> analyseVarDeclareList(bool constFlag);
		//<声名>
		std::optional<CompilationError> analyzeDeclare(bool constFlag);
		//<函数定义>
		std::optional<CompilationError> analyseFunctionDeclaration();
		//<参数序列>
		std::optional<CompilationError> analyseParams(Token&);
		//<参数>
		std::optional<CompilationError> analyseParam();
		//<复合语句>
		std::optional<CompilationError> analyseCompoundStatement();
		//<语句>
		std::optional<CompilationError> analyseStatement();
		//<单句>
		std::optional<CompilationError> analyseSingleStatement();
		//<循环语句>
		std::optional<CompilationError> analyseLoopStatement();
		//<do-while循环>
		std::optional<CompilationError> analyseDoStatement();
		//<for循环>
		std::optional<CompilationError> analyseForStatement();
		//<返回语句>
		std::optional<CompilationError> analyseReturnStatement();
		//<判断语句>
		std::optional<CompilationError> analyseConditionStatement();
		//<逻辑表达式>
		std::optional<CompilationError> analyseCondition();
		// <表达式>
		std::optional<CompilationError> analyseExpression();
		// <赋值语句>
		std::optional<CompilationError> analyseAssignmentStatement();
		//<读入语句>
		std::optional<CompilationError> analyseScanStatement();
		// <输出语句>
		std::optional<CompilationError> analyseOutputStatement();
		//<输出序列>
		std::optional<CompilationError> analysePrintList();
		// <项>
		std::optional<CompilationError> analyseItem();
		// <因子>
		std::optional<CompilationError> analyseFactor();
		// <函数调用>
		std::optional<CompilationError> analyseFunctionCall();
		//<表达式列表>
		std::optional<CompilationError> analyseExpressionList(int32_t param);

		// 返回下一个 token
		std::optional<Token> nextToken();
		// 回退一个 token
		void unreadToken();
		//判断标识符是否是函数
		bool isFunction(Token& tk);
		//是否重复定义
		bool ReDeclare(Token& tk);
		//存入相应的符号表
		bool insertTable(Token& tk, bool isConst, bool initailized);
		//向函数表中增加函数
		bool insertFunction(Token& tk,int32_t param);
		//加入void型，如果函数位void型则不能解析为表达式
		void addVoid(Token& tk);
		//标识符是否是void类型
		bool isVoid(Token& tk);
		//查找函数的参数个数
		int32_t getParamCount(Token& tk);
		//查找函数的索引
		int32_t getFunctionIndex(Token& tk);
		//获取符号在相应符号表中的索引
		int32_t getSymbleIndex(Token& tk);
		//查找标识符所在的层级，如果是局部变量则返回1，全局变量则返回0，没有找到则返回-1
		int32_t getLevel(Token& tk);
		//压入对应的指令，如果是全局状态直接压入_instruction,如果是局部状态则压入当前函数的最后一条指令
		bool addInstruction(Instruction instruction);
		//给变量赋值
		bool assign(Token& tk);
		//判断符号是不是常量
		bool isConst(Token& tk);
		//判断符号是否初始化
		bool isInitailized(Token& tk);
		//向跳转指令栈中添加跳转指令的地址
		void pushJump(int32_t);
		//弹出跳转指令栈栈顶的值并返回
		int32_t popJump();
		//把第n条以后的指令全部从指令栈里转移到_for_stack中，用于for语句的处理
		void moveInstructions(int32_t n);
		//用于从_for_stack中取出应有的指令，附加在语句块生成的指令之后
		void recoverFromForStack();
		//输出文本文件
		void printTextFile(std::ostream& output);
		//输出文本指令
		void printTextInstruction(Instruction instruction, std::ostream& output);
		//32位转大端
		void binary4byte(int number, std::ostream& output);
		//16位转大端
		void binary2byte(int number, std::ostream& output);
		//输出二进制文件
		void printBinary(std::ostream& output);
		//输出二进制指令
		void printBinaryInstruction(Instruction instruction, std::ostream& output);
	private:
		std::vector<Token> _tokens;
		std::size_t _offset;
		std::vector<Instruction> _instructions;
		std::pair<uint64_t, uint64_t> _current_pos;
		std::vector<std::string>_void_func;
		std::vector<int32_t>_jump_stack;
		std::vector<std::vector<Instruction>> _for_instructions;
		//全局表和局部表
		table globalTable,partTable;
		std::vector<function> _functions;
		// 下一个 token 在栈的偏移
		int32_t _nextTokenIndex;
		int32_t _functionIndex;
		bool isGlobal;
	};
}
