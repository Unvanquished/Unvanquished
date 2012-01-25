/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the OpenWolf GPL Source Code (OpenWolf Source Code).  

OpenWolf Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenWolf Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the OpenWolf Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the OpenWolf 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/

#include "cg_local.h"

// digibob
// Panel Handling
// ======================================================
panel_button_t *cg_focusButton;

void CG_PanelButton_RenderEdit(panel_button_t * button)
{
	qboolean        useCvar = button->data[0] ? qfalse : qtrue;
	int             offset = -1;

	if(useCvar)
	{
		char            buffer[256 + 1];

		trap_Cvar_VariableStringBuffer(button->text, buffer, sizeof(buffer));

		if(cg_focusButton == button && ((cg.time / 1000) % 2))
		{
			if(trap_Key_GetOverstrikeMode())
			{
				Q_strcat(buffer, sizeof(buffer), "^0|");
			}
			else
			{
				Q_strcat(buffer, sizeof(buffer), "^0_");
			}
		}
		else
		{
			Q_strcat(buffer, sizeof(buffer), " ");
		}

		do
		{
			offset++;
			if(buffer + offset == '\0')
			{
				break;
			}
		} while(CG_Text_Width_Ext(buffer + offset, button->font->scalex, 0, button->font->font) > button->rect.w);

		CG_Text_Paint_Ext(button->rect.x, button->rect.y + button->rect.h, button->font->scalex, button->font->scaley,
						  button->font->colour, va("^7%s", buffer + offset), 0, 0, button->font->style, button->font->font);

		//  CG_FillRect( button->rect.x, button->rect.y, button->rect.w, button->rect.h, colorRed );
	}
	else
	{
		int             maxlen = button->data[0];
		char           *s;

		if(cg_focusButton == button && ((cg.time / 1000) % 2))
		{
			if(trap_Key_GetOverstrikeMode())
			{
				s = va("^7%s^0|", button->text);
			}
			else
			{
				s = va("^7%s^0_", button->text);
			}
		}
		else
		{
			s = va("^7%s ", button->text);	// space hack to make the text not blink
		}

		do
		{
			offset++;
			if(s + offset == '\0')
			{
				break;
			}
		} while(CG_Text_Width_Ext(s + offset, button->font->scalex, 0, button->font->font) > button->rect.w);

		CG_Text_Paint_Ext(button->rect.x, button->rect.y + button->rect.h, button->font->scalex, button->font->scaley,
						  button->font->colour, s + offset, 0, 0, button->font->style, button->font->font);

		//  CG_FillRect( button->rect.x, button->rect.y, button->rect.w, button->rect.h, colorRed );
	}
}

qboolean CG_PanelButton_EditClick(panel_button_t * button, int key)
{
	if(key == K_MOUSE1)
	{
		if(!CG_CursorInRect(&button->rect) && cg_focusButton == button)
		{
			CG_PanelButtons_SetFocusButton(NULL);
			if(button->onFinish)
			{
				button->onFinish(button);
			}
			return qfalse;
		}
		else
		{
			CG_PanelButtons_SetFocusButton(button);
			return qtrue;
		}
	}
	else
	{
		char            buffer[256];
		char           *s;
		int             len, maxlen;
		qboolean        useCvar = button->data[0] ? qfalse : qtrue;

		if(useCvar)
		{
			maxlen = sizeof(buffer);
			trap_Cvar_VariableStringBuffer(button->text, buffer, sizeof(buffer));
			len = strlen(buffer);
		}
		else
		{
			maxlen = button->data[0];
			s = (char *)button->text;
			len = strlen(s);
		}

		if(key & K_CHAR_FLAG)
		{
			key &= ~K_CHAR_FLAG;

			if(key == 'h' - 'a' + 1)
			{					// ctrl-h is backspace
				if(len)
				{
					if(useCvar)
					{
						buffer[len - 1] = '\0';
						trap_Cvar_Set(button->text, buffer);
					}
					else
					{
						s[len - 1] = '\0';
					}
				}
				return qtrue;
			}

			if(key < 32)
			{
				return qtrue;
			}

			if(len >= maxlen - 1)
			{
				return qtrue;
			}

			if(useCvar)
			{
				buffer[len] = key;
				buffer[len + 1] = '\0';
				trap_Cvar_Set(button->text, buffer);
			}
			else
			{
				s[len] = key;
				s[len + 1] = '\0';
			}
			return qtrue;
		}
		else
		{
			// Gordon: FIXME: have this work with all our stuff (use data[x] to store cursorpos etc)
/*			if ( key == K_DEL || key == K_KP_DEL ) {
				if ( item->cursorPos < len ) {
					memmove( buff + item->cursorPos, buff + item->cursorPos + 1, len - item->cursorPos);
					DC->setCVar(item->cvar, buff);
				}
				return qtrue;
			}*/

/*			if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW )
			{
				if (editPtr->maxPaintChars && item->cursorPos >= editPtr->paintOffset + editPtr->maxPaintChars && item->cursorPos < len) {
					item->cursorPos++;
					editPtr->paintOffset++;
					return qtrue;
				}
				if (item->cursorPos < len) {
					item->cursorPos++;
				}
				return qtrue;
			}

			if ( key == K_LEFTARROW || key == K_KP_LEFTARROW )
			{
				if ( item->cursorPos > 0 ) {
					item->cursorPos--;
				}
				if (item->cursorPos < editPtr->paintOffset) {
					editPtr->paintOffset--;
				}
				return qtrue;
			}

			if ( key == K_HOME || key == K_KP_HOME) {// || ( tolower(key) == 'a' && trap_Key_IsDown( K_CTRL ) ) ) {
				item->cursorPos = 0;
				editPtr->paintOffset = 0;
				return qtrue;
			}

			if ( key == K_END || key == K_KP_END)  {// ( tolower(key) == 'e' && trap_Key_IsDown( K_CTRL ) ) ) {
				item->cursorPos = len;
				if(item->cursorPos > editPtr->maxPaintChars) {
					editPtr->paintOffset = len - editPtr->maxPaintChars;
				}
				return qtrue;
			}

			if ( key == K_INS || key == K_KP_INS ) {
				DC->setOverstrikeMode(!DC->getOverstrikeMode());
				return qtrue;
			}*/

			if(key == K_ENTER || key == K_KP_ENTER)
			{
				if(button->onFinish)
				{
					button->onFinish(button);
				}
			}
		}
	}

	return qtrue;
}

qboolean CG_PanelButtonsKeyEvent(int key, qboolean down, panel_button_t ** buttons)
{
	panel_button_t *button;

	if(cg_focusButton)
	{
		for(; *buttons; buttons++)
		{
			button = (*buttons);

			if(button == cg_focusButton)
			{
				if(button->onKeyDown && down)
				{
					if(!button->onKeyDown(button, key))
					{
						if(cg_focusButton)
						{
							return qfalse;
						}
					}
				}
				if(button->onKeyUp && !down)
				{
					if(!button->onKeyUp(button, key))
					{
						if(cg_focusButton)
						{
							return qfalse;
						}
					}
				}
			}
		}
	}

	if(down)
	{
		for(; *buttons; buttons++)
		{
			button = (*buttons);

			if(button->onKeyDown)
			{
				if(CG_CursorInRect(&button->rect))
				{
					if(button->onKeyDown(button, key))
					{
						return qtrue;
					}
				}
			}
		}
	}
	else
	{
		for(; *buttons; buttons++)
		{
			button = (*buttons);

			if(button->onKeyUp && CG_CursorInRect(&button->rect))
			{
				if(button->onKeyUp(button, key))
				{
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

void CG_PanelButtonsSetup(panel_button_t ** buttons)
{
	panel_button_t *button;

	for(; *buttons; buttons++)
	{
		button = (*buttons);

		if(button->shaderNormal)
		{
			button->hShaderNormal = trap_R_RegisterShaderNoMip(button->shaderNormal);
		}
	}
}

void CG_PanelButtonsRender(panel_button_t ** buttons)
{
	panel_button_t *button;

	for(; *buttons; buttons++)
	{
		button = (*buttons);

		if(button->onDraw)
		{
			button->onDraw(button);
		}
	}
}

void CG_PanelButtonsRender_Text(panel_button_t * button)
{
	if(!button->font)
	{
		return;
	}

	CG_Text_Paint_Ext(button->rect.x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour,
					  button->text, 0, 0, button->font->style, button->font->font);
}

void CG_PanelButtonsRender_Img(panel_button_t * button)
{
	CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, button->hShaderNormal);
}

panel_button_t *CG_PanelButtons_GetFocusButton(void)
{
	return cg_focusButton;
}

void CG_PanelButtons_SetFocusButton(panel_button_t * button)
{
	cg_focusButton = button;
}
