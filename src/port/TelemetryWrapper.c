#include "global.h"
#include "port/TelemetryRaw.h"

// Exports the player's state with roll/pitch split into their game components.
// NOTE: the game computes bankAngle = rot.z + zRotBank + zRotBarrelRoll, so we never
// use bankAngle directly (the old code added rot.z twice and included the barrel roll spin).
void GetTelemetryRaw(TelemetryRaw* out) {
    memset(out, 0, sizeof(*out));

    out->paused = (gPlayState == PLAY_PAUSE);
    if (gPlayer == NULL) {
        return;
    }

    Player* p = &gPlayer[0];
    out->playerState = p->state;
    out->form = p->form;
    out->levelMode = gLevelMode;

    out->active = (gPlayState == PLAY_UPDATE) &&
                  (p->state == PLAYERSTATE_ACTIVE || p->state == PLAYERSTATE_U_TURN) &&
                  (p->form != FORM_ON_FOOT);

    out->pos[0] = p->pos.x;
    out->pos[1] = p->pos.y;
    out->pos[2] = -p->trueZpos;
    out->vel[0] = p->vel.x;
    out->vel[1] = p->vel.y;
    out->vel[2] = p->vel.z;
    out->baseSpeed = p->baseSpeed;
    out->boostSpeed = p->boostSpeed;
    out->knockback[0] = p->knockback.x;
    out->knockback[1] = p->knockback.y;
    out->knockback[2] = p->knockback.z;

    out->pitchStick = p->rot.x + p->xRot_120;
    out->pitchAerobatic = p->aerobaticPitch;
    out->yaw = p->rot.y;
    out->rollStick = p->rot.z;
    out->rollBank = p->zRotBank;
    out->rollBarrel = p->zRotBarrelRoll;

    out->rollState = p->rollState;
    out->rollRate = p->rollRate;
    out->boostActive = p->boostActive;
    out->somersault = p->somersault;
    out->damage = p->damage;
    out->hitTimer = p->hitTimer;
}
