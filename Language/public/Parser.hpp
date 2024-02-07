
#pragma once

#include "AST.hpp"
#include "RegularCompiler.hpp"
#include "ContextFreeCompiler.hpp"

namespace tp {

	template <typename tAlphabetType, typename TokenType>
	class Parser {

		typedef RegularAutomata<tAlphabetType, TokenType, TokenType::InTransition, TokenType::Failed> RegularTable;
		typedef FiniteStateAutomation<tAlphabetType, TokenType> RegularGraph;
		typedef RegularCompiler<tAlphabetType, TokenType, TokenType::InTransition, TokenType::Failed> RegularCompiler;
		typedef FiniteStateAutomation<tAlphabetType, TokenType> RegularNonDetGraph;

		// ContextFreeCompiler::Alphabet PushDownTable;
		typedef FiniteStateAutomation<ContextFreeCompiler::Alphabet, ContextFreeCompiler::Item> ContextFreeDFA;
		typedef ContextFreeCompiler::NFA ContextFreeNFA;

	public:
		Parser() = default;

	public:
		void compileTables(const ContextFreeGrammar& cfGrammar, const RegularGrammar<tAlphabetType, TokenType>& reGrammar) {
			// Compile Regular Grammar
			{
				RegularNonDetGraph nfa;
				RegularCompiler compiler;
				compiler.compile(nfa, reGrammar);

				RegularGraph dfa(nfa);
				mRegularTable.construct(dfa);
			}

			// compile context free grammar
			{
				ContextFreeNFA nfa;
				ContextFreeCompiler compiler;
				compiler.compile(cfGrammar, nfa);

				ContextFreeDFA dfa(nfa);
			}
		}

		void parse(const tAlphabetType* sentence, ualni sentenceLength, AST& out) {}

	public:
		// save load compiled tables
		RegularTable mRegularTable;
	};
}
