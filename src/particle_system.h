#pragma once
#include "define.h"
#include "math/ftic_math.h"

typedef struct Particle
{
    V2 position;
    V2 velocity;
    V2 acceleration;
    V2 dimension;
    V2 life;
    V2 start_dimensions;
    V4 color;
    b8 size_change;
    b8 alpha_change;
} Particle;

typedef struct UnorderedCircularParticleBuffer
{
    u32 index;
    u32 size;
    u32 capacity;
    Particle* data;
} UnorderedCircularParticleBuffer;

void particle_buffer_create(UnorderedCircularParticleBuffer* buffer, u32 capacity);
Particle* particle_buffer_get_next(UnorderedCircularParticleBuffer* buffer);
void particle_buffer_set_next(UnorderedCircularParticleBuffer* buffer, const Particle* particle);
void particle_update(Particle* particle, const f64 delta_time);
void particle_buffer_update(UnorderedCircularParticleBuffer* buffer, const f64 delta_time);
