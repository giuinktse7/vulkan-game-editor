#pragma once

#include <stdint.h>

#include "logger.h"
#include "util.h"

struct Outfit
{
    enum class Addon : uint8_t
    {
        None = 0,
        First = 1,
        Second = 1 << 1
    };

    struct Look
    {
        void setHead(uint8_t head)
        {
            data.looks.head = head;
        }
        void setBody(uint8_t body)
        {
            data.looks.body = body;
        }
        void setLegs(uint8_t legs)
        {
            data.looks.legs = legs;
        }
        void setFeet(uint8_t feet)
        {
            data.looks.feet = feet;
        }

        uint8_t head() const noexcept
        {
            return data.looks.head;
        }
        uint8_t body() const noexcept
        {
            return data.looks.body;
        }
        uint8_t legs() const noexcept
        {
            return data.looks.legs;
        }
        uint8_t feet() const noexcept
        {
            return data.looks.feet;
        }

        uint16_t type = 0;
        uint16_t mount = 0;
        uint8_t item = 0;
        Addon addon = Addon::None;
        union
        {
            struct
            {
                uint8_t head;
                uint8_t body;
                uint8_t legs;
                uint8_t feet;
            } looks;
            uint32_t id = 0;
        } data;
    } look;

    uint32_t id() const noexcept
    {
        return look.data.id;
    }

    bool isOnlyLooktype() const noexcept
    {
        return look.data.id == 0 && look.addon == Addon::None && look.mount == 0 && look.item == 0 && look.type != 0;
    }

    Outfit(uint16_t looktype)
    {
        look.type = looktype;
    }

    Outfit(uint16_t looktype, uint8_t lookhead, uint8_t lookbody, uint8_t looklegs, uint8_t lookfeet, Addon addons = Outfit::Addon::None, uint16_t mountType = 0)
    {
        look.type = looktype;
        look.data.looks.head = lookhead;
        look.data.looks.body = lookbody;
        look.data.looks.legs = looklegs;
        look.data.looks.feet = lookfeet;
        look.addon = addons;
        look.mount = mountType;
    }

    Outfit(Look look)
        : look(look) {}
};

VME_ENUM_OPERATORS(Outfit::Addon)