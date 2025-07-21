#include <iostream>
#include <windows.h>
#include "obfuscator.hpp"

using namespace std;

/*void test1() {
	VariableTable vt = {};
	NewVariable("x", vt, 0x1aaaaaaa);
	NewVariable("y", vt, 0x2ccccccc);

	Item* graph = NewBinary(OperationType::ADD, NewVariable("x", vt), NewVariable("y", vt));

	cout << hex << graphToString(graph, vt) << " " << evalGraph(graph, vt) << endl;

	Item* o = Obfuscate(graph, vt);

	string output = graphToString(o, vt);

	cout << output << " " << evalGraph(o, vt) << endl;

	if (OpenClipboard(0))
	{
		HGLOBAL clipbuffer;
		char* buffer;
		EmptyClipboard();
		clipbuffer = GlobalAlloc(GMEM_DDESHARE, output.size() + 1);
		buffer = (char*)GlobalLock(clipbuffer);
		strcpy(buffer, LPCSTR(output.c_str()));
		GlobalUnlock(clipbuffer);
		SetClipboardData(CF_TEXT, clipbuffer);
		CloseClipboard();
	}

	FreeGraph(o);
	cout << node_count << endl;
}*/

int main() {
	VariableTable vt = {};
	NewVariable("x", vt, 0x95fe);
	NewVariable("y", vt, 0x161f);

	Item* graph = NewHighLevel(HighLevelOperation::MOD16, NewVariable("x", vt), NewVariable("y", vt));
	cout << graphToString(graph, vt) << endl;
	graph = CompileHighLevel(graph);
	graph = OptimizeComplete(graph);

	string output = graphToString(graph, vt);

	cout << output << endl;
	cout << "Eval: " << evalGraph(graph, vt) << endl;
	cout << "Correct: " << 0x95fe % 0x161f << endl;

	if (OpenClipboard(0))
	{
		HGLOBAL clipbuffer;
		char* buffer;
		EmptyClipboard();
		clipbuffer = GlobalAlloc(GMEM_DDESHARE, output.size() + 1);
		buffer = (char*)GlobalLock(clipbuffer);
		strcpy(buffer, LPCSTR(output.c_str()));
		GlobalUnlock(clipbuffer);
		SetClipboardData(CF_TEXT, clipbuffer);
		CloseClipboard();
	}

	FreeGraph(graph);
	cout << node_count << endl;
}