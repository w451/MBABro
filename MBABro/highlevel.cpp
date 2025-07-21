#include "obfuscator.hpp"

#include <iostream>

__declspec(restrict)HighLevelOperator* NewHighLevel(HighLevelOperation ot, Item* __restrict item1, Item* __restrict item2) {
	node_count++;
	HighLevelOperator* v = (HighLevelOperator*)malloc(sizeof(HighLevelOperator));
	v->it = ItemType::HIGHLEVEL;
	v->op1 = item1;
	v->op2 = item2;
	v->ot = ot;
	return v;
}

__declspec(restrict)Item* CompileHighLevel(Item* __restrict head) {
	if (head->it == ItemType::VALUE) {
		return head;
	} else if (head->it == ItemType::VARIABLE) {
		return head;
	} else if (head->it == ItemType::UNARY) {
		UnaryOperator* uo = (UnaryOperator*)head;
		uo->op1 = CompileHighLevel(uo->op1);
		return uo;
	} else if (head->it == ItemType::BINARY) {
		BinaryOperator* uo = (BinaryOperator*)head;
		uo->op1 = CompileHighLevel(uo->op1);
		uo->op2 = CompileHighLevel(uo->op2);
		return uo;
	} else if (head->it == ItemType::HIGHLEVEL) {
		HighLevelOperator* hlo = (HighLevelOperator*)head;

		Item* result = 0;

		if (hlo->ot == HighLevelOperation::BTEST) {
			result = NewBinary(OperationType::POW, NewValue(0), NewBinary(OperationType::AND, NewUnary(OperationType::NOT, hlo->op1), hlo->op2));
		} else if (hlo->ot == HighLevelOperation::SHR) {
			if (hlo->op2->it != ItemType::VALUE) {
				__debugbreak();
			}

			result = 0;

			Value* v = (Value*)hlo->op2;
			for (int x = v->val; x < 32; x++) {
				if (result) {
					result = NewBinary(OperationType::ADD, result,
						NewBinary(OperationType::MULTIPLY,
							NewHighLevel(HighLevelOperation::BTEST, CopyGraph(hlo->op1), NewValue(powi(2, x))),
							NewValue(powi(2, x - v->val))
						)
					);
				} else {
					result = NewBinary(OperationType::MULTIPLY,
						NewHighLevel(HighLevelOperation::BTEST, hlo->op1, NewValue(powi(2, x))),
						NewValue(powi(2, x - v->val))
					);
				}
			}
			FreeGraph(hlo->op2);
		} else if (hlo->ot == HighLevelOperation::LESS) {
			#define btest(a, b) NewHighLevel(HighLevelOperation::BTEST, a, b)
			#define shr(a, b) NewHighLevel(HighLevelOperation::SHR, a, b)
			#define negative(a) NewUnary(OperationType::NEGATIVE, a)
			#define not(a) NewUnary(OperationType::NOT, a)
			#define or(a, b) NewBinary(OperationType::OR, a, b)
			#define and(a, b) NewBinary(OperationType::AND, a, b)
			#define add(a, b) NewBinary(OperationType::ADD, a, b)
			#define val(a) NewValue(a)
			#define pow(a,b) NewBinary(OperationType::POW, a, b)
			#define mul(a,b) NewBinary(OperationType::MULTIPLY, a, b)
			#define greater(a,b) NewHighLevel(HighLevelOperation::GREATER, a, b)
			#define geq(a,b) NewHighLevel(HighLevelOperation::GEQ, a, b)
			#define geq16(a,b) NewHighLevel(HighLevelOperation::GEQ16, a, b)
			#define mod16(a,b) NewHighLevel(HighLevelOperation::MOD16, a, b)
			//python3 code: btest((shr(x, 1) - shr(y, 1))&0xffffffff, 0x80000000) |  (0**((shr(x,1) - shr(y,1)) & 0xffffffff) & (y & 1 & ~x))
			result = or(btest(add(shr(hlo->op1, val(1)), negative(shr(hlo->op2, val(1)))), val(0x80000000)), and(pow(val(0), add(shr(CopyGraph(hlo->op1), val(1)), negative(shr(CopyGraph(hlo->op2), val(1))))), and(not(CopyGraph(hlo->op1)), and(CopyGraph(hlo->op2), val(1)))));
		}else if (hlo->ot == HighLevelOperation::GREATER) {
			result = or(btest(add(shr(hlo->op2, val(1)), negative(shr(hlo->op1, val(1)))), val(0x80000000)), and(pow(val(0), add(shr(CopyGraph(hlo->op2), val(1)), negative(shr(CopyGraph(hlo->op1), val(1))))), and(not(CopyGraph(hlo->op2)), and(CopyGraph(hlo->op1), val(1)))));
		} else if (hlo->ot == HighLevelOperation::EQ) {
			result = pow(val(0), add(hlo->op1, negative(hlo->op2)));
		} else if (hlo->ot == HighLevelOperation::GEQ) {
			result = or(greater(hlo->op1, hlo->op2), pow(val(0), add(CopyGraph(hlo->op1), negative(CopyGraph(hlo->op2)))));
		} else if (hlo->ot == HighLevelOperation::GEQ16) {
			result = pow(val(0), btest(add(hlo->op1, negative(hlo->op2)), val(0x80000000)));
		} else if (hlo->ot == HighLevelOperation::MOD16) {
			Item* fork = add(hlo->op1, mul(geq16(CopyGraph(hlo->op1), mul(hlo->op2, val(1 << 15))), negative(mul(CopyGraph(hlo->op2), val(1 << 15)))));
			for (int x = 14; x >= 1; x--) {
				fork = add(fork, mul(geq16(CopyGraph(fork), mul(CopyGraph(hlo->op2), val(1 << x))), negative(mul(CopyGraph(hlo->op2), val(1 << x)))));
			}
			fork = add(fork, mul(geq16(CopyGraph(fork), CopyGraph(hlo->op2)), negative(CopyGraph(hlo->op2))));
			result = fork;
		} 
		//This is just too massive
		/*else if (hlo->ot == HighLevelOperation::MOD32B16) {
			result = mod16(add(mul(mod16(shr(hlo->op1, val(16)), hlo->op2), add(mod16(val(0xffff), CopyGraph(hlo->op2)), val(1))), and(CopyGraph(hlo->op1), val(0xffff))), CopyGraph(hlo->op2));
		}*/
		 
		FreeNode(hlo);
		return CompileHighLevel(result);
	}
}