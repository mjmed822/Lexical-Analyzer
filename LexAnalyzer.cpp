/*
* Lexical Analyzer
* Matthew Medina
*
* CS280001
* Fall 2020
*/
#include <iostream>
#include <regex>
#include <map>
#include "lex.h"

static std::map<Token, std::string> tokenPrint{
	{ PRINT, "PRINT" }, { IF, "IF" },
	{ BEGIN, "BEGIN" }, { END, "END" },
	{ THEN, "THEN" }, { IDENT, "IDENT" },
	{ ICONST, "ICONST" }, { SCONST, "SCONST" },
	{ RCONST, "RCONST" }, { PLUS, "PLUS" },
	{ MINUS, "MINUS" }, { MULT, "MULT" },
	{ DIV, "DIV" }, { EQ, "EQ" },
	{ LPAREN, "LPAREN" }, { RPAREN, "RPAREN" },
	{ SCOMA, "SCOMA" }, { COMA, "COMA" },
	{ ERR, "ERR" }, { DONE, "DONE" }
};


ostream& operator<<(ostream& out, const LexItem& tok) {
	std::string *token = &tokenPrint[tok.GetToken()];
	std::cout << *token;

	bool eval = (tok.GetToken() == SCONST) || (tok.GetToken() == RCONST) ||
		(tok.GetToken() == ICONST) || (tok.GetToken() == IDENT) ||
		(tok.GetToken() == ERR);

	if (eval)
		std::cout << " (" << tok.GetLexeme() << ")";
	return out;
}

LexItem currentToken;
LexItem previousToken;

LexItem getNextToken(istream& in, int& linenum) {
	enum TokenState { START, INID, INSTRING, ININT, INREAL, INCOMMENT, SIGN } lexstate = START;
	std::string lexeme;
	char ch;
	char nextChar;

	// Search until a token is found or eof is reached.
	while (in.get(ch)) {
		switch (lexstate) {
			// Basic state of searching for a token
		case START:
			if (ch == '\n')
				linenum++;

			// If eof is found finish searching
			if (in.peek() == -1) {
				if (previousToken.GetToken() != END)
					return LexItem(ERR, "No END Token", previousToken.GetLinenum());
				return LexItem(DONE, lexeme, linenum);
			}

			// Spaces are meaningless for token analysis and are skipped
			if (isspace(ch))
				continue;

			lexeme = ch;

			// Check for comment with //
			if (ch == '/' && char(in.peek()) == '/') {
				lexstate = INCOMMENT;
				continue;
			}

			// Check for signs
			if (ch == '+' || ch == '-' || ch == '*' ||
				ch == '/' || ch == '(' || ch == ')' ||
				ch == '=' || ch == ',' || ch == ';') {
				lexstate = SIGN;
				continue;
			}

			// Check for string
			if (ch == '\"') {
				lexstate = INSTRING;
				continue;
			}

			// Check for ints
			if (isdigit(ch)) {
				lexstate = ININT;
				continue;
			}

			// Check for reals
			if (ch == '.') {
				lexstate = INREAL;
				continue;
			}

			// Check for identifiers
			if (isalpha(ch)) {
				lexstate = INID;
				continue;
			}

			// If a character cannot be classified into a state it must be an error
			return LexItem(ERR, lexeme, linenum);

		case INID:
			// Regex is used to match strings to proper formatting
			if (regex_match(lexeme + ch, std::regex("[a-zA-Z][a-zA-Z0-9]*")))
				lexeme += ch;
			if (in.peek() == -1 || !regex_match(lexeme + ch, std::regex("[a-zA-Z][a-zA-Z0-9]*"))) {
				lexstate = START;
				in.putback(ch);

				// Check for reserved keywords as identifiers
				if (lexeme == "begin") {
					if (previousToken.GetToken() != ERR)
						return LexItem(ERR, lexeme, linenum);
					currentToken = LexItem(BEGIN, lexeme, linenum);
				}
				else if (lexeme == "print")
					currentToken = LexItem(PRINT, lexeme, linenum);
				else if (lexeme == "end") {
					if (previousToken.GetToken() != SCOMA)
						return LexItem(ERR, previousToken.GetLexeme(), linenum);
					currentToken = LexItem(END, lexeme, linenum);
				}
				else if (lexeme == "if")
					currentToken = LexItem(IF, lexeme, linenum);
				else if (lexeme == "then")
					currentToken = LexItem(THEN, lexeme, linenum);
				else {
					if (previousToken.GetToken() == IDENT)
						return LexItem(ERR, lexeme, linenum);
					currentToken = LexItem(IDENT, lexeme, linenum);
				}

				// Check for no begin token
				if (currentToken != BEGIN && previousToken == ERR)
					return LexItem(ERR, "No BEGIN Token", currentToken.GetLinenum());
				previousToken = currentToken;
				return currentToken;
			}
			break;

		case INSTRING:
			// Check for no begin token
			if (previousToken == ERR)
				return LexItem(ERR, "No Begin Token", linenum);
			// String cannot contain multiple lines, must be an error
			if (ch == 10)
				return LexItem(ERR, lexeme, linenum);

			// Check lexeme for unfished string
			if (regex_match(lexeme + ch, std::regex("\"[ -~]*"))) {
				if (ch == '\\' && in.peek() == '\"') {
					lexeme += ch;
					in.get(ch);
					lexeme += ch;
					continue;
				}
				else
					lexeme += ch;
			}

			// Check lexeme for finished string
			if (regex_match(lexeme + ch, std::regex("\"[ -~]*\""))) {
				lexstate = START;
				currentToken = LexItem(SCONST, lexeme, linenum);
				previousToken = currentToken;
				return currentToken;
			}
			break;

		case ININT:
			// Check for no begin token
			if (previousToken == ERR)
				return LexItem(ERR, "No Begin Token", linenum);
			// Checks if an alpha ch is next to an integer number
			if (isalpha(ch))
				return LexItem(ERR, lexeme + ch, linenum);
			if (regex_match(lexeme + ch, std::regex("[0-9]+"))) {
				lexeme += ch;
			}
			// If a period is found then the int is actual a real number
			else if (ch == '.') {
				lexstate = INREAL;
				in.putback(ch);
				continue;
			}
			else {
				lexstate = START;
				in.putback(ch);
				currentToken = LexItem(ICONST, lexeme, linenum);
				previousToken = currentToken;
				return currentToken;
			}
			break;

		case INREAL:
			// Check for no begin token
			if (previousToken == ERR)
				return LexItem(ERR, "No Begin Token", linenum);
			// Checks if an alpha ch is next to a real number
			if (isalpha(ch))
				return LexItem(ERR, lexeme + ch, linenum);
			if (regex_match(lexeme + ch, std::regex("[0-9]*\\.[0-9]+"))) {
				lexeme += ch;
			}
			else if (regex_match(lexeme + ch, std::regex("[0-9]*\\.[0-9]*"))) {
				lexeme += ch;
			}
			else {
				if (lexeme[lexeme.length() - 1] == '.')
					return LexItem(ERR, lexeme, linenum);
				lexstate = START;
				in.putback(ch);
				currentToken = LexItem(RCONST, lexeme, linenum);
				previousToken = currentToken;
				return currentToken;
			}
			break;

		case INCOMMENT:
			// Because comment is not a token it can be ignored
			if (ch == '\n') {
				linenum++;
				lexstate = START;
			}
			continue;

		case SIGN:
			// Check for no begin token
			if (previousToken == ERR)
				return LexItem(ERR, "No Begin Token", linenum);
			if (lexeme == "+" || lexeme == "*" || lexeme == "/") {
				Token token = previousToken.GetToken();
				if (token == IDENT || token == ICONST || token == RCONST) {
					lexstate = START;
					in.putback(ch);
					if (lexeme == "+")
						currentToken = LexItem(PLUS, lexeme, linenum);
					else if (lexeme == "*")
						currentToken = LexItem(MULT, lexeme, linenum);
					else
						currentToken = LexItem(DIV, lexeme, linenum);
					previousToken = currentToken;
					return currentToken;
				}
				else
					return LexItem(ERR, lexeme + ch, linenum);
			}
			if (lexeme == "-") {
				Token token = previousToken.GetToken();
				if (token == IDENT || token == ICONST || token == RCONST || token == EQ) {
					lexstate = START;
					in.putback(ch);
					currentToken = LexItem(MINUS, lexeme, linenum);
					previousToken = currentToken;
					return currentToken;
				}
				else
					return LexItem(ERR, lexeme + ch, linenum);
			}
			if (lexeme == "(") {
				Token token = previousToken.GetToken();
				if (token == IF || token == EQ || token == PLUS || token == MINUS ||
					token == MULT || token == DIV) {
					lexstate = START;
					in.putback(ch);
					currentToken = LexItem(LPAREN, lexeme, linenum);
					previousToken = currentToken;
					return currentToken;
				}
				else
					return LexItem(ERR, lexeme + ch, linenum);
			}
			if (lexeme == ")") {
				Token token = previousToken.GetToken();
				if (token == ICONST || token == RCONST || token == IDENT) {
					lexstate = START;
					in.putback(ch);
					currentToken = LexItem(RPAREN, lexeme, linenum);
					previousToken = currentToken;
					return currentToken;
				}
				else
					return LexItem(ERR, lexeme + ch, linenum);
			}
			if (lexeme == "=") {
				Token token = previousToken.GetToken();
				if (token == IDENT) {
					lexstate = START;
					in.putback(ch);
					currentToken = LexItem(EQ, lexeme, linenum);
					previousToken = currentToken;
					return currentToken;
				}
				else
					return LexItem(ERR, lexeme + ch, linenum);
			}
			if (lexeme == ",") {
				Token token = previousToken.GetToken();
				if (token == SCONST) {
					lexstate = START;
					in.putback(ch);
					currentToken = LexItem(COMA, lexeme, linenum);
					previousToken = currentToken;
					return currentToken;
				}
				else
					return LexItem(ERR, lexeme + ch, linenum);
			}
			if (lexeme == ";") {
				Token token = previousToken.GetToken();
				if (token == SCONST || token == ICONST || token == RCONST || token == IDENT) {
					lexstate = START;
					in.putback(ch);
					currentToken = LexItem(SCOMA, lexeme, linenum);
					previousToken = currentToken;
					return currentToken;
				}
				else
					return LexItem(ERR, lexeme + ch, linenum);
			}
			break;
		}
	}
	return LexItem(DONE, "", linenum);
}