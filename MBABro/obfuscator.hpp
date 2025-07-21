#include "transform.hpp"

__declspec(restrict) Item* Obfuscate(Item* __restrict head, VariableTable& vt);
__declspec(restrict) Item* MakeZeroPolynomial(Item* __restrict head);
__declspec(restrict) Item* MakeOnePolynomial(Item* __restrict head);

__declspec(restrict) Item* CompileHighLevel(Item* __restrict head); //Converts a graph consisting of HL ops to basic operations

