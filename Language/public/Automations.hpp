#pragma once

/*
 * General-purpose automations.
 * Automations must have ability to be saved and loaded to reduce startup of a program that uses it.
 * */

#include "Common.hpp"
#include "TypeInfo.hpp"
#include "Buffer.hpp"
#include "List.hpp"

namespace tp {

	// Finite-State automation
	template<typename tState, typename tAlphabet>
	class FSAutomation {
	public:
		FSAutomation() = default;
		~FSAutomation() = default;

		[[nodiscard]] bool isDeterministic() const { return false; }
		void makeDeterministic() {}

	private:
		List<tState*> mStates;
	};

	// Push-Down Finite-State automation
	template<typename tState, typename tAlphabet>
	class PDFSAutomation {
		typedef FSAutomation<tState, tAlphabet> tAutomation;
	private:
		tAutomation mAutomation;
	};
}