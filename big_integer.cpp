#include "big_integer.h"
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <iostream>

using namespace std;

static const big_integer B_ZERO = big_integer(0);
static const big_integer B_ONE = big_integer(1);

big_integer::big_integer()
{
    data.push_back(0);
    sign = true;
}

big_integer::big_integer(big_integer const &other)
{
    this->data = other.data;
    this->sign = other.sign;
}

big_integer::big_integer(int a)
{
    int64_t res = a;
    sign = true;
    if (res < 0)
    {
        sign = false;
        res = -res;
    }
    data.push_back(static_cast<uint32_t>(res % BASE));
    data.push_back(static_cast<uint32_t>((res / BASE) % BASE));
    delete_zeroes();
}

big_integer::big_integer(uint32_t a)
{
    data.push_back(a);
    sign = true;
}

big_integer::big_integer(std::string const &str) : big_integer()
{
    size_t begin_of_str = 0;

    if (str[0] == '-')
    {
        this->sign = false;
        begin_of_str = 1;
    }

    uint32_t digit;
    for (size_t i = begin_of_str; i < str.size(); i++)
    {
        digit = static_cast<uint32_t>(str[i] - '0');
        this->mul_long_short(10);
        this->add_long_short(digit);
    }

    delete_zeroes();
}

big_integer::~big_integer()
{ }

big_integer &big_integer::operator =(big_integer const &other)
{
    this->data = other.data;
    this->sign = other.sign;

    return *this;
}

big_integer &big_integer::operator +=(big_integer const &rhs)
{
    if (this->sign == rhs.sign) return this->abs_add(rhs);
    if (this->compare_by_abs(rhs) >= 0)
    {
        return this->abs_sub(rhs, false);
    }
    else
    {
        this->sign = !this->sign;
        return this->abs_sub(rhs, true);
    }
}

big_integer &big_integer::operator-=(big_integer const &rhs)
{
    if (this->sign != rhs.sign) return this->abs_add(rhs);
    if (this->compare_by_abs(rhs) >= 0)
    {
        return this->abs_sub(rhs, false);
    }
    else
    {
        this->sign = !this->sign;
        return this->abs_sub(rhs, true);
    }
}

big_integer &big_integer::abs_sub(big_integer rhs, bool swap)
{
    big_integer *right_op;
    big_integer *left_op;
    uint32_t carry = 0;
    uint32_t right;
    uint32_t left;
    uint32_t res;
    size_t lim;

    if (!swap)
    {
        left_op = this;
        right_op = &rhs;
        lim = rhs.data.size();
    }
    else
    {
        left_op = &rhs;
        right_op = this;
        lim = this->data.size();
    }

    for (size_t i = 0; i < lim || carry; i++)
    {
        left = (left_op->data[i]);
        right = carry + (i < right_op->data.size() ? right_op->data[i] : 0);

        if (left < right) {
            carry = 1;
            res = left + static_cast<uint32_t> (BASE - right);
        }
        else
        {
            carry = 0;
            res = left - right;
        }

        this->data[i] = static_cast<uint32_t> (res);
    }

    delete_zeroes();
    return *this;
}

big_integer &big_integer::abs_add(big_integer const &other)
{
    uint64_t carry = 0;
    size_t const ma = std::max(this->data.size(), other.data.size());
    for (size_t i = 0; i < ma || carry; i++)
    {
        if (i == this->data.size()) this->data.push_back(0);
        uint64_t a = static_cast<int64_t> (this->data[i]) +
                     static_cast<int64_t> (i < other.data.size() ? other.data[i] : 0) + carry;
        this->data[i] = static_cast<uint32_t> (a % BASE);
        carry = a / BASE;
    }

    this->delete_zeroes();
    return *this;
}

big_integer &big_integer::operator*=(big_integer const &rhs)
{
    this->sign = this->sign == rhs.sign;
    if (rhs.data.size() == 1)
    {
        this->mul_long_short(rhs.data[0]);
        return *this;
    }

    uint64_t t, mul_carry = 0, add_carry = 0;

    for (std::ptrdiff_t i = this->data.size() - 1; i >= 0; i--)
    {
        mul_carry = 0;
        uint64_t mul = this->data[i];
        for (size_t j = 0; j < rhs.data.size(); j++)
        {
            t = mul * static_cast<int64_t>(rhs.data[j]) + mul_carry;
            mul_carry = t / BASE;
            t = t % BASE;

            if (j == 0) this->data[i] = 0;
            size_t k = i + j;
            if (k == this->data.size()) this->data.push_back(0);
            uint64_t tmp2 = static_cast<uint64_t>(this->data[k]) + t;
            add_carry = tmp2 / BASE;
            tmp2 = tmp2 % BASE;

            this->data[k] = static_cast<uint32_t> (tmp2);

            while (add_carry != 0)
            {
                k++;
                if (k == this->data.size()) this->data.push_back(0);
                tmp2 = static_cast<uint64_t>(this->data[k]) + add_carry;
                add_carry = tmp2 / BASE;
                this->data[k] = static_cast<uint32_t> (tmp2 % BASE);
            }
        }

        size_t k = i + rhs.data.size();
        while (mul_carry != 0)
        {
            if (k == this->data.size()) this->data.push_back(0);
            uint64_t tmp2 = static_cast<uint64_t>(this->data[k]) + mul_carry;
            mul_carry = tmp2 / BASE;
            this->data[k] = static_cast<uint32_t> (tmp2 % BASE);
            k++;
        }
    }
    delete_zeroes();

    return *this;
}

big_integer &big_integer::operator/=(big_integer const &rhs)
{
    if (this->compare_by_abs(rhs) < 0)
    {
        return *this = big_integer(0);
    }

    bool sign = this->sign == rhs.sign;

    if (rhs.data.size() == 1)
    {
        this->div_and_mod_by_short(rhs.data[0]);
        this->sign = sign;
        return *this;
    }

    this->sign = true;
    big_integer divn(rhs);
    divn.sign = true;

    uint32_t normalize = static_cast<uint32_t> (BASE / (divn.data.back() + 1));
    this->mul_long_short(normalize);
    divn.mul_long_short(normalize);

    uint32_t cached = divn.data.back();
    size_t n = divn.data.size();
    size_t m = this->data.size() - divn.data.size();

    big_integer res;
    res.data.resize(m + 1);

    big_integer shifted(divn);
    shifted <<= (m * 32);

    if (*this >= shifted)
    {
        res.data[m] = 1;
        *this -= shifted;
    }
    else
    {
        res.data[m] = 0;
    }

    for (std::ptrdiff_t i = m - 1; i >= 0; --i)
    {
        uint64_t cur = BASE * static_cast<int64_t>(this->data[n + i])
                       + static_cast<int64_t>(this->data[n + i - 1]);
        cur /= static_cast<int64_t>(cached);
        if (cur > BASE - 1)
        {
            cur = BASE - 1;
        }
        res.data[i] = static_cast<uint32_t>(cur);

        shifted >>= 32;
        big_integer k = shifted * static_cast<uint32_t>(cur);
        *this -= (k);

        while (*this < B_ZERO)
        {
            res.data[i]--;
            *this += shifted;
        }

    }

    *this = res;
    this->sign = sign;
    delete_zeroes();

    return *this;
}

big_integer &big_integer::operator%=(big_integer const &rhs)
{
    return *this -= ((*this / rhs) * rhs);
}

big_integer &big_integer::operator&=(big_integer const &rhs)
{
    big_integer right_op(rhs);
    uint32_t left, right;

    this->convert();
    right_op.convert();

    size_t end_of_digits = std::max(this->data.size(), right_op.data.size());
    for (size_t i = 0; i < end_of_digits; ++i)
    {
        left = (i < this->data.size() ? this->data[i] : (this->sign ? 0 : std::numeric_limits<uint32_t>::max()));
        right = (i < right_op.data.size() ? right_op.data[i] : (right_op.sign ? 0 : std::numeric_limits<uint32_t>::max()));
        if (i == this->data.size()) this->data.push_back(0);
        this->data[i] = left & right;
    }

    this->sign = this->sign || right_op.sign;

    this->convert();
    delete_zeroes();

    return *this;
}

big_integer &big_integer::operator|=(big_integer const &rhs)
{
    big_integer right_op(rhs);
    uint32_t left, right;

    this->convert();
    right_op.convert();

    size_t end_of_digits = std::max(this->data.size(), right_op.data.size());

    for (size_t i = 0; i < end_of_digits; ++i)
    {
        left = (i < this->data.size() ? this->data[i] : (this->sign ? 0 : std::numeric_limits<uint32_t>::max()));
        right = (i < right_op.data.size() ? right_op.data[i] : (right_op.sign ? 0 : std::numeric_limits<uint32_t>::max()));
        if (i == this->data.size()) this->data.push_back(0);
        this->data[i] = left | right;
    }

    this->sign = this->sign && right_op.sign;

    this->convert();
    delete_zeroes();

    return *this;
}

big_integer &big_integer::operator^=(big_integer const &rhs)
{
    big_integer right_op(rhs);
    uint32_t left, right;

    right_op.convert();
    this->convert();

    size_t end_of_digits = std::max(this->data.size(), right_op.data.size());

    for (size_t i = 0; i < end_of_digits; ++i)
    {
        left = (i < this->data.size() ? this->data[i] : (this->sign ? 0 : std::numeric_limits<uint32_t>::max()));
        right = (i < right_op.data.size() ? right_op.data[i] : (right_op.sign ? 0 : std::numeric_limits<uint32_t>::max()));
        if (i == this->data.size()) this->data.push_back(0);
        this->data[i] = left ^ right;
    }

    this->sign = !(this->sign ^ right_op.sign);

    this->convert();
    delete_zeroes();

    return *this;
}

big_integer &big_integer::operator<<=(int rhs)
{
    if (rhs == 0 || is_zero()) return *this;
    if (rhs < 0) return this->operator>>=(-rhs);
    uint32_t blocks = static_cast<uint32_t>(rhs) / 32;
    uint32_t bytes = static_cast<uint32_t>(rhs) % 32;
    data.push_back(0);

    if (bytes != 0)
    {
        for (size_t i = data.size() - 1; i > 0; --i)
        {
            uint32_t next, cur;
            next = ~((1U << (32U - bytes)) - 1U);
            next &= data[i - 1];
            next >>= (32 - bytes);

            cur = data[i - 1];
            cur <<= bytes;

            data[i - 1] = cur;
            data[i] |= next;
        }
    }

    size_t size0 = data.size();
    data.resize(data.size() + blocks);

    for (std::ptrdiff_t i = size0 + blocks - 1; i >= blocks; --i)
    {
        data[i] = data[i - blocks];
    }
    for (size_t i = 0; i < blocks; ++i)
    {
        data[i] = 0;
    }

    delete_zeroes();
    return *this;
}

big_integer &big_integer::operator>>=(int rhs)
{
    if (rhs == 0 || is_zero()) return *this;
    if (rhs < 0) return this->operator<<=(-rhs);

    uint32_t blocks = static_cast<uint32_t>(rhs) / 32;
    uint32_t bytes = static_cast<uint32_t>(rhs) % 32;

    this->convert();

    if (bytes != 0)
    {
        for (size_t i = blocks; i < data.size(); i++)
        {
            uint32_t cur, prev;
            cur = data[i];
            cur >>= bytes;

            if (i == data.size() - 1 && !this->sign)
            {
                uint32_t mask = ~((1U << (32U - bytes)) - 1U);
                cur |= mask;
            }

            data[i] = cur;
            if (i < data.size() - 1)
            {
                prev = (1U << bytes) - 1;
                prev &= data[i + 1];
                prev <<= (32 - bytes);
                data[i] |= prev;
            }
        }
    }

    for (size_t i = blocks; i < data.size(); i++)
    {
        data[i - blocks] = data[i];
    }

    uint32_t mask = (this->sign ? 0 : (std::numeric_limits<uint32_t>::max() - 1));
    for (size_t i = 0; i < blocks; i++)
    {
        data[data.size() - i - 1] = mask;
    }

    this->convert();
    delete_zeroes();

    return *this;
}

big_integer big_integer::operator+() const
{
    return *this;
}

big_integer big_integer::operator-() const
{
    big_integer r(*this);
    r.sign = !r.sign;
    return r;
}

big_integer big_integer::operator~() const
{
    big_integer r(*this);
    ++r;
    r.sign = !r.sign;
    return r;
}

big_integer &big_integer::operator++()
{
    this->operator+=(B_ONE);
    return *this;
}

big_integer big_integer::operator++(int)
{
    big_integer r = *this;
    ++*this;
    return r;
}

big_integer &big_integer::operator--()
{
    this->operator-=(B_ONE);
    return *this;
}

big_integer big_integer::operator--(int)
{
    big_integer r = *this;
    --*this;
    return r;
}

std::string to_string(big_integer const &a) {
    if (a.is_zero())
    {
        return "0";
    }

    big_integer x(a);
    std::string res = "";

    while (!x.is_zero())
    {
        int8_t digit = (int8_t)x.div_and_mod_by_short(10);
        res.push_back((int8_t)(digit) + (int8_t) '0');
    }

    while (res[res.size() - 1] == '0')
    {
        res.pop_back();
    }
    if (!x.sign) res.push_back('-');

    std::reverse(res.begin(), res.end());
    return res;
}

big_integer operator+(big_integer a, big_integer const &b)
{
    return a += b;
}

big_integer operator-(big_integer a, big_integer const &b)
{
    return a -= b;
}

big_integer operator*(big_integer a, big_integer const &b)
{
    return a *= b;
}

big_integer operator/(big_integer a, big_integer const &b)
{
    return a /= b;
}

big_integer operator%(big_integer a, big_integer const &b)
{
    return a %= b;
}

big_integer operator&(big_integer a, big_integer const &b)
{
    return a &= b;
}

big_integer operator|(big_integer a, big_integer const &b)
{
    return a |= b;
}

big_integer operator^(big_integer a, big_integer const &b)
{
    return a ^= b;
}

big_integer operator<<(big_integer a, int b)
{
    return a <<= b;
}

big_integer operator>>(big_integer a, int b)
{
    return a >>= b;
}

bool operator==(big_integer const &a, big_integer const &b)
{

    return a.compare_to(b) == 0;
}

bool operator!=(big_integer const &a, big_integer const &b)
{
    return a.compare_to(b) != 0;
}

bool operator<(big_integer const &a, big_integer const &b)
{
    return a.compare_to(b) < 0;
}

bool operator>(big_integer const &a, big_integer const &b)
{
    return a.compare_to(b) > 0;
}

bool operator<=(big_integer const &a, big_integer const &b)
{
    return a.compare_to(b) <= 0;
}

bool operator>=(big_integer const &a, big_integer const &b)
{
    return a.compare_to(b) >= 0;
}

std::ostream &operator<<(std::ostream &s, big_integer const &a)
{
    return s << to_string(a);
}

int8_t big_integer::compare_to(big_integer const &other) const
{
    if (this->is_zero() && other.is_zero()) return 0;
    if (this->sign && !other.sign) return 1;
    if (!this->sign && other.sign) return -1;
    if (this->sign && other.sign)
    {
        if (this->data.size() > other.data.size()) return 1;
        if (this->data.size() < other.data.size()) return -1;
        for (size_t i = this->data.size(); i > 0; --i)
        {
            if (this->data[i - 1] > other.data[i - 1]) return 1;
            if (this->data[i - 1] < other.data[i - 1]) return -1;
        }
    }
    else
    {
        if (this->data.size() > other.data.size()) return -1;
        if (this->data.size() < other.data.size()) return 1;
        for (size_t i = this->data.size(); i > 0; --i)
        {
            if (this->data[i - 1] > other.data[i - 1]) return -1;
            if (this->data[i - 1] < other.data[i - 1]) return 1;
        }
    }
    return 0;
}

int8_t big_integer::compare_by_abs(big_integer const &other) const
{
    if (this->data.size() > other.data.size()) return 1;
    if (this->data.size() < other.data.size()) return -1;
    for (size_t i = this->data.size(); i > 0; --i)
    {
        if (this->data[i - 1] > other.data[i - 1]) return 1;
        if (this->data[i - 1] < other.data[i - 1]) return -1;
    }
    return 0;
}

bool big_integer::is_zero() const
{
    return (this->data.size() == 1 && this->data[0] == 0);
}

big_integer &big_integer::convert()
{
    if (!this->sign)
    {
        this->abs_sub(B_ONE, false);
        for (size_t i = 0; i < data.size(); ++i)
        {
            data[i] = ~data[i];
        }
    }
    return *this;
}

big_integer &big_integer::add_long_short(uint32_t x)
{
    uint64_t tmp, carry = 0;
    tmp = static_cast<uint64_t>(this->data[0]) + static_cast<uint64_t>(x);
    carry = tmp / BASE;
    this->data[0] = static_cast<uint32_t> (tmp % BASE);

    size_t k = 1;

    while (carry != 0)
    {
        if (k == this->data.size()) data.push_back(0);
        tmp = static_cast<uint64_t>(data[k]) + carry;
        carry = tmp / BASE;
        data[k] = static_cast<uint32_t> (tmp % BASE);
        ++k;
    }
    return *this;
}

big_integer &big_integer::mul_long_short(uint32_t x)
{
    uint64_t res, carry = 0;
    for (size_t i = 0; i < data.size(); i++)
    {
        res = static_cast<uint64_t>(this->data[i]) * static_cast<uint64_t>(x) + carry;
        carry = res / BASE;
        this->data[i] = static_cast<uint32_t> (res % BASE);

    }

    size_t k = data.size();

    while (carry != 0)
    {
        if (k == this->data.size()) data.push_back(0);
        res = static_cast<uint64_t>(data[k]) + carry;
        carry = res / BASE;
        data[k] = static_cast<uint32_t>(res % BASE);
        k++;
    }
    return *this;
}

uint32_t big_integer::div_and_mod_by_short(uint32_t x)
{
    uint64_t carry = 0;
    for (size_t i = this->data.size(); i > 0; --i)
    {
        uint64_t temp = static_cast<int64_t>(this->data[i - 1]) + carry * BASE;
        this->data[i - 1] = static_cast<uint32_t> (temp / static_cast<uint64_t>(x));
        carry = temp % static_cast<uint64_t>(x);
    }

    delete_zeroes();
    return static_cast<uint32_t> (carry);
}

void big_integer::delete_zeroes()
{
    while (this->data.size() > 1 && this->data.back() == 0)
    {
        this->data.pop_back();
    }
}

