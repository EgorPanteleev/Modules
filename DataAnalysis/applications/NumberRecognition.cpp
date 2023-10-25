#include "FCNN.hpp"
#include "LocalConnection.hpp"
#include "NewPlacement.hpp"

using namespace tp;

struct Dataset {
	ualni length = 0;
	Pair<ualni, ualni> imageSize = { 0, 0 };
	Buffer<uint1> labels;
	Buffer<Buffer<uint1>> images;
};

bool loadDataset(Dataset& out, const String& location) {
	LocalConnection dataset;
	dataset.connect(LocalConnection::Location(location), LocalConnection::Type(true));

	if (!dataset.getConnectionStatus().isOpened()) {
		return false;
	}

	LocalConnection::Byte length;
	dataset.readBytes(&length, 1);

	LocalConnection::Byte sizeX;
	dataset.readBytes(&sizeX, 1);

	LocalConnection::Byte sizeY;
	dataset.readBytes(&sizeY, 1);

	out.length = ((ualni) length) * 1000;
	out.imageSize = { sizeX, sizeY };

	out.labels.reserve(out.length);
	out.images.reserve(out.length);

	for (auto i : Range(out.length)) {
		auto& image = out.images[i];
		image.reserve(sizeX * sizeY);
		dataset.readBytes((LocalConnection::Byte*) image.getBuff(), sizeX * sizeY);
	}

	LocalConnection::Byte label;
	dataset.readBytes((LocalConnection::Byte*) out.labels.getBuff(), out.length);

	return true;
}

struct NumberRec {
	NumberRec() {
		Buffer<halni> layers = { 784, 128, 10 };
		nn.initializeRandom(layers);

		Dataset dataset;

		if (!loadDataset(dataset, "rsc/mnist")) {
			printf("Cant Load Mnist Dataset\n");
			return;
		}

		mTestcases.reserve(dataset.images.size());
		for (auto i : Range(dataset.images.size())) {
			auto& image = dataset.images[i];
			auto label = dataset.labels[i];

			auto& testcase = mTestcases[i];

			testcase.output.reserve(10);

			for (auto dig : Range(10)) {
				testcase.output[dig] = label == dig ? 1 : 0;
			}

			testcase.input.reserve(image.size());

			for (auto pxl : Range(image.size())) {
				testcase.input[pxl] = (halnf) image[pxl] / 255.f;
			}
		}

		output.reserve(10);
	}

	halnf eval(ualni idx) {
		nn.evaluate(mTestcases[idx].input, output);
		return nn.calcCost(mTestcases[idx].output);
	}

	void applyGrad(ualni idx) {
		nn.calcGrad(mTestcases[idx].output);
		nn.applyGrad(step);
	}

	static halni getMaxIdx(const Buffer<halnf>& in) {
		halni out = 0;
		for (auto i : Range(in.size())) {
			if (in[i] > in[out]) {
				out = i;
			}
		}
		return out;
	}

	bool testIncorrect(ualni idx) {
		nn.evaluate(mTestcases[idx].input, output);
		return getMaxIdx(mTestcases[idx].output) != getMaxIdx(output);
	}

	void debLog(halni idx) {
		printf("\n Got      %i - ", getMaxIdx(output));
		for (auto val : output) {
			printf("%f ", val.data());
		}
		printf("\n Expected %i - ", getMaxIdx(mTestcases[idx].output));
		for (auto val : mTestcases[idx].output) {
			printf("%f ", val.data());
		}
		printf("\n\n");
	}

	void displayImage(ualni idx) {
		auto& testcase = mTestcases[idx];
		printf("Image : %i\n", int(getMaxIdx(testcase.output)));
		for (auto i : Range(28)) {
			for (auto j : Range(28)) {
				printf("%c", char(testcase.input[j * 28 + i] * 255));
			}
			printf("\n");
		}
	}

	halnf test(const Range<halni>& range) {
		halnf avgCost = 0;
		for (auto i : range) {
			avgCost += eval(i);
		}
		avgCost /= (halnf) range.idxDiff();
		return avgCost;
	}

	void trainStep(const Range<halni>& range) {
		nn.clearGrad();
		for (auto i : range) {
			nn.evaluate(mTestcases[i].input, output);
			nn.calcGrad(mTestcases[i].output);
		}
		nn.applyGrad(step);
	}

public:
	struct Image {
		Buffer<halnf> input;
		Buffer<halnf> output;
	};

public:
	Buffer<Image> mTestcases;

	FCNN nn;
	Buffer<halnf> output;

	halnf step = 0.01f;
};

int main() {
	ModuleManifest* deps[] = { &gModuleDataAnalysis, &gModuleConnection, nullptr };
	ModuleManifest module = ModuleManifest("NumRec", nullptr, nullptr, deps);

	if (!module.initialize()) {
		return 1;
	}

	{
		NumberRec app;

		auto trainRange = Range(0, 100);
		auto testRange = Range(0, 100);

		halnf cost = 100;
		while (cost > 0.1f) {
			cost = app.test(trainRange);
			app.trainStep(trainRange);
			printf("Cost - %f\n", cost);
		}

		auto errors = 0;
		for (auto i : testRange) {
			if (app.testIncorrect(i)) {
				errors++;
			}
			// app.debLog(i);
			// app.displayImage(i);
		}

		printf("\n\nIncorrect - %i out of %i\n\n", errors, testRange.idxDiff());
	}

	module.deinitialize();

	return 0;
}