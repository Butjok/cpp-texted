#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <optional>

struct Token {
	int type = EOF;
	std::string_view text = std::string_view();
};
struct Lexer {
	Token currentToken = Token();
	std::vector<std::string>* lines = nullptr;
	std::string sourceCode = "";
	int position = 0;

	virtual ~Lexer() = default;
	virtual void Reset();
	virtual void NextToken() {}
	virtual const char *TokenTypeName(int type) const { return "Unknown"; }
};

struct CppLexer : public Lexer {

	void NextToken() override;
	const char *TokenTypeName(int type) const override;

	bool TryConsumeWhitespace();
	bool TryConsumeIdentifierOrKeyword();
	bool TryConsumeLineComment();
	bool TryConsumeBlockComment();
	bool TryConsumeDirective();
	bool TryConsumeString();
	bool TryConsumeCharLiteral();
};
