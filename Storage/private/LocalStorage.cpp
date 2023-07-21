#include "LocalStorage.hpp"

#include "SystemAPI.hpp"

#include <cstdio>

using namespace tp;


bool FileLocation::exists() const {
	FILE* file = fopen(mLocation.read(), "r");
	if (file) {
		// File exists, close it and return 1
		fclose(file);
		return true;
	}
	return false;
}

bool File::connect(const FileLocation& location, const FileConnectionType& connectionInfo) {
	DEBUG_ASSERT(!mStatus.isOpened());
	if (mStatus.isOpened()) return false;

	auto handle = new FileSystemHandle();

	switch (connectionInfo.getType()) {
		case FileConnectionType::READ:
			handle->open(location.getLocation().read(), false);
			break;

		case FileConnectionType::WRITE:
			handle->open(location.getLocation().read(), true);
			break;

		default:
			break;
	};

	if (!handle->isOpen()) {
		mStatus.setStatus(FileConnectionStatus::DENIED);
		delete handle;
		return false;
	}

	mStatus.setStatus(FileConnectionStatus::OPENED);
	mHandle = handle;
	mConnectionType = connectionInfo;
	return true;
}

bool File::disconnect() {
	DEBUG_ASSERT(mStatus.isOpened());
	if (!mStatus.isOpened() || !mHandle) return false;
	mHandle->close();
	delete mHandle;
	mStatus.setStatus(FileConnectionStatus::CLOSED);
	return true;
}

bool File::setPointer(BytePointer pointer) {
	DEBUG_ASSERT(mStatus.isOpened());
	if (!mStatus.isOpened()) return false;
	return true;
}

bool File::readBytes(Byte* bytes, SizeBytes size) {
	DEBUG_ASSERT(mStatus.isOpened() && mConnectionType.isWrite());
	if (!mStatus.isOpened() || !mConnectionType.isWrite()) return false;

	mHandle->seekp(mPointer);
	mHandle->read(bytes, size);
	mPointer += size;
	return true;
}

bool File::writeBytes(const Byte* bytes, SizeBytes size) {
	DEBUG_ASSERT(mStatus.isOpened() && mConnectionType.isRead());
	if (!mStatus.isOpened() || !mConnectionType.isRead()) return false;

	mHandle->seekp(mPointer);
	mHandle->write(bytes, size);
	mPointer += size;
	return true;
}

File::SizeBytes File::size() {
	DEBUG_ASSERT(mStatus.isOpened());
	if (!mStatus.isOpened() || !mHandle) return 0;
	return mHandle->size();
}