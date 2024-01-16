#include "Archiver.hpp"
#include "Testing.hpp"
#include "Tests.hpp"

using namespace tp;

TEST_DEF_STATIC(SimpleReference) {
	tp::List<TestClass, tp::HeapAlloc> list = { TestClass(1), TestClass(2), TestClass(3), TestClass(4) };

	list.pushBack(TestClass(5));
	list.pushFront(TestClass(0));

	ualni i = -1;
	for (auto iter : list) {
		i++;
		TEST_EQUAL(iter->getVal(), i);
	}

	TEST(i == 5);

	list.removeAll();
}

TEST_DEF_STATIC(SimplePointer) {
	tp::List<TestClass*, tp::HeapAlloc> list = { new TestClass(1), new TestClass(2), new TestClass(3), new TestClass(4) };

	list.pushBack(new TestClass(5));
	list.pushFront(new TestClass(0));

	ualni i = -1;
	for (auto iter : list) {
		i++;
		TEST_EQUAL(iter->getVal(), i);
	}

	TEST(i == 5);

	for (auto iter : list) {
		delete iter.data();
	}

	list.removeAll();
}

TEST_DEF_STATIC(Copy) {
	tp::List<TestClass, tp::HeapAlloc> list = { TestClass(1), TestClass(2), TestClass(3), TestClass(4) };
	tp::List<TestClass, tp::HeapAlloc> list2 = list;

	TEST_EQUAL(list, list2);

	list.removeAll();
	list2.removeAll();
}

TEST_DEF_STATIC(Serialization) {
	tp::List<TestClass, tp::HeapAlloc> list = { TestClass(1), TestClass(2), TestClass(3), TestClass(4) };

	ArchiverExample<1024, false> write;
	ArchiverExample<1024, true> read;

	write % list;

	list.removeAll();

	memCopy(read.mBuff, write.mBuff, sizeof(write.mBuff));

	read % list;

	ualni i = 0;
	for (auto iter : list) {
		i++;
		TEST_EQUAL(iter->getVal(), i);
	}
	TEST(i == 4);

	list.removeAll();
}

TEST_DEF(List) {
	testSimplePointer();
	testSimpleReference();
	testSerialization();
}
