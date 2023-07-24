#pragma once

#include "GraphicsCommon.hpp"

namespace tp {

	class AnimValue {
		halnf mValPrev = 0;
		halnf mVal = 0;
		time_ms mTimeStart = 0;
		halni mTimeAnim = 250;

		static bool gInTransition;

	private:
		[[nodiscard]] halnf interpolate() const;

	public:
		AnimValue();
		explicit AnimValue(halnf val);

		[[nodiscard]] halnf prev() const { return mValPrev; }
		void set(halnf val);
		void setNoTransition(halnf);
		void setAnimTime(halni time);
		[[nodiscard]] halnf get() const;
		[[nodiscard]] halnf getTarget() const;
		[[nodiscard]] bool inTransition() const;
		explicit operator halnf() const;
		void operator=(halnf val);
	};

	class AnimRect : Rect<AnimValue> {
	public:
		AnimRect() {
			set({ 0, 0, 0, 0 });
			setAnimTime(450);
		}

		[[nodiscard]] RectF get() const;
		[[nodiscard]] RectF getTarget() const;
		void setNoTransition(RectF);
		void set(const RectF&);
		void setAnimTime(halni time_ms);
	};

	class AnimColor {
		AnimRect mColor;
	public:
		[[nodiscard]] RGBA get() const;
		void set(const RGBA&);
	};
};