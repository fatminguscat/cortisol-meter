#pragma once

#include <Geode/Geode.hpp>

namespace cm {

// Loads a supplied PNG from resources/ (pass it through _spr). Returns nullptr when the art is
// missing, so the meter can fall back to its plain bar. Nothing here animates.
// `pixelArt` turns off texture smoothing so blocky sprites stay crisp when scaled.
cocos2d::CCSprite* loadArt(char const* sprName, bool pixelArt);

// An empty node of `box` size whose anchor point is `anchor`, so it can be positioned by the
// screen corner/edge it hugs and scaled away from that corner.
cocos2d::CCNode* makePanel(cocos2d::CCSize box, cocos2d::CCPoint anchor);

// Adds `art` to `parent`, scaled uniformly to fit inside `box` (letterboxed, never stretched)
// and aligned inside the box at `align` (0..1 on each axis).
void placeArt(cocos2d::CCNode* parent, cocos2d::CCNode* art, cocos2d::CCRect box, cocos2d::CCPoint align);

}
