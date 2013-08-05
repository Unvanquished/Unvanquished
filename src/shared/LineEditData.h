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

#ifndef SHARED_LINE_EDIT_DATA_H_
#define SHARED_LINE_EDIT_DATA_H_

//FIXME: the namespace and the class name aren't very explicit
//TODO: use unicode
namespace Util {

    class LineEditData {
        public:
            static const int defaultScrollSize = 16;

            LineEditData(int size, int scrollSize = defaultScrollSize);
            ~LineEditData();

            const char* GetText();
            const char* GetViewText();
            int GetCursorPos();
            int GetViewCursorPos();

            void CursorLeft();
            void CursorRight();
            void CursorStart();
            void CursorEnd();

            void DeleteNext();
            void DeletePrev();

            void AddChar(char a);

            void Clear();

            void SetWidth(int width);

        protected:
            char* buffer;
            int bufferSize;
            int scrollSize;
            int width;
            int scroll = 0;
            int cursor = 0;
    };

}

#endif // SHARED_LINE_EDIT_DATA_H_
