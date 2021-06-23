#pragma once

#include <stdint.h>

struct Outfit
{
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
        uint8_t item = 0;
        uint8_t addon = 0;
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

    Outfit(uint16_t looktype)
    {
        look.type = looktype;
    }

    Outfit(Look look)
        : look(look) {}
};