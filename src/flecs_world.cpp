// Include Flecs header (implementation is in flecs/src/*.c files)
#include <flecs.h>

#include "flecs_world.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

// Static singleton instance
FlecsWorld* FlecsWorld::singleton = nullptr;

void FlecsWorld::_bind_methods() {
    // Lifecycle
    ClassDB::bind_method(D_METHOD("initialize"), &FlecsWorld::initialize);
    ClassDB::bind_method(D_METHOD("shutdown"), &FlecsWorld::shutdown);
    ClassDB::bind_method(D_METHOD("is_initialized"), &FlecsWorld::is_initialized);

    // Core operations
    ClassDB::bind_method(D_METHOD("progress", "delta"), &FlecsWorld::progress);

    // Stats
    ClassDB::bind_method(D_METHOD("get_entity_count"), &FlecsWorld::get_entity_count);
    ClassDB::bind_method(D_METHOD("get_world_info"), &FlecsWorld::get_world_info);

    // Settings
    ClassDB::bind_method(D_METHOD("set_target_fps", "fps"), &FlecsWorld::set_target_fps);
    ClassDB::bind_method(D_METHOD("get_target_fps"), &FlecsWorld::get_target_fps);

    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &FlecsWorld::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &FlecsWorld::is_enabled);
}

FlecsWorld::FlecsWorld() {
    ERR_FAIL_COND(singleton != nullptr);
    singleton = this;
}

FlecsWorld::~FlecsWorld() {
    if (initialized) {
        shutdown();
    }
    singleton = nullptr;
}

FlecsWorld* FlecsWorld::get_singleton() {
    return singleton;
}

void FlecsWorld::initialize() {
    if (initialized) {
        UtilityFunctions::print("FlecsWorld: Already initialized");
        return;
    }

    world = ecs_init();
    if (!world) {
        UtilityFunctions::printerr("FlecsWorld: Failed to initialize Flecs world");
        return;
    }

    initialized = true;
    UtilityFunctions::print("FlecsWorld: Initialized Flecs v",
        FLECS_VERSION_MAJOR, ".", FLECS_VERSION_MINOR, ".", FLECS_VERSION_PATCH);
}

void FlecsWorld::shutdown() {
    if (!initialized) {
        return;
    }

    if (world) {
        ecs_fini(world);
        world = nullptr;
    }

    initialized = false;
    UtilityFunctions::print("FlecsWorld: Shutdown complete");
}

bool FlecsWorld::is_initialized() const {
    return initialized;
}

bool FlecsWorld::progress(double delta) {
    if (!initialized || !world) {
        return false;
    }

    return ecs_progress(world, (ecs_ftime_t)delta);
}

ecs_world_t* FlecsWorld::get_world() const {
    return world;
}

int64_t FlecsWorld::get_entity_count() const {
    if (!initialized || !world) {
        return 0;
    }

    // Count all entities (returns count of entities matching the wildcard)
    return ecs_count_id(world, EcsAny);
}

Dictionary FlecsWorld::get_world_info() const {
    Dictionary info_dict;

    if (!initialized || !world) {
        return info_dict;
    }

    const ecs_world_info_t* info = ecs_get_world_info(world);
    if (!info) {
        return info_dict;
    }

    info_dict["table_create_total"] = (int64_t)info->table_create_total;
    info_dict["table_delete_total"] = (int64_t)info->table_delete_total;
    info_dict["systems_ran"] = (int64_t)info->systems_ran_frame;
    info_dict["observers_ran"] = (int64_t)info->observers_ran_frame;
    info_dict["frame_count"] = (int64_t)info->frame_count_total;
    info_dict["world_time"] = info->world_time_total;
    info_dict["delta_time"] = (double)info->delta_time;
    info_dict["target_fps"] = (double)info->target_fps;
    info_dict["id_create_total"] = (int64_t)info->id_create_total;
    info_dict["id_delete_total"] = (int64_t)info->id_delete_total;

    return info_dict;
}

void FlecsWorld::set_target_fps(double fps) {
    if (!initialized || !world) {
        return;
    }

    ecs_set_target_fps(world, (ecs_ftime_t)fps);
}

double FlecsWorld::get_target_fps() const {
    if (!initialized || !world) {
        return 0.0;
    }

    const ecs_world_info_t* info = ecs_get_world_info(world);
    if (!info) {
        return 0.0;
    }

    return info->target_fps;
}

void FlecsWorld::set_enabled(bool enabled) {
    if (!initialized || !world) {
        return;
    }

    // Use quit to disable (progress will return false after quit)
    // Note: Once quit is called, the world cannot be re-enabled without reinitializing
    if (!enabled) {
        ecs_quit(world);
    }
}

bool FlecsWorld::is_enabled() const {
    if (!initialized || !world) {
        return false;
    }

    return !ecs_should_quit(world);
}
