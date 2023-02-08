/*
	Copyright (C) 2008 - 2022

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY.

	See the COPYING file for more details.
*/

#pragma once


#include "gui/widgets/styled_widget.hpp"
#include "gui/core/widget_definition.hpp"
#include "gui/core/window_builder.hpp"
#include "gui/dialogs/help_browser.hpp"

#include "help/help_impl.hpp"
#include "font/standard_colors.hpp"
#include "gui/dialogs/modal_dialog.hpp"
#include "font/sdl_ttf_compat.hpp"
#include "draw.hpp"
#include "game_config.hpp"
#include "picture.hpp"
#include "preferences/general.hpp"

#include <iostream>

namespace gui2
{
namespace implementation
{
	struct builder_label_rich;
}

// ------------ WIDGET -----------{

/**
 * @ingroup GUIWidgetWML
 *
 * A rich label displays a text, the text can be wrapped but no scrollbars are provided.
 *
 * Although the label itself has no event interaction it still has two states.
 * The reason is that labels are often used as visual indication of the state of the widget it labels.
 *
 * Note: The above is outdated, if "link_aware" is enabled then there is interaction.
 *
 * The following states exist:
 * * state_enabled - the label is enabled.
 * * state_disabled - the label is disabled.
 *
 * Key                |Type                                |Default |Description
 * -------------------|------------------------------------|--------|-------------
 * link_aware         | @ref guivartype_f_bool "f_bool"    |false   |Whether the label is link aware. This means it is rendered with links highlighted, and responds to click events on those links.
 * link_color         | @ref guivartype_string "string"    |\#ffff00|The color to render links with. This string will be used verbatim in pango markup for each link.
 *
 * The label specific variables:
 * Key                |Type                                |Default|Description
 * -------------------|------------------------------------|-------|-------------
 * wrap               | @ref guivartype_bool "bool"        |false  |Is wrapping enabled for the label.
 * characters_per_line| @ref guivartype_unsigned "unsigned"|0      |Sets the maximum number of characters per line. The amount is an approximate since the width of a character differs. E.g. iii is smaller than MMM. When the value is non-zero it also implies can_wrap is true. When having long strings wrapping them can increase readability, often 66 characters per line is considered the optimum for a one column text.
 */
class label_rich : public styled_widget
{
	friend struct implementation::builder_label_rich;

public:
	explicit label_rich(const implementation::builder_label_rich& builder);

	/** See @ref widget::can_wrap. */
	virtual bool can_wrap() const override
	{
		return can_wrap_ || characters_per_line_ != 0;
	}

	/** See @ref styled_widget::get_characters_per_line. */
	virtual unsigned get_characters_per_line() const override
	{
		return characters_per_line_;
	}

	/** See @ref styled_widget::get_link_aware. */
	virtual bool get_link_aware() const override
	{
		return link_aware_;
	}

	/** See @ref styled_widget::get_link_aware. */
	virtual color_t get_link_color() const override
	{
		return link_color_;
	}

	/** See @ref styled_widget::set_active. */
	virtual void set_active(const bool active) override;

	/** See @ref styled_widget::get_active. */
	virtual bool get_active() const override
	{
		return state_ != DISABLED;
	}

	/** See @ref styled_widget::get_state. */
	virtual unsigned get_state() const override
	{
		return state_;
	}

	/** See @ref widget::disable_click_dismiss. */
	bool disable_click_dismiss() const override
	{
		return false;
	}

	/** See @ref widget::can_mouse_focus. */
	virtual bool can_mouse_focus() const override
	{
		return !tooltip().empty() || get_link_aware();
	}

	/** See @ref styled_widget::update_canvas. */
	virtual void update_canvas() override;

	/***** ***** ***** setters / getters for members ***** ****** *****/

	void set_can_wrap(const bool wrap)
	{
		can_wrap_ = wrap;
	}

	void set_characters_per_line(const unsigned characters_per_line)
	{
		characters_per_line_ = characters_per_line;
	}

	void set_link_aware(bool l);

	void set_link_color(const color_t& color);

	void set_can_shrink(bool can_shrink)
	{
		can_shrink_ = can_shrink;
	}

	void set_text_alpha(unsigned short alpha);
	enum ALIGNMENT {LEFT, MIDDLE, RIGHT, HERE};
	void show_topic(const help::topic &t);

private:
	/**
	 * Possible states of the widget.
	 *
	 * Note the order of the states must be the same as defined in settings.hpp.
	 */
	enum state_t {
		ENABLED,
		DISABLED,
	};


	/**
	 * Return the ID that is cross-referenced at the (screen)
	 * coordinates x, y. If no cross-reference is there, return the
	 * empty string.
	 */
	std::string ref_at(const int x, const int y);

protected:
	virtual void scroll(unsigned int pos);
	virtual void set_inner_location(const SDL_Rect& rect);

private:
	/** Convert a string to an alignment. Throw parse_error if unsuccessful. */
	ALIGNMENT str_to_align(const std::string &s);

	/**
	 * An item that is displayed in the text area. Contains the surface
	 * that should be blitted along with some other information.
	 */
	struct item {

		item(const texture& tex, int x, int y, const std::string& text="",
			 const std::string& reference_to="", bool floating=false,
			 bool box=false, ALIGNMENT alignment=HERE);

		item(const texture& tex, int x, int y,
			 bool floating, bool box=false, ALIGNMENT=HERE);

		/** Relative coordinates of this item. */
		rect rect_;

		texture tex;

		// If this item contains text, this will contain that text.
		std::string text;

		// If this item contains a cross-reference, this is the id
		// of the referenced topic.
		std::string ref_to;

		// If this item is floating, that is, if things should be filled
		// around it.
		bool floating;
		bool box;
		ALIGNMENT align;
	};

	/** Function object to find an item at the specified coordinates. */
	class item_at {
	public:
		item_at(const int x, const int y) : x_(x), y_(y) {}
		bool operator()(const item&) const;
	private:
		const int x_, y_;
	};

	/**
	 * Update the vector with the items of the shown topic, creating
	 * surfaces for everything and putting things where they belong.
	 */
	void set_items();

	// Create appropriate items from configs. Items will be added to the
	// internal vector. These methods check that the necessary
	// attributes are specified.
	void handle_ref_cfg(const config &cfg);
	void handle_img_cfg(const config &cfg);
	void handle_bold_cfg(const config &cfg);
	void handle_italic_cfg(const config &cfg);
	void handle_header_cfg(const config &cfg);
	void handle_jump_cfg(const config &cfg);
	void handle_format_cfg(const config &cfg);

	void draw_contents();

	/**
	 * Add an item with text. If ref_dst is something else than the
	 * empty string, the text item will be underlined to show that it
	 * is a cross-reference. The item will also remember what the
	 * reference points to. If font_size is below zero, the default
	 * will be used.
	 */
	void add_text_item(const std::string& text, const std::string& ref_dst="",
					   bool broken_link = false,
					   int font_size=-1, bool bold=false, bool italic=false,
					   color_t color=font::NORMAL_COLOR);

	/** Add an image item with the specified attributes. */
	void add_img_item(const std::string& path, const std::string& alignment, const bool floating,
					  const bool box);

	/** Move the current input point to the next line. */
	void down_one_line();

	/** Adjust the heights of the items in the last row to make it look good. */
	void adjust_last_row();

	/** Return the width that remain on the line the current input point is at. */
	int get_remaining_width();

	/**
	 * Return the least x coordinate at which something of the
	 * specified height can be drawn at the specified y coordinate
	 * without interfering with floating images.
	 */
	int get_min_x(const int y, const int height=0);

	/** Analogous with get_min_x but return the maximum X. */
	int get_max_x(const int y, const int height=0);

	/**
	 * Find the lowest y coordinate where a floating img of the
	 * specified width and at the specified x coordinate can be
	 * placed. Start looking at desired_y and continue downwards. Only
	 * check against other floating things, since text and inline
	 * images only can be above this place if called correctly.
	 */
	int get_y_for_floating_img(const int width, const int x, const int desired_y);

	/** Add an item to the internal list, update the locations and row height. */
	void add_item(const item& itm);

	std::list<item> items_;
	std::list<item *> last_row_;
	const help::section &toplevel_;
	help::topic const *shown_topic_;
	const int title_spacing_ = 10;
	/** The current input location when creating items. */
	std::pair<int, int> curr_loc_;
	const unsigned min_row_height_ = 5;
	unsigned curr_row_height_;
	/** The height of all items in total. */
	int contents_height_;
	
	void set_state(const state_t state);

	/**
	 * Current state of the widget.
	 *
	 * The state of the widget determines what to render and how the widget
	 * reacts to certain 'events'.
	 */
	state_t state_;

	/** Holds the label can wrap or not. */
	bool can_wrap_;

	/**
	 * The maximum number of characters per line.
	 *
	 * The maximum is not an exact maximum, it uses the average character width.
	 */
	unsigned characters_per_line_;

	/**
	 * Whether the label is link aware, rendering links with special formatting
	 * and handling click events.
	 */
	bool link_aware_;

	/**
	 * What color links will be rendered in.
	 */
	color_t link_color_;

	bool can_shrink_;

	unsigned short text_alpha_;

	/** Inherited from styled_widget. */
	virtual bool text_can_shrink() override
	{
		return can_shrink_;
	}

public:
	/** Static type getter that does not rely on the widget being constructed. */
	static const std::string& type();

private:
	/** Inherited from styled_widget, implemented by REGISTER_WIDGET. */
	virtual const std::string& get_control_type() const override;

	/***** ***** ***** signal handlers ***** ****** *****/

	/**
	 * Left click signal handler: checks if we clicked on a hyperlink
	 */
	void signal_handler_left_button_click(bool& handled);

	/**
	 * Right click signal handler: checks if we clicked on a hyperlink, copied to clipboard
	 */
	void signal_handler_right_button_click(bool& handled);

	/**
	 * Mouse motion signal handler: checks if the cursor is on a hyperlink
	 */
	void signal_handler_mouse_motion(bool& handled, const point& coordinate);

	/**
	 * Mouse leave signal handler: checks if the cursor left a hyperlink
	 */
	void signal_handler_mouse_leave(bool& handled);

	/**
	 * Implementation detail for (re)setting the hyperlink cursor.
	 */
	void update_mouse_cursor(bool enable);
};

// }---------- DEFINITION ---------{

struct label_rich_definition : public styled_widget_definition
{

	explicit label_rich_definition(const config& cfg);

	struct resolution : public resolution_definition
	{
		explicit resolution(const config& cfg);

		color_t link_color;
	};
};

// }---------- BUILDER -----------{

namespace implementation
{

struct builder_label_rich : public builder_styled_widget
{
	builder_label_rich(const config& cfg);

	using builder_styled_widget::build;

	virtual std::unique_ptr<widget> build() const override;

	bool wrap;

	unsigned characters_per_line;

	PangoAlignment text_alignment;

	bool can_shrink;
	bool link_aware;
	
};

} // namespace implementation

// }------------ END --------------

} // namespace gui2
