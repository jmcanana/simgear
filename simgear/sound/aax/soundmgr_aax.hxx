// soundmgr.hxx -- Sound effect management class
//
// Sound manager initially written by David Findlay
// <david_j_findlay@yahoo.com.au> 2001
//
// C++-ified by Curtis Olson, started March 2001.
// Modified for the new SoundSystem by Erik Hofman, October 2009
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
// Copyright (C) 2009 Erik Hofman <erik@ehofman.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

/**
 * \file soundmgr.hxx
 * Provides a sound manager class to keep track of
 * multiple sounds and manage playing them with different effects and
 * timings.
 */

#ifndef _SG_SOUNDMGR_AAX_HXX
#define _SG_SOUNDMGR_AAX_HXX 1

#include <string>
#include <vector>
#include <map>

#include <aax.h>

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/SGMathFwd.hxx>

#include "sample_group.hxx"

struct refUint {
    unsigned int refctr;
    ALuint id;

    refUint() { refctr = 0; id = (ALuint)-1; };
    refUint(ALuint i) { refctr = 1; id = i; };
    ~refUint() {};
};

typedef std::map < std::string, refUint > buffer_map;
typedef buffer_map::iterator buffer_map_iterator;
typedef buffer_map::const_iterator  const_buffer_map_iterator;

typedef std::map < std::string, SGSharedPtr<SGSampleGroup> > sample_group_map;
typedef sample_group_map::iterator sample_group_map_iterator;
typedef sample_group_map::const_iterator const_sample_group_map_iterator;

/**
 * Manage a collection of SGSampleGroup instances
 */
class SGSoundMgr : public SGSubsystem
{
public:

    SGSoundMgr();
    ~SGSoundMgr();

    void init(const char *devname = NULL);
    void bind();
    void unbind();
    void update(double dt);
    
    void suspend();
    void resume();
    void stop();

    inline void reinit() { stop(); init(); }

    /**
     * Test is the sound manager is in a working condition.
     * @return true is the sound manager is working
     */
    inline bool is_working() const { return _working; }

    /**
     * Set the sound manager to a  working condition.
     */
    void activate();

    /**
     * Test is the sound manager is in an active and working condition.
     * @return true is the sound manager is active
     */
    inline bool is_active() const { return _active; }

    /**
     * Register a sample group to the sound manager.
     * @para sgrp Pointer to a sample group to add
     * @param refname Reference name of the sample group
     * @return true if successful, false otherwise
     */
    bool add( SGSampleGroup *sgrp, const std::string& refname );

    /** 
     * Remove a sample group from the sound manager.
     * @param refname Reference name of the sample group to remove
     * @return true if successful, false otherwise
     */
    bool remove( const std::string& refname );

    /**
     * Test if a specified sample group is registered at the sound manager
     * @param refname Reference name of the sample group test for
     * @return true if the specified sample group exists
     */
    bool exists( const std::string& refname );

    /**
     * Find a specified sample group in the sound manager
     * @param refname Reference name of the sample group to find
     * @return A pointer to the SGSampleGroup
     */
    SGSampleGroup *find( const string& refname, bool create = false );

    /**
     * Set the Cartesian position of the sound manager.
     * @param pos listener position
     */
    void set_position( const SGVec3d& pos, const SGGeod& pos_geod ) {
        _base_pos = pos; _geod_pos = pos_geod; _changed = true;
    }

    void set_position_offset( const SGVec3d& pos ) {
        _offset_pos = pos; _changed = true;
    }

    /**
     * Get the position of the sound manager.
     * This is in the same coordinate system as OpenGL; y=up, z=back, x=right
     * @return listener position
     */
    SGVec3d& get_position() { return _absolute_pos; }

    /**
     * Set the velocity vector (in meters per second) of the sound manager
     * This is the horizontal local frame; x=north, y=east, z=down
     * @param Velocity vector
     */
    void set_velocity( const SGVec3d& vel ) {
        _velocity = vel; _changed = true;
    }

    /**
     * Get the velocity vector of the sound manager
     * This is in the same coordinate system as OpenGL; y=up, z=back, x=right.
     * @return Velocity vector of the listener
     */
    inline SGVec3f get_velocity() { return toVec3f(_velocity); }

    /**
     * Set the orientation of the sound manager
     * @param ori Quaternation containing the orientation information
     */
    void set_orientation( const SGQuatd& ori ) {
        _orientation = ori; _changed = true;
    }

    /**
     * Get the orientation of the sound manager
     * @return Quaternation containing the orientation information
     */
    inline const SGQuatd& get_orientation() { return _orientation; }

    /**
     * Get the direction vector of the sound manager
     * This is in the same coordinate system as OpenGL; y=up, z=back, x=right.
     * @return Look-at direction of the listener
     */
    SGVec3f get_direction() {
        return SGVec3f(_at_up_vec[0], _at_up_vec[1], _at_up_vec[2]);
    }

    enum {
        NO_SOURCE = (unsigned int)-1,
        NO_BUFFER = (unsigned int)-1
    };

    /**
     * Set the master volume.
     * @param vol Volume (must be between 0.0 and 1.0)
     */
    void set_volume( float vol );

    /**
     * Get the master volume.
     * @return Volume (must be between 0.0 and 1.0)
     */
    inline float get_volume() { return _volume; }

    /**
     * Get a free source-id
     * @return NO_SOURCE if no source is available
     */
    unsigned int request_source();

    /**
     * Free an source-id for future use
     * @param source source-id to free
     */
    void release_source( unsigned int source );

    /**
     * Get a free buffer-id
     * The buffer-id will be asigned to the sample by calling this function.
     * @param sample Pointer to an audio sample to assign the buffer-id to
     * @return NO_BUFFER if loading of the buffer failed.
     */
    unsigned int request_buffer(SGSoundSample *sample);

    /**
     * Free an buffer-id for this sample
     * @param sample Pointer to an audio sample for which to free the buffer
     */
    void release_buffer( SGSoundSample *sample );

    /**
     * Test if the position of the sound manager has changed.
     * The value will be set to false upon the next call to update_late()
     * @return true if the position has changed
     */
    inline bool has_changed() { return _changed; }

    /**
     * Some implementations seem to need the velocity miltyplied by a
     * factor of 100 to make them distinct. I've not found if this is
     * a problem in the implementation or in out code. Until then
     * this function is used to detect the problematic implementations.
     */
    inline bool bad_doppler_effect() { return _bad_doppler; }

    /**
     * Load a sample file and return it's configuration and data.
     * @param samplepath Path to the file to load
     * @param data Pointer to a variable that points to the allocated data
     * @param format Pointer to a vairable that gets the format
     * @param size Pointer to a vairable that gets the sample size in bytes
     * @param freq Pointer to a vairable that gets the sample frequency in Herz
     * @return true if succesful, false on error
     */
    bool load(string &samplepath, void **data, int *format,
                                         size_t *size, int *freq );

    /**
     * Get a list of available playback devices.
     */
    std::vector<const char*> get_available_devices();

    /**
     * Get the current vendor or rendering backend.
     */
    const std::string& get_vendor() { return _vendor; }
    const std::string& get_renderer() { return _renderer; }

private:
    static int _alut_init;

    bool _working;
    bool _active;
    bool _changed;
    float _volume;

    aaxConfig _config;

    // Position of the listener.
    SGVec3d _absolute_pos;
    SGVec3d _offset_pos;
    SGVec3d _base_pos;
    SGGeod _geod_pos;

    // Velocity of the listener.
    SGVec3d _velocity;

    // Orientation of the listener. 
    // first 3 elements are "at" vector, second 3 are "up" vector
    SGQuatd _orientation;

    sample_group_map _sample_groups;
    buffer_map _buffers;

    std::vector<aaxEmitter> _free_sources;
    std::vector<aaxEmitter> _sources_in_use;

    bool _bad_doppler;
    std::string _renderer;
    std::string _vendor;

    void update_pos_and_orientation();
    void update_sample_config( SGSampleGroup *sound );
};


#endif // _SG_SOUNDMGR_AAX_HXX

