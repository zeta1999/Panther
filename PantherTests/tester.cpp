

#include "tester.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <Windows.h>

void Tester::RunTest(TestFunction testFunction) {
	std::string result = testFunction();
	if (result.size() != 0) {
		std::cout << "Tester failed test." << std::endl;
		std::cout << result << std::endl;
	}
}

void Tester::RunTextFieldTest(std::string name, TextFieldTestFunction testFunction, std::string input, std::string expectedOutput) {
	// write the input to the file
	FILE* f;
	fopen_s(&f, TEMPORARY_FILE, "wb+");
	fwrite(input.c_str(), input.size(), 1, f);
	fclose(f);
	// create a temporary text field
	TextField* testField = new TextField(NULL, std::string(TEMPORARY_FILE), true);
	assert(testField->GetTextFile().IsLoaded());
	// run the actual test
	std::string result = testFunction(testField);
	if (result.size() != 0) {
		std::cout << "Tester failed test " << name << " with output:" << std::endl;
		std::cout << result << std::endl;
		delete testField;
		return;
	}
	// if the test did not return an error message, check the output of the text field after the operation
	std::string resultingText = "";
	TextFile* textFile = &testField->GetTextFile();
	for (int i = 0; i < textFile->GetLineCount(); i++) {
		resultingText += std::string(textFile->GetLine(i)->GetLine(), textFile->GetLine(i)->GetLength());
		if (i + 1 < textFile->GetLineCount())
			resultingText += "\n";
	}
	if (resultingText != expectedOutput) {
		std::cout << "Tester failed test " << name << " with incorrect output." << std::endl;
		std::cout << "Expected output:" << std::endl;
		std::cout << expectedOutput << std::endl;
		std::cout << "Actual output:" << std::endl;
		std::cout << resultingText << std::endl;
	} else {
		std::cout << "SUCCESS: Test " << name << std::endl;
	}
}

void Tester::RunTextFieldFileTest(std::string name, TextFieldTestFunction testFunction, std::string path, std::string expectedOutput) {
	std::ostringstream buf;
	path = std::string(__FILE__).substr(0, strlen(__FILE__) - strlen("tester.cpp")) + path;
	std::ifstream input (path.c_str()); 
	buf << input.rdbuf(); 
	RunTextFieldTest(name, testFunction, buf.str(), expectedOutput);
}
