#include "guis/GuiCollectionSystemsOptions.h"

#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiSettings.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Window.h"

GuiCollectionSystemsOptions::GuiCollectionSystemsOptions(Window* window) : GuiComponent(window), mMenu(window, _T("GAME COLLECTION SETTINGS"))
{
	initializeMenu();
}

void GuiCollectionSystemsOptions::initializeMenu()
{
	addChild(&mMenu);

	// get collections

	addSystemsToMenu();

	// add "Create New Custom Collection from Theme"

	std::vector<std::string> unusedFolders = CollectionSystemManager::get()->getUnusedSystemsFromTheme();
	if (unusedFolders.size() > 0)
	{
		addEntry(_T("CREATE NEW CUSTOM COLLECTION FROM THEME"), ThemeData::getMenuTheme()->Text.color, true,
		[this, unusedFolders] {
			auto s = new GuiSettings(mWindow, _T("SELECT THEME FOLDER"));
			std::shared_ptr< OptionListComponent<std::string> > folderThemes = std::make_shared< OptionListComponent<std::string> >(mWindow, "SELECT THEME FOLDER", true);

			// add Custom Systems
			for(auto it = unusedFolders.cbegin() ; it != unusedFolders.cend() ; it++ )
			{
				ComponentListRow row;
				std::string name = *it;

				std::function<void()> createCollectionCall = [name, this, s] {
					createCollection(name);
				};
				row.makeAcceptInputHandler(createCollectionCall);

				auto themeFolder = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(name), ThemeData::getMenuTheme()->TextSmall.font, ThemeData::getMenuTheme()->TextSmall.color);
				row.addElement(themeFolder, true);
				s->addRow(row);
			}

			s->updatePosition();
			mWindow->pushGui(s);
		});
	}

	ComponentListRow row;
	row.addElement(std::make_shared<TextComponent>(mWindow, _T("CREATE NEW CUSTOM COLLECTION"), ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color), true);
	auto createCustomCollection = [this](const std::string& newVal) {
		std::string name = newVal;
		// we need to store the first Gui and remove it, as it'll be deleted by the actual Gui
		Window* window = mWindow;
		GuiComponent* topGui = window->peekGui();
		window->removeGui(topGui);
		createCollection(name);
	};
	row.makeAcceptInputHandler([this, createCustomCollection] {
		mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _T("New Collection Name"), "", createCustomCollection, false));
	});

	mMenu.addRow(row);

	bundleCustomCollections = std::make_shared<SwitchComponent>(mWindow);
	bundleCustomCollections->setState(Settings::getInstance()->getBool("UseCustomCollectionsSystem"));
	mMenu.addWithLabel(_T("GROUP UNTHEMED CUSTOM COLLECTIONS"), bundleCustomCollections);

	sortAllSystemsSwitch = std::make_shared<SwitchComponent>(mWindow);
	sortAllSystemsSwitch->setState(Settings::getInstance()->getBool("SortAllSystems"));
	mMenu.addWithLabel(_T("SORT CUSTOM COLLECTIONS AND SYSTEMS"), sortAllSystemsSwitch);

	favoritesFirstSwitch = std::make_shared<SwitchComponent>(mWindow);
	favoritesFirstSwitch->setState(Settings::getInstance()->getBool("FavoritesFirst"));
	mMenu.addWithLabel(_T("DISPLAY FAVORITES FIRST IN GAMELIST"), favoritesFirstSwitch);

	toggleSystemNameInCollections = std::make_shared<SwitchComponent>(mWindow);
	toggleSystemNameInCollections->setState(Settings::getInstance()->getBool("CollectionShowSystemInfo"));
	mMenu.addWithLabel(_T("SHOW SYSTEM NAME IN COLLECTIONS"), toggleSystemNameInCollections);

	if(CollectionSystemManager::get()->isEditing())
	{
		row.elements.clear();
		row.addElement(std::make_shared<TextComponent>(mWindow, _T("FINISH EDITING COLLECTION")+" '" + Utils::String::toUpper(CollectionSystemManager::get()->getEditingCollection()) + "'", ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color), true);
		row.makeAcceptInputHandler(std::bind(&GuiCollectionSystemsOptions::exitEditMode, this));
		mMenu.addRow(row);
	}

	mMenu.addButton(_T("BACK"), _T("BACK"), std::bind(&GuiCollectionSystemsOptions::applySettings, this));

	mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);
}

void GuiCollectionSystemsOptions::addEntry(std::string name, unsigned int color, bool add_arrow, const std::function<void()>& func)
{
	auto theme = ThemeData::getMenuTheme();

	std::shared_ptr<Font> font = theme->Text.font; // Font::get(FONT_SIZE_MEDIUM);

	// populate the list
	ComponentListRow row;
	row.addElement(std::make_shared<TextComponent>(mWindow, name, font, theme->Text.color), true);

	if(add_arrow)
	{
		std::shared_ptr<ImageComponent> bracket = makeArrow(mWindow);
		row.addElement(bracket, false);
	}

	row.makeAcceptInputHandler(func);

	mMenu.addRow(row);
}

void GuiCollectionSystemsOptions::createCollection(std::string inName) {
	std::string name = CollectionSystemManager::get()->getValidNewCollectionName(inName);
	SystemData* newSys = CollectionSystemManager::get()->addNewCustomCollection(name);
	customOptionList->add(name, name, true);
	std::string outAuto = Utils::String::vectorToCommaString(autoOptionList->getSelectedObjects());
	std::string outCustom = Utils::String::vectorToCommaString(customOptionList->getSelectedObjects());
	updateSettings(outAuto, outCustom);
	ViewController::get()->goToSystemView(newSys);

	Window* window = mWindow;
	CollectionSystemManager::get()->setEditMode(name);
	while(window->peekGui() && window->peekGui() != ViewController::get())
		delete window->peekGui();
	return;
}

void GuiCollectionSystemsOptions::exitEditMode()
{
	CollectionSystemManager::get()->exitEditMode();
	applySettings();
}

GuiCollectionSystemsOptions::~GuiCollectionSystemsOptions()
{

}

void GuiCollectionSystemsOptions::addSystemsToMenu()
{

	std::map<std::string, CollectionSystemData> autoSystems = CollectionSystemManager::get()->getAutoCollectionSystems();

	autoOptionList = std::make_shared< OptionListComponent<std::string> >(mWindow, _T("SELECT COLLECTIONS"), true);

	// add Auto Systems
	for(std::map<std::string, CollectionSystemData>::const_iterator it = autoSystems.cbegin() ; it != autoSystems.cend() ; it++ )
	{
		autoOptionList->add(it->second.decl.longName, it->second.decl.name, it->second.isEnabled);
	}
	mMenu.addWithLabel(_T("AUTOMATIC GAME COLLECTIONS"), autoOptionList);

	std::map<std::string, CollectionSystemData> customSystems = CollectionSystemManager::get()->getCustomCollectionSystems();

	customOptionList = std::make_shared< OptionListComponent<std::string> >(mWindow, _T("SELECT COLLECTIONS"), true);

	// add Custom Systems
	for(std::map<std::string, CollectionSystemData>::const_iterator it = customSystems.cbegin() ; it != customSystems.cend() ; it++ )
	{
		customOptionList->add(it->second.decl.longName, it->second.decl.name, it->second.isEnabled);
	}
	mMenu.addWithLabel(_T("CUSTOM GAME COLLECTIONS"), customOptionList);
}

void GuiCollectionSystemsOptions::applySettings()
{
	std::string outAuto = Utils::String::vectorToCommaString(autoOptionList->getSelectedObjects());
	std::string prevAuto = Settings::getInstance()->getString("CollectionSystemsAuto");
	std::string outCustom = Utils::String::vectorToCommaString(customOptionList->getSelectedObjects());
	std::string prevCustom = Settings::getInstance()->getString("CollectionSystemsCustom");

	bool outSort = sortAllSystemsSwitch->getState();
	bool prevSort = Settings::getInstance()->getBool("SortAllSystems");
	bool outBundle = bundleCustomCollections->getState();
	bool prevBundle = Settings::getInstance()->getBool("UseCustomCollectionsSystem");

	bool prevShow = Settings::getInstance()->getBool("CollectionShowSystemInfo");
	bool outShow = toggleSystemNameInCollections->getState();

	bool outFavoritesFirst = favoritesFirstSwitch->getState();
	bool prevFavoritesFirst = Settings::getInstance()->getBool("FavoritesFirst");
	
	bool needUpdateSettings = prevAuto != outAuto || prevCustom != outCustom || outSort != prevSort || outBundle != prevBundle || outFavoritesFirst != prevFavoritesFirst || prevShow != outShow;
	if (needUpdateSettings)
	{
		updateSettings(outAuto, outCustom);
	}

	delete this;
}

void GuiCollectionSystemsOptions::updateSettings(std::string newAutoSettings, std::string newCustomSettings)
{
	Settings::getInstance()->setString("CollectionSystemsAuto", newAutoSettings);
	Settings::getInstance()->setString("CollectionSystemsCustom", newCustomSettings);
	Settings::getInstance()->setBool("SortAllSystems", sortAllSystemsSwitch->getState());
	Settings::getInstance()->setBool("UseCustomCollectionsSystem", bundleCustomCollections->getState());
	Settings::getInstance()->setBool("CollectionShowSystemInfo", toggleSystemNameInCollections->getState());
	Settings::getInstance()->setBool("FavoritesFirst", favoritesFirstSwitch->getState());

	Settings::getInstance()->saveFile();
	CollectionSystemManager::get()->loadEnabledListFromSettings();
	CollectionSystemManager::get()->updateSystemsList();
	ViewController::get()->goToStart();
	ViewController::get()->reloadAll();
}

bool GuiCollectionSystemsOptions::input(InputConfig* config, Input input)
{
	bool consumed = GuiComponent::input(config, input);
	if(consumed)
		return true;

	if(config->isMappedTo("b", input) && input.value != 0)
	{
		applySettings();
	}


	return false;
}

std::vector<HelpPrompt> GuiCollectionSystemsOptions::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("b", _T("BACK")));
	return prompts;
}
