 
#ifndef _GAMEOBJECT_MODEL_H
#define _GAMEOBJECT_MODEL_H

#include <G3D/Matrix3.h>
#include <G3D/Vector3.h>
#include <G3D/AABox.h>
#include <G3D/Ray.h>

#include "Define.h"

namespace VMAP
{
    class WorldModel;
}

class GameObject;
struct GameObjectDisplayInfoEntry;

class GameObjectModel /*, public Intersectable*/
{
    uint32 phasemask;
    G3D::AABox iBound;
    G3D::Matrix3 iInvRot;
    G3D::Vector3 iPos;
    //G3D::Vector3 iRot;
    float iInvScale;
    float iScale;
    VMAP::WorldModel* iModel;

    GameObjectModel() : phasemask(0), iModel(NULL) {}
    bool initialize(const GameObject& go, const GameObjectDisplayInfoEntry& info);

public:
    std::string name;

    const G3D::AABox& getBounds() const { return iBound; }

    ~GameObjectModel();

    const G3D::Vector3& getPosition() const { return iPos;}

    /**	Enables\disables collision. */
    void disable() { phasemask = 0;}
    void enable(uint32 ph_mask) { phasemask = ph_mask;}

    bool intersectRay(const G3D::Ray& Ray, float& MaxDist, bool StopAtFirstHit, uint32 ph_mask) const;

    static GameObjectModel* Create(const GameObject& go);
};

#endif // _GAMEOBJECT_MODEL_H