#include "audio.hpp"
#include "util/common.hpp"
#include "util/log.hpp"

#include <cmath>

namespace melv
{

bool AudioPlayer::initialize() {
    SDL_AudioDeviceID playback_device = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
    SDL_AudioDeviceID sdl_device = SDL_OpenAudioDevice(playback_device, nullptr);
    if (!sdl_device)
    {
        SDL_Log("Could not open audio device using SDL: %s\n", SDL_GetError());
        return false;
    }

    MIX_Mixer* mix_mixer = MIX_CreateMixerDevice(sdl_device, nullptr);
    if (!mix_mixer)
    {
        log_error("Couldn't create MIX_Mixer object: %s\n", SDL_GetError());
        return false;
    }

    this->device = device;
    this->mixer = mix_mixer;
    return true;
}

void AudioPlayer::cleanup() {
    MIX_DestroyMixer(mixer);
}

TrackId AudioPlayer::make_track()
{
    MIX_Track* track = MIX_CreateTrack(mixer);
    if (!track)
    {
        return NullTrackId;
    }

    return tracks.add(track);
}

bool AudioPlayer::set_track_audio(TrackId track, MIX_Audio* audio)
{
    return MIX_SetTrackAudio(tracks.get(track), audio);
}

void AudioPlayer::destroy_track(TrackId& track)
{
    MIX_DestroyTrack(tracks.get(track));
    tracks.remove(track);
}

void AudioPlayer::tag_track(TrackId track, const char* tag)
{
    MIX_TagTrack(tracks[track], tag);
}

void AudioPlayer::set_master_gain(float gain)
{
    MIX_SetMixerGain(mixer, gain);
}

void AudioPlayer::stop_all(s64 fade_out_ms)
{
    MIX_StopAllTracks(mixer, fade_out_ms);
}

void AudioPlayer::pause_all()
{
    MIX_PauseAllTracks(mixer);
}

void AudioPlayer::resume_all()
{
    MIX_ResumeAllTracks(mixer);
}

void AudioPlayer::play_track(TrackId track)
{
    MIX_Track* t = tracks.get(track);

    SDL_PropertiesID options = SDL_CreateProperties();

    SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, 1);

    MIX_PlayTrack(t, options);
    SDL_DestroyProperties(options);
}

void AudioPlayer::stop_track(TrackId track, s64 fade_out_ms)
{
    MIX_Track* t = tracks[track];
    MIX_StopTrack(t, MIX_TrackMSToFrames(t, fade_out_ms));
}

void AudioPlayer::stop_track_frames(TrackId track, s64 fade_out_frames)
{
    MIX_StopTrack(tracks[track], fade_out_frames);
}

void AudioPlayer::pause_track(TrackId track)
{
    MIX_Track* t = tracks.get(track);

    MIX_PauseTrack(t);
}

void AudioPlayer::resume_track(TrackId track)
{
    MIX_Track* t = tracks.get(track);

    MIX_ResumeTrack(t);
}

void AudioPlayer::set_track_gain(TrackId track, float gain)
{
    MIX_SetTrackGain(tracks[track], gain);
}

void AudioPlayer::play_tag(const char* tag)
{
    SDL_PropertiesID options = SDL_CreateProperties();

    SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, 1);

    MIX_PlayTag(mixer, tag, options);

    SDL_DestroyProperties(options);
}

void AudioPlayer::stop_tag(const char* tag, s64 fade_out_ms)
{
    MIX_StopTag(mixer, tag, fade_out_ms);
}

void AudioPlayer::pause_tag(const char* tag)
{
    MIX_PauseTag(mixer, tag);
}

void AudioPlayer::resume_tag(const char* tag)
{
    MIX_ResumeTag(mixer, tag);
}

void AudioPlayer::set_tag_gain(const char* tag, float gain)
{
    MIX_SetTagGain(mixer, tag, gain);
}

} // namespace
