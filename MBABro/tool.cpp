#include "mbabro.hpp"
#include <iostream>

unsigned long long node_count = 0;

unsigned long long powi(unsigned long long base, unsigned long long exp)
{
	unsigned long long  res = 1;
	while (exp) {
		if (exp & 1)
			res *= base;
		exp >>= 1;
		base *= base;
	}
	return res;
}

unsigned long evalGraph(Item* __restrict head, VariableTable& vt) {
	unsigned long result = 0;
	if (head->it == ItemType::VALUE) {
		Value* v = (Value*)head;
		result = v->val;
	} else if (head->it == ItemType::VARIABLE) {
		Variable* v = (Variable*)head;
		result = vt.at(v->vnum).value;
	} else if (head->it == ItemType::UNARY) {
		UnaryOperator* uo = (UnaryOperator*)head;
		if (uo->ot == OperationType::NEGATIVE) {
			result = ~evalGraph(uo->op1, vt) + 1;
		} else if (uo->ot == OperationType::NOT) {
			result = ~evalGraph(uo->op1, vt);
		}
	} else if (head->it == ItemType::BINARY) {
		BinaryOperator* bo = (BinaryOperator*)head;
		if (bo->ot == OperationType::ADD) {
			result = evalGraph(bo->op1, vt) + evalGraph(bo->op2, vt);
		} else if (bo->ot == OperationType::AND) {
			result = evalGraph(bo->op1, vt) & evalGraph(bo->op2, vt);
		} else if (bo->ot == OperationType::MULTIPLY) {
			result = evalGraph(bo->op1, vt) * evalGraph(bo->op2, vt);
		} else if (bo->ot == OperationType::OR) {
			result = evalGraph(bo->op1, vt) | evalGraph(bo->op2, vt);
		} else if (bo->ot == OperationType::XOR) {
			result = evalGraph(bo->op1, vt) ^ evalGraph(bo->op2, vt);
		} else if (bo->ot == OperationType::POW) {
			result = powi(evalGraph(bo->op1, vt), evalGraph(bo->op2, vt));
		}
	} else if (head->it == ItemType::HIGHLEVEL) {
		HighLevelOperator* hlo = (HighLevelOperator*)head;
		if (hlo->ot == HighLevelOperation::BTEST) {
			unsigned long v1 = evalGraph(hlo->op1, vt);
			unsigned long v2 = evalGraph(hlo->op2, vt);
			result = (v1 & v2) == v2;
		} else if (hlo->ot == HighLevelOperation::SHR) {
			result = evalGraph(hlo->op1, vt) >> evalGraph(hlo->op2, vt);
		} else if (hlo->ot == HighLevelOperation::GREATER) {
			result = evalGraph(hlo->op1, vt) > evalGraph(hlo->op2, vt);
		} else if (hlo->ot == HighLevelOperation::GEQ || hlo->ot == HighLevelOperation::GEQ16) {
			result = evalGraph(hlo->op1, vt) >= evalGraph(hlo->op2, vt);
		} else {
			__debugbreak();
		}
	}
	return result;
}

std::string graphToString(Item* __restrict head, VariableTable& vt) {
	if (head->it == ItemType::VALUE) {
		return std::to_string(((Value*)head)->val);
	} else if (head->it == ItemType::VARIABLE) {
		Variable* v = (Variable*)head;
		if (v->vnum >= vt.size()) {
			return "ERR: UDEF VAR";
		} 
		return vt.at(v->vnum).name;
	} else if (head->it == ItemType::UNARY) {
		UnaryOperator* uo = (UnaryOperator*)head;
		if(uo->ot == OperationType::NEGATIVE){
			if(uo->op1->it == ItemType::BINARY || uo->op1->it == ItemType::HIGHLEVEL){
				return "-(" + graphToString(uo->op1, vt) + ")";
			} else {
				return "-" + graphToString(uo->op1, vt);
			}
		} else if (uo->ot == OperationType::NOT) {
			if (uo->op1->it == ItemType::BINARY || uo->op1->it == ItemType::HIGHLEVEL) {
				return "~(" + graphToString(uo->op1, vt) + ")";
			}
			else {
				return "~" + graphToString(uo->op1, vt);
			}
		}
	} else if (head->it == ItemType::BINARY) { 
		BinaryOperator* bo = (BinaryOperator*)head;
		std::string res = graphToString(bo->op1, vt);
		if(bo->op1->it == ItemType::BINARY || bo->op1->it == ItemType::HIGHLEVEL){
			res = "(" + res + ")";
		}
		if(bo->ot == OperationType::ADD){
			res += " + ";
		} else if (bo->ot == OperationType::AND) {
			res += " & ";
		} else if (bo->ot == OperationType::MULTIPLY) {
			res += " * ";
		} else if (bo->ot == OperationType::OR) {
			res += " | ";
		} else if (bo->ot == OperationType::XOR) {
			res += " ^ ";
		} else if (bo->ot == OperationType::POW) {
			res += " ** ";
		}
		if (bo->op2->it == ItemType::BINARY || bo->op2->it == ItemType::HIGHLEVEL) {
			res += "(" + graphToString(bo->op2, vt) + ")";
		} else {
			res += graphToString(bo->op2, vt);
		}
		
		return res;
	} else if (head->it == ItemType::HIGHLEVEL) {
		HighLevelOperator* bo = (HighLevelOperator*)head;
		std::string res = graphToString(bo->op1, vt);
		if (bo->op1->it == ItemType::BINARY || bo->op1->it == ItemType::HIGHLEVEL) {
			res = "(" + res + ")";
		}
		if (bo->ot == HighLevelOperation::BTEST) {
			res += " ?~ ";
		} else if (bo->ot == HighLevelOperation::SHR) {
			res += " >> ";
		} else if (bo->ot == HighLevelOperation::LESS) {
			res += " ?< ";
		} else if (bo->ot == HighLevelOperation::GREATER) {
			res += " ?> ";
		} else if (bo->ot == HighLevelOperation::EQ) {
			res += " ?= ";
		} else if (bo->ot == HighLevelOperation::GEQ || bo->ot == HighLevelOperation::GEQ16) {
			res += " >=? ";
		}

		if (bo->op2->it == ItemType::BINARY || bo->op2->it == ItemType::HIGHLEVEL) {
			res += "(" + graphToString(bo->op2, vt) + ")";
		} else {
			res += graphToString(bo->op2, vt);
		}

		return res;
	}
}

__declspec(restrict) Value* NewValue(unsigned long value){
	node_count++;
	Value* v = (Value*)malloc(sizeof(Value));
	v->it = ItemType::VALUE;
	v->val = value;
	return v;
}
__declspec(restrict) UnaryOperator* NewUnary(OperationType ot,  Item* __restrict item){
	node_count++;
	UnaryOperator* v = (UnaryOperator*)malloc(sizeof(UnaryOperator));
	v->it = ItemType::UNARY;
	v->op1 = item;
	v->ot = ot;
	return v;
}
__declspec(restrict) BinaryOperator* NewBinary(OperationType ot, Item* __restrict item1, Item* __restrict item2){
	node_count++;
	BinaryOperator* v = (BinaryOperator*)malloc(sizeof(BinaryOperator));
	v->it = ItemType::BINARY;
	v->op1 = item1;
	v->op2 = item2;
	v->ot = ot;
	return v;
}

__declspec(restrict) Variable* NewSubVar(int num){
	node_count++;
	Variable* v = (Variable*)malloc(sizeof(Variable));
	v->it = ItemType::VARIABLE;
	v->vnum = num;
	return v;
}

__declspec(restrict) Variable* NewVariable(std::string name, VariableTable& vt, unsigned long value, bool extraneous) {
	node_count++;
	int found = vt.size();
	for (int x = 0; x < vt.size(); x++) {
		if(vt.at(x).name == name) {
			found = x;
		}
	}

	Variable* v = (Variable*)malloc(sizeof(Variable));
	v->it = ItemType::VARIABLE;
	v->vnum = found;
	if(found == vt.size()){
		VarEntry ve = {};
		ve.name = name;
		ve.value = value;
		ve.extraneous = extraneous;
		vt.push_back(ve);
	}
	return v;
}