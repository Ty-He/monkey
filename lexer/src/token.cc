#include <unordered_map>
#include "lexer/token.hpp"

namespace token {

// map: code token -> TokenType
std::unordered_map<std::string, TokenType> keywords{
	{"fn", FUNCTION},
	{"let", LET},
	{"true", TRUE},
	{"false", FALSE},
	{"if", IF},
	{"else", ELSE},
	{"return", RETURN}
};

TokenType lookup_ident(std::string ident)
{
	if (keywords.contains(ident)) {
		return keywords[ident];
	}
	return IDENT;
}

}
