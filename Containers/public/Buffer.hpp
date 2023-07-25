
#pragma once

#include "ContainersCommon.hpp"

namespace tp {

	template<ualni tScale = 2>
	inline ualni BufferResizeScaling(ualni const size) { return size * tScale; }

	template<ualni tScale = 2>
	inline ualni BufferResizeScalingDown(ualni const size) { return size / tScale; }

	template<ualni tAddition = 1>
	inline ualni BufferResizeAddition(ualni const size) { return size + tAddition; }

	template<ualni tAddition = 1>
	inline ualni BufferResizeAdditionDown(ualni const size) { return size - tAddition; }


	template<typename tType, ualni tSize>
	class ConstSizeBuffer {
		typedef SelCopyArg<tType> Arg;

	private:
		tType mBuff[tSize];
		ualni mLoad = 0;

	public:
		ConstSizeBuffer() = default;

	public:
		[[nodiscard]] ualni size() const { return mLoad; }
		[[nodiscard]] ualni getBuffSize() const { return tSize; }

		tType& first() {
			DEBUG_ASSERT(mLoad)
			return *mBuff;
		}

		const tType& first() const {
			DEBUG_ASSERT(mLoad)
			return *mBuff;
		}

		tType& last() {
			DEBUG_ASSERT(mLoad)
			return mBuff[mLoad - 1];
		}

		const tType& last() const {
			DEBUG_ASSERT(mLoad)
			return mBuff[mLoad - 1];
		}

		tType& operator[](ualni aIdx) {
			DEBUG_ASSERT(aIdx >= 0 && aIdx < mLoad)
			return mBuff[aIdx];
		}

		const tType& operator[](ualni aIdx) const {
			DEBUG_ASSERT(aIdx >= 0 && aIdx < mLoad)
			return mBuff[aIdx];
		}

		void append(Arg data) {
			ASSERT(mLoad != tSize)
			new (&mBuff[mLoad]) tType(data);
			mLoad++;
		}

		void pop() {
			DEBUG_ASSERT(mLoad)
			mBuff[mLoad].~tType();
			mLoad--;
		}

		ConstSizeBuffer& operator=(const tp::init_list<tType>& init) {
			for (auto arg : init) {
				append(arg);
			}
			return *this;
		}

	public:

		class IteratorPointer {
		protected:
			tType* mIter;
		public:
			IteratorPointer() = default;
			tType& operator->() { return *mIter; }
			const tType& operator->() const { return *mIter; }
		};

		class IteratorReference {
		protected:
			tType* mIter;
		public:
			IteratorReference() = default;
			tType* operator->() { return mIter; }
			const tType* operator->() const { return mIter; }
		};

		class Iterator : public TypeSelect<TypeTraits<tType>::isPointer, IteratorPointer, IteratorReference>::Result {
		public:

			explicit Iterator(tType* iter) { this->mIter = iter; }

			const Iterator& operator*() const { return *this; }

			Iterator& operator++() {
				this->mIter++;
				return *this;
			}

			bool operator==(const Iterator& left) const { return left.mIter == this->mIter; }
			bool operator!=(const Iterator& left) const { return left.mIter != this->mIter; }
		};

		[[nodiscard]] Iterator begin() const {
			return Iterator(mBuff);
		}

		[[nodiscard]] Iterator end() const {
			return Iterator(mBuff + mLoad);
		}
	};

	template<
		typename tType,
		class tAllocator = DefaultAllocator,
		ualni (tResizePolicy)(ualni) = BufferResizeScaling<2>,
		ualni (tResizePolicyDown)(ualni) = BufferResizeScalingDown<2>,
		ualni tMinSize = 4
	>
	class Buffer {
		typedef SelCopyArg<tType> Arg;

	private:
		tType* mBuff;
		tAllocator mAllocator;
		ualni mSize;
		ualni mLoad;

	public:
		Buffer() : mSize(tMinSize), mLoad(0) {
			mBuff = (tType*) mAllocator.allocate(sizeof(tType) * tMinSize);
		}

		explicit Buffer(ualni size) : mSize(size), mLoad(0) {
			mBuff = (tType*) mAllocator.allocate(sizeof(tType) * size);
		}

		Buffer(const Buffer& in) : mSize(in.mSize), mLoad(in.mLoad) {
			mBuff = (tType*) mAllocator.allocate(sizeof(tType) * mSize);
			for (ualni i = 0; i < mLoad; i++) new (&mBuff[i]) tType(in.mBuff[i]);
		}

		Buffer& operator=(const Buffer& in) {
			if (this == &in) return *this;
			this->~Buffer();
			new (this) Buffer(in);
			return *this;
		}

		~Buffer() {
			for (ualni i = 0; i < mLoad; i++) mBuff[i].~tType();
			mAllocator.deallocate(mBuff);
		}

	public:
		[[nodiscard]] ualni size() const {
			return mLoad; 
		}

		[[nodiscard]] ualni getBuffSize() const {
			return mSize; 
		}
		
		tType& first() {
      DEBUG_ASSERT(mLoad)
			return *mBuff; 
		}

		const tType& first() const {
      DEBUG_ASSERT(mLoad)
			return *mBuff; 
		}

		tType& last() {
      DEBUG_ASSERT(mLoad)
			return mBuff[mLoad - 1]; 
		}

		const tType& last() const {
      DEBUG_ASSERT(mLoad)
			return mBuff[mLoad - 1]; 
		}

		tType& operator[](ualni idx) {
			DEBUG_ASSERT(idx < mLoad && idx >= 0)
			return mBuff[idx];
		}

		const tType& operator[](ualni idx) const {
			DEBUG_ASSERT(idx < mLoad && idx >= 0)
			return mBuff[idx];
		}

	public:
		bool operator==(const Buffer& in) const {
			if (this == &in) return true;
			if (mLoad != in.mLoad) return false;
			for (ualni i = 0; i < mLoad; i++) {
				if (mBuff[i] != in.mBuff[i]) return false;
			}
			return true;
		}

	public:
		Buffer& operator=(const tp::init_list<tType>& init) {
			// TODO : optimize
			for (auto arg : init) {
				append(arg);
			}
			return *this;
		}

		void append(Arg data) {
			if (mLoad == mSize) resizeBuffer(tResizePolicy(mSize));
			new (&mBuff[mLoad]) tType(data);
			mLoad++;
		}

    void append(const Buffer& in) {
      if (!in.mLoad) return;
      auto newLoad = mLoad + in.mLoad;
      auto newSize = mSize;
      while (newLoad >= newSize) newSize = tResizePolicy(newSize);
      if (newSize != mSize) resizeBuffer(newSize);
      for (auto i = mLoad; i < newLoad; i++) new (&mBuff[i]) tType(in.mBuff[i - mLoad]);
      mLoad = newLoad;
    }

		void pop() {
			DEBUG_ASSERT(mLoad)
			mBuff[mLoad].~tType();
			mLoad--;
			ualni prevSize = tResizePolicyDown(mSize);
      DEBUG_ASSERT(prevSize < mSize)
			if (prevSize > mLoad) resizeBuffer(prevSize);
		}

		void reserve(ualni aSize) {
			for (ualni i = 0; i < mLoad; i++) mBuff[i].~tType();
			mBuff = (tType*) mAllocator.allocate(sizeof(tType) * aSize);
			mSize = aSize;
			mLoad = aSize;
			for (ualni i = 0; i < mLoad; i++) new (mBuff + i) tType();
		}

	public:
		class IteratorPointer {
		protected:
			tType* mIter;
		public:
			IteratorPointer() = default;
			tType& operator->() { return *mIter; }
			const tType& operator->() const { return *mIter; }
		};

		class IteratorReference {
		protected:
			tType* mIter;
		public:
			IteratorReference() = default;
			tType* operator->() { return mIter; }
			const tType* operator->() const { return mIter; }
		};

		class Iterator : public TypeSelect<TypeTraits<tType>::isPointer, IteratorPointer, IteratorReference>::Result {
		public:

			explicit Iterator(tType* iter) { this->mIter = iter; }

			const Iterator& operator*() const { return *this; }

			tType& data() {
				return *this->mIter;
			}

			Iterator& operator++() {
				this->mIter++;
				return *this;
			}

			bool operator==(const Iterator& left) const { return left.mIter == this->mIter; }
			bool operator!=(const Iterator& left) const { return left.mIter != this->mIter; }
		};

		[[nodiscard]] Iterator begin() const {
			return Iterator(mBuff);
		}

		[[nodiscard]] Iterator end() const {
			return Iterator(mBuff + mLoad);
		}

  private:
    void resizeBuffer(ualni newSize) {
      DEBUG_ASSERT(newSize >= mLoad)
      auto const oldBuff = mBuff;
      mBuff = (tType*) mAllocator.allocate(sizeof(tType) * newSize);
      for (ualni i = 0; i < mLoad; i++) new (&mBuff[i]) tType(oldBuff[i]);
      for (ualni i = 0; i < mLoad; i++) oldBuff[i].~tType();
      mAllocator.deallocate(oldBuff);
      mSize = newSize;
    }
	};


	template<typename tType>
	void generatePermutations(const Buffer<Buffer<tType>>& in, Buffer<Buffer<tType>>& out) {
		typedef long long Idx;

		// sanity check
		for (const auto & vec : in) {
			if (!vec.size()) return;
		}

		out.resize(in.size());
		auto len = Idx(1);
		for (const auto & vec : in) len *= (Idx) vec.size();
		for (auto i = 0; i < in.size(); i++) out[i].resize(len);

		auto dub = Idx(1);
		for (auto power = (Idx) in.size() - 1; power >= 0; power--) {
			auto& vec = in[power];
			long long i = 0;
			while (i < len) {
				for (auto val : vec) {
					for (auto j = 0; j < dub; j++) {
						out[power][i] = val;
						i++;
					}
				}
			}
			dub *= (Idx) vec.size();
		}
	}

}