#include <iostream>
#include "vector_with_opt.h"

using namespace std;

vector_with_opt::vector_with_opt()
{
    is_big_obj = false;
    small_obj = 0;
    v_size = 0;
}

vector_with_opt::~vector_with_opt()
{
    if (is_big_obj)
    {
        safe_delete();
    }
}

vector_with_opt &vector_with_opt::operator=(vector_with_opt const &other)
{
    if (this == &other)
        return *this;

    if (is_big_obj)
    {
        safe_delete();
    }

    if (!other.is_big_obj)
    {
        small_obj = other.small_obj;
    }
    else
    {
        other.big_object->link_count++;
        this->big_object = other.big_object;
    }
    this->v_size = other.v_size;
    this->is_big_obj = other.is_big_obj;
    return *this;
}

uint32_t &vector_with_opt::operator[](size_t index)
{
    if (is_big_obj)
    {
        make_own_copy();
        return big_object->data[index];
    }
    return small_obj;
}

uint32_t const &vector_with_opt::operator[](size_t index) const
{
    if (is_big_obj) return big_object->data[index];
    else return small_obj;
}

void vector_with_opt::resize(size_t new_size)
{
    if (is_big_obj)
    {
        make_own_copy();
        std::vector<uint32_t> buf(big_object->data);
        buf.resize(new_size);
        big_object->data = buf;
        v_size = new_size;
        return;
    }
    if (!is_big_obj && new_size >= 2)
    {
        uint32_t elem = small_obj;
        vector_with_link *new_v = new vector_with_link(elem, new_size);
        std::swap(new_v, big_object);

        v_size = new_size;
        is_big_obj = true;
        return;
    }
}

void vector_with_opt::push_back(uint32_t elem)
{
    if (is_big_obj)
    {
        make_own_copy();
        big_object->data.push_back(elem);
        ++v_size;
        return;
    }

    if (v_size == 0)
    {
        small_obj = elem;
        ++v_size;
    }
    else
    {
        uint32_t buff = small_obj;
        vector_with_link *new_v = new vector_with_link(buff, elem);
        std::swap(new_v, big_object);

        is_big_obj = true;
        v_size++;
    }
    return;
}

void vector_with_opt::pop_back()
{
    size_t new_size = v_size - 1;
    if (is_big_obj)
    {
        make_own_copy();
        big_object->data.pop_back();
        if (new_size == 1)
        {
            uint32_t buff = big_object->data[0];
            safe_delete();
            is_big_obj = false;
            small_obj = buff;
        }
        v_size = new_size;
    }
    return;
}

size_t vector_with_opt::size() const
{
    return v_size;
}

uint32_t &vector_with_opt::back()
{
    if (is_big_obj)
    {
        make_own_copy();
        return big_object->data.back();
    }
    return small_obj;
}

void vector_with_opt::safe_delete()
{
    if (big_object->link_count > 1)
    {
        big_object->link_count--;
    }
    else
    {
        delete big_object;
    }
}

void vector_with_opt::make_own_copy()
{
    if (big_object->link_count > 1)
    {
        vector_with_link *new_v = new vector_with_link(big_object->data);
        big_object->link_count--;
        big_object = new_v;
    }
}

