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

#include <QFileInfo>
#include <QString>

#include <vector>

// ------------------------------------------------------------------------

class Files
{
public:

    [[nodiscard]] QString absolutePath() const { return m_files[m_current].absoluteFilePath(); }
    [[nodiscard]] std::size_t count() const noexcept { return m_files.size(); }
    [[nodiscard]] QString directory() const noexcept { return m_directory; }
    [[nodiscard]] int index() const noexcept { return m_current; }
    [[nodiscard]] QString path() const { return m_files[m_current].filePath(); }
    [[nodiscard]] bool haveImages() const noexcept { return m_current != INVALID_INDEX; }
    void setDirectory(const QString& directory) { m_directory = directory; }

    void next(bool step = false) noexcept;
    void openDirectory(const QString& directory);
    void previous(bool step = false) noexcept;
    [[nodiscard]] bool readDirectory();

private:

    static const int INVALID_INDEX{-1};

    int m_current{INVALID_INDEX};
    QString m_directory{};
    std::vector<QFileInfo> m_files{};
};

