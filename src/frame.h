//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2025 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

#pragma once

// ------------------------------------------------------------------------

class Frame
{
public:

    [[nodiscard]] bool next() noexcept
    {
        if (m_max == 0)
        {
            return false;
        }

        m_index = (m_index == m_max) ? 0 : m_index + 1;
        return true;
    }

    [[nodiscard]] bool previous() noexcept
    {
        if (m_max == 0)
        {
            return false;
        }

        m_index = (m_index == 0) ? m_max : m_index - 1;
        return true;
    }

    void set(int max) noexcept
    {
        m_index = 0;
        m_max = max;
    }

    int index() const noexcept { return m_index; }
    int max() const noexcept { return m_max; }

private:

    int m_index{};
    int m_max{};
};

