#ifndef ASSEMBLY_PARSER_HPP
#define ASSEMBLY_PARSER_HPP

#include "X86AssemblyAST.hpp"
#include "X86AssemblyLexer.hpp"
#include <cstdlib>
#include <iostream>

struct X86AssemblyParser {

	static std::vector<std::unique_ptr<X86AssemblyAST::Expression>> astRegisters;
	static std::vector<std::unique_ptr<X86AssemblyAST::RAM>> stackMemory;

	static void AddRegister(std::string name) {

		for(auto const& i : astRegisters) {

			if(i->name == name) {
				return;
			}
		}

		auto Reg = std::make_unique<X86AssemblyAST::Register>(name, std::make_unique<X86AssemblyAST::I32>());

		astRegisters.push_back(std::move(Reg));
	}

	static void AddStackMemory(std::string pointerName) {

		for(auto const& i : stackMemory) {

			if(i->pointerName == pointerName) {
				return;
			}
		}

		std::string stackName = std::string("stackMemory") + std::to_string(stackMemory.size());
		auto sMem = std::make_unique<X86AssemblyAST::RAM>(stackName, pointerName, std::make_unique<X86AssemblyAST::I32>());

		stackMemory.push_back(std::move(sMem));
	}

	static std::string GetStackMemoryName(std::string pointerName) {

		for(auto const& i : stackMemory) {

			if(i->pointerName == pointerName) {
				return i->name;
			}
		}

		return "";
	}

	static bool StackMemoryExists(std::string name) {

		for(auto const& i : stackMemory) {

			if(i->name == name) {
				return true;
			}
		}

		return false;
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseNumber() {

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::NumValString[0] == '$') {
			X86AssemblyLexer::NumValString.erase(0, 1);
		}

		if(X86AssemblyLexer::CurrentToken == '(') {

			X86AssemblyLexer::GetNextToken();

			std::string pointerName = X86AssemblyLexer::NumValString + "(%rbp)";

			if(X86AssemblyLexer::IdentifierStr == "%rbp") {
				AddStackMemory(pointerName);
			}

			X86AssemblyLexer::GetNextToken();

			if(X86AssemblyLexer::CurrentToken != ')') {

				std::cout << "Expected ')'.\n";
				exit(1);
				return nullptr;
			}

			X86AssemblyLexer::GetNextToken();

			return std::make_unique<X86AssemblyAST::Variable>(GetStackMemoryName(pointerName));
		}

		return std::make_unique<X86AssemblyAST::IntNumber>(std::stoi(X86AssemblyLexer::NumValString));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseFunction(std::string name) {

		X86AssemblyLexer::GetNextToken();

		std::vector<std::unique_ptr<X86AssemblyAST::Expression>> allInstructions;

		X86AssemblyAST::Attributes attrs;

		while(X86AssemblyLexer::CurrentToken != X86AssemblyToken::X86EndOfFile) {

			auto Expr = ParseExpression();

			if(X86AssemblyAST::IsSEH(Expr.get())) {
				attrs.isStackProtected = true;
			}
			else {
				allInstructions.push_back(std::move(Expr));
			}

			if(dynamic_cast<X86AssemblyAST::Return*>(Expr.get())) {
				break;
			}
		}

		return std::make_unique<X86AssemblyAST::Function>(name, attrs, std::move(allInstructions), std::move(astRegisters), std::move(stackMemory));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseIdentifier() {

		std::string idName = X86AssemblyLexer::IdentifierStr;

		if(idName[0] == '%') {
			idName.erase(0, 1);
			AddRegister(idName);
		}

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::CurrentToken == ':') {
			return ParseFunction(idName);
		}

		return std::make_unique<X86AssemblyAST::Variable>(idName);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseReturn() {

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::Return>();
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseAdd(std::string type) {

		X86AssemblyLexer::GetNextToken();

		auto One = ParseExpression();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in Assembly file.\n";
		}

		X86AssemblyLexer::GetNextToken();

		auto Two = ParseExpression();

		return std::make_unique<X86AssemblyAST::Add>(std::move(One), std::move(Two), type);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseSub(std::string type) {

		X86AssemblyLexer::GetNextToken();

		auto One = ParseExpression();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in Assembly file.\n";
		}

		X86AssemblyLexer::GetNextToken();

		auto Two = ParseExpression();

		return std::make_unique<X86AssemblyAST::Sub>(std::move(One), std::move(Two), type);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseMov(std::string type) {

		X86AssemblyLexer::GetNextToken();

		auto One = ParseExpression();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in Assembly file.\n";
		}

		X86AssemblyLexer::GetNextToken();

		auto Two = ParseExpression();

		bool isMem = StackMemoryExists(Two->name);

		return std::make_unique<X86AssemblyAST::Mov>(std::move(One), std::move(Two), type, isMem);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseLea(std::string type) {

		X86AssemblyLexer::GetNextToken();

		auto One = ParseExpression();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in Assembly file.\n";
		}

		X86AssemblyLexer::GetNextToken();

		auto Two = ParseExpression();

		return std::make_unique<X86AssemblyAST::Lea>(std::move(One), std::move(Two), type);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParsePush(std::string type) {

		X86AssemblyLexer::GetNextToken();

		auto Expr = ParseExpression();

		return std::make_unique<X86AssemblyAST::Push>(std::move(Expr), type);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParsePop(std::string type) {

		X86AssemblyLexer::GetNextToken();

		auto Expr = ParseExpression();

		return std::make_unique<X86AssemblyAST::Pop>(std::move(Expr), type);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseCall(std::string type) {

		X86AssemblyLexer::GetNextToken();

		auto Expr = ParseExpression();

		return std::make_unique<X86AssemblyAST::Call>(std::move(Expr), type);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseText() {

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::Comment>("[Assembly] Begin of .text section.");
	}

	static int GetDefScl() {

		if(X86AssemblyLexer::IdentifierStr != ".scl") {
			std::cout << "Expected '.scl'.\n";
			exit(1);
			return 0;
		}

		X86AssemblyLexer::GetNextToken();

		int sclNum = std::stoi(X86AssemblyLexer::NumValString);

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::CurrentToken != ';') {
			std::cout << "Expected ';' to end '.scl'.\n";
			exit(1);
			return 0;
		}

		X86AssemblyLexer::GetNextToken();

		return sclNum;
	}

	static int GetDefType() {

		if(X86AssemblyLexer::IdentifierStr != ".type") {
			std::cout << "Expected '.type'.\n";
			exit(1);
			return 0;
		}

		X86AssemblyLexer::GetNextToken();

		int typeNum = std::stoi(X86AssemblyLexer::NumValString);

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::CurrentToken != ';') {
			std::cout << "Expected ';' to end '.type'.\n";
			exit(1);
			return 0;
		}

		X86AssemblyLexer::GetNextToken();

		return typeNum;
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseDef() {

		X86AssemblyLexer::GetNextToken();

		std::string defineName = X86AssemblyLexer::IdentifierStr;

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::CurrentToken != ';') {
			std::cout << "Expected ';'.\n";
			exit(1);
			return nullptr;
		}

		X86AssemblyLexer::GetNextToken();

		int scl = GetDefScl();
		int ty = GetDefType();

		if(X86AssemblyLexer::IdentifierStr != ".endef") {
			std::cout << "Expected '.endef' to close Assembly definition.\n";
			exit(1);
			return nullptr;
		}

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::Def>(defineName, scl, ty);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseGlobl() {

		X86AssemblyLexer::GetNextToken();

		std::string defName = X86AssemblyLexer::IdentifierStr;

		X86AssemblyLexer::GetNextToken();

		std::string msg = "[Assembly] Set definition '";
		msg += defName;
		msg += "' as Global for the Linker.";

		return std::make_unique<X86AssemblyAST::Comment>(msg);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseSet() {

		X86AssemblyLexer::GetNextToken();

		std::string defName = X86AssemblyLexer::IdentifierStr;

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in '.set'\n";
			exit(1);
			return nullptr;
		}

		X86AssemblyLexer::GetNextToken();

		auto Expr = ParseExpression();

		return std::make_unique<X86AssemblyAST::Set>(defName, std::move(Expr));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseFile() {

		X86AssemblyLexer::GetNextToken();

		std::string fileName = X86AssemblyLexer::StringString;

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::File>(fileName);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseP2Align() {

		X86AssemblyLexer::GetNextToken();

		int bytes = std::stoi(X86AssemblyLexer::NumValString);

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' to split p2align/align arguments.\n";
			exit(1);
			return nullptr;
		}

		X86AssemblyLexer::GetNextToken();

		int limit = std::stoi(X86AssemblyLexer::NumValString);

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::P2Align>(bytes, limit);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseSEH() {

		X86AssemblyLexer::GetNextToken();

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::EnableSEH>();
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseSEHEnd() {

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::EnableSEH>();
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseSEHSetFrame() {

		// Skip '.seh_setframe'
		X86AssemblyLexer::GetNextToken();

		auto Reg = ParseExpression();

		// Check for the comma just in case.
		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in '.seh_setframe'.\n";
			exit(1);
			return nullptr;
		}

		// Skip the comma if it exists.
		X86AssemblyLexer::GetNextToken();

		auto Bytes = ParseExpression();

		return std::make_unique<X86AssemblyAST::EnableSEH>();
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseExpression() {

		if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Identifier) { return ParseIdentifier(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Return) { return ParseReturn(); }

		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86AddQ) { return ParseAdd("Q"); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86SubQ) { return ParseSub("Q"); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86LeaQ) { return ParseLea("Q"); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86PushQ) { return ParsePush("Q"); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86PopQ) { return ParsePop("Q"); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86CallQ) { return ParseCall("Q"); }

		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86AddL) { return ParseAdd("L"); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86MovL) { return ParseMov("L"); }

		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Text) { return ParseText(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Def) { return ParseDef(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Globl) { return ParseGlobl(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Set) { return ParseSet(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86File) { return ParseFile(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86P2Align) { return ParseP2Align(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Number) { return ParseNumber(); }

		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86SEH) { return ParseSEH(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86SEHEnd) { return ParseSEHEnd(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86SEHSetFrame) { return ParseSEHSetFrame(); }

		std::cout << "Unknown token or identifier found.\n";
		exit(1);

		return nullptr;
	}

	static std::string HandleExpression() {

		auto Expr = ParseExpression();

		std::string res = "";

		if(Expr != nullptr) {
			res += Expr->codegen();
			res += "\n";
		}

		return res;
	}

	static std::string MainLoop() {

		std::string result;

		X86AssemblyLexer::GetNextToken();

		while(X86AssemblyLexer::CurrentToken != X86AssemblyToken::X86EndOfFile) {

			if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86EndOfFile) { break; }

			result += HandleExpression();
		}

		return result;
	}
};

#endif