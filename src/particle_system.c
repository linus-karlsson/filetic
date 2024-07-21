#include "particle_system.h"
#include "util.h"

void particle_buffer_create(UnorderedCircularParticleBuffer* buffer,
                            u32 capacity)
{
    buffer->index = 0;
    buffer->size = 0;
    buffer->capacity = capacity;
    buffer->data = (Particle*)calloc(capacity, sizeof(Particle));
}

Particle* particle_buffer_get_next(UnorderedCircularParticleBuffer* buffer)
{
    return buffer->data + buffer->index;
}

void particle_buffer_set_next(UnorderedCircularParticleBuffer* buffer,
                              const Particle* particle)
{
    buffer->data[buffer->index++].start_dimensions = particle->dimension;
    buffer->size = min(buffer->size + 1, buffer->capacity);
    buffer->index %= buffer->capacity;
}

void particle_update(Particle* particle, const f64 delta_time)
{
    particle->velocity = v2_add(particle->velocity,
                                v2_s_multi(particle->acceleration, (f32)delta_time));
    particle->position =
        v2_add(particle->position, v2_s_multi(particle->velocity, (f32)delta_time));
}

void particle_buffer_update(UnorderedCircularParticleBuffer* buffer,
                            const f64 delta_time)
{
    for (u32 i = 0; i < buffer->size; ++i)
    {
        Particle* particle = buffer->data + i;
        if (particle->life.x > 0.0f)
        {
            particle_update(particle, delta_time);
            particle->life.x -= (f32)delta_time;
            particle->life.x = max(particle->life.x, 0.0f);
            const f32 life_remaining = particle->life.x / particle->life.y;
            particle->dimension =
                v2_s_multi(particle->start_dimensions, life_remaining);
        }
        else
        {
            if (i != buffer->size - 1)
            {
                buffer->data[i] = buffer->data[buffer->size - 1];
            }
            buffer->size--;
            buffer->index = buffer->size;
            --i;
        }
    }
}

