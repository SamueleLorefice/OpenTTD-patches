/* $Id$ */

/** @file widget.cpp Handling of the default/simple widgets. */

#include "stdafx.h"
#include "company_func.h"
#include "gfx_func.h"
#include "window_gui.h"
#include "debug.h"
#include "strings_func.h"

#include "table/sprites.h"
#include "table/strings.h"

static const char *UPARROW   = "\xEE\x8A\xA0"; ///< String containing an upwards pointing arrow.
static const char *DOWNARROW = "\xEE\x8A\xAA"; ///< String containing a downwards pointing arrow.

/**
 * Compute the vertical position of the draggable part of scrollbar
 * @param sb     Scrollbar list data
 * @param top    Top position of the scrollbar (top position of the up-button)
 * @param bottom Bottom position of the scrollbar (bottom position of the down-button)
 * @return A Point, with x containing the top coordinate of the draggable part, and
 *                       y containing the bottom coordinate of the draggable part
 */
static Point HandleScrollbarHittest(const Scrollbar *sb, int top, int bottom)
{
	Point pt;
	int height, count, pos, cap;

	top += 10;   // top    points to just below the up-button
	bottom -= 9; // bottom points to top of the down-button

	height = (bottom - top);

	pos = sb->pos;
	count = sb->count;
	cap = sb->cap;

	if (count != 0) top += height * pos / count;

	if (cap > count) cap = count;
	if (count != 0) bottom -= (count - pos - cap) * height / count;

	pt.x = top;
	pt.y = bottom - 1;
	return pt;
}

/** Special handling for the scrollbar widget type.
 * Handles the special scrolling buttons and other
 * scrolling.
 * @param w Window on which a scroll was performed.
 * @param wi Pointer to the scrollbar widget.
 * @param x The X coordinate of the mouse click.
 * @param y The Y coordinate of the mouse click. */
void ScrollbarClickHandler(Window *w, const Widget *wi, int x, int y)
{
	int mi, ma, pos;
	Scrollbar *sb;

	switch (wi->type) {
		case WWT_SCROLLBAR:
			/* vertical scroller */
			w->flags4 &= ~WF_HSCROLL;
			w->flags4 &= ~WF_SCROLL2;
			mi = wi->top;
			ma = wi->bottom;
			pos = y;
			sb = &w->vscroll;
			break;

		case WWT_SCROLL2BAR:
			/* 2nd vertical scroller */
			w->flags4 &= ~WF_HSCROLL;
			w->flags4 |= WF_SCROLL2;
			mi = wi->top;
			ma = wi->bottom;
			pos = y;
			sb = &w->vscroll2;
			break;

		case  WWT_HSCROLLBAR:
			/* horizontal scroller */
			w->flags4 &= ~WF_SCROLL2;
			w->flags4 |= WF_HSCROLL;
			mi = wi->left;
			ma = wi->right;
			pos = x;
			sb = &w->hscroll;
			break;

		default: NOT_REACHED();
	}
	if (pos <= mi + 9) {
		/* Pressing the upper button? */
		w->flags4 |= WF_SCROLL_UP;
		if (_scroller_click_timeout == 0) {
			_scroller_click_timeout = 6;
			if (sb->pos != 0) sb->pos--;
		}
		_left_button_clicked = false;
	} else if (pos >= ma - 10) {
		/* Pressing the lower button? */
		w->flags4 |= WF_SCROLL_DOWN;

		if (_scroller_click_timeout == 0) {
			_scroller_click_timeout = 6;
			if ((byte)(sb->pos + sb->cap) < sb->count)
				sb->pos++;
		}
		_left_button_clicked = false;
	} else {
		Point pt = HandleScrollbarHittest(sb, mi, ma);

		if (pos < pt.x) {
			sb->pos = max(sb->pos - sb->cap, 0);
		} else if (pos > pt.y) {
			sb->pos = min(
				sb->pos + sb->cap,
				max(sb->count - sb->cap, 0)
			);
		} else {
			_scrollbar_start_pos = pt.x - mi - 9;
			_scrollbar_size = ma - mi - 23;
			w->flags4 |= WF_SCROLL_MIDDLE;
			_scrolling_scrollbar = true;
			_cursorpos_drag_start = _cursor.pos;
		}
	}

	w->SetDirty();
}

/** Returns the index for the widget located at the given position
 * relative to the window. It includes all widget-corner pixels as well.
 * @param *w Window to look inside
 * @param  x The Window client X coordinate
 * @param  y The Window client y coordinate
 * @return A widget index, or -1 if no widget was found.
 */
int GetWidgetFromPos(const Window *w, int x, int y)
{
	uint index;
	int found_index = -1;

	/* Go through the widgets and check if we find the widget that the coordinate is
	 * inside. */
	for (index = 0; index < w->widget_count; index++) {
		const Widget *wi = &w->widget[index];
		if (wi->type == WWT_EMPTY || wi->type == WWT_FRAME) continue;

		if (x >= wi->left && x <= wi->right && y >= wi->top &&  y <= wi->bottom &&
				!w->IsWidgetHidden(index)) {
			found_index = index;
		}
	}

	return found_index;
}

/**
 * Draw frame rectangle.
 * @param left   Left edge of the frame
 * @param top    Top edge of the frame
 * @param right  Right edge of the frame
 * @param bottom Bottom edge of the frame
 * @param colour Colour table to use. @see _colour_gradient
 * @param flags  Flags controlling how to draw the frame. @see FrameFlags
 */
void DrawFrameRect(int left, int top, int right, int bottom, Colours colour, FrameFlags flags)
{
	uint dark         = _colour_gradient[colour][3];
	uint medium_dark  = _colour_gradient[colour][5];
	uint medium_light = _colour_gradient[colour][6];
	uint light        = _colour_gradient[colour][7];

	if (flags & FR_TRANSPARENT) {
		GfxFillRect(left, top, right, bottom, PALETTE_TO_TRANSPARENT, FILLRECT_RECOLOUR);
	} else {
		uint interior;

		if (flags & FR_LOWERED) {
			GfxFillRect(left,     top,     left,  bottom,     dark);
			GfxFillRect(left + 1, top,     right, top,        dark);
			GfxFillRect(right,    top + 1, right, bottom - 1, light);
			GfxFillRect(left + 1, bottom,  right, bottom,     light);
			interior = (flags & FR_DARKENED ? medium_dark : medium_light);
		} else {
			GfxFillRect(left,     top,    left,      bottom - 1, light);
			GfxFillRect(left + 1, top,    right - 1, top,        light);
			GfxFillRect(right,    top,    right,     bottom - 1, dark);
			GfxFillRect(left,     bottom, right,     bottom,     dark);
			interior = medium_dark;
		}
		if (!(flags & FR_BORDERONLY)) {
			GfxFillRect(left + 1, top + 1, right - 1, bottom - 1, interior);
		}
	}
}


/**
 * Paint all widgets of a window.
 */
void Window::DrawWidgets() const
{
	const DrawPixelInfo *dpi = _cur_dpi;

	for (uint i = 0; i < this->widget_count; i++) {
		const Widget *wi = &this->widget[i];
		bool clicked = this->IsWidgetLowered(i);
		Rect r;

		if (dpi->left > (r.right = wi->right) ||
				dpi->left + dpi->width <= (r.left = wi->left) ||
				dpi->top > (r.bottom = wi->bottom) ||
				dpi->top + dpi->height <= (r.top = wi->top) ||
				this->IsWidgetHidden(i)) {
			continue;
		}

		switch (wi->type & WWT_MASK) {
		case WWT_IMGBTN:
		case WWT_IMGBTN_2: {
			SpriteID img = wi->data;
			assert(img != 0);
			DrawFrameRect(r.left, r.top, r.right, r.bottom, wi->colour, (clicked) ? FR_LOWERED : FR_NONE);

			/* show different image when clicked for WWT_IMGBTN_2 */
			if ((wi->type & WWT_MASK) == WWT_IMGBTN_2 && clicked) img++;
			DrawSprite(img, PAL_NONE, r.left + 1 + clicked, r.top + 1 + clicked);
			break;
		}

		case WWT_PANEL:
			assert(wi->data == 0);
			DrawFrameRect(r.left, r.top, r.right, r.bottom, wi->colour, (clicked) ? FR_LOWERED : FR_NONE);
			break;

		case WWT_EDITBOX:
			DrawFrameRect(r.left, r.top, r.right, r.bottom, wi->colour, FR_LOWERED | FR_DARKENED);
			break;

		case WWT_TEXTBTN:
		case WWT_TEXTBTN_2:
			DrawFrameRect(r.left, r.top, r.right, r.bottom, wi->colour, (clicked) ? FR_LOWERED : FR_NONE);
			/* FALL THROUGH */

		case WWT_LABEL: {
			StringID str = wi->data;

			if ((wi->type & WWT_MASK) == WWT_TEXTBTN_2 && clicked) str++;

			DrawString(r.left + clicked, r.right + clicked, ((r.top + r.bottom + 1) >> 1) - 5 + clicked, str, TC_FROMSTRING, SA_CENTER);
			break;
		}

		case WWT_TEXT: {
			const StringID str = wi->data;

			if (str != STR_NULL) DrawString(r.left, r.right, r.top, str, (TextColour)wi->colour);
			break;
		}

		case WWT_INSET: {
			const StringID str = wi->data;
			DrawFrameRect(r.left, r.top, r.right, r.bottom, wi->colour, FR_LOWERED | FR_DARKENED);

			if (str != STR_NULL) DrawString(r.left + 2, r.right - 2, r.top + 1, str);
			break;
		}

		case WWT_MATRIX: {
			DrawFrameRect(r.left, r.top, r.right, r.bottom, wi->colour, (clicked) ? FR_LOWERED : FR_NONE);

			int c = GB(wi->data, 0, 8);
			int amt1 = (wi->right - wi->left + 1) / c;

			int d = GB(wi->data, 8, 8);
			int amt2 = (wi->bottom - wi->top + 1) / d;

			int colour = _colour_gradient[wi->colour & 0xF][6];

			int x = r.left;
			for (int ctr = c; ctr > 1; ctr--) {
				x += amt1;
				GfxFillRect(x, r.top + 1, x, r.bottom - 1, colour);
			}

			x = r.top;
			for (int ctr = d; ctr > 1; ctr--) {
				x += amt2;
				GfxFillRect(r.left + 1, x, r.right - 1, x, colour);
			}

			colour = _colour_gradient[wi->colour & 0xF][4];

			x = r.left - 1;
			for (int ctr = c; ctr > 1; ctr--) {
				x += amt1;
				GfxFillRect(x, r.top + 1, x, r.bottom - 1, colour);
			}

			x = r.top - 1;
			for (int ctr = d; ctr > 1; ctr--) {
				x += amt2;
				GfxFillRect(r.left + 1, x, r.right - 1, x, colour);
			}

			break;
		}

		/* vertical scrollbar */
		case WWT_SCROLLBAR: {
			assert(wi->data == 0);
			assert(r.right - r.left == 11); // To ensure the same sizes are used everywhere!

			/* draw up/down buttons */
			clicked = ((this->flags4 & (WF_SCROLL_UP | WF_HSCROLL | WF_SCROLL2)) == WF_SCROLL_UP);
			DrawFrameRect(r.left, r.top, r.right, r.top + 9, wi->colour, (clicked) ? FR_LOWERED : FR_NONE);
			DrawString(r.left + clicked, r.right + clicked, r.top + clicked, UPARROW, TC_BLACK, SA_CENTER);

			clicked = (((this->flags4 & (WF_SCROLL_DOWN | WF_HSCROLL | WF_SCROLL2)) == WF_SCROLL_DOWN));
			DrawFrameRect(r.left, r.bottom - 9, r.right, r.bottom, wi->colour, (clicked) ? FR_LOWERED : FR_NONE);
			DrawString(r.left + clicked, r.right + clicked, r.bottom - 9 + clicked, DOWNARROW, TC_BLACK, SA_CENTER);

			int c1 = _colour_gradient[wi->colour & 0xF][3];
			int c2 = _colour_gradient[wi->colour & 0xF][7];

			/* draw "shaded" background */
			GfxFillRect(r.left, r.top + 10, r.right, r.bottom - 10, c2);
			GfxFillRect(r.left, r.top + 10, r.right, r.bottom - 10, c1, FILLRECT_CHECKER);

			/* draw shaded lines */
			GfxFillRect(r.left + 2, r.top + 10, r.left + 2, r.bottom - 10, c1);
			GfxFillRect(r.left + 3, r.top + 10, r.left + 3, r.bottom - 10, c2);
			GfxFillRect(r.left + 7, r.top + 10, r.left + 7, r.bottom - 10, c1);
			GfxFillRect(r.left + 8, r.top + 10, r.left + 8, r.bottom - 10, c2);

			Point pt = HandleScrollbarHittest(&this->vscroll, r.top, r.bottom);
			DrawFrameRect(r.left, pt.x, r.right, pt.y, wi->colour, (this->flags4 & (WF_SCROLL_MIDDLE | WF_HSCROLL | WF_SCROLL2)) == WF_SCROLL_MIDDLE ? FR_LOWERED : FR_NONE);
			break;
		}

		case WWT_SCROLL2BAR: {
			assert(wi->data == 0);
			assert(r.right - r.left == 11); // To ensure the same sizes are used everywhere!

			/* draw up/down buttons */
			clicked = ((this->flags4 & (WF_SCROLL_UP | WF_HSCROLL | WF_SCROLL2)) == (WF_SCROLL_UP | WF_SCROLL2));
			DrawFrameRect(r.left, r.top, r.right, r.top + 9, wi->colour,  (clicked) ? FR_LOWERED : FR_NONE);
			DrawString(r.left + clicked, r.right + clicked, r.top + clicked, UPARROW, TC_BLACK, SA_CENTER);

			clicked = ((this->flags4 & (WF_SCROLL_DOWN | WF_HSCROLL | WF_SCROLL2)) == (WF_SCROLL_DOWN | WF_SCROLL2));
			DrawFrameRect(r.left, r.bottom - 9, r.right, r.bottom, wi->colour,  (clicked) ? FR_LOWERED : FR_NONE);
			DrawString(r.left + clicked, r.right + clicked, r.bottom - 9 + clicked, DOWNARROW, TC_BLACK, SA_CENTER);

			int c1 = _colour_gradient[wi->colour & 0xF][3];
			int c2 = _colour_gradient[wi->colour & 0xF][7];

			/* draw "shaded" background */
			GfxFillRect(r.left, r.top + 10, r.right, r.bottom - 10, c2);
			GfxFillRect(r.left, r.top + 10, r.right, r.bottom - 10, c1, FILLRECT_CHECKER);

			/* draw shaded lines */
			GfxFillRect(r.left + 2, r.top + 10, r.left + 2, r.bottom - 10, c1);
			GfxFillRect(r.left + 3, r.top + 10, r.left + 3, r.bottom - 10, c2);
			GfxFillRect(r.left + 7, r.top + 10, r.left + 7, r.bottom - 10, c1);
			GfxFillRect(r.left + 8, r.top + 10, r.left + 8, r.bottom - 10, c2);

			Point pt = HandleScrollbarHittest(&this->vscroll2, r.top, r.bottom);
			DrawFrameRect(r.left, pt.x, r.right, pt.y, wi->colour, (this->flags4 & (WF_SCROLL_MIDDLE | WF_HSCROLL | WF_SCROLL2)) == (WF_SCROLL_MIDDLE | WF_SCROLL2) ? FR_LOWERED : FR_NONE);
			break;
		}

		/* horizontal scrollbar */
		case WWT_HSCROLLBAR: {
			assert(wi->data == 0);
			assert(r.bottom - r.top == 11); // To ensure the same sizes are used everywhere!

			clicked = ((this->flags4 & (WF_SCROLL_UP | WF_HSCROLL)) == (WF_SCROLL_UP | WF_HSCROLL));
			DrawFrameRect(r.left, r.top, r.left + 9, r.bottom, wi->colour, (clicked) ? FR_LOWERED : FR_NONE);
			DrawSprite(SPR_ARROW_LEFT, PAL_NONE, r.left + 1 + clicked, r.top + 1 + clicked);

			clicked = ((this->flags4 & (WF_SCROLL_DOWN | WF_HSCROLL)) == (WF_SCROLL_DOWN | WF_HSCROLL));
			DrawFrameRect(r.right - 9, r.top, r.right, r.bottom, wi->colour, (clicked) ? FR_LOWERED : FR_NONE);
			DrawSprite(SPR_ARROW_RIGHT, PAL_NONE, r.right - 8 + clicked, r.top + 1 + clicked);

			int c1 = _colour_gradient[wi->colour & 0xF][3];
			int c2 = _colour_gradient[wi->colour & 0xF][7];

			/* draw "shaded" background */
			GfxFillRect(r.left + 10, r.top, r.right - 10, r.bottom, c2);
			GfxFillRect(r.left + 10, r.top, r.right - 10, r.bottom, c1, FILLRECT_CHECKER);

			/* draw shaded lines */
			GfxFillRect(r.left + 10, r.top + 2, r.right - 10, r.top + 2, c1);
			GfxFillRect(r.left + 10, r.top + 3, r.right - 10, r.top + 3, c2);
			GfxFillRect(r.left + 10, r.top + 7, r.right - 10, r.top + 7, c1);
			GfxFillRect(r.left + 10, r.top + 8, r.right - 10, r.top + 8, c2);

			/* draw actual scrollbar */
			Point pt = HandleScrollbarHittest(&this->hscroll, r.left, r.right);
			DrawFrameRect(pt.x, r.top, pt.y, r.bottom, wi->colour, (this->flags4 & (WF_SCROLL_MIDDLE | WF_HSCROLL)) == (WF_SCROLL_MIDDLE | WF_HSCROLL) ? FR_LOWERED : FR_NONE);

			break;
		}

		case WWT_FRAME: {
			const StringID str = wi->data;
			int x2 = r.left; // by default the left side is the left side of the widget

			if (str != STR_NULL) x2 = DrawString(r.left + 6, r.right - 6, r.top, str);

			int c1 = _colour_gradient[wi->colour][3];
			int c2 = _colour_gradient[wi->colour][7];

			if (_dynlang.text_dir == TD_LTR) {
				/* Line from upper left corner to start of text */
				GfxFillRect(r.left, r.top + 4, r.left + 4, r.top + 4, c1);
				GfxFillRect(r.left + 1, r.top + 5, r.left + 4, r.top + 5, c2);

				/* Line from end of text to upper right corner */
				GfxFillRect(x2, r.top + 4, r.right - 1, r.top + 4, c1);
				GfxFillRect(x2, r.top + 5, r.right - 2, r.top + 5, c2);
			} else {
				/* Line from upper left corner to start of text */
				GfxFillRect(r.left, r.top + 4, x2 - 2, r.top + 4, c1);
				GfxFillRect(r.left + 1, r.top + 5, x2 - 2, r.top + 5, c2);

				/* Line from end of text to upper right corner */
				GfxFillRect(r.right - 5, r.top + 4, r.right - 1, r.top + 4, c1);
				GfxFillRect(r.right - 5, r.top + 5, r.right - 2, r.top + 5, c2);
			}

			/* Line from upper left corner to bottom left corner */
			GfxFillRect(r.left, r.top + 5, r.left, r.bottom - 1, c1);
			GfxFillRect(r.left + 1, r.top + 6, r.left + 1, r.bottom - 2, c2);

			/* Line from upper right corner to bottom right corner */
			GfxFillRect(r.right - 1, r.top + 5, r.right - 1, r.bottom - 2, c1);
			GfxFillRect(r.right, r.top + 4, r.right, r.bottom - 1, c2);

			GfxFillRect(r.left + 1, r.bottom - 1, r.right - 1, r.bottom - 1, c1);
			GfxFillRect(r.left, r.bottom, r.right, r.bottom, c2);

			break;
		}

		case WWT_STICKYBOX:
			assert(wi->data == 0);
			assert(r.right - r.left == 11); // To ensure the same sizes are used everywhere!

			clicked = !!(this->flags4 & WF_STICKY);
			DrawFrameRect(r.left, r.top, r.right, r.bottom, wi->colour, (clicked) ? FR_LOWERED : FR_NONE);
			DrawSprite((clicked) ? SPR_PIN_UP : SPR_PIN_DOWN, PAL_NONE, r.left + 2 + clicked, r.top + 3 + clicked);
			break;

		case WWT_RESIZEBOX:
			assert(wi->data == 0);
			assert(r.right - r.left == 11); // To ensure the same sizes are used everywhere!

			clicked = !!(this->flags4 & WF_SIZING);
			DrawFrameRect(r.left, r.top, r.right, r.bottom, wi->colour, (clicked) ? FR_LOWERED : FR_NONE);
			if (wi->left < (this->width / 2)) {
				DrawSprite(SPR_WINDOW_RESIZE_LEFT, PAL_NONE, r.left + 2, r.top + 3 + clicked);
			} else {
				DrawSprite(SPR_WINDOW_RESIZE_RIGHT, PAL_NONE, r.left + 3 + clicked, r.top + 3 + clicked);
			}
			break;

		case WWT_CLOSEBOX: {
			const StringID str = wi->data;

			assert(str == STR_BLACK_CROSS || str == STR_SILVER_CROSS); // black or silver cross
			assert(r.right - r.left == 10); // To ensure the same sizes are used everywhere

			DrawFrameRect(r.left, r.top, r.right, r.bottom, wi->colour, FR_NONE);
			DrawString(r.left, r.right, r.top + 2, str, TC_FROMSTRING, SA_CENTER);
			break;
		}

		case WWT_CAPTION:
			assert(r.bottom - r.top == 13); // To ensure the same sizes are used everywhere!
			DrawFrameRect(r.left, r.top, r.right, r.bottom, wi->colour, FR_BORDERONLY);
			DrawFrameRect(r.left + 1, r.top + 1, r.right - 1, r.bottom - 1, wi->colour, (this->owner == INVALID_OWNER) ? FR_LOWERED | FR_DARKENED : FR_LOWERED | FR_DARKENED | FR_BORDERONLY);

			if (this->owner != INVALID_OWNER) {
				GfxFillRect(r.left + 2, r.top + 2, r.right - 2, r.bottom - 2, _colour_gradient[_company_colours[this->owner]][4]);
			}

			DrawString(r.left + 2, r.right - 2, r.top + 2, wi->data, TC_FROMSTRING, SA_CENTER);
			break;

		case WWT_DROPDOWN: {
			assert(r.bottom - r.top == 11); // ensure consistent size

			StringID str = wi->data;
			if (_dynlang.text_dir == TD_LTR) {
				DrawFrameRect(r.left, r.top, r.right - 12, r.bottom, wi->colour, FR_NONE);
				DrawFrameRect(r.right - 11, r.top, r.right, r.bottom, wi->colour, clicked ? FR_LOWERED : FR_NONE);
				DrawString(r.right - (clicked ? 10 : 11), r.right, r.top + (clicked ? 2 : 1), STR_ARROW_DOWN, TC_BLACK, SA_CENTER);
				if (str != STR_NULL) DrawString(r.left + 2, r.right - 14, r.top + 1, str, TC_BLACK);
			} else {
				DrawFrameRect(r.left + 12, r.top, r.right, r.bottom, wi->colour, FR_NONE);
				DrawFrameRect(r.left, r.top, r.left + 11, r.bottom, wi->colour, clicked ? FR_LOWERED : FR_NONE);
				DrawString(r.left + clicked, r.left + 11, r.top + (clicked ? 2 : 1), STR_ARROW_DOWN, TC_BLACK, SA_CENTER);
				if (str != STR_NULL) DrawString(r.left + 14, r.right - 2, r.top + 1, str, TC_BLACK);
			}
			break;
		}
		}

		if (this->IsWidgetDisabled(i)) {
			GfxFillRect(r.left + 1, r.top + 1, r.right - 1, r.bottom - 1, _colour_gradient[wi->colour & 0xF][2], FILLRECT_CHECKER);
		}
	}


	if (this->flags4 & WF_WHITE_BORDER_MASK) {
		DrawFrameRect(0, 0, this->width - 1, this->height - 1, COLOUR_WHITE, FR_BORDERONLY);
	}

}

/**
 * Evenly distribute the combined horizontal length of two consecutive widgets.
 * @param w Window containing the widgets.
 * @param a Left widget to resize.
 * @param b Right widget to resize.
 * @note Widgets are assumed to lie against each other.
 */
static void ResizeWidgets(Window *w, byte a, byte b)
{
	int16 offset = w->widget[a].left;
	int16 length = w->widget[b].right - offset;

	w->widget[a].right = (length / 2) + offset;

	w->widget[b].left  = w->widget[a].right + 1;
}

/**
 * Evenly distribute the combined horizontal length of three consecutive widgets.
 * @param w Window containing the widgets.
 * @param a Left widget to resize.
 * @param b Middle widget to resize.
 * @param c Right widget to resize.
 * @note Widgets are assumed to lie against each other.
 */
static void ResizeWidgets(Window *w, byte a, byte b, byte c)
{
	int16 offset = w->widget[a].left;
	int16 length = w->widget[c].right - offset;

	w->widget[a].right = length / 3;
	w->widget[b].right = w->widget[a].right * 2;

	w->widget[a].right += offset;
	w->widget[b].right += offset;

	/* Now the right side of the buttons are set. We will now set the left sides next to them */
	w->widget[b].left  = w->widget[a].right + 1;
	w->widget[c].left  = w->widget[b].right + 1;
}

/** Evenly distribute some widgets when resizing horizontally (often a button row)
 *  When only two arguments are given, the widgets are presumed to be on a line and only the ends are given
 * @param w Window to modify
 * @param left The leftmost widget to resize
 * @param right The rightmost widget to resize. Since right side of it is used, remember to set it to RESIZE_RIGHT
 */
void ResizeButtons(Window *w, byte left, byte right)
{
	int16 num_widgets = right - left + 1;

	if (num_widgets < 2) NOT_REACHED();

	switch (num_widgets) {
		case 2: ResizeWidgets(w, left, right); break;
		case 3: ResizeWidgets(w, left, left + 1, right); break;
		default: {
			/* Looks like we got more than 3 widgets to resize
			 * Now we will find the middle of the space desinated for the widgets
			 * and place half of the widgets on each side of it and call recursively.
			 * Eventually we will get down to blocks of 2-3 widgets and we got code to handle those cases */
			int16 offset = w->widget[left].left;
			int16 length = w->widget[right].right - offset;
			byte widget = ((num_widgets - 1)/ 2) + left; // rightmost widget of the left side

			/* Now we need to find the middle of the widgets.
			 * It will not always be the middle because if we got an uneven number of widgets,
			 *   we will need it to be 2/5, 3/7 and so on
			 * To get this, we multiply with num_widgets/num_widgets. Since we calculate in int, we will get:
			 *
			 *    num_widgets/2 (rounding down)
			 *   ---------------
			 *     num_widgets
			 *
			 * as multiplier to length. We just multiply before divide to that we stay in the int area though */
			int16 middle = ((length * num_widgets) / (2 * num_widgets)) + offset;

			/* Set left and right on the widgets, that's next to our "middle" */
			w->widget[widget].right = middle;
			w->widget[widget + 1].left = w->widget[widget].right + 1;
			/* Now resize the left and right of the middle */
			ResizeButtons(w, left, widget);
			ResizeButtons(w, widget + 1, right);
		}
	}
}

/** Resize a widget and shuffle other widgets around to fit. */
void ResizeWindowForWidget(Window *w, uint widget, int delta_x, int delta_y)
{
	int right  = w->widget[widget].right;
	int bottom = w->widget[widget].bottom;

	for (uint i = 0; i < w->widget_count; i++) {
		if (w->widget[i].left >= right && i != widget) w->widget[i].left += delta_x;
		if (w->widget[i].right >= right) w->widget[i].right += delta_x;
		if (w->widget[i].top >= bottom && i != widget) w->widget[i].top += delta_y;
		if (w->widget[i].bottom >= bottom) w->widget[i].bottom += delta_y;
	}

	/* A hidden widget has bottom == top or right == left, we need to make it
	 * one less to fit in its new gap. */
	if (right  == w->widget[widget].left) w->widget[widget].right--;
	if (bottom == w->widget[widget].top)  w->widget[widget].bottom--;

	if (w->widget[widget].left > w->widget[widget].right)  w->widget[widget].right  = w->widget[widget].left;
	if (w->widget[widget].top  > w->widget[widget].bottom) w->widget[widget].bottom = w->widget[widget].top;

	w->width  += delta_x;
	w->height += delta_y;
	w->resize.width  += delta_x;
	w->resize.height += delta_y;
}

/**
 * Draw a sort button's up or down arrow symbol.
 * @param widget Sort button widget
 * @param state State of sort button
 */
void Window::DrawSortButtonState(int widget, SortButtonState state) const
{
	if (state == SBS_OFF) return;

	int offset = this->IsWidgetLowered(widget) ? 1 : 0;
	int base = offset + (_dynlang.text_dir == TD_LTR ? this->widget[widget].right - 11 : this->widget[widget].left);
	DrawString(base, base + 11, this->widget[widget].top + 1 + offset, state == SBS_DOWN ? DOWNARROW : UPARROW, TC_BLACK, SA_CENTER);
}


/**
 * @defgroup NestedWidgets Hierarchical widgets.
 * Hierarchical widgets, also known as nested widgets, are widgets stored in a tree. At the leafs of the tree are (mostly) the 'real' widgets
 * visible to the user. At higher levels, widgets get organized in container widgets, until all widgets of the window are merged.
 *
 * \section nestedwidgetkinds Hierarchical widget kinds
 * A leaf widget is one of
 * <ul>
 * <li> #NWidgetLeaf for widgets visible for the user, or
 * <li> #NWidgetSpacer for creating (flexible) empty space between widgets.
 * </ul>
 * The purpose of a leaf widget is to provide interaction with the user by displaying settings, and/or allowing changing the settings.
 *
 * A container widget is one of
 * <ul>
 * <li> #NWidgetHorizontal for organizing child widgets in a (horizontal) row. The row switches order depending on the language setting (thus supporting
 *      right-to-left languages),
 * <li> #NWidgetHorizontalLTR for organizing child widgets in a (horizontal) row, always in the same order. All childs below this container will also
 *      never swap order.
 * <li> #NWidgetVertical for organizing child widgets underneath each other.
 * <li> #NWidgetBackground for adding a background behind its child widget.
 * <li> #NWidgetStacked for stacking child widgets on top of each other.
 * </ul>
 * The purpose of a container widget is to structure its leafs and sub-containers to allow proper resizing.
 *
 * \section nestedwidgetscomputations Hierarchical widget computations
 * The first 'computation' is the creation of the nested widgets tree by calling the constructors of the widgets listed above and calling \c Add() for every child,
 * or by means of specifying the tree as a collection of nested widgets parts and instantiating the tree from the array.
 *
 * After the creation step,
 * - The leafs have their own minimal size (\e min_x, \e min_y), filling (\e fill_x, \e fill_y), and resize steps (\e resize_x, \e resize_y).
 * - Containers only know what their children are, \e fill_x, \e fill_y, \e resize_x, and \e resize_y are not initialized.
 *
 * Computations in the nested widgets take place as follows:
 * <ol>
 * <li> A bottom-up sweep by recursively calling NWidgetBase::SetupSmallestSize() to initialize the smallest size (\e smallest_x, \e smallest_y) and
 *      to propagate filling and resize steps upwards to the root of the tree.
 * <li> A top-down sweep by recursively calling NWidgetBase::AssignSizePosition() to make the smallest sizes consistent over the entire tree, and to assign
 *      the top-left (\e pos_x, \e pos_y) position of each widget in the tree. This step uses \e fill_x and \e fill_y at each node in the tree to decide how to
 *      fill each widget towards consistent sizes.
 *      For generating a widget array, resize step sizes are made consistent.
 * </ol>
 *
 * @see NestedWidgetParts
 */

/**
 * Base class constructor.
 * @param tp Nested widget type.
 */
NWidgetBase::NWidgetBase(WidgetType tp) : ZeroedMemoryAllocator()
{
	this->type = tp;
}

/* ~NWidgetContainer() takes care of #next and #prev data members. */

/**
 * @fn int NWidgetBase::SetupSmallestSize()
 * @brief Compute smallest size needed by the widget.
 *
 * The smallest size of a widget is the smallest size that a widget needs to
 * display itself properly.
 * In addition, filling and resizing of the widget are computed.
 * @return Biggest index in the widget array of all child widgets.
 *
 * @note After the computation, the results can be queried by accessing the data members of the widget.
 */

/**
 * @fn void NWidgetBase::AssignSizePosition(uint x, uint y, uint given_width, uint given_height, bool allow_resize_x, bool allow_resize_y, bool rtl)
 * @brief Assign size and position to the widget.
 * @param x              Horizontal offset of the widget relative to the left edge of the window.
 * @param y              Vertical offset of the widget relative to the top edge of the window.
 * @param given_width    Width allocated to the widget.
 * @param given_height   Height allocated to the widget.
 * @param allow_resize_x Horizontal resizing is allowed.
 * @param allow_resize_y Vertical resizing is allowed.
 * @param rtl            Adapt for right-to-left languages (position contents of horizontal containers backwards).
 */

/**
 * @fn void NWidgetBase::StoreWidgets(Widget *widgets, int length, bool left_moving, bool top_moving, bool rtl)
 * @brief Store all child widgets with a valid index into the widget array.
 * @param widgets     Widget array to store the nested widgets in.
 * @param length      Length of the array.
 * @param left_moving Left edge of the widget may move due to resizing (right edge if \a rtl).
 * @param top_moving  Top edge of the widget may move due to resizing.
 * @param rtl         Adapt for right-to-left languages (position contents of horizontal containers backwards).
 *
 * @note When storing a nested widget, the function should check first that the type in the \a widgets array is #WWT_LAST.
 *       This is used to detect double widget allocations as well as holes in the widget array.
 */

/**
 * Constructor for resizable nested widgets.
 * @param tp     Nested widget type.
 * @param fill_x Allow horizontal filling from initial size.
 * @param fill_y Allow vertical filling from initial size.
 */
NWidgetResizeBase::NWidgetResizeBase(WidgetType tp, bool fill_x, bool fill_y) : NWidgetBase(tp)
{
	this->fill_x = fill_x;
	this->fill_y = fill_y;
}

/**
 * Set minimal size of the widget.
 * @param min_x Horizontal minimal size of the widget.
 * @param min_y Vertical minimal size of the widget.
 */
void NWidgetResizeBase::SetMinimalSize(uint min_x, uint min_y)
{
	this->min_x = min_x;
	this->min_y = min_y;
}

/**
 * Set the filling of the widget from initial size.
 * @param fill_x Allow horizontal filling from initial size.
 * @param fill_y Allow vertical filling from initial size.
 */
void NWidgetResizeBase::SetFill(bool fill_x, bool fill_y)
{
	this->fill_x = fill_x;
	this->fill_y = fill_y;
}

/**
 * Set resize step of the widget.
 * @param resize_x Resize step in horizontal direction, value \c 0 means no resize, otherwise the step size in pixels.
 * @param resize_y Resize step in vertical direction, value \c 0 means no resize, otherwise the step size in pixels.
 */
void NWidgetResizeBase::SetResize(uint resize_x, uint resize_y)
{
	this->resize_x = resize_x;
	this->resize_y = resize_y;
}

void NWidgetResizeBase::AssignSizePosition(uint x, uint y, uint given_width, uint given_height, bool allow_resize_x, bool allow_resize_y, bool rtl)
{
	this->pos_x = x;
	this->pos_y = y;
	this->smallest_x = given_width;
	this->smallest_y = given_height;
	if (!allow_resize_x) this->resize_x = 0;
	if (!allow_resize_y) this->resize_y = 0;
}

/**
 * Initialization of a 'real' widget.
 * @param tp          Type of the widget.
 * @param colour      Colour of the widget.
 * @param fill_x      Default horizontal filling.
 * @param fill_y      Default vertical filling.
 * @param widget_data Data component of the widget. @see Widget::data
 * @param tool_tip    Tool tip of the widget. @see Widget::tootips
 */
NWidgetCore::NWidgetCore(WidgetType tp, Colours colour, bool fill_x, bool fill_y, uint16 widget_data, StringID tool_tip) : NWidgetResizeBase(tp, fill_x, fill_y)
{
	this->colour = colour;
	this->index = -1;
	this->widget_data = widget_data;
	this->tool_tip = tool_tip;
}

/**
 * Set index of the nested widget in the widget array.
 * @param index Index to use.
 */
void NWidgetCore::SetIndex(int index)
{
	assert(index >= 0);
	this->index = index;
}

/**
 * Set data and tool tip of the nested widget.
 * @param widget_data Data to use.
 * @param tool_tip    Tool tip string to use.
 */
void NWidgetCore::SetDataTip(uint16 widget_data, StringID tool_tip)
{
	this->widget_data = widget_data;
	this->tool_tip = tool_tip;
}

int NWidgetCore::SetupSmallestSize()
{
	this->smallest_x = this->min_x;
	this->smallest_y = this->min_y;
	/* All other data is already at the right place. */
	return this->index;
}

void NWidgetCore::StoreWidgets(Widget *widgets, int length, bool left_moving, bool top_moving, bool rtl)
{
	if (this->index < 0) return;

	assert(this->index < length);
	Widget *w = widgets + this->index;
	assert(w->type == WWT_LAST);

	DisplayFlags flags = RESIZE_NONE; // resize flags.
	/* Compute vertical resizing. */
	if (top_moving) {
		flags |= RESIZE_TB; // Only 1 widget can resize in the widget array.
	} else if(this->resize_y > 0) {
		flags |= RESIZE_BOTTOM;
	}
	/* Compute horizontal resizing. */
	if (left_moving) {
		flags |= RESIZE_LR; // Only 1 widget can resize in the widget array.
	} else if (this->resize_x > 0) {
		flags |= RESIZE_RIGHT;
	}

	/* Copy nested widget data into its widget array entry. */
	w->type = this->type;
	w->display_flags = flags;
	w->colour = this->colour;
	w->left = this->pos_x;
	w->right = this->pos_x + this->smallest_x - 1;
	w->top = this->pos_y;
	w->bottom = this->pos_y + this->smallest_y - 1;
	w->data = this->widget_data;
	w->tooltips = this->tool_tip;
}

/**
 * Constructor container baseclass.
 * @param tp Type of the container.
 */
NWidgetContainer::NWidgetContainer(WidgetType tp) : NWidgetBase(tp)
{
	this->head = NULL;
	this->tail = NULL;
}

NWidgetContainer::~NWidgetContainer()
{
	while (this->head != NULL) {
		NWidgetBase *wid = this->head->next;
		delete this->head;
		this->head = wid;
	}
	this->tail = NULL;
}

/**
 * Append widget \a wid to container.
 * @param wid Widget to append.
 */
void NWidgetContainer::Add(NWidgetBase *wid)
{
	assert(wid->next == NULL && wid->prev == NULL);

	if (this->head == NULL) {
		this->head = wid;
		this->tail = wid;
	} else {
		assert(this->tail != NULL);
		assert(this->tail->next == NULL);

		this->tail->next = wid;
		wid->prev = this->tail;
		this->tail = wid;
	}
}

/**
 * Widgets stacked on top of each other.
 * @param tp Kind of stacking, must be either #NWID_SELECTION or #NWID_LAYERED.
 */
NWidgetStacked::NWidgetStacked(WidgetType tp) : NWidgetContainer(tp)
{
}

int NWidgetStacked::SetupSmallestSize()
{
	/* First sweep, recurse down and compute minimal size and filling. */
	int biggest_index = -1;
	this->smallest_x = 0;
	this->smallest_y = 0;
	this->fill_x = (this->head != NULL);
	this->fill_y = (this->head != NULL);
	this->resize_x = (this->head != NULL) ? 1 : 0;
	this->resize_y = (this->head != NULL) ? 1 : 0;
	for (NWidgetBase *child_wid = this->head; child_wid != NULL; child_wid = child_wid->next) {
		int idx = child_wid->SetupSmallestSize();
		biggest_index = max(biggest_index, idx);

		this->smallest_x = max(this->smallest_x, child_wid->smallest_x + child_wid->padding_left + child_wid->padding_right);
		this->smallest_y = max(this->smallest_y, child_wid->smallest_y + child_wid->padding_top + child_wid->padding_bottom);
		this->fill_x &= child_wid->fill_x;
		this->fill_y &= child_wid->fill_y;
		this->resize_x = LeastCommonMultiple(this->resize_x, child_wid->resize_x);
		this->resize_y = LeastCommonMultiple(this->resize_y, child_wid->resize_y);
	}
	return biggest_index;
}

void NWidgetStacked::AssignSizePosition(uint x, uint y, uint given_width, uint given_height, bool allow_resize_x, bool allow_resize_y, bool rtl)
{
	assert(given_width >= this->smallest_x && given_height >= this->smallest_y);

	this->pos_x = x;
	this->pos_y = y;
	this->smallest_x = given_width;
	this->smallest_y = given_height;
	if (!allow_resize_x) this->resize_x = 0;
	if (!allow_resize_y) this->resize_y = 0;

	for (NWidgetBase *child_wid = this->head; child_wid != NULL; child_wid = child_wid->next) {
		/* Decide about horizontal position and filling of the child. */
		uint child_width;
		int child_pos_x;
		if (child_wid->fill_x) {
			child_width = given_width - child_wid->padding_left - child_wid->padding_right;
			child_pos_x = (rtl ? child_wid->padding_right : child_wid->padding_left);
		} else {
			child_width = child_wid->smallest_x;
			child_pos_x = (given_width - child_wid->padding_left - child_wid->padding_right - child_width) / 2 + (rtl ? child_wid->padding_right : child_wid->padding_left);
		}

		/* Decide about vertical position and filling of the child. */
		uint child_height;
		int child_pos_y;
		if (child_wid->fill_y) {
			child_height = given_height - child_wid->padding_top - child_wid->padding_bottom;
			child_pos_y = 0;
		} else {
			child_height = child_wid->smallest_y;
			child_pos_y = (given_height - child_wid->padding_top - child_wid->padding_bottom - child_height) / 2;
		}
		child_wid->AssignSizePosition(x + child_pos_x, y + child_pos_y, child_width, child_height, (this->resize_x > 0), (this->resize_y > 0), rtl);
	}
}

void NWidgetStacked::StoreWidgets(Widget *widgets, int length, bool left_moving, bool top_moving, bool rtl)
{
	for (NWidgetBase *child_wid = this->head; child_wid != NULL; child_wid = child_wid->next) {
		child_wid->StoreWidgets(widgets, length, left_moving, top_moving, rtl);
	}
}

NWidgetPIPContainer::NWidgetPIPContainer(WidgetType tp) : NWidgetContainer(tp)
{
}

/**
 * Set additional pre/inter/post space for the container.
 *
 * @param pip_pre   Additional space in front of the first child widget (above
 *                  for the vertical container, at the left for the horizontal container).
 * @param pip_inter Additional space between two child widgets.
 * @param pip_post  Additional space after the last child widget (below for the
 *                  vertical container, at the right for the horizontal container).
 */
void NWidgetPIPContainer::SetPIP(uint8 pip_pre, uint8 pip_inter, uint8 pip_post)
{
	this->pip_pre = pip_pre;
	this->pip_inter = pip_inter;
	this->pip_post = pip_post;
}

/** Horizontal container widget. */
NWidgetHorizontal::NWidgetHorizontal() : NWidgetPIPContainer(NWID_HORIZONTAL)
{
}

int NWidgetHorizontal::SetupSmallestSize()
{
	int biggest_index = -1;
	this->smallest_x = 0;   // Sum of minimal size of all childs.
	this->smallest_y = 0;   // Biggest child.
	this->fill_x = false;   // true if at least one child allows fill_x.
	this->fill_y = true;    // true if all childs allow fill_y.
	this->resize_x = 0;     // smallest non-zero child widget resize step.
	this->resize_y = 1;     // smallest common child resize step

	if (this->head != NULL) this->head->padding_left += this->pip_pre;
	for (NWidgetBase *child_wid = this->head; child_wid != NULL; child_wid = child_wid->next) {
		int idx = child_wid->SetupSmallestSize();
		biggest_index = max(biggest_index, idx);

		if (child_wid->next != NULL) {
			child_wid->padding_right += this->pip_inter;
		} else {
			child_wid->padding_right += this->pip_post;
		}

		this->smallest_x += child_wid->smallest_x + child_wid->padding_left + child_wid->padding_right;
		this->smallest_y = max(this->smallest_y, child_wid->smallest_y + child_wid->padding_top + child_wid->padding_bottom);
		this->fill_x |= child_wid->fill_x;
		this->fill_y &= child_wid->fill_y;

		if (child_wid->resize_x > 0) {
			if (this->resize_x == 0 || this->resize_x > child_wid->resize_x) this->resize_x = child_wid->resize_x;
		}
		this->resize_y = LeastCommonMultiple(this->resize_y, child_wid->resize_y);
	}
	/* We need to zero the PIP settings so we can re-initialize the tree. */
	this->pip_pre = this->pip_inter = this->pip_post = 0;

	return biggest_index;
}

void NWidgetHorizontal::AssignSizePosition(uint x, uint y, uint given_width, uint given_height, bool allow_resize_x, bool allow_resize_y, bool rtl)
{
	assert(given_width >= this->smallest_x && given_height >= this->smallest_y);

	uint additional_length = given_width - this->smallest_x; // Additional width given to us.
	this->pos_x = x;
	this->pos_y = y;
	this->smallest_x = given_width;
	this->smallest_y = given_height;
	if (!allow_resize_x) this->resize_x = 0;
	if (!allow_resize_y) this->resize_y = 0;

	/* Count number of childs that would like a piece of the pie. */
	int num_changing_childs = 0; // Number of childs that can change size.
	NWidgetBase *child_wid;
	for (child_wid = this->head; child_wid != NULL; child_wid = child_wid->next) {
		if (child_wid->fill_x) num_changing_childs++;
	}

	/* Fill and position the child widgets. */
	uint position = 0; // Place to put next child relative to origin of the container.
	allow_resize_x = (this->resize_x > 0);
	child_wid = rtl ? this->tail : this->head;
	while (child_wid != NULL) {
		assert(given_height >= child_wid->smallest_y + child_wid->padding_top + child_wid->padding_bottom);
		/* Decide about vertical filling of the child. */
		uint child_height; // Height of the child widget.
		uint child_pos_y; // Vertical position of child relative to the top of the container.
		if (child_wid->fill_y) {
			child_height = given_height - child_wid->padding_top - child_wid->padding_bottom;
			child_pos_y = child_wid->padding_top;
		} else {
			child_height = child_wid->smallest_y;
			child_pos_y = (given_height - child_wid->padding_top - child_wid->padding_bottom - child_height) / 2 + child_wid->padding_top;
		}

		/* Decide about horizontal filling of the child. */
		uint child_width;
		child_width = child_wid->smallest_x;
		if (child_wid->fill_x && num_changing_childs > 0) {
			/* Hand out a piece of the pie while compensating for rounding errors. */
			uint increment = additional_length / num_changing_childs;
			additional_length -= increment;
			num_changing_childs--;

			child_width += increment;
		}

		child_wid->AssignSizePosition(x + position + (rtl ? child_wid->padding_right : child_wid->padding_left), y + child_pos_y, child_width, child_height, allow_resize_x, (this->resize_y > 0), rtl);
		position += child_width + child_wid->padding_right + child_wid->padding_left;
		if (child_wid->resize_x > 0) allow_resize_x = false; // Widget array allows only one child resizing

		child_wid = rtl ? child_wid->prev : child_wid->next;
	}
}

void NWidgetHorizontal::StoreWidgets(Widget *widgets, int length, bool left_moving, bool top_moving, bool rtl)
{
	NWidgetBase *child_wid = rtl ? this->tail : this->head;
	while (child_wid != NULL) {
		child_wid->StoreWidgets(widgets, length, left_moving, top_moving, rtl);
		left_moving |= (child_wid->resize_x > 0);

		child_wid = rtl ? child_wid->prev : child_wid->next;
	}
}

/** Horizontal left-to-right container widget. */
NWidgetHorizontalLTR::NWidgetHorizontalLTR() : NWidgetHorizontal()
{
	this->type = NWID_HORIZONTAL_LTR;
}

void NWidgetHorizontalLTR::AssignSizePosition(uint x, uint y, uint given_width, uint given_height, bool allow_resize_x, bool allow_resize_y, bool rtl)
{
	NWidgetHorizontal::AssignSizePosition(x, y, given_width, given_height, allow_resize_x, allow_resize_y, false);
}

void NWidgetHorizontalLTR::StoreWidgets(Widget *widgets, int length, bool left_moving, bool top_moving, bool rtl)
{
	NWidgetHorizontal::StoreWidgets(widgets, length, left_moving, top_moving, false);
}

/** Vertical container widget. */
NWidgetVertical::NWidgetVertical() : NWidgetPIPContainer(NWID_VERTICAL)
{
}

int NWidgetVertical::SetupSmallestSize()
{
	int biggest_index = -1;
	this->smallest_x = 0;   // Biggest child.
	this->smallest_y = 0;   // Sum of minimal size of all childs.
	this->fill_x = true;    // true if all childs allow fill_x.
	this->fill_y = false;   // true if at least one child allows fill_y.
	this->resize_x = 1;     // smallest common child resize step
	this->resize_y = 0;     // smallest non-zero child widget resize step.

	if (this->head != NULL) this->head->padding_top += this->pip_pre;
	for (NWidgetBase *child_wid = this->head; child_wid != NULL; child_wid = child_wid->next) {
		int idx = child_wid->SetupSmallestSize();
		biggest_index = max(biggest_index, idx);

		if (child_wid->next != NULL) {
			child_wid->padding_bottom += this->pip_inter;
		} else {
			child_wid->padding_bottom += this->pip_post;
		}

		this->smallest_y += child_wid->smallest_y + child_wid->padding_top + child_wid->padding_bottom;
		this->smallest_x = max(this->smallest_x, child_wid->smallest_x + child_wid->padding_left + child_wid->padding_right);
		this->fill_y |= child_wid->fill_y;
		this->fill_x &= child_wid->fill_x;

		if (child_wid->resize_y > 0) {
			if (this->resize_y == 0 || this->resize_y > child_wid->resize_y) this->resize_y = child_wid->resize_y;
		}
		this->resize_x = LeastCommonMultiple(this->resize_x, child_wid->resize_x);
	}
	/* We need to zero the PIP settings so we can re-initialize the tree. */
	this->pip_pre = this->pip_inter = this->pip_post = 0;

	return biggest_index;
}

void NWidgetVertical::AssignSizePosition(uint x, uint y, uint given_width, uint given_height, bool allow_resize_x, bool allow_resize_y, bool rtl)
{
	assert(given_width >= this->smallest_x && given_height >= this->smallest_y);

	int additional_length = given_height - this->smallest_y; // Additional height given to us.
	this->pos_x = x;
	this->pos_y = y;
	this->smallest_x = given_width;
	this->smallest_y = given_height;
	if (!allow_resize_x) this->resize_x = 0;
	if (!allow_resize_y) this->resize_y = 0;

	/* count number of childs that would like a piece of the pie. */
	int num_changing_childs = 0; // Number of childs that can change size.
	for (NWidgetBase *child_wid = this->head; child_wid != NULL; child_wid = child_wid->next) {
		if (child_wid->fill_y) num_changing_childs++;
	}

	/* Fill and position the child widgets. */
	uint position = 0; // Place to put next child relative to origin of the container.
	allow_resize_y = (this->resize_y > 0);
	for (NWidgetBase *child_wid = this->head; child_wid != NULL; child_wid = child_wid->next) {
		assert(given_width >= child_wid->smallest_x + child_wid->padding_left + child_wid->padding_right);
		/* Decide about horizontal filling of the child. */
		uint child_width; // Width of the child widget.
		uint child_pos_x; // Horizontal position of child relative to the left of the container.
		if (child_wid->fill_x) {
			child_width = given_width - child_wid->padding_left - child_wid->padding_right;
			child_pos_x = (rtl ? child_wid->padding_right : child_wid->padding_left);
		} else {
			child_width = child_wid->smallest_x;
			child_pos_x = (given_width - child_wid->padding_left - child_wid->padding_right - child_width) / 2 + (rtl ? child_wid->padding_right : child_wid->padding_left);
		}

		/* Decide about vertical filling of the child. */
		uint child_height;
		child_height = child_wid->smallest_y;
		if (child_wid->fill_y && num_changing_childs > 0) {
			/* Hand out a piece of the pie while compensating for rounding errors. */
			uint increment = additional_length / num_changing_childs;
			additional_length -= increment;
			num_changing_childs--;

			child_height += increment;
		}

		child_wid->AssignSizePosition(x + child_pos_x, y + position + child_wid->padding_top, child_width, child_height, (this->resize_x > 0), allow_resize_y, rtl);
		position += child_height + child_wid->padding_top + child_wid->padding_bottom;
		if (child_wid->resize_y > 0) allow_resize_y = false; // Widget array allows only one child resizing
	}
}

void NWidgetVertical::StoreWidgets(Widget *widgets, int length, bool left_moving, bool top_moving, bool rtl)
{
	for (NWidgetBase *child_wid = this->head; child_wid != NULL; child_wid = child_wid->next) {
		child_wid->StoreWidgets(widgets, length, left_moving, top_moving, rtl);
		top_moving |= (child_wid->resize_y > 0);
	}
}

/**
 * Generic spacer widget.
 * @param length Horizontal size of the spacer widget.
 * @param height Vertical size of the spacer widget.
 */
NWidgetSpacer::NWidgetSpacer(int length, int height) : NWidgetResizeBase(NWID_SPACER, false, false)
{
	this->SetMinimalSize(length, height);
	this->SetResize(0, 0);
}

int NWidgetSpacer::SetupSmallestSize()
{
	this->smallest_x = this->min_x;
	this->smallest_y = this->min_y;
	return -1;
}

void NWidgetSpacer::StoreWidgets(Widget *widgets, int length, bool left_moving, bool top_moving, bool rtl)
{
	/* Spacer widgets are never stored in the widget array. */
}

/**
 * Constructor parent nested widgets.
 * @param tp     Type of parent widget.
 * @param colour Colour of the parent widget.
 * @param index  Index in the widget array used by the window system.
 * @param child  Child container widget (if supplied). If not supplied, a
 *               vertical container will be inserted while adding the first
 *               child widget.
 */
NWidgetBackground::NWidgetBackground(WidgetType tp, Colours colour, int index, NWidgetPIPContainer *child) : NWidgetCore(tp, colour, true, true, 0x0, STR_NULL)
{
	this->SetIndex(index);
	assert(tp == WWT_PANEL || tp == WWT_INSET || tp == WWT_FRAME);
	assert(index >= 0);
	this->child = child;
}

NWidgetBackground::~NWidgetBackground()
{
	if (this->child != NULL) delete this->child;
}

/**
 * Add a child to the parent.
 * @param nwid Nested widget to add to the background widget.
 *
 * Unless a child container has been given in the constructor, a parent behaves as a vertical container.
 * You can add several childs to it, and they are put underneath each other.
 */
void NWidgetBackground::Add(NWidgetBase *nwid)
{
	if (this->child == NULL) {
		this->child = new NWidgetVertical();
	}
	this->child->Add(nwid);
}

/**
 * Set additional pre/inter/post space for the background widget.
 *
 * @param pip_pre   Additional space in front of the first child widget (above
 *                  for the vertical container, at the left for the horizontal container).
 * @param pip_inter Additional space between two child widgets.
 * @param pip_post  Additional space after the last child widget (below for the
 *                  vertical container, at the right for the horizontal container).
 * @note Using this function implies that the widget has (or will have) child widgets.
 */
void NWidgetBackground::SetPIP(uint8 pip_pre, uint8 pip_inter, uint8 pip_post)
{
	if (this->child == NULL) {
		this->child = new NWidgetVertical();
	}
	this->child->SetPIP(pip_pre, pip_inter, pip_post);
}

int NWidgetBackground::SetupSmallestSize()
{
	int biggest_index = this->index;
	if (this->child != NULL) {
		int idx = this->child->SetupSmallestSize();
		biggest_index = max(biggest_index, idx);

		this->smallest_x = this->child->smallest_x;
		this->smallest_y = this->child->smallest_y;
		this->fill_x = this->child->fill_x;
		this->fill_y = this->child->fill_y;
		this->resize_x = this->child->resize_x;
		this->resize_y = this->child->resize_y;
	} else {
		this->smallest_x = this->min_x;
		this->smallest_y = this->min_y;
	}

	return biggest_index;
}

void NWidgetBackground::AssignSizePosition(uint x, uint y, uint given_width, uint given_height, bool allow_resize_x, bool allow_resize_y, bool rtl)
{
	this->pos_x = x;
	this->pos_y = y;
	this->smallest_x = given_width;
	this->smallest_y = given_height;
	if (!allow_resize_x) this->resize_x = 0;
	if (!allow_resize_y) this->resize_y = 0;

	if (this->child != NULL) {
		uint x_offset = (rtl ? this->child->padding_right : this->child->padding_left);
		uint width = given_width - this->child->padding_right - this->child->padding_left;
		uint height = given_height - this->child->padding_top - this->child->padding_bottom;
		this->child->AssignSizePosition(x + x_offset, y + this->child->padding_top, width, height, (this->resize_x > 0), (this->resize_y > 0), rtl);
	}
}

void NWidgetBackground::StoreWidgets(Widget *widgets, int length, bool left_moving, bool top_moving, bool rtl)
{
	NWidgetCore::StoreWidgets(widgets, length, left_moving, top_moving, rtl);
	if (this->child != NULL) this->child->StoreWidgets(widgets, length, left_moving, top_moving, rtl);
}

/**
 * Nested leaf widget.
 * @param tp     Type of leaf widget.
 * @param colour Colour of the leaf widget.
 * @param index  Index in the widget array used by the window system.
 * @param data   Data of the widget.
 * @param tip    Tooltip of the widget.
 */
NWidgetLeaf::NWidgetLeaf(WidgetType tp, Colours colour, int index, uint16 data, StringID tip) : NWidgetCore(tp, colour, true, true, data, tip)
{
	this->SetIndex(index);
	this->SetMinimalSize(0, 0);
	this->SetResize(0, 0);

	switch (tp) {
		case WWT_EMPTY:
			break;

		case WWT_PUSHBTN:
			this->SetFill(false, false);
			break;

		case WWT_IMGBTN:
		case WWT_PUSHIMGBTN:
		case WWT_IMGBTN_2:
			this->SetFill(false, false);
			break;

		case WWT_TEXTBTN:
		case WWT_PUSHTXTBTN:
		case WWT_TEXTBTN_2:
		case WWT_LABEL:
		case WWT_TEXT:
		case WWT_MATRIX:
		case WWT_EDITBOX:
			this->SetFill(false, false);
			break;

		case WWT_SCROLLBAR:
		case WWT_SCROLL2BAR:
			this->SetFill(false, true);
			this->SetResize(0, 1);
			this->min_x = 12;
			this->SetDataTip(0x0, STR_TOOLTIP_VSCROLL_BAR_SCROLLS_LIST);
			break;

		case WWT_CAPTION:
			this->SetFill(true, false);
			this->SetResize(1, 0);
			this->min_y = 14;
			this->SetDataTip(data, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS);
			break;

		case WWT_HSCROLLBAR:
			this->SetFill(true, false);
			this->SetResize(1, 0);
			this->min_y = 12;
			this->SetDataTip(0x0, STR_TOOLTIP_HSCROLL_BAR_SCROLLS_LIST);
			break;

		case WWT_STICKYBOX:
			this->SetFill(false, false);
			this->SetMinimalSize(12, 14);
			this->SetDataTip(STR_NULL, STR_STICKY_BUTTON);
			break;

		case WWT_RESIZEBOX:
			this->SetFill(false, false);
			this->SetMinimalSize(12, 12);
			this->SetDataTip(STR_NULL, STR_RESIZE_BUTTON);
			break;

		case WWT_CLOSEBOX:
			this->SetFill(false, false);
			this->SetMinimalSize(11, 14);
			this->SetDataTip(STR_BLACK_CROSS, STR_TOOLTIP_CLOSE_WINDOW);
			break;

		case WWT_DROPDOWN:
			this->SetFill(false, false);
			this->min_y = 12;
			break;

		default:
			NOT_REACHED();
	}
}

/**
 * Intialize nested widget tree and convert to widget array.
 * @param nwid Nested widget tree.
 * @param rtl  Direction of the language.
 * @return Widget array with the converted widgets.
 * @note Caller should release returned widget array with \c free(widgets).
 * @ingroup NestedWidgets
 */
Widget *InitializeNWidgets(NWidgetBase *nwid, bool rtl)
{
	/* Initialize nested widgets. */
	int biggest_index = nwid->SetupSmallestSize();
	nwid->AssignSizePosition(0, 0, nwid->smallest_x, nwid->smallest_y, (nwid->resize_x > 0), (nwid->resize_y > 0), rtl);

	/* Construct a local widget array and initialize all its types to #WWT_LAST. */
	Widget *widgets = MallocT<Widget>(biggest_index + 2);
	int i;
	for (i = 0; i < biggest_index + 2; i++) {
		widgets[i].type = WWT_LAST;
	}

	/* Store nested widgets in the array. */
	nwid->StoreWidgets(widgets, biggest_index + 1, false, false, rtl);

	/* Check that all widgets are used. */
	for (i = 0; i < biggest_index + 2; i++) {
		if (widgets[i].type == WWT_LAST) break;
	}
	assert(i == biggest_index + 1);

	/* Fill terminating widget */
	static const Widget last_widget = {WIDGETS_END};
	widgets[biggest_index + 1] = last_widget;

	return widgets;
}

/**
 * Compare two widget arrays with each other, and report differences.
 * @param orig Pointer to original widget array.
 * @param gen  Pointer to generated widget array (from the nested widgets).
 * @param report Report differences to 'misc' debug stream.
 * @return Both widget arrays are equal.
 */
bool CompareWidgetArrays(const Widget *orig, const Widget *gen, bool report)
{
#define CHECK(var, prn) \
	if (ow->var != gw->var) { \
		same = false; \
		if (report) DEBUG(misc, 1, "index %d, \"" #var "\" field: original " prn ", generated " prn, idx, ow->var, gw->var); \
	}
#define CHECK_COORD(var) \
	if (ow->var != gw->var) { \
		same = false; \
		if (report) DEBUG(misc, 1, "index %d, \"" #var "\" field: original %d, generated %d, (difference %d)", idx, ow->var, gw->var, ow->var - gw->var); \
	}

	bool same = true;
	for(int idx = 0; ; idx++) {
		const Widget *ow = orig + idx;
		const Widget *gw = gen + idx;

		CHECK(type, "%d")
		CHECK(display_flags, "0x%x")
		CHECK(colour, "%d")
		CHECK_COORD(left)
		CHECK_COORD(right)
		CHECK_COORD(top)
		CHECK_COORD(bottom)
		CHECK(data, "%u")
		CHECK(tooltips, "%u")

		if (ow->type == WWT_LAST || gw->type == WWT_LAST) break;
	}

	return same;

#undef CHECK
#undef CHECK_COORD
}

/* == Conversion code from NWidgetPart array to NWidgetBase* tree == */

/**
 * Construct a single nested widget in \a *dest from its parts.
 *
 * Construct a NWidgetBase object from a #NWidget function, and apply all
 * settings that follow it, until encountering a #EndContainer, another
 * #NWidget, or the end of the parts array.
 *
 * @param parts Array with parts of the nested widget.
 * @param count Length of the \a parts array.
 * @param dest  Address of pointer to use for returning the composed widget.
 * @param fill_dest Fill the composed widget with child widgets.
 * @return Number of widget part elements used to compose the widget.
 */
static int MakeNWidget(const NWidgetPart *parts, int count, NWidgetBase **dest, bool *fill_dest)
{
	int num_used = 0;

	*dest = NULL;
	*fill_dest = false;

	while (count > num_used) {
		switch (parts->type) {
			case NWID_SPACER:
				if (*dest != NULL) return num_used;
				*dest = new NWidgetSpacer(0, 0);
				break;

			case NWID_HORIZONTAL:
				if (*dest != NULL) return num_used;
				*dest = new NWidgetHorizontal();
				*fill_dest = true;
				break;

			case NWID_HORIZONTAL_LTR:
				if (*dest != NULL) return num_used;
				*dest = new NWidgetHorizontalLTR();
				*fill_dest = true;
				break;

			case WWT_PANEL:
			case WWT_INSET:
			case WWT_FRAME:
				if (*dest != NULL) return num_used;
				*dest = new NWidgetBackground(parts->type, parts->u.widget.colour, parts->u.widget.index);
				*fill_dest = true;
				break;

			case NWID_VERTICAL:
				if (*dest != NULL) return num_used;
				*dest = new NWidgetVertical();
				*fill_dest = true;
				break;

			case WPT_FUNCTION:
				if (*dest != NULL) return num_used;
				*dest = parts->u.func_ptr();
				*fill_dest = false;
				break;

			case NWID_SELECTION:
			case NWID_LAYERED:
				if (*dest != NULL) return num_used;
				*dest = new NWidgetStacked(parts->type);
				*fill_dest = true;
				break;


			case WPT_RESIZE: {
				NWidgetResizeBase *nwrb = dynamic_cast<NWidgetResizeBase *>(*dest);
				if (nwrb != NULL) {
					assert(parts->u.xy.x >= 0 && parts->u.xy.y >= 0);
					nwrb->SetResize(parts->u.xy.x, parts->u.xy.y);
				}
				break;
			}

			case WPT_RESIZE_PTR: {
				NWidgetResizeBase *nwrb = dynamic_cast<NWidgetResizeBase *>(*dest);
				if (nwrb != NULL) {
					assert(parts->u.xy_ptr->x >= 0 && parts->u.xy_ptr->y >= 0);
					nwrb->SetResize(parts->u.xy_ptr->x, parts->u.xy_ptr->y);
				}
				break;
			}

			case WPT_MINSIZE: {
				NWidgetResizeBase *nwrb = dynamic_cast<NWidgetResizeBase *>(*dest);
				if (nwrb != NULL) {
					assert(parts->u.xy.x >= 0 && parts->u.xy.y >= 0);
					nwrb->SetMinimalSize(parts->u.xy.x, parts->u.xy.y);
				}
				break;
			}

			case WPT_MINSIZE_PTR: {
				NWidgetResizeBase *nwrb = dynamic_cast<NWidgetResizeBase *>(*dest);
				if (nwrb != NULL) {
					assert(parts->u.xy_ptr->x >= 0 && parts->u.xy_ptr->y >= 0);
					nwrb->SetMinimalSize((uint)(parts->u.xy_ptr->x), (uint)(parts->u.xy_ptr->y));
				}
				break;
			}

			case WPT_FILL: {
				NWidgetResizeBase *nwrb = dynamic_cast<NWidgetResizeBase *>(*dest);
				if (nwrb != NULL) nwrb->SetFill(parts->u.xy.x != 0, parts->u.xy.y != 0);
				break;
			}

			case WPT_DATATIP: {
				NWidgetCore *nwc = dynamic_cast<NWidgetCore *>(*dest);
				if (nwc != NULL) {
					nwc->widget_data = parts->u.data_tip.data;
					nwc->tool_tip = parts->u.data_tip.tooltip;
				}
				break;
			}

			case WPT_DATATIP_PTR: {
				NWidgetCore *nwc = dynamic_cast<NWidgetCore *>(*dest);
				if (nwc != NULL) {
					nwc->widget_data = parts->u.datatip_ptr->data;
					nwc->tool_tip = parts->u.datatip_ptr->tooltip;
				}
				break;
			}

			case WPT_PADDING:
				if (*dest != NULL) (*dest)->SetPadding(parts->u.padding.top, parts->u.padding.right, parts->u.padding.bottom, parts->u.padding.left);
				break;

			case WPT_PIPSPACE: {
				NWidgetPIPContainer *nwc = dynamic_cast<NWidgetPIPContainer *>(*dest);
				if (nwc != NULL) nwc->SetPIP(parts->u.pip.pre,  parts->u.pip.inter, parts->u.pip.post);

				NWidgetBackground *nwb = dynamic_cast<NWidgetBackground *>(*dest);
				if (nwb != NULL) nwb->SetPIP(parts->u.pip.pre,  parts->u.pip.inter, parts->u.pip.post);
				break;
			}

			case WPT_ENDCONTAINER:
				return num_used;

			default:
				if (*dest != NULL) return num_used;
				assert((parts->type & WWT_MASK) < NWID_HORIZONTAL);
				*dest = new NWidgetLeaf(parts->type, parts->u.widget.colour, parts->u.widget.index, 0x0, STR_NULL);
				break;
		}
		num_used++;
		parts++;
	}

	return num_used;
}

/**
 * Build a nested widget tree by recursively filling containers with nested widgets read from their parts.
 * @param parts  Array with parts of the nested widgets.
 * @param count  Length of the \a parts array.
 * @param parent Container to use for storing the child widgets.
 * @return Number of widget part elements used to fill the container.
 */
static int MakeWidgetTree(const NWidgetPart *parts, int count, NWidgetBase *parent)
{
	/* Given parent must be either a #NWidgetContainer or a #NWidgetBackground object. */
	NWidgetContainer *nwid_cont = dynamic_cast<NWidgetContainer *>(parent);
	NWidgetBackground *nwid_parent = dynamic_cast<NWidgetBackground *>(parent);
	assert((nwid_cont != NULL && nwid_parent == NULL) || (nwid_cont == NULL && nwid_parent != NULL));

	int total_used = 0;
	while (true) {
		NWidgetBase *sub_widget = NULL;
		bool fill_sub = false;
		int num_used = MakeNWidget(parts, count - total_used, &sub_widget, &fill_sub);
		parts += num_used;
		total_used += num_used;

		/* Break out of loop when end reached */
		if (sub_widget == NULL) break;

		/* Add sub_widget to parent container. */
		if (nwid_cont) nwid_cont->Add(sub_widget);
		if (nwid_parent) nwid_parent->Add(sub_widget);

		/* If sub-widget is a container, recursively fill that container. */
		WidgetType tp = sub_widget->type;
		if (fill_sub && (tp == NWID_HORIZONTAL || tp == NWID_HORIZONTAL_LTR || tp == NWID_VERTICAL
							|| tp == WWT_PANEL || tp == WWT_FRAME || tp == WWT_INSET || tp == NWID_SELECTION || tp == NWID_LAYERED)) {
			int num_used = MakeWidgetTree(parts, count - total_used, sub_widget);
			parts += num_used;
			total_used += num_used;
		}
	}

	if (count == total_used) return total_used; // Reached the end of the array of parts?

	assert(total_used < count);
	assert(parts->type == WPT_ENDCONTAINER);
	return total_used + 1; // *parts is also 'used'
}

/**
 * Construct a nested widget tree from an array of parts.
 * @param parts Array with parts of the widgets.
 * @param count Length of the \a parts array.
 * @return Root of the nested widget tree, a vertical container containing the entire GUI.
 * @ingroup NestedWidgetParts
 */
NWidgetContainer *MakeNWidgets(const NWidgetPart *parts, int count)
{
	NWidgetContainer *cont = new NWidgetVertical();
	MakeWidgetTree(parts, count, cont);
	return cont;
}

/**
 * Construct a #Widget array from a nested widget parts array, taking care of all the steps and checks.
 * Also cache the result and use the cache if possible.
 * @param[in] parts        Array with parts of the widgets.
 * @param     parts_length Length of the \a parts array.
 * @param[in] orig_wid     Pointer to original widget array.
 * @param     wid_cache    Pointer to the cache for storing the generated widget array (use \c NULL to prevent caching).
 * @return Cached value if available, otherwise the generated widget array. If \a wid_cache is \c NULL, the caller should free the returned array.
 *
 * @pre Before the first call, \c *wid_cache should be \c NULL.
 * @post The widget array stored in the \c *wid_cache should be free-ed by the caller.
 */
const Widget *InitializeWidgetArrayFromNestedWidgets(const NWidgetPart *parts, int parts_length, const Widget *orig_wid, Widget **wid_cache)
{
	const bool rtl = false; // Direction of the language is left-to-right

	if (wid_cache != NULL && *wid_cache != NULL) return *wid_cache;

	assert(parts != NULL && parts_length > 0);
	NWidgetContainer *nwid = MakeNWidgets(parts, parts_length);
	Widget *gen_wid = InitializeNWidgets(nwid, rtl);

	if (!rtl && orig_wid) {
		/* There are two descriptions, compare them.
		 * Comparing only makes sense when using a left-to-right language.
		 */
		bool ok = CompareWidgetArrays(orig_wid, gen_wid, false);
		if (ok) {
			DEBUG(misc, 1, "Nested widgets are equal, min-size(%u, %u)", nwid->smallest_x, nwid->smallest_y);
		} else {
			DEBUG(misc, 0, "Nested widgets give different results");
			CompareWidgetArrays(orig_wid, gen_wid, true);
		}
	}
	delete nwid;

	if (wid_cache != NULL) *wid_cache = gen_wid;
	return gen_wid;
}
