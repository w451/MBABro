#include "operator.hpp"
#include <string>

std::string graphToString(Item* __restrict head, VariableTable& vt);
unsigned long evalGraph(Item* __restrict head, VariableTable& vt);

extern unsigned long long node_count;

__declspec(restrict) Value* NewValue(unsigned long value);
__declspec(restrict) UnaryOperator* NewUnary(OperationType ot, Item* __restrict item);
__declspec(restrict) BinaryOperator* NewBinary(OperationType ot, Item* __restrict item1, Item* __restrict item2);
__declspec(restrict) HighLevelOperator* NewHighLevel(HighLevelOperation ot, Item* __restrict item1, Item* __restrict item2);
__declspec(restrict) Variable* NewVariable(std::string name, VariableTable& vt, unsigned long value=0, bool extraneous=false);
unsigned long long powi(unsigned long long base, unsigned long long exp);