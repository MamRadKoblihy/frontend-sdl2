#pragma once

#include "FileChooser.h"
#include "HelpWindow.h"
#include "MainMenu.h"
#include "ToastMessage.h"
#include "SettingsWindow.h"

#include "notifications/DisplayToastNotification.h"

#include <SDL2/SDL.h>

#include <Poco/Logger.h>
#include <Poco/NObserver.h>

#include <Poco/Util/PropertyFileConfiguration.h>
#include <Poco/Util/Subsystem.h>

struct ImFont;
class ProjectMWrapper;
class SDLRenderingWindow;

class ProjectMGUI : public Poco::Util::Subsystem
{
public:
    const char* name() const override;

    void initialize(Poco::Util::Application& app) override;

    void uninitialize() override;

    /**
     * @brief Sets the UI configuration file.
     * @param config The properties file instance which stores the UI settings.
     */
    void UIConfiguration(Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> config);

    /**
     * @brief Returns the UI configuration file.
     * @return The properties file instance which stores the UI settings.
     */
    Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> UIConfiguration();

    /**
     * @brief Sets the command line override map.
     * @param config The properties file instance which stores the UI settings.
     */
    void CommandLineConfiguration(Poco::AutoPtr<Poco::Util::MapConfiguration> config);

    /**
     * @brief Returns the command line override map.
     * @return The properties file instance which stores the UI settings.
     */
    Poco::AutoPtr<Poco::Util::MapConfiguration> CommandLineConfiguration();

    /**
     * @brief Updates the font size after DPI changes.
     */
    void UpdateFontSize();

    /**
     * @brief Processes SDL input events in Dear ImGui.
     * @param event The SDL event.
     */
    void ProcessInput(const SDL_Event& event);

    /**
     * @brief Displays or hides the UI.
     */
    void Toggle();

    /**
     * @brief Draws the UI, including toasts.
     * If neither a toast nor the UI are visible, this is basically a no-op.
     */
    void Draw();

    /**
     * @brief Tells the caller whether the UI currently wants the keyboard input.
     * @return True if the UI wants the keyboard input, false if the app should process the events.
     */
    bool WantsKeyboardInput();

    /**
     * @brief Tells the caller whether the UI currently wants the mouse input.
     * @return True if the UI wants the mouse input, false if the app should process the events.
     */
    bool WantsMouseInput();

    /**
     * @brief Pushes the "toast" font to the render stack
     */
    void PushToastFont();

    /**
     * @brief Pushes the "UI" font to the render stack
     */
    void PushUIFont();

    /**
     * @brief Pops the last font from the stack
     */
    void PopFont();

    /**
     * @brief Displays the settings window.
     */
    void ShowSettingsWindow();

    /**
     * @brief Displays the help window.
     */
    void ShowHelpWindow();

private:
    void DisplayToastNotificationHandler(const Poco::AutoPtr<DisplayToastNotification>& notification);

    ProjectMWrapper* _projectMWrapper{nullptr};

    Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> _uiConfig; //!< The UI configuration, used to store/reset changes made in the UI's settings dialog.
    Poco::AutoPtr<Poco::Util::MapConfiguration> _commandLineOverrides; //!< The command-line override settings.

    Poco::NObserver<ProjectMGUI, DisplayToastNotification> _displayToastNotificationObserver{*this, &ProjectMGUI::DisplayToastNotificationHandler};

    SDL_Window* _renderingWindow{nullptr}; //!< Pointer to the SDL window used for rendering.
    SDL_GLContext _glContext{nullptr}; //!< Pointer to the OpenGL context associated with the window.
    ImFont* _uiFont{nullptr}; //!< Main UI font (monospaced).
    ImFont* _toastFont{nullptr}; //!< Toast message font (sans-serif, larger).

    uint64_t _lastFrameTicks{0}; //!< Tick count of the last frame (see SDL_GetTicks64)

    float _dpi{0.0f}; //!< Last DPI value.

    MainMenu _mainMenu{*this};
    SettingsWindow _settingsWindow{*this}; //!< The settings window.
    FileChooser _fileChooser{"Load Preset", {"milk"}}; //!< File chooser dialog.
    HelpWindow _helpWindow; //!< Help window with shortcuts and tips.

    std::unique_ptr<ToastMessage> _toast; //!< Current toast to be displayed.

    bool _visible{false}; //!< Flag for settings window visibility.

    Poco::Logger& _logger{Poco::Logger::get("ProjectMGUI")}; //!< The class logger.
};
