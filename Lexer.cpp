#include <set>
#include <fstream>
#include <sstream>
#include <cassert>
#include "Lexer.h"

using namespace std;

enum {
	Invalid,
	Identifier,
	Keyword,
	Literal,
	Operator,
	Punctuation,
	Whitespace,
	LineComment,
	BlockComment,
	Directive,
	StringLiteral,
	CharLiteral,
};

const char *CppLexer::TokenTypeName(int type) const {
	switch (type) {
		case Invalid: return "Invalid";
		case Identifier: return "Identifier";
		case Keyword: return "Keyword";
		case Literal: return "Literal";
		case Operator: return "Operator";
		case Punctuation: return "Punctuation";
		case Whitespace: return "Whitespace";
		case LineComment: return "LineComment";
		case BlockComment: return "BlockComment";
		case Directive: return "Directive";
		case StringLiteral: return "StringLiteral";
		case CharLiteral: return "CharLiteral";
		case EOF: return "EOF";
		default: return "Unknown";
	}
}

void Lexer::Reset() {
	position = 0;
	sourceCode.clear();
	if (lines) {
		auto totalSize = 0;
		for (const auto& line: *lines)
			totalSize += line.size() + 1;
		sourceCode.reserve(totalSize);
		for (const auto& line: *lines) {
			sourceCode.append(line);
			sourceCode.push_back('\n'); // add newline character
		}
	}
	currentToken = Token();
}

void CppLexer::NextToken() {
	if (position >= sourceCode.size()) {
		currentToken.type = EOF;
		currentToken.text = std::string_view();
		return;
	}
	if (TryConsumeLineComment() ||
		TryConsumeBlockComment() ||
		TryConsumeWhitespace() ||
		TryConsumeIdentifierOrKeyword() ||
		TryConsumeDirective() ||
		TryConsumeString() ||
		TryConsumeCharLiteral())
		return;

	// others are invalid tokens
	currentToken.type = Invalid;
	currentToken.text = std::string_view(&sourceCode[position], 1);
	position++;
}

bool CppLexer::TryConsumeWhitespace() {
	auto startPosition = position;
	while (position < sourceCode.size() && isspace(sourceCode[position]))
		position++;
	if (startPosition < position) {
		currentToken.type = Whitespace;
		currentToken.text = std::string_view(&sourceCode[startPosition], position - startPosition);
		return true;
	}
	return false;
}

static const std::set<std::string_view> keywords = {
	"if", "else", "while", "for", "return", "int", "float", "double", "char", "void",
	"public", "private", "protected", "class", "struct", "namespace", "using",
	"static", "const", "volatile", "enum", "bool", "auto",
	"true","false","nullptr","nullopt"
};

bool CppLexer::TryConsumeIdentifierOrKeyword() {
	auto startPosition = position;
	if (isalpha(sourceCode[position]) || sourceCode[position] == '_') {
		position++;
		while (position < sourceCode.size() && (isalnum(sourceCode[position]) || sourceCode[position] == '_'))
			position++;
		currentToken.text = std::string_view(&sourceCode[startPosition], position - startPosition);
		currentToken.type = keywords.find(currentToken.text) != keywords.end() ? Keyword : Identifier;
		return true;
	}
	return false;
}

bool CppLexer::TryConsumeLineComment() {
	if (position + 1 < sourceCode.size() && sourceCode[position] == '/' && sourceCode[position + 1] == '/') {
		auto startPosition = position;
		position += 2; // skip "//"
		while (position < sourceCode.size()) {
			auto c = sourceCode[position];
			position++;
			if (c == '\n')
				break;
		}
		currentToken.type = LineComment;
		currentToken.text = std::string_view(&sourceCode[startPosition], position - startPosition);
		return true;
	}
	return false;
}

bool CppLexer::TryConsumeBlockComment() {
	if (position + 1 < sourceCode.size() && sourceCode[position] == '/' && sourceCode[position + 1] == '*') {
		auto startPosition = position;
		position += 2; // skip "/*"
		while (position < sourceCode.size()) {
			if (sourceCode[position] == '*' && position + 1 < sourceCode.size() && sourceCode[position + 1] == '/') {
				position += 2; // skip "*/"
				currentToken.type = BlockComment;
				currentToken.text = std::string_view(&sourceCode[startPosition], position - startPosition);
				return true;
			}
			if (position + 1 >= sourceCode.size()) {
				currentToken.type = BlockComment;
				currentToken.text = std::string_view(&sourceCode[startPosition], position + 1 - startPosition);
				return true;
			}
			position++;
		}
	}
	return false;
}

bool CppLexer::TryConsumeDirective() {
	if (position < sourceCode.size() && sourceCode[position] == '#') {
		auto startPosition = position;
		position++; // skip '#'
		while (position < sourceCode.size() && isspace(sourceCode[position]))
			position++;
		while (position < sourceCode.size() && !isspace(sourceCode[position]) && sourceCode[position] != '\n')
			position++;
		currentToken.type = Directive;
		currentToken.text = std::string_view(&sourceCode[startPosition], position - startPosition);
		return true;
	}
	return false;
}

bool CppLexer::TryConsumeString() {
	if (position < sourceCode.size() && sourceCode[position] == '"') {
		auto startPosition = position;
		position++; // skip '"'
		while (position < sourceCode.size()) {
			if (sourceCode[position] == '"') {
				position++; // skip closing '"'
				currentToken.type = StringLiteral;
				currentToken.text = std::string_view(&sourceCode[startPosition], position - startPosition);
				return true;
			}
			if (sourceCode[position] == '\\') {
				position++; // skip escape character
			}
			position++;
		}
		currentToken.type = StringLiteral;
		currentToken.text = std::string_view(&sourceCode[startPosition], position - startPosition);
		return true;
	}
	return false;
}

bool CppLexer::TryConsumeCharLiteral() {
	if (position < sourceCode.size() && sourceCode[position] == '\'') {
		auto startPosition = position;
		position++; // skip '\''
		if (position < sourceCode.size()) {
			if (sourceCode[position] == '\\') {
				position++; // skip escape character
			}
			if (position < sourceCode.size()) {
				position++; // skip character
				if (position < sourceCode.size() && sourceCode[position] == '\'') {
					position++; // skip closing '\''
					currentToken.type = CharLiteral;
					currentToken.text = std::string_view(&sourceCode[startPosition], position - startPosition);
					return true;
				}
			}
		}
		currentToken.type = CharLiteral;
		currentToken.text = std::string_view(&sourceCode[startPosition], position - startPosition);
		return true;
	}
	return false;
}