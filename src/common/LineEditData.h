/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2013 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include <string>

#ifndef COMMON_LINE_EDIT_DATA_H_
#define COMMON_LINE_EDIT_DATA_H_

//FIXME: the namespace and the class name aren't very explicit
namespace Util {

    class LineEditData {
        public:
            static const int defaultScrollSize = 16;

            LineEditData(int size, int scrollSize = defaultScrollSize);

            const std::u32string& GetText() const;
            std::u32string& GetText();
            const char32_t* GetViewText() const;
            unsigned GetViewStartPos() const;
            unsigned GetCursorPos() const;
            unsigned GetViewCursorPos() const;

            void SetText(std::u32string text);

            void CursorLeft(int times = 1);
            void CursorRight(int times = 1);
            void CursorStart();
            void CursorEnd();
            void SetCursor(int pos);

            void DeleteNext(int times = 1);
            void DeletePrev(int times = 1);

            void AddChar(char32_t a);

            void Clear();

            void SetWidth(int width);
            unsigned GetWidth() const;

        private:
            void UpdateScroll();
            std::u32string buffer;
            int scrollSize;
            unsigned width;
            unsigned scroll;
            unsigned cursor;
    };

}

#endif // COMMON_LINE_EDIT_DATA_H_
