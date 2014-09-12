// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

#ifndef COMPONENTS_ENTITIES_H_
#define COMPONENTS_ENTITIES_H_

#include "Components.h"

    #include "DeferedFreeingComponent.h"
    #include "IgnitableComponent.h"

// Entity definitions


    // Definition of EmptyEntity

    class EmptyEntity: public Entity {
            static const MessageHandler messageHandlers[];
            static const int componentOffsets[];

        public:
            EmptyEntity(
                     struct gentity_s* oldEnt
            );
            virtual ~EmptyEntity();

    };


    // Definition of FireEntity

    class FireEntity: public Entity {
            static const MessageHandler messageHandlers[];
            static const int componentOffsets[];

        public:
            FireEntity(
                     struct gentity_s* oldEnt
            );
            virtual ~FireEntity();

                DeferedFreeingComponent c_DeferedFreeingComponent;
                IgnitableComponent c_IgnitableComponent;
    };


    // Definition of AlienBuildableEntity

    class AlienBuildableEntity: public Entity {
            static const MessageHandler messageHandlers[];
            static const int componentOffsets[];

        public:
            AlienBuildableEntity(
                     struct gentity_s* oldEnt
            );
            virtual ~AlienBuildableEntity();

                IgnitableComponent c_IgnitableComponent;
    };



#endif // COMPONENTS_ENTITIES_H_
