#include "ProjectMWrapper.h"

#include "SDLRenderingWindow.h"

#include "notifications/DisplayToastNotification.h"

#include <Poco/NotificationCenter.h>

#include <Poco/Util/Application.h>

#include <SDL2/SDL_opengl.h>

const char* ProjectMWrapper::name() const
{
    return "ProjectM Wrapper";
}

void ProjectMWrapper::initialize(Poco::Util::Application& app)
{
    _config = app.config().createView("projectM");

    if (!_projectM)
    {
        auto& sdlWindow = app.getSubsystem<SDLRenderingWindow>();

        int canvasWidth{0};
        int canvasHeight{0};

        sdlWindow.GetDrawableSize(canvasWidth, canvasHeight);

        auto presetPath = _config->getString("presetPath", app.config().getString("application.dir", ""));
        auto texturePath = _config->getString("texturePath", app.config().getString("", ""));

        _projectM = projectm_create();

        projectm_set_window_size(_projectM, canvasWidth, canvasHeight);
        projectm_set_fps(_projectM, _config->getInt("fps", 60));
        projectm_set_mesh_size(_projectM, _config->getInt("meshX", 220), _config->getInt("meshY", 125));
        projectm_set_aspect_correction(_projectM, _config->getBool("aspectCorrectionEnabled", true));
        projectm_set_preset_locked(_projectM, _config->getBool("presetLocked", false));

        // Preset display settings
        projectm_set_preset_duration(_projectM, _config->getInt("displayDuration", 30));
        projectm_set_soft_cut_duration(_projectM, _config->getInt("transitionDuration", 3));
        projectm_set_hard_cut_enabled(_projectM, _config->getBool("hardCutsEnabled", false));
        projectm_set_hard_cut_duration(_projectM, _config->getInt("hardCutDuration", 20));
        projectm_set_hard_cut_sensitivity(_projectM, static_cast<float>(_config->getDouble("hardCutSensitivity", 1.0)));
        projectm_set_beat_sensitivity(_projectM, static_cast<float>(_config->getDouble("beatSensitivity", 1.0)));

        if (!texturePath.empty())
        {
            const char* texturePathList[1]{&texturePath[0]};
            projectm_set_texture_search_paths(_projectM, texturePathList, 1);
        }

        // Playlist
        _playlist = projectm_playlist_create(_projectM);

        projectm_playlist_set_shuffle(_playlist, _config->getBool("shuffleEnabled", true));
        if (!presetPath.empty())
        {
            projectm_playlist_add_path(_playlist, presetPath.c_str(), true, false);
            projectm_playlist_sort(_playlist, 0, projectm_playlist_size(_playlist), SORT_PREDICATE_FILENAME_ONLY, SORT_ORDER_ASCENDING);
        }

        projectm_playlist_set_preset_switched_event_callback(_playlist, &ProjectMWrapper::PresetSwitchedEvent, static_cast<void*>(this));
    }

    Poco::NotificationCenter::defaultCenter().addObserver(_playbackControlNotificationObserver);
}

void ProjectMWrapper::uninitialize()
{

    Poco::NotificationCenter::defaultCenter().removeObserver(_playbackControlNotificationObserver);

    if (_projectM)
    {
        projectm_destroy(_projectM);
        _projectM = nullptr;
    }

    if (_playlist)
    {
        projectm_playlist_destroy(_playlist);
        _playlist = nullptr;
    }
}

projectm_handle ProjectMWrapper::ProjectM() const
{
    return _projectM;
}

projectm_playlist_handle ProjectMWrapper::Playlist() const
{
    return _playlist;
}

int ProjectMWrapper::TargetFPS()
{
    return _config->getInt("fps", 60);
}

void ProjectMWrapper::RenderFrame() const
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    projectm_opengl_render_frame(_projectM);
}

void ProjectMWrapper::DisplayInitialPreset()
{
    if (!_config->getBool("enableSplash", true))
    {
        if (_config->getBool("shuffleEnabled", true))
        {
            projectm_playlist_play_next(_playlist, true);
        }
        else
        {
            projectm_playlist_set_position(_playlist, 0, true);
        }
    }
}

void ProjectMWrapper::ChangeBeatSensitivity(float value)
{
    projectm_set_beat_sensitivity(_projectM, projectm_get_beat_sensitivity(_projectM) + value);
    Poco::NotificationCenter::defaultCenter().postNotification(
        new DisplayToastNotification(Poco::format("Beat Sensitivity: %.2hf", projectm_get_beat_sensitivity(_projectM))));
}

void ProjectMWrapper::PresetSwitchedEvent(bool isHardCut, unsigned int index, void* context)
{
    auto that = reinterpret_cast<ProjectMWrapper*>(context);
    auto presetName = projectm_playlist_item(that->_playlist, index);
    poco_information_f1(that->_logger, "Displaying preset: %s", std::string(presetName));
    projectm_free_string(presetName);

    Poco::NotificationCenter::defaultCenter().postNotification(new UpdateWindowTitleNotification);
}

void ProjectMWrapper::PlaybackControlNotificationHandler(const Poco::AutoPtr<PlaybackControlNotification>& notification)
{
    switch (notification->ControlAction())
    {
        case PlaybackControlNotification::Action::NextPreset:
            projectm_playlist_play_next(_playlist, true);
            break;

        case PlaybackControlNotification::Action::PreviousPreset:
            projectm_playlist_play_previous(_playlist, true);
            break;

        case PlaybackControlNotification::Action::LastPreset:
            projectm_playlist_play_last(_playlist, true);
            break;

        case PlaybackControlNotification::Action::RandomPreset: {
            bool shuffleEnabled = projectm_playlist_get_shuffle(_playlist);
            projectm_playlist_set_shuffle(_playlist, true);
            projectm_playlist_play_next(_playlist, true);
            projectm_playlist_set_shuffle(_playlist, shuffleEnabled);
            break;
        }

        case PlaybackControlNotification::Action::ToggleShuffle:
            projectm_playlist_set_shuffle(_playlist, !projectm_playlist_get_shuffle(_playlist));
            _config->setBool("shuffleEnabled", projectm_playlist_get_shuffle(_playlist));
            break;

        case PlaybackControlNotification::Action::TogglePresetLocked: {
            bool locked = !projectm_get_preset_locked(_projectM);
            projectm_set_preset_locked(_projectM, locked);
            _config->setBool("presetLocked", locked);
            Poco::NotificationCenter::defaultCenter().postNotification(new UpdateWindowTitleNotification);
            break;
        }
    }
}
