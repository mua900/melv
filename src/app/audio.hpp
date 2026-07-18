#pragma once

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "util/common.hpp"
#include "util/template.hpp"

// a track which can be set to play different audio sources one at a time
using TrackId = u32;
constexpr TrackId NullTrackId = -1;

// this is a wrapper around SDL mixer
struct AudioPlayer {
    SDL_AudioDeviceID device = {};
    MIX_Mixer* mixer = {};
    BucketList<MIX_Track*> tracks;

    bool initialize();
    void cleanup();
    TrackId make_track();
    bool set_track_audio(TrackId track, MIX_Audio* audio);
    void destroy_track(TrackId& track);
    void tag_track(TrackId track, const char* tag);
    int get_track_count() const { return tracks.count(); }

    void set_master_gain(float gain);

    void stop_all(s64 fade_out_ms);

    void pause_all();
    void resume_all();

    void play_track(TrackId track);
    void stop_track(TrackId track, s64 fade_out_ms);
    void stop_track_frames(TrackId track, s64 fade_out_frames);
    void pause_track(TrackId track);
    void resume_track(TrackId track);
    void set_track_gain(TrackId track, float gain);

    void play_tag(const char* tag);
    void stop_tag(const char* tag, s64 fade_out_ms);
    void pause_tag(const char* tag);
    void resume_tag(const char* tag);
    void set_tag_gain(const char* tag, float gain);
};
