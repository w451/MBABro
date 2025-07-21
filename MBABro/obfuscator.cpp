#include "obfuscator.hpp"
#include <intrin.h>
#include <stack>

__declspec(restrict) Item* Obfuscate(Item* __restrict head, VariableTable& vt) {

	float p = .3;

	//Linear substitution passes
	for (int x = 0; x < 1; x++) {
		AddSubstitution(OperationType::ADD, p, NewBinary(OperationType::ADD,
			NewBinary(OperationType::AND, NewSubVar(0), NewSubVar(1)),
			NewBinary(OperationType::OR, NewSubVar(0), NewSubVar(1))
		));

		AddSubstitution(OperationType::OR, p, NewBinary(OperationType::OR,
			NewBinary(OperationType::XOR, NewSubVar(0), NewSubVar(1)),
			NewBinary(OperationType::AND, NewSubVar(0), NewSubVar(1))
		));

		AddSubstitution(OperationType::OR, p, NewBinary(OperationType::ADD,
			NewSubVar(0),
			NewBinary(OperationType::AND, NewSubVar(1), NewUnary(OperationType::NOT, NewSubVar(0)))
		));

		AddSubstitution(OperationType::AND, p,
			NewUnary(OperationType::NOT,
				NewBinary(OperationType::OR, NewUnary(OperationType::NOT, NewSubVar(0)), NewUnary(OperationType::NOT, NewSubVar(1)))
			)
		);

		AddSubstitution(OperationType::NOT, p, NewBinary(OperationType::ADD,
			NewUnary(OperationType::NEGATIVE, NewSubVar(0)),
			NewValue(-1)
		));

		AddSubstitution(OperationType::XOR, p, NewBinary(OperationType::ADD,
			NewBinary(OperationType::ADD, NewSubVar(0), NewSubVar(1)),
			NewBinary(OperationType::MULTIPLY,
				NewValue(-2),
				NewBinary(OperationType::AND, NewSubVar(0), NewSubVar(1))
			)
		));
	}

	head = AddExtraneousVars(head, 1, vt, "z");

	Item* obf_main = ApplySubstitutions(CopyGraph(head));

	for (int var = 0; var < vt.size(); var++) {
		if (!vt.at(var).extraneous) {
			Item* vsolve = SolveForVar(head, head, var);
			if (vsolve) {
				Item* obfvsolve = ApplySubstitutions(vsolve);
				obf_main = SubstituteVariable(obf_main, var, obfvsolve);
				FreeGraph(obfvsolve);
			}
		}
	}

	for (int n = 0; n < 8; n++) {
	

		unsigned int usexor = 0;
		_rdrand32_step(&usexor);
		if (usexor & 4) {
			Item* nom = MakeZeroPolynomial(head);
			usexor &= 3;

			if (usexor == 0) {
				obf_main = NewBinary(OperationType::ADD, obf_main, nom);
			} else if (usexor == 1) {
				obf_main = NewBinary(OperationType::XOR, obf_main, nom);
			} else if (usexor == 2) {
				obf_main = NewBinary(OperationType::OR, obf_main, nom);
			} else if (usexor == 3) {
				obf_main = NewBinary(OperationType::AND, obf_main, NewBinary(OperationType::ADD, NewValue(-1), nom));
			}
		} else {
			Item* nom = MakeOnePolynomial(head);
			obf_main = NewBinary(OperationType::MULTIPLY, obf_main, nom);
		}
	}

	obf_main = OptimizeComplete(obf_main);
	RandomizeGraph(obf_main);

	ClearSubstitutions();
	FreeGraph(head);
	return obf_main;
}

__declspec(restrict) Item* MakeZeroPolynomial(Item* __restrict head) {
	Item* obf = ApplySubstitutions(CopyGraph(head));

	unsigned int usexor = 0;
	_rdrand32_step(&usexor);
	usexor &= 1;

	Item* result = 0;

	if (usexor) {
		result = NewBinary(OperationType::XOR, CopyGraph(head), obf);
	} else {
		result = NewBinary(OperationType::ADD, NewUnary(OperationType::NEGATIVE, CopyGraph(head)), obf);
	}

	//Incorporate terms from the main expression to confuse factoring algorithms

	std::vector<Item*> items;
	std::stack<Item*> it;
	it.push(head);
	while (it.size() > 0) {
		Item* i = it.top();
		it.pop();
		//if (CountNodes(i) < 20) {
			items.push_back(i);
		//}
		if (i->it == ItemType::UNARY) {
			UnaryOperator* uo = (UnaryOperator*)i;
			it.push(uo->op1);
		} else if (i->it == ItemType::BINARY) {
			BinaryOperator* bo = (BinaryOperator*)i;
			it.push(bo->op1);
			it.push(bo->op2);
		}
	}

	for (int x = 0; x < 3; x++) {
		unsigned int ritem = 0;
		_rdrand32_step(&ritem);
		ritem %= items.size();

		unsigned int op = 0;
		_rdrand32_step(&op);
		op &= 1;

		if (op) {
			result = NewBinary(OperationType::AND, result, CopyGraph(items.at(ritem)));
		} else {
			result = NewBinary(OperationType::MULTIPLY, result, CopyGraph(items.at(ritem)));
		}
	}

	return result;
}

__declspec(restrict) Item* MakeOnePolynomial(Item* __restrict head) {
	Item* zp = MakeZeroPolynomial(head);
	return NewUnary(OperationType::NEGATIVE, NewUnary(OperationType::NOT, zp));
}