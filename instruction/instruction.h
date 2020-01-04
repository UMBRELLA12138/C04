#pragma once

#include <cstdint>
#include <utility>

namespace miniplc0 {

	enum Operation {
		ILL = 0,
		LIT,
		LOD,
		STO,
		ADD,
		SUB,
		MUL,
		DIV,
		WRT,
		nop,
		bipush,
		ipush,
		pop,
		loadc,
		loada,
		iload,
		aload,
		iaload,
		istore,
		iastore,
		iadd,
		isub,
		imul,
		idiv,
		ineg,
		icmp,
		jmp,
		je,
		jne,
		jl,
		jge,
		jg,
		jle,
		call,
		ret,
		iret,
		iprint,
		cprint,
		printl,
		iscan,
	};

	class Instruction final {
	private:
		using int32_t = std::int32_t;
	public:
		friend void swap(Instruction& lhs, Instruction& rhs);
	public:
		Instruction(Operation opr, int32_t x, int32_t y) : _opr(opr), _x(x), _y(y) {}
		Instruction(Operation opr, int32_t x) : _opr(opr), _x(x), _y(-1) {}
		Instruction(Operation opr) : _opr(opr), _x(-1), _y(-1) {}
		Instruction() : Instruction(Operation::ILL, 0, 0) {}
		Instruction(const Instruction& i) { _opr = i._opr; _x = i._x; _y = i._y; }
		Instruction(Instruction&& i) :Instruction() { swap(*this, i); }
		Instruction& operator=(Instruction i) { swap(*this, i); return *this; }
		bool operator==(const Instruction& i) const { return _opr == i._opr && _x == i._x && _y == i._y; }

		Operation GetOperation() const { return _opr; }
		int32_t GetX() const { return _x; }
		int32_t GetY() const { return _y; }
		void SetX(int32_t x) { _x = x; }
		void SetY(int32_t y) { _y = y; }
		void SetOperation(Operation opr) { _opr = opr; }
	private:
		Operation _opr;
		int32_t _x;
		int32_t _y;
	};

	inline void swap(Instruction& lhs, Instruction& rhs) {
		using std::swap;
		swap(lhs._opr, rhs._opr);
		swap(lhs._x, rhs._x);
		swap(lhs._y, rhs._y);
	}
}