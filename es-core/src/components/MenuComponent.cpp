#include "components/MenuComponent.h"

#include "components/ButtonComponent.h"

#define BUTTON_GRID_VERT_PADDING 32
#define BUTTON_GRID_HORIZ_PADDING 10

#define TITLE_HEIGHT (mTitle->getFont()->getLetterHeight() + TITLE_VERT_PADDING)

MenuComponent::MenuComponent(Window* window, std::string title, const std::shared_ptr<Font>& titleFont) : GuiComponent(window),
	mBackground(window), mGrid(window, Vector2i(1, 3))
{
	auto theme = ThemeData::getMenuTheme();

	addChild(&mBackground);
	addChild(&mGrid);
	
	mBackground.setImagePath(theme->Background.path); // ":/frame.png"
	mBackground.setCenterColor(theme->Background.color);
	mBackground.setEdgeColor(theme->Background.color);

	// set up title
	mTitle = std::make_shared<TextComponent>(mWindow);
	mTitle->setHorizontalAlignment(ALIGN_CENTER);
	mTitle->setColor(theme->Title.color); // 0x555555FF
	setTitle(title.c_str(), theme->Title.font); //  titleFont
	mGrid.setEntry(mTitle, Vector2i(0, 0), false);

	// set up list which will never change (externally, anyway)
	mList = std::make_shared<ComponentList>(mWindow);
	mGrid.setEntry(mList, Vector2i(0, 1), true);

	mGrid.setUnhandledInputCallback([this](InputConfig* config, Input input) -> bool {
		if (config->isMappedLike("down", input)) {
			mGrid.setCursorTo(mList);
			mList->setCursorIndex(0);
			return true;
		}
		if (config->isMappedLike("up", input)) {
			mList->setCursorIndex(mList->size() - 1);
			if (mButtons.size()) {
				mGrid.moveCursor(Vector2i(0, 1));
			}
			else {
				mGrid.setCursorTo(mList);
			}
			return true;
		}
		return false;
	});

	updateGrid();
	updateSize();

	mGrid.resetCursor();
}

void MenuComponent::addWithLabel(const std::string& label, const std::shared_ptr<GuiComponent>& comp, bool setCursorHere, bool invert_when_selected)
{
	auto theme = ThemeData::getMenuTheme();
	
	ComponentListRow row;
	row.addElement(std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(label), theme->Text.font, theme->Text.color), true);
	row.addElement(comp, false, invert_when_selected);
	addRow(row, setCursorHere);
}

void MenuComponent::setTitle(const char* title, const std::shared_ptr<Font>& font)
{
	mTitle->setText(Utils::String::toUpper(title));
	mTitle->setFont(font);
}

float MenuComponent::getButtonGridHeight() const
{
	auto menuTheme = ThemeData::getMenuTheme();
	return (mButtonGrid ? mButtonGrid->getSize().y() : menuTheme->Text.font->getHeight() + BUTTON_GRID_VERT_PADDING);
	//return (mButtonGrid ? mButtonGrid->getSize().y() : Font::get(FONT_SIZE_MEDIUM)->getHeight() + BUTTON_GRID_VERT_PADDING);
}

void MenuComponent::updateSize()
{
	const float maxHeight = Renderer::getScreenHeight() * 0.75f;
	float height = TITLE_HEIGHT + mList->getTotalRowHeight() + getButtonGridHeight() + 2;
	if(height > maxHeight)
	{
		height = TITLE_HEIGHT + getButtonGridHeight();
		int i = 0;
		while(i < mList->size())
		{
			float rowHeight = mList->getRowHeight(i);
			if(height + rowHeight < maxHeight)
				height += rowHeight;
			else
				break;
			i++;
		}
	}

	float width = (float)Math::min((int)Renderer::getScreenHeight(), (int)(Renderer::getScreenWidth() * 0.90f));
	setSize(width, height);
}

void MenuComponent::onSizeChanged()
{
	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	// update grid row/col sizes
	mGrid.setRowHeightPerc(0, TITLE_HEIGHT / mSize.y());
	mGrid.setRowHeightPerc(2, getButtonGridHeight() / mSize.y());

	mGrid.setSize(mSize);
}

void MenuComponent::addButton(const std::string& name, const std::string& helpText, const std::function<void()>& callback)
{
	mButtons.push_back(std::make_shared<ButtonComponent>(mWindow, Utils::String::toUpper(name), helpText, callback));
	updateGrid();
	updateSize();
}

void MenuComponent::updateGrid()
{
	if(mButtonGrid)
		mGrid.removeEntry(mButtonGrid);

	mButtonGrid.reset();

	if(mButtons.size())
	{
		mButtonGrid = makeButtonGrid(mWindow, mButtons);
		mGrid.setEntry(mButtonGrid, Vector2i(0, 2), true, false);
	}
}

std::vector<HelpPrompt> MenuComponent::getHelpPrompts()
{
	return mGrid.getHelpPrompts();
}

std::shared_ptr<ComponentGrid> makeMultiDimButtonGrid(Window* window, const std::vector< std::vector< std::shared_ptr<ButtonComponent> > >& buttons, float outerWidth)
{

	const int sizeX = (int)buttons.at(0).size();
	const int sizeY = (int)buttons.size();
	const float buttonHeight = buttons.at(0).at(0)->getSize().y();
	const float gridHeight = (buttonHeight + BUTTON_GRID_VERT_PADDING + 2) * sizeY;

	float horizPadding = (float)BUTTON_GRID_HORIZ_PADDING;
	float gridWidth, buttonWidth;

//	do {
		gridWidth = outerWidth - horizPadding; // to get centered because size * (button size + BUTTON_GRID_VERT_PADDING) let a half BUTTON_GRID_VERT_PADDING left / right marge
		buttonWidth = (gridWidth / sizeX) - horizPadding;
	//	horizPadding -= 2;
//	} while ((buttonWidth < 100) && (horizPadding > 2));

	std::shared_ptr<ComponentGrid> grid = std::make_shared<ComponentGrid>(window, Vector2i(sizeX, sizeY));

	grid->setSize(gridWidth, gridHeight);

	for (int x = 0; x < sizeX; x++)
		grid->setColWidthPerc(x, (float)1 / sizeX);

	for (int y = 0; y < sizeY; y++)
	{
		for (int x = 0; x < sizeX; x++)
		{
			const std::shared_ptr<ButtonComponent>& button = buttons.at(y).at(x);
			button->setSize(buttonWidth, buttonHeight);
			grid->setEntry(button, Vector2i(x, y), true, false);
		}
	}

	return grid;
}

std::shared_ptr<ComponentGrid> makeButtonGrid(Window* window, const std::vector< std::shared_ptr<ButtonComponent> >& buttons)
{
	std::shared_ptr<ComponentGrid> buttonGrid = std::make_shared<ComponentGrid>(window, Vector2i((int)buttons.size(), 2));

	float buttonGridWidth = (float)BUTTON_GRID_HORIZ_PADDING * buttons.size(); // initialize to padding
	for(int i = 0; i < (int)buttons.size(); i++)
	{
		buttonGrid->setEntry(buttons.at(i), Vector2i(i, 0), true, false);
		buttonGridWidth += buttons.at(i)->getSize().x();
	}
	for(unsigned int i = 0; i < buttons.size(); i++)
	{
		buttonGrid->setColWidthPerc(i, (buttons.at(i)->getSize().x() + BUTTON_GRID_HORIZ_PADDING) / buttonGridWidth);
	}

	buttonGrid->setSize(buttonGridWidth, buttons.at(0)->getSize().y() + BUTTON_GRID_VERT_PADDING + 2);
	buttonGrid->setRowHeightPerc(1, 2 / buttonGrid->getSize().y()); // spacer row to deal with dropshadow to make buttons look centered

	return buttonGrid;
}

std::shared_ptr<ImageComponent> makeArrow(Window* window)
{
	auto menuTheme = ThemeData::getMenuTheme();

	auto bracket = std::make_shared<ImageComponent>(window);
	bracket->setImage(ThemeData::getMenuTheme()->Icons.arrow); // ":/arrow.svg");
	bracket->setColorShift(menuTheme->Text.color);
	bracket->setResize(0, round(menuTheme->Text.font->getLetterHeight()));
	//bracket->setResize(0, Math::round(Font::get(FONT_SIZE_MEDIUM)->getLetterHeight()));
	return bracket;
}
