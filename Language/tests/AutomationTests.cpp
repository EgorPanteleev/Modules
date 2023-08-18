
#include "NewPlacement.hpp"

#include "Automations.hpp"
#include "Buffer.hpp"
#include "Testing.hpp"

using namespace tp;

template<typename tAlphabet, typename tTransition>
class State {
public:
	class Transition {
		State* state;
	public:
		Transition() = default;
		virtual ~Transition() = default;

		virtual bool isTransition(tAlphabet symbol) = 0;
	};

public:
	State() = default;
	virtual ~State() = default;

	virtual bool isAccepting() = 0;

private:
	Buffer<tTransition> transitions;
};

TEST_DEF_STATIC(Simple) {



}

TEST_DEF() {
	testSimple();
}