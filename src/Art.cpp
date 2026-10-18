#include "Art.hpp"

#include <algorithm>

using namespace geode::prelude;

namespace cm {

CCSprite* loadArt(char const* sprName, bool pixelArt) {
    auto sprite = CCSprite::create(sprName);
    // Geode swaps a missing file for a checkerboard instead of returning nullptr.
    if (!sprite || sprite->isUsingFallback() || !sprite->getTexture()) {
        log::warn("missing art {}, skipping whatever needs it", sprName);
        return nullptr;
    }
    if (pixelArt) sprite->getTexture()->setAliasTexParameters();
    return sprite;
}

CCNode* makePanel(CCSize box, CCPoint anchor) {
    auto panel = CCNode::create();
    panel->setContentSize(box);
    panel->setAnchorPoint(anchor);
    return panel;
}

void placeArt(CCNode* parent, CCNode* art, CCRect box, CCPoint align) {
    auto size = art->getContentSize();
    if (size.width <= 0.f || size.height <= 0.f) return;
    art->setScale(std::min(box.size.width / size.width, box.size.height / size.height));
    art->setAnchorPoint(align);
    art->setPosition({
        box.origin.x + box.size.width * align.x,
        box.origin.y + box.size.height * align.y,
    });
    parent->addChild(art);
}

}
