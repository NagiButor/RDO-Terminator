#pragma once

namespace YimMenu::Features::LightningOptions
{
    // distance (metres) from the target where the bolt hits, 0 = right on target
    float& StrikeDistance();
    // strikes the currently selected player (must be called on a script fiber)
    void StrikeNow();
    // strikes an exact world point with flash + thunder + damage (must be called on a script fiber)
    void StrikeAtExact(float x, float y, float z);
}
