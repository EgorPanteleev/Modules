
#include "Buffer.hpp"
#include "Utils.hpp"

namespace tp {

	template< typename tType, ualni tSizeX, ualni tSizeY >
	using ConstSizeBuffer2D = ConstSizeBuffer<ConstSizeBuffer<tType, tSizeX>, tSizeY>;

	template <
	    typename tType,
			class tAllocator = DefaultAllocator
			>
	class Buffer2D {
	public:
		typedef ualni Index;
		typedef Pair<Index, Index> Index2D;
		typedef SelCopyArg<tType> tTypeArg;

	private:
		tAllocator mAlloc;
		tType* mBuff = nullptr;
		Index2D mSize = { 0, 0 };

		void deleteBuffer() {
			if (!mBuff) return;
			for (ualni i = 0; i < mSize.x * mSize.y; i++) mBuff[i].~tType();
			mAlloc.deallocate(mBuff);
		}

		void allocateBuffer(Index2D size) {
			deleteBuffer();
			mBuff = (tType*) mAlloc.allocate(sizeof(tType) * size.x * size.y);
			for (ualni i = 0; i < mSize.x * mSize.y; i++) new (mBuff + i) tType();
		}

	public:

		Buffer2D() = default;

		~Buffer2D() {
			deleteBuffer();
		}

		explicit Buffer2D(Index2D aSize) {
			reserve(aSize);
		}

		[[nodiscard]] Index2D size() const {
			return { mSize.x, mSize.y };
		}

		tType* getBuff() const {
			return mBuff;
		}

		inline tType& get(const Index2D& at) {
			DEBUG_ASSERT(at.x < mSize.x && at.y < mSize.y && at.x >= 0 && at.y >= 0)
			return *(mBuff + mSize.x * at.y + at.x);
		}

		inline const tType& get(const Index2D& at) const {
			DEBUG_ASSERT(at.x < mSize.x && at.y < mSize.y && at.x >= 0 && at.y >= 0)
			return *(mBuff + mSize.x * at.y + at.x);
		}

		inline void set(const Index2D& at, tTypeArg value) {
			DEBUG_ASSERT(at.x < mSize.x && at.y < mSize.y && at.x >= 0 && at.y >= 0)
			*(mBuff + mSize.x * at.y + at.x) = value;
		}

		void reserve(const Index2D& newSize) {
			if (mSize.x != newSize.x || mSize.y != newSize.y) {
				allocateBuffer(newSize);
				mSize = newSize;
			}
		}

		void assign(tType value) {
			Index len = mSize.x * mSize.y;
			for (Index i = 0; i < len; i++) {
				mBuff[i] = value;
			}
		}
	};
}