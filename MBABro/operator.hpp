#pragma once
#include <vector>
#include <string>

#pragma pack(1)
enum class ItemType : unsigned char {
	VALUE,
	UNARY,
	BINARY,
	VARIABLE,
	HIGHLEVEL,
};

enum class OperationType : unsigned char {
	NEGATIVE,
	NOT,
	ADD,
	MULTIPLY,
	AND,
	OR,
	XOR,
	POW
};

enum class HighLevelOperation : unsigned char {
	BTEST,
	SHR,
	LESS,
	GREATER,
	EQ,
	GEQ,
	GEQ16,
	MOD16
};

struct Item {
	ItemType it;
};

struct Value : Item {
	unsigned long val;
};

struct UnaryOperator : Item {
	OperationType ot;
	Item* op1;
};

struct BinaryOperator : UnaryOperator {
	Item* op2;
};

struct Variable : Item {
	unsigned long vnum;
};

struct HighLevelOperator : Item {
	HighLevelOperation ot;
	Item* op1;
	Item* op2;
};

struct VarEntry {
	std::string name;
	unsigned long value;
	bool extraneous = false;
};

typedef std::vector<VarEntry> VariableTable;
