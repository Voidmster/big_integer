#ifndef BIGINT_OPT_VECTOR_H
#define BIGINT_OPT_VECTOR_H

#include <vector>
#include <iosfwd>
#include <limits>
#include <stdint.h>

struct vector_with_opt
{
    struct vector_with_link
    {
        std::vector<uint32_t> data;
        size_t link_count;

        vector_with_link(uint32_t elem, size_t new_size)
        {
            data.resize(new_size);
            data[0] = elem;
            link_count = 1;
        }

        vector_with_link(uint32_t elem1, uint32_t elem2)
        {
            data.push_back(elem1);
            data.push_back(elem2);
            link_count = 1;
        }

        vector_with_link(std::vector<uint32_t> new_data)
        {
            data = new_data;
            link_count = 1;
        }
    };

private:
    union
    {
        vector_with_link *big_object;
        uint32_t small_obj;
    };
    size_t v_size;
    bool is_big_obj;
    void make_own_copy();
    void safe_delete();

public:
    vector_with_opt();
    ~vector_with_opt();

    vector_with_opt &operator=(vector_with_opt const &other);
    uint32_t& operator[](size_t index);
    uint32_t const& operator[](size_t index) const;

    void push_back(uint32_t elem);
    void resize(size_t new_size);
    void pop_back();
    size_t size() const;

    uint32_t &back();
};
#endif //BIGINT_OPT_VECTOR_H
