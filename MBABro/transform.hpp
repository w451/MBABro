#include "mbabro.hpp"

extern VariableTable sub_vt;

struct OperationSubstitution {
	OperationType type;
	Item* substitution;
	float prob;
};

extern std::vector<OperationSubstitution> substitutions;

void AddSubstitution(OperationType ot, float p, Item* __restrict sub);
void ClearSubstitutions();
__declspec(restrict) Item* ApplySubstitution(OperationSubstitution os, Item* __restrict head);
__declspec(restrict)Item* ApplySubstitutions(Item* __restrict head);
__declspec(restrict)Item* UnarySubstitute(OperationSubstitution os, UnaryOperator* __restrict uo);
__declspec(restrict)Item* BinarySubstitute(OperationSubstitution os, BinaryOperator* __restrict bo);
__declspec(restrict)Item* CopyGraph(Item* __restrict head);
void FreeGraph(Item* __restrict head);
void FreeNode(Item* __restrict head);
Variable* NewSubVar(int num);
__declspec(restrict)Item* Optimize(Item* __restrict head);
__declspec(restrict)Item* OptimizeComplete(Item* __restrict head);
bool FindReversibleVar(Item* __restrict i, unsigned long vnum);
int SearchDecision(Item* __restrict head, unsigned long vnum);
__declspec(restrict) Item* SolveForVar(Item* __restrict e1, Item* __restrict e2, unsigned long vnum);
__declspec(restrict)Item* AddExtraneousVars(Item* __restrict head, int count, VariableTable& vt, std::string prefix = "evar");
__declspec(restrict)Item* SubstituteVariable(Item* __restrict head, int vnum, Item* __restrict sub);
unsigned long long CountNodes(Item* __restrict head);

void RandomizeGraph(Item* head);