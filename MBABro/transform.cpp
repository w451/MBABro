#include "transform.hpp"
#include <stack>
#include <immintrin.h>
#include <random>
#include <iostream>

VariableTable sub_vt = { {"sub1", 0}, {"sub2", 0} };
std::vector<OperationSubstitution> substitutions;

void AddSubstitution(OperationType ot, float p, Item* __restrict sub) {
	OperationSubstitution os = {};
	os.type = ot;
	os.substitution = sub;
	os.prob = p;
	substitutions.push_back(os);
}

void ClearSubstitutions(){
	for(OperationSubstitution os : substitutions){
		FreeGraph(os.substitution);
	}
	substitutions.clear();
}

__declspec(restrict)Item* ApplySubstitutions(Item* __restrict head){
	for(OperationSubstitution os : substitutions){
		head = ApplySubstitution(os, head);
	}
	return head;
}

__declspec(restrict) Item* ApplySubstitution(OperationSubstitution os, Item* __restrict head) {
	static std::random_device rd;
	static std::mt19937 e2(rd());
	static std::uniform_real_distribution<> dist(0, 1);

	if (head->it == ItemType::UNARY) {
		UnaryOperator* uo = (UnaryOperator*)head;
		if (uo->ot == os.type && os.prob >= dist(e2)) {
			return UnarySubstitute(os, uo);
		} else {
			uo->op1 = ApplySubstitution(os, uo->op1);
			return uo;
		}
	}
	else if (head->it == ItemType::BINARY) {
		BinaryOperator* bo = (BinaryOperator*)head;
		if (bo->ot == os.type && os.prob >= dist(e2)) {
			return BinarySubstitute(os, bo);
		} else {
			bo->op1 = ApplySubstitution(os, bo->op1);
			bo->op2 = ApplySubstitution(os, bo->op2);
			return bo;
		}
	} else {
		return head;
	}
}

__declspec(restrict) Item* CopyGraph(Item* __restrict head){
	if (head->it == ItemType::UNARY) {
		UnaryOperator* uo = (UnaryOperator*)head;
		UnaryOperator* cuo = NewUnary(uo->ot, CopyGraph(uo->op1));
		return cuo;
	}
	else if (head->it == ItemType::BINARY) {
		BinaryOperator* bo = (BinaryOperator*)head;
		BinaryOperator* cbo = NewBinary(bo->ot, CopyGraph(bo->op1), CopyGraph(bo->op2));
		return cbo;
	} else if(head->it == ItemType::VALUE){
		Value* v = (Value*)head;
		Value* cv = NewValue(v->val);
		return cv;
	}
	else if (head->it == ItemType::VARIABLE) {
		node_count++;
		Variable* v = (Variable*)head;
		Variable* vcopy = (Variable*)malloc(sizeof(Variable));
		vcopy->it = ItemType::VARIABLE;
		vcopy->vnum = v->vnum;
		return vcopy;
	} else if (head->it == ItemType::HIGHLEVEL) {
		HighLevelOperator* bo = (HighLevelOperator*)head;
		HighLevelOperator* cbo = NewHighLevel(bo->ot, CopyGraph(bo->op1), CopyGraph(bo->op2));
		return cbo;
	}
}

void FreeNode(Item* __restrict head){
	node_count--;
	free(head);
}

void FreeGraph(Item* __restrict head){
	if (head->it == ItemType::UNARY) {
		UnaryOperator* uo = (UnaryOperator*)head;
		FreeGraph(uo->op1);
	}
	else if (head->it == ItemType::BINARY) {
		BinaryOperator* bo = (BinaryOperator*)head;
		FreeGraph(bo->op1);
		FreeGraph(bo->op2);
	} else if (head->it == ItemType::HIGHLEVEL) {
		HighLevelOperator* bo = (HighLevelOperator*)head;
		FreeGraph(bo->op1);
		FreeGraph(bo->op2);
	}
	node_count--;
	free(head);
}

__declspec(restrict) Item* UnarySubstitute(OperationSubstitution os, UnaryOperator* __restrict uo){
	Item* var1 = uo->op1;
	Item* newg = CopyGraph(os.substitution);

	std::stack<Item*> nodes;
	nodes.push(newg);
	while(nodes.size() > 0){
		Item* node = nodes.top();
		nodes.pop();

		if (node->it == ItemType::UNARY) {
			UnaryOperator* nodeuo = (UnaryOperator*)node;

			if(nodeuo->op1->it == ItemType::VARIABLE && ((Variable*)nodeuo->op1)->vnum == 0){
				FreeGraph(nodeuo->op1);
				nodeuo->op1 = CopyGraph(var1);
			} else {
				nodes.push(nodeuo->op1);
			}
			
		} else if (node->it == ItemType::BINARY) {
			BinaryOperator* bo = (BinaryOperator*)node;

			if (bo->op1->it == ItemType::VARIABLE && ((Variable*)bo->op1)->vnum == 0) {
				FreeGraph(bo->op1);
				bo->op1 = CopyGraph(var1);
			}
			else {
				nodes.push(bo->op1);
			}

			if (bo->op2->it == ItemType::VARIABLE && ((Variable*)bo->op2)->vnum == 0) {
				FreeGraph(bo->op2);
				bo->op2 = CopyGraph(var1);
			}
			else {
				nodes.push(bo->op2);
			}
		}
	}

	FreeGraph(uo);
	return newg;
}

__declspec(restrict) Item* AddExtraneousVars(Item* __restrict head, int count, VariableTable& vt, std::string prefix) {
	for (int x = 0; x < count; x++) {
		unsigned int rand = 0;
		_rdrand32_step(&rand);
		unsigned int rand2 = 0;
		_rdrand32_step(&rand2);

		std::string vname;

		if (count == 1) {
			vname = prefix;
		} else {
			vname = prefix + std::to_string(x);
		}

		if(rand&1){
			//^ z ^ z
			head = NewBinary(OperationType::XOR, 
				NewBinary(OperationType::XOR,
					NewVariable(vname, vt, rand2, true),
					head
					),
				NewVariable(vname, vt)
				);
		} else {
			//+ z - z		
			head = NewBinary(OperationType::ADD,
				NewBinary(OperationType::ADD,
					NewVariable(vname, vt, rand2, true),
					head
				),
				NewUnary(OperationType::NEGATIVE, NewVariable(vname, vt))
			);
		}
	}
	return head;
}

__declspec(restrict) Item* BinarySubstitute(OperationSubstitution os, BinaryOperator* __restrict bo){
	Item* var1 = bo->op1;
	Item* var2 = bo->op2;

	unsigned int rnd = 0; //Switches variable positions
	_rdrand32_step(&rnd);
	if (rnd & 1) {
		Item* tmp = var1;
		var1 = var2;
		var2 = tmp;
	}

	Item* newg = CopyGraph(os.substitution);

	std::stack<Item*> nodes;
	nodes.push(newg);
	while (nodes.size() > 0) {
		Item* node = nodes.top();
		nodes.pop();

		if (node->it == ItemType::UNARY) {
			UnaryOperator* nodeuo = (UnaryOperator*)node;

			if (nodeuo->op1->it == ItemType::VARIABLE && ((Variable*)nodeuo->op1)->vnum == 0) {
				FreeGraph(nodeuo->op1);
				nodeuo->op1 = CopyGraph(var1);
			} else if (nodeuo->op1->it == ItemType::VARIABLE && ((Variable*)nodeuo->op1)->vnum == 1) {
				FreeGraph(nodeuo->op1);
				nodeuo->op1 = CopyGraph(var2);
			}
			else {
				nodes.push(nodeuo->op1);
			}

		}
		else if (node->it == ItemType::BINARY) {
			BinaryOperator* bo = (BinaryOperator*)node;

			if (bo->op1->it == ItemType::VARIABLE && ((Variable*)bo->op1)->vnum == 0) {
				FreeGraph(bo->op1);
				bo->op1 = CopyGraph(var1);
			} else if (bo->op1->it == ItemType::VARIABLE && ((Variable*)bo->op1)->vnum == 1) {
				FreeGraph(bo->op1);
				bo->op1 = CopyGraph(var2);
			}
			else {
				nodes.push(bo->op1);
			}

			if (bo->op2->it == ItemType::VARIABLE && ((Variable*)bo->op2)->vnum == 0) {
				FreeGraph(bo->op2);
				bo->op2 = CopyGraph(var1);
			} else if (bo->op2->it == ItemType::VARIABLE && ((Variable*)bo->op2)->vnum == 1) {
				FreeGraph(bo->op2);
				bo->op2 = CopyGraph(var2);
			}
			else {
				nodes.push(bo->op2);
			}
		}
	}

	FreeGraph(bo);
	return newg;
}

void RandomizeGraph(Item* __restrict head){
	if(head->it == ItemType::BINARY){
		BinaryOperator* bo = (BinaryOperator*)head;
		unsigned int rnd = 0;
		_rdrand32_step(&rnd);
		if(rnd & 1){
			Item* tmp = bo->op1;
			bo->op1 = bo->op2;
			bo->op2 = tmp;
		}
		RandomizeGraph(bo->op1);
		RandomizeGraph(bo->op2);
	} else if (head->it == ItemType::UNARY){
		UnaryOperator* uo = (UnaryOperator*)head;
		RandomizeGraph(uo->op1);
	}
}

__declspec(restrict) Item* OptimizeComplete(Item* __restrict head) {
	unsigned long long lastnc = node_count;
	while(1){
		head = Optimize(head);
		if(node_count == lastnc){
			break;
		}
		lastnc = node_count;
	}
	return head;
}

bool FindReversibleVar(Item* __restrict i, unsigned long vnum){
	if(i->it == ItemType::VARIABLE){
		if(((Variable*)i)->vnum == vnum){
			return true;
		}
	} else if (i->it == ItemType::UNARY) {
		UnaryOperator* uo = (UnaryOperator*)i;
		return FindReversibleVar(uo->op1, vnum);
	} else if (i->it == ItemType::BINARY) {
		BinaryOperator* bo = (BinaryOperator*)i;
		if(bo->ot == OperationType::AND || bo->ot == OperationType::OR || bo->ot == OperationType::MULTIPLY){
			return false;
		}
		return FindReversibleVar(bo->op1, vnum) || FindReversibleVar(bo->op2, vnum);
	}
	return false;
}

int SearchDecision(Item* __restrict head, unsigned long vnum){
	if (head->it == ItemType::BINARY) {
		BinaryOperator* bo = (BinaryOperator*)head;
		if (bo->ot == OperationType::AND || bo->ot == OperationType::OR || bo->ot == OperationType::MULTIPLY) {
			return -1;
		}
		if(FindReversibleVar(bo->op1, vnum)){
			return 0;
		}
		if (FindReversibleVar(bo->op2, vnum)) {
			return 1;
		}
	}
	return -1;
}

__declspec(restrict) Item* SolveForVar(Item* __restrict e1, Item* __restrict e2, unsigned long vnum){
	Item* left = CopyGraph(e1);
	Item* right = CopyGraph(e2);
	while(1){
		if(left->it == ItemType::UNARY){
			UnaryOperator* uo = (UnaryOperator*)left;
			if(uo->ot == OperationType::NEGATIVE){
				right = NewUnary(OperationType::NEGATIVE, right);
				left = uo->op1;
				FreeNode(uo);
			} else if (uo->ot == OperationType::NOT) {
				right = NewUnary(OperationType::NOT, right);
				left = uo->op1;
				FreeNode(uo);
			}
		} else if (left->it == ItemType::BINARY) {
			BinaryOperator* bo = (BinaryOperator*)left;
			int dec = SearchDecision(left, vnum);
			if(dec == -1){
				return 0;
			} else if (dec == 0) {
				if(bo->ot == OperationType::ADD){
					right = NewBinary(OperationType::ADD, NewUnary(OperationType::NEGATIVE, bo->op2), right);
					left = bo->op1;
					FreeNode(bo);
				} else if (bo->ot == OperationType::XOR) {
					right = NewBinary(OperationType::XOR, bo->op2, right);
					left = bo->op1;
					FreeNode(bo);
				}
			} else if (dec == 1) {
				if(bo->ot == OperationType::ADD){
					right = NewBinary(OperationType::ADD, NewUnary(OperationType::NEGATIVE, bo->op1), right);
					left = bo->op2;
					FreeNode(bo);
				} else if (bo->ot == OperationType::XOR) {
					right = NewBinary(OperationType::XOR, bo->op1, right);
					left = bo->op2;
					FreeNode(bo);
				}
			}
		} else if(left->it == ItemType::VARIABLE){
			Variable* v = (Variable*)left;
			if(v->vnum == vnum){
				FreeGraph(left);
				return right;
			} else {
				FreeGraph(left);
				FreeGraph(right);
				return 0;
			}
		} else if (left->it == ItemType::VALUE) {
			FreeGraph(left);
			FreeGraph(right);
			return 0;
		}
	}
}

__declspec(restrict) Item* Optimize(Item* __restrict head){
	if (head->it == ItemType::UNARY) {
		UnaryOperator* uo = (UnaryOperator*)head;
		if(uo->op1->it == ItemType::UNARY){
			UnaryOperator* uo2 = (UnaryOperator*)uo->op1;
			Item* ret = 0;
			if(uo->ot == uo2->ot){ //Skip over -- or ~~
				ret = uo2->op1;
				FreeNode(uo);
				FreeNode(uo2);
			} else { //-~ or ~- reduce to x +/- 1
				int val = uo->ot == OperationType::NEGATIVE ? 1 : -1;

				ret = NewBinary(OperationType::ADD, NewValue(val), uo2->op1);
				FreeNode(uo);
				FreeNode(uo2);
			}
			return Optimize(ret);
		} else if(uo->op1->it == ItemType::VALUE){
			Value* v = (Value*)uo->op1;
			if(uo->ot == OperationType::NEGATIVE){
				v->val = -v->val;
			} else if (uo->ot == OperationType::NOT) {
				v->val = ~v->val;
			}

			FreeNode(uo);
			return v;
		} else if (uo->op1->it == ItemType::BINARY) {
			
			BinaryOperator* bo = (BinaryOperator*)uo->op1;

			if(uo->ot == OperationType::NOT){
				//distribute not over & | if one argument has ~
				//distribute not over ^ always, favor giving ~ to an argument with ~
				bool op1not = bo->op1->it == ItemType::UNARY && ((UnaryOperator*)bo->op1)->ot == OperationType::NOT;
				bool op2not = bo->op2->it == ItemType::UNARY && ((UnaryOperator*)bo->op2)->ot == OperationType::NOT;

				int notspot = 0;
				unsigned int random2 = 0;
				_rdrand32_step(&random2);
				random2 &= 1;

				if (op1not && !op2not) {
					notspot = 0;
				}
				else if (!op1not && op2not) {
					notspot = 1;
				}
				else if (op1not == op2not) {
					unsigned int tmp = 0;
					_rdrand32_step(&tmp);
					notspot = tmp & 1;
				}

				if((bo->ot == OperationType::OR || bo->ot == OperationType::AND) && (op1not || op2not)){
					if(bo->ot == OperationType::OR){
						bo->ot = OperationType::AND;
					} else if (bo->ot == OperationType::AND) {
						bo->ot = OperationType::OR;
					}

					if(notspot == 0){
						UnaryOperator* skip1 = (UnaryOperator*)bo->op1;
						bo->op1 = skip1->op1;
						bo->op2 = NewUnary(OperationType::NOT, bo->op2);
						FreeNode(head);
						FreeNode(skip1);
						return Optimize(bo);
					} else if (notspot == 1) {
						UnaryOperator* skip2 = (UnaryOperator*)bo->op2;
						bo->op2 = skip2->op1;
						bo->op1 = NewUnary(OperationType::NOT, bo->op1);
						FreeNode(head);
						FreeNode(skip2);
						return Optimize(bo);
					}
				} else if (bo->ot == OperationType::XOR) {
					
					if (!op1not && !op2not) {
						if(random2){
							bo->op2 = NewUnary(OperationType::NOT, bo->op2);
						} else {
							bo->op1 = NewUnary(OperationType::NOT, bo->op1);
						}
						FreeNode(head);
						return Optimize(bo);
					} else if (notspot == 0) {
						UnaryOperator* skip1 = (UnaryOperator*)bo->op1;
						bo->op1 = skip1->op1;
						FreeNode(head);
						FreeNode(skip1);
						return Optimize(bo);
					} else if (notspot == 1) {
						UnaryOperator* skip2 = (UnaryOperator*)bo->op2;
						bo->op2 = skip2->op1;
						FreeNode(head);
						FreeNode(skip2);
						return Optimize(bo);
					}
				}
			} else if (uo->ot == OperationType::NEGATIVE) {
				bool op1neg = bo->op1->it == ItemType::UNARY && ((UnaryOperator*)bo->op1)->ot == OperationType::NEGATIVE;
				bool op2neg = bo->op2->it == ItemType::UNARY && ((UnaryOperator*)bo->op2)->ot == OperationType::NEGATIVE;

				if(bo->ot == OperationType::ADD && (op1neg || op2neg)){
					
					if (op1neg && op2neg) {
						UnaryOperator* skip1 = (UnaryOperator*)bo->op1;
						UnaryOperator* skip2 = (UnaryOperator*)bo->op1;
						bo->op1 = (skip1->op1);
						bo->op2 = (skip2->op1);
						FreeNode(skip1);
						FreeNode(skip2);
						FreeNode(head);
						return Optimize(bo);
					} else if (op1neg) {
						UnaryOperator* skip1 = (UnaryOperator*)bo->op1;
						bo->op1 = (skip1->op1);
						bo->op2 = (NewUnary(OperationType::NEGATIVE,bo->op2));
						FreeNode(skip1);
						FreeNode(head);
						return Optimize(bo);
					} else if (op2neg) {
						UnaryOperator* skip2 = (UnaryOperator*)bo->op2;
						bo->op2 = (skip2->op1);
						bo->op1 = (NewUnary(OperationType::NEGATIVE,bo->op1));
						FreeNode(skip2);
						FreeNode(head);
						return Optimize(bo);
					}
					
				}
			}
		}
		uo->op1 = Optimize(uo->op1);
	} else if (head->it == ItemType::BINARY) {
		BinaryOperator* bo = (BinaryOperator*)head;

		Item* first = bo->op1;
		Item* second = bo->op2;

		if(first->it == ItemType::VALUE || second->it == ItemType::VALUE){
			Value* val = (Value*)(first->it == ItemType::VALUE ? first : second);
			Item* other = first->it != ItemType::VALUE ? first : second;
			
			if(bo->ot==OperationType::AND){
				if(val->val == 0){
					FreeGraph(bo);
					return NewValue(0);
				} else if (val->val == 0xffffffff){
					FreeNode(val);
					FreeNode(bo);
					return Optimize(other);
				}
			} else if (bo->ot == OperationType::OR) {
				if (val->val == 0) {
					FreeNode(bo);
					FreeNode(val);
					return Optimize(other);
				} else if (val->val == 0xffffffff) {
					FreeGraph(bo);
					return NewValue(0xffffffff);
				}
			} else if (bo->ot == OperationType::XOR) {
				if (val->val == 0) {
					FreeNode(bo);
					FreeNode(val);
					return Optimize(other);
				} else if (val->val == 0xffffffff) {
					FreeNode(bo);
					FreeNode(val);
					return Optimize(NewUnary(OperationType::NOT, other));
				}
			} else if (bo->ot == OperationType::MULTIPLY) {
				if (val->val == 0) {
					FreeGraph(bo);
					return NewValue(0);
				} else if (val->val == 1) {
					FreeNode(bo);
					FreeNode(val);
					return Optimize(other);
				} else if (val->val == 0xffffffff) {
					FreeNode(bo);
					FreeNode(val);
					return Optimize(NewUnary(OperationType::NEGATIVE, other));
				}
			} else if (bo->ot == OperationType::ADD) { 
				
				if (val->val == 0) {
					FreeNode(val);
					FreeNode(bo);
					return Optimize(other);
				}
			}
		}

		if (first->it == ItemType::VALUE && second->it == ItemType::VALUE) {
			Value* v1 = (Value*)first;
			Value* v2 = (Value*)second;
			long value = 0;
			if(bo->ot == OperationType::ADD){
				value = v1->val + v2->val;
			} else if (bo->ot == OperationType::MULTIPLY) {
				value = v1->val * v2->val;
			} else if (bo->ot == OperationType::XOR) {
				value = v1->val ^ v2->val;
			} else if (bo->ot == OperationType::OR) {
				value = v1->val | v2->val;
			} else if (bo->ot == OperationType::AND) {
				value = v1->val & v2->val;
			}
			FreeNode(bo);
			FreeNode(v1);
			FreeNode(v2);
			return NewValue(value);
		}

		if(bo->ot == OperationType::XOR){
			if(first->it == ItemType::UNARY && ((UnaryOperator*)first)->ot == OperationType::NOT
				&& second->it == ItemType::UNARY && ((UnaryOperator*)second)->ot == OperationType::NOT){
				bo->op1 = ((UnaryOperator*)first)->op1;
				bo->op2 = ((UnaryOperator*)second)->op1;
				FreeNode(first);
				FreeNode(second);
				return Optimize(bo);
			}
		} else if (bo->ot == OperationType::MULTIPLY) {
			unsigned int binaryItem = -1;
			BinaryOperator* bini = 0;
			Item* other = 0;
			if(first->it == ItemType::BINARY && second->it == ItemType::BINARY){
				_rdrand32_step(&binaryItem);
				binaryItem &= 1;
			} else if (first->it == ItemType::BINARY){
				binaryItem = 0;
			} else if (second->it == ItemType::BINARY) {
				binaryItem = 1;
			}
			
			if(binaryItem == 0){
				bini = (BinaryOperator*)first;
				other = second;
			} else if (binaryItem == 1) {
				bini = (BinaryOperator*)second;
				other = first;
			}
			
			if (bini) {
				if(bini->ot == OperationType::ADD){ //Distribute multiplication over addition
					Item* res = NewBinary(OperationType::ADD,
						NewBinary(OperationType::MULTIPLY, bini->op1, other),
						NewBinary(OperationType::MULTIPLY, bini->op2, CopyGraph(other))
					);

					FreeNode(bini);
					FreeNode(head);
					return Optimize(res);
				} else if (bini->ot != OperationType::MULTIPLY) {
					if (other->it == ItemType::VALUE) {
						Value* v = (Value*)other;
						Item* prev = 0;
						for (unsigned long x = 1; x != 0; x<<=1) {
							if(v->val & x){
								if(!prev){
									prev = NewBinary(bini->ot, 
										NewBinary(OperationType::MULTIPLY, NewValue(x), bini->op1),
										NewBinary(OperationType::MULTIPLY, NewValue(x), bini->op2)
									);
								} else {
									prev = NewBinary(OperationType::ADD, prev, 
										NewBinary(bini->ot,
											NewBinary(OperationType::MULTIPLY, NewValue(x), CopyGraph(bini->op1)),
											NewBinary(OperationType::MULTIPLY, NewValue(x), CopyGraph(bini->op2))
										)
									);
								}
							}
						}

						if(prev){ 
							FreeNode(v);
							FreeNode(bo);
							FreeNode(bini);

							return Optimize(prev);
						}
					}
					
				} else { //* over * with 2 values
					if (other->it == ItemType::VALUE) {
						Value* v = (Value*)other;
						if(bini->op1->it == ItemType::VALUE){
							Item* res = NewBinary(OperationType::MULTIPLY, NewValue(v->val * ((Value*)bini->op1)->val), bini->op2);
							FreeGraph(bini->op1);
							FreeNode(bini);
							FreeNode(v);
							FreeNode(head);
							return Optimize(res);
						} else if(bini->op2->it == ItemType::VALUE){
							Item* res = NewBinary(OperationType::MULTIPLY, NewValue(v->val * ((Value*)bini->op2)->val), bini->op1);
							FreeGraph(bini->op2);
							FreeNode(bini);
							FreeNode(v);
							FreeNode(head);
							return Optimize(res);
						}
					}
				}
			} 
		} else if (bo->ot == OperationType::AND) {
			unsigned int binaryItem = -1;
			BinaryOperator* bini = 0;
			Item* other = 0;
			if (first->it == ItemType::BINARY && second->it == ItemType::BINARY) {
				_rdrand32_step(&binaryItem);
				binaryItem &= 1;
			} else if (first->it == ItemType::BINARY) {
				binaryItem = 0;
			} else if (second->it == ItemType::BINARY) {
				binaryItem = 1;
			}

			if (binaryItem == 0) {
				bini = (BinaryOperator*)first;
				other = second;
			} else if (binaryItem == 1) {
				bini = (BinaryOperator*)second;
				other = first;
			}

			/*if (bini && bini->ot == OperationType::OR) { // & over |
				other = other;
				Item* result = NewBinary(OperationType::OR,
					NewBinary(OperationType::AND, other, bini->op1),
					NewBinary(OperationType::AND, CopyGraph(other), bini->op2)
				);
				FreeNode(bo);
				FreeNode(bini);
				return result;
			}*/

		} else if (bo->ot == OperationType::OR) {
			unsigned int binaryItem = -1;
			BinaryOperator* bini = 0;
			Item* other = 0;
			

			if (first->it == ItemType::BINARY && second->it == ItemType::BINARY) {
				_rdrand32_step(&binaryItem);
				binaryItem &= 1;
			} else if (first->it == ItemType::BINARY) {
				binaryItem = 0;
			} else if (second->it == ItemType::BINARY) {
				binaryItem = 1;
			}

			if (binaryItem == 0) {
				bini = (BinaryOperator*)first;
				other = second;
			} else if (binaryItem == 1) {
				bini = (BinaryOperator*)second;
				other = first;
			}

			if (bini && bini->ot == OperationType::AND) { // | over &
				Item* result = NewBinary(OperationType::AND,
					NewBinary(OperationType::OR, other, bini->op1),
					NewBinary(OperationType::OR, CopyGraph(other), bini->op2)
				);
				FreeNode(bo);
				FreeNode(bini);
				return Optimize(result);
			}

		}

		bo->op1 = Optimize(bo->op1);
		bo->op2 = Optimize(bo->op2);
	}

	//TODO Simplify repeated operations
	//TODO Solve for 1, solve for 0, multiply/add/XOR/OR/AND
	return head;
}

__declspec(restrict) Item* SubstituteVariable(Item* __restrict head, int vnum, Item* __restrict sub) {
	if (head->it == ItemType::VALUE) {
		return head;
	} else if (head->it == ItemType::UNARY) {
		UnaryOperator* uo = (UnaryOperator*)head;
		uo->op1 = SubstituteVariable(uo->op1, vnum, sub);
		return uo;
	} else if (head->it == ItemType::BINARY) {
		BinaryOperator* bo = (BinaryOperator*)head;
		bo->op1 = SubstituteVariable(bo->op1, vnum, sub);
		bo->op2 = SubstituteVariable(bo->op2, vnum, sub);
		return bo;
	} else if (head->it == ItemType::VARIABLE) {
		Variable* v = (Variable*)head;
		if (v->vnum == vnum) {
			FreeNode(v);
			return CopyGraph(sub);
		} else {
			return v;
		}
	}
}

unsigned long long CountNodes(Item* __restrict head) {
	if (head->it == ItemType::UNARY) {
		UnaryOperator* uo = (UnaryOperator*)head;
		return 1 + CountNodes(uo->op1);
	} else if (head->it == ItemType::BINARY) {
		BinaryOperator* bo = (BinaryOperator*)head;
		return 1 + CountNodes(bo->op1) + CountNodes(bo->op2);
	} else if (head->it == ItemType::HIGHLEVEL) {
		HighLevelOperator* bo = (HighLevelOperator*)head;
		return 1 + CountNodes(bo->op1) + CountNodes(bo->op2);
	} else if (head->it == ItemType::VALUE) {
		return 1;
	} else if (head->it == ItemType::VARIABLE) {
		return 1;
	} 
}