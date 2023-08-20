#pragma once

#include "GraphicsCommon.hpp"

namespace tp {
	class Window;

	class Graphics {
	public:
		class GL {
			class Context;
			Context* mContext;
		private:
			friend Graphics;

			GL();
			~GL();

			void init();
			void deinit();
			static void proc();
			static void draw();
		public:
			// TODO : API
		};

		class GUI {
			class Context;
			Context* mContext;
		private:
			friend Graphics;

			GUI();
			~GUI();

			void init(Window* window);
			void deinit();
			void proc();
			void draw();

			void setStyle();

		public:
			void drawDebugInfoWindow();

			halnf getFontSize() const;
			halnf getUIScale() const;

			void setDPMM(ualni);
			void setFontSize(ualni);
			void setUIScale(ualni);
		};

		class Canvas {
			class Context;
			Context* mContext;
		private:
			friend Graphics;

			Canvas();
			~Canvas();

			void init();
			void deinit();
			void proc();
			void draw();

		public:
			// TODO : API
		};

	private:
		friend Window;

		Graphics() = default;
		void init(Window* window);
		void deinit();
		void draw();
		void proc();

	public:
		GUI& getDebugGui() { return mGui; }

	private:
		GUI mGui;
		GL mGl;
		Canvas mCanvas;
	};
}