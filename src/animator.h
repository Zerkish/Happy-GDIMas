#ifndef ANIMATOR_H
#define ANIMATOR_H
#include "graphics.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

typedef int animationId_t;

#define INVALID_ANIMATION -1

#define SIGN(X) ((X) / ((X) < 0 ? -(X) : (X)))

typedef struct animation_s {
    // How much time to spend per frame
    float frameLength;

    // Identifier for this whole animation
    int id;

    // The sequence of clips used in the animation.
    RECT *clips;
    int numClips;

    // Loop this animation?
    int loop;
} animation_t;

typedef struct animatedSprite_s {
    sprite_t sprite;
    ghandle_t spriteHandle;

    // Sequence of positions to move to, in order.
    const POINT *movementSequence;
    int movementSequenceLength;
    int nextPoint;
    float moveSpeed;

    animation_t *animations;
    int numAnimations;
    animationId_t current;

    // Should not be modified from outside.
    float _frameTime;
    int _frame;
    int _sequence;

} animatedSprite_t;

// Sets a new animation for the sprite.
__inline void AnimatorSetAnimation( animatedSprite_t *sprite, animationId_t id ) {
    assert( id >= 0 && id < sprite->numAnimations );
    sprite->current = id;
    sprite->_frameTime = 0.0f;
    sprite->_frame = 0;
    sprite->sprite.clip = sprite->animations[sprite->current].clips[sprite->_frame];
}


__inline void AnimatorUpdate( animatedSprite_t *sprite, float delta ) {
    if ( sprite->current == INVALID_ANIMATION )
        return;

    sprite->_frameTime += delta;

    if ( sprite->current != INVALID_ANIMATION ) {

        if ( sprite->_frameTime > sprite->animations[sprite->current].frameLength ) {

            ++sprite->_frame;

            if ( sprite->animations[sprite->current].numClips == sprite->_frame &&
                sprite->animations[sprite->current].loop ) {

                sprite->_frame = 0;
                sprite->sprite.clip = sprite->animations[sprite->current].clips[sprite->_frame];
            } else if ( sprite->animations[sprite->current].numClips == sprite->_frame ) {
                sprite->current = INVALID_ANIMATION;
            } else {
                sprite->sprite.clip = sprite->animations[sprite->current].clips[sprite->_frame];
            }

            GraphicsUpdateSprite( sprite->spriteHandle, &sprite->sprite );
            sprite->_frameTime = 0.0f;
        }
    }

    // Linear movement.
    if ( sprite->movementSequenceLength > 0 && sprite->nextPoint < sprite->movementSequenceLength ) {
        int sx = (int)sprite->sprite.x;
        int sy = (int)sprite->sprite.y;

        const POINT *next = &sprite->movementSequence[sprite->nextPoint];

        if ( sx == next->x && sy == next->y ) {
            ++sprite->nextPoint;
        } else {
            float diffx = (float)( next->x - sx );
            float diffy = (float)( next->y - sy );

            if ( fabsf( diffx ) > 0 || fabsf( diffy ) > 0 ) {
                float dx = 0;
                float dy = 0;
                float len = sqrtf( diffx * diffx + diffy * diffy );

                if ( len > 0 ) {
                    dx = diffx / len;
                    dy = diffy / len;
                } else {
                    assert( 0 );
                }

                dx *= sprite->moveSpeed * delta;
                dy *= sprite->moveSpeed * delta;

                // Make sure we don't overshoot the target.
                if ( fabsf( diffx ) < fabsf( dx ) || fabsf( diffy ) < fabsf( dy ) ) {
                    sprite->sprite.x = (float)next->x;
                    sprite->sprite.y = (float)next->y;
                } else {
                    sprite->sprite.x += dx;
                    sprite->sprite.y += dy;
                }
            }
            GraphicsUpdateSprite( sprite->spriteHandle, &sprite->sprite );

        }
    }

}

#endif // !ANIMATOR_H