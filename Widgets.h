#pragma once

#include <raylib.h>
#include <string>
#include <functional>
#include <list>

namespace UI {

	Color YiqContrast(const Color& color);

	extern Font font;

	Vector2 MeasureText(const std::string& text);
	void DrawText(const std::string& text, int x, int y, Color color);

	struct Margin {
		float left = 0, right = 0, top = 0, bottom = 0;
	};

	enum class Visibility {
		Visible, Collapsed
	};

	struct UIWidget {
		Visibility visibility = Visibility::Visible;
		Rectangle layout = {0, 0, 0, 0};
		bool isHovered = false;
		bool isActive = false;
		virtual ~UIWidget() = default;
		virtual Vector2 MinSize() const = 0;
		virtual void LayoutWidget(const Rectangle& rectangle) = 0;
		virtual void Draw() = 0;
		virtual bool IsLeaf() const = 0;
	};

	struct NullWidget : public UIWidget {
		Vector2 MinSize() const override { return {0, 0}; }
		void LayoutWidget(const Rectangle& rectangle) override {
			layout = rectangle;
		}
		void Draw() override {}
		bool IsLeaf() const override { return true; }
	};

	struct Label : public UIWidget {

		std::string text = "";
		Color color = BLACK;
		Margin padding = Margin();
		Color backgroundColor = WHITE;
		std::function<std::string()> textLambda = nullptr;

		std::string Text() const { return textLambda ? textLambda() : text; }

		explicit Label(std::string text, Color color = BLACK, const Margin& padding = Margin{5, 5, 5, 5});
		Vector2 MinSize() const override;
		void LayoutWidget(const Rectangle& rectangle) override;
		void Draw() override;
		bool IsLeaf() const override { return true; }
	};

	struct Button : public Label {

		std::function<void()> onClick = nullptr;

		explicit Button(std::string text, const Margin& padding = Margin{10, 10, 5, 5});
		void Draw() override;
	};

	struct Input : public Label {
		std::vector<std::string>& lines;
		int m_cursorDesiredColumn = 0;
		int m_cursorLine = 0;
		std::function<void()> onChange = nullptr;

		int CursorLine();
		void SetCursorLine(int line);
		int CursorColumn();
		void SetCursorColumn(int column);
		std::string& Line();

		Input (std::vector<std::string>& lines, Color color = BLACK, const Margin& padding = Margin{5, 5, 5, 5});
		Vector2 MinSize() const override;
		void Draw() override;
		void HandleChar(int c);
		void HandleKey(int key);
	};

	struct VerticalBox : public UIWidget {
		struct Slot {
			std::shared_ptr<UIWidget> widget = nullptr;
			float expandRatio = 0;
			Margin padding = {0, 0, 0, 0};
		};
		std::vector<std::shared_ptr<Slot>> slots;

		Vector2 MinSize() const override;
		std::shared_ptr<Slot> AddSlot(const std::shared_ptr<UIWidget>& child);
		void LayoutWidget(const Rectangle& rectangle) override;
		void Draw() override;
		bool IsLeaf() const override { return false; }
	};

	struct HorizontalBox : public UIWidget {
		struct Slot {
			std::shared_ptr<UIWidget> widget = nullptr;
			float expandRatio = 0;
			Margin padding = {0, 0, 0, 0};
		};
		std::vector<std::shared_ptr<Slot>> slots;

		Vector2 MinSize() const override;
		std::shared_ptr<Slot> AddSlot(const std::shared_ptr<UIWidget>& child);
		void LayoutWidget(const Rectangle& rectangle) override;
		void Draw() override;
		bool IsLeaf() const override { return false; }
	};

	std::shared_ptr<UIWidget> FindLeafWidgetAtPosition(const std::shared_ptr<UIWidget>& root, const Vector2& position);
	void Tick(const std::shared_ptr<UIWidget>& root);
}