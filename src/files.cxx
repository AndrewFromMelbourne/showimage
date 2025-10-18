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

#include <QDirIterator>

#include "files.h"

//-------------------------------------------------------------------------

void
Files::next(bool step) noexcept
{
    if (not haveImages())
    {
        return;
    }

    if (step)
    {
        m_current += 10;
        if (m_current >= m_files.size())
        {
            m_current = 0;
        }
    }
    else
    {
        m_current = (m_current == m_files.size() - 1) ? 0 : m_current + 1;
    }
}

//-------------------------------------------------------------------------

void
Files::previous(bool step) noexcept
{
    if (not haveImages())
    {
        return;
    }

    if (step)
    {
        m_current -= 10;
        if (m_current < 0)
        {
            m_current = m_files.size() - 1;
        }
    }
    else
    {
        m_current = (m_current == 0) ? m_files.size() - 1 : m_current - 1;
    }
}

//-------------------------------------------------------------------------

bool
Files::readDirectory()
{
    m_files.clear();

    if (m_directory.length() > 0)
    {
        QDirIterator iter(m_directory,
                          {"*.bmp", "*.gif", "*.jpg", "*.jpeg", "*.png"},
                          QDir::Files,
                          QDirIterator::Subdirectories);
        while (iter.hasNext())
        {
            auto fileInfo = iter.nextFileInfo();

            if (fileInfo.isFile())
            {
                m_files.push_back(fileInfo);
            }
        }
    }

    if (m_files.size() > 0)
    {
        std::ranges::sort(
            m_files,
            [](const auto& lhs, const auto& rhs)
            {
                return lhs.absoluteFilePath() < rhs.absoluteFilePath();
            });

        m_current = 0;
        return true;
    }

    m_current = INVALID_INDEX;
    return false;
}

