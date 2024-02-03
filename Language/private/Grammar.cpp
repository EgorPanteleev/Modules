
#include "Grammar.hpp"

using namespace tp;

ContextFreeGrammar::Arg::Arg(const String& id, bool terminal, bool epsilon) {
	mId = id;
	mIsTerminal = terminal;
	mIsEpsilon = epsilon;
}

const String& ContextFreeGrammar::Arg::getId() const { return mId; }

bool ContextFreeGrammar::Arg::operator==(const Arg& in) const {
	return (mId == in.mId) && (mIsEpsilon == in.mIsEpsilon) && (mIsTerminal == in.mIsTerminal);
}

ContextFreeGrammar::Rule::Rule(const String& id, const InitialierList<Arg>& args) {
	mId = id;
	mArgs = args;
}

bool ContextFreeGrammar::Rule::operator==(const Rule& in) const { return (mId == in.mId) && (mArgs == in.mArgs); }

bool ContextFreeGrammar::Rule::isProductive() const {
	for (auto arg : mArgs) {
		if (arg->getId() == mId) return false;
	}
	return true;
}

bool ContextFreeGrammar::NonTerminal::isProductive() const {
	for (auto rule : rules) {
		if (rule->isProductive()) return true;
	}
	return false;
}

bool ContextFreeGrammar::NonTerminal::isLooped(Map<String, ualni>& processed, const String& id) const {
	for (auto ref : referencing) {
		if (processed.presents(ref->key)) return true;
	}
	processed.put(id, {});
	for (auto ref : referencing) {
		if (ref->val->isLooped(processed, ref->key)) return true;
	}
	return false;
}

void ContextFreeGrammar::addRule(const Rule& rule) { mRules.append(rule); }

void ContextFreeGrammar::addRule(const String& id, const InitialierList<Arg>& args) { addRule(Rule(id, args)); }

void ContextFreeGrammar::setStart(const String& startRule) { mStartTerminal = startRule; }