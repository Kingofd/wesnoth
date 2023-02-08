/*
	Copyright (C) 2008 - 2022
	by Mark de Wever <koraq@xs4all.nl>
	Part of the Battle for Wesnoth Project https://www.wesnoth.org/

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY.

	See the COPYING file for more details.
*/

#define GETTEXT_DOMAIN "wesnoth-lib"

#include "gui/widgets/label_rich.hpp"

#include "gui/core/log.hpp"

#include "gui/core/widget_definition.hpp"
#include "gui/core/window_builder.hpp"
#include "gui/core/register_widget.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"

#include "cursor.hpp"
#include "desktop/clipboard.hpp"
#include "desktop/open.hpp"
#include "gettext.hpp"

#include <functional>
#include <string>
#include <sstream>

namespace gui2
{

// ------------ WIDGET -----------{

REGISTER_WIDGET(label_rich)

label_rich::label_rich(const implementation::builder_label_rich& builder)
	: styled_widget(builder, type())
	, state_(ENABLED)
	, can_wrap_(builder.wrap)
	, characters_per_line_(builder.characters_per_line)
	, link_aware_(builder.link_aware)
	, link_color_(color_t::from_hex_string("ffff00"))
	, can_shrink_(builder.can_shrink)
	, toplevel_ (toplevel_)
	, text_alpha_(ALPHA_OPAQUE)
{
	connect_signal<event::LEFT_BUTTON_CLICK>(
		std::bind(&label_rich::signal_handler_left_button_click, this, std::placeholders::_3));
	connect_signal<event::RIGHT_BUTTON_CLICK>(
		std::bind(&label_rich::signal_handler_right_button_click, this, std::placeholders::_3));
	connect_signal<event::MOUSE_MOTION>(
		std::bind(&label_rich::signal_handler_mouse_motion, this, std::placeholders::_3, std::placeholders::_5));
	connect_signal<event::MOUSE_LEAVE>(
		std::bind(&label_rich::signal_handler_mouse_leave, this, std::placeholders::_3));
}

label_rich::item::item(const texture& _tex, int x, int y, const std::string& _text,
						   const std::string& reference_to, bool _floating,
						   bool _box, ALIGNMENT alignment) :
	rect_(),
	tex(_tex),
	text(_text),
	ref_to(reference_to),
	floating(_floating), box(_box),
	align(alignment)
{
	rect_.x = x;
	rect_.y = y;
	rect_.w = box ? tex.w() + help::box_width * 2 : tex.w();
	rect_.h = box ? tex.h() + help::box_width * 2 : tex.h();
}

label_rich::item::item(const texture& _tex, int x, int y, bool _floating,
						   bool _box, ALIGNMENT alignment) :
	rect_(),
	tex(_tex),
	text(""),
	ref_to(""),
	floating(_floating),
	box(_box), align(alignment)
{
	rect_.x = x;
	rect_.y = y;
	rect_.w = box ? tex.w() + help::box_width * 2 : tex.w();
	rect_.h = box ? tex.h() + help::box_width * 2 : tex.h();
}

void label_rich::update_canvas()
{
	// Inherit.
	styled_widget::update_canvas();

	for(auto& tmp : get_canvases()) {
		tmp.set_variable("text_alpha", wfl::variant(text_alpha_));
	}
}

void label_rich::set_text_alpha(unsigned short alpha)
{
	if(alpha != text_alpha_) {
		text_alpha_ = alpha;
		update_canvas();
		queue_redraw();
	}
}

void label_rich::set_active(const bool active)
{
	if(get_active() != active) {
		set_state(active ? ENABLED : DISABLED);
	}
}

void label_rich::set_link_aware(bool link_aware)
{
	if(link_aware != link_aware_) {
		link_aware_ = link_aware;
		update_canvas();
		queue_redraw();
	}
}

void label_rich::set_link_color(const color_t& color)
{
	if(color != link_color_) {
		link_color_ = color;
		update_canvas();
		queue_redraw();
	}
}

void label_rich::set_state(const state_t state)
{
	if(state != state_) {
		state_ = state;
		queue_redraw();
	}
}

void label_rich::signal_handler_left_button_click(bool& handled)
{
	DBG_GUI_E << "label_rich click";

	if (!get_link_aware()) {
		return; // without marking event as "handled".
	}

	if (!desktop::open_object_is_supported()) {
		show_message("", _("Opening links is not supported, contact your packager"), dialogs::message::auto_close);
		handled = true;
		return;
	}

	point mouse = get_mouse_position();

	mouse.x -= get_x();
	mouse.y -= get_y();

	std::string link = get_label_link(mouse);

	if (link.length() == 0) {
		return ; // without marking event as "handled"
	}

	DBG_GUI_E << "Clicked Link:\"" << link << "\"";

	const int res = show_message(_("Open link?"), link, dialogs::message::yes_no_buttons);
	if(res == gui2::retval::OK) {
		desktop::open_object(link);
	}

	handled = true;
}

void label_rich::signal_handler_right_button_click(bool& handled)
{
	DBG_GUI_E << "label_rich right click";

	if (!get_link_aware()) {
		return ; // without marking event as "handled".
	}

	point mouse = get_mouse_position();

	mouse.x -= get_x();
	mouse.y -= get_y();

	std::string link = get_label_link(mouse);

	if (link.length() == 0) {
		return ; // without marking event as "handled"
	}

	DBG_GUI_E << "Right Clicked Link:\"" << link << "\"";

	desktop::clipboard::copy_to_clipboard(link, false);

	(void) show_message("", _("Copied link!"), dialogs::message::auto_close);

	handled = true;
}

void label_rich::signal_handler_mouse_motion(bool& handled, const point& coordinate)
{
	DBG_GUI_E << "label_rich mouse motion";

	if(!get_link_aware()) {
		return; // without marking event as "handled"
	}

	point mouse = coordinate;

	mouse.x -= get_x();
	mouse.y -= get_y();

	update_mouse_cursor(!get_label_link(mouse).empty());

	handled = true;
}

void label_rich::signal_handler_mouse_leave(bool& handled)
{
	DBG_GUI_E << "label_rich mouse leave";

	if(!get_link_aware()) {
		return; // without marking event as "handled"
	}

	// We left the widget, so just unconditionally reset the cursor
	update_mouse_cursor(false);

	handled = true;
}

void label_rich::update_mouse_cursor(bool enable)
{
	// Someone else may set the mouse cursor for us to something unusual (e.g.
	// the WAIT cursor) so we ought to mess with that only if it's set to
	// NORMAL or HYPERLINK.

	if(enable && cursor::get() == cursor::NORMAL) {
		cursor::set(cursor::HYPERLINK);
	} else if(!enable && cursor::get() == cursor::HYPERLINK) {
		cursor::set(cursor::NORMAL);
	}
}


void label_rich::set_inner_location(const SDL_Rect& /*rect*/)
{
	if (shown_topic_)
		set_items();
}

void label_rich::show_topic(const help::topic &t)
{
	shown_topic_ = &t;
	set_items();
	queue_redraw();
	DBG_GUI_P << "Showing topic: " << t.id << ": " << t.title;
}

void label_rich::set_items()
{
	last_row_.clear();
	items_.clear();
	curr_loc_.first = 0;
	curr_loc_.second = 0;
	curr_row_height_ = min_row_height_;
	// Add the title item.
	const std::string show_title = font::pango_line_ellipsize(
		shown_topic_->title, help::title_size, 0/*inner_location().w*/);
	texture tex(font::pango_render_text(show_title, help::title_size,
		font::NORMAL_COLOR, font::pango_text::STYLE_BOLD));
	if (tex) {
		add_item(item(tex, 0, 0, show_title));
		curr_loc_.second = title_spacing_;
		contents_height_ = title_spacing_;
		down_one_line();
	}
	// Parse and add the text.
	const config& parsed_items = shown_topic_->text.parsed_text();
	for (auto item : parsed_items.all_children_range()) {
		if(item.key == "ref") {
			handle_ref_cfg(item.cfg);
			std::cout << "Handling ref for label!" << std::endl;
		}
		else {
			add_text_item(item.cfg["text"]);
		}
	}
	down_one_line(); // End the last line.
	//int h = get_size().x;
	//set_origin(); //set_position(0); 
	//set_full_size(calculate_best_size());
	set_layout_size(get_best_size());//set_shown_size(h); 
}

void label_rich::handle_ref_cfg(const config &cfg)
{
	const std::string dst = cfg["dst"];
	const std::string text = cfg["text"] + " Rich Text, baby!";
	bool force = cfg["force"].to_bool();

	if (dst.empty()) {
		std::stringstream msg;
		msg << "Ref markup must have dst attribute. Please submit a bug"
		       " report if you have not modified the game files yourself. Erroneous config: ";
		//write(msg, cfg);
		throw help::parse_error(msg.str());
	}

	if (find_topic(toplevel_, dst) == nullptr && !force) {
		// detect the broken link but quietly silence the hyperlink for normal user
		add_text_item(text, game_config::debug ? dst : "", true);

		if (game_config::debug) {
			std::stringstream msg;
			msg << "Reference to non-existent topic '" << dst
			    << "'. Please submit a bug report if you have not"
			       "modified the game files yourself. Erroneous config: ";
			throw help::parse_error(msg.str());
		}
	} else {
		add_text_item(text, dst);
	}
}

void label_rich::handle_img_cfg(const config &cfg)
{
	const std::string src = cfg["src"];
	const std::string align = cfg["align"];
	bool floating = cfg["float"].to_bool();
	bool box = cfg["box"].to_bool(true);
	if (src.empty()) {
		throw help::parse_error("Img markup must have src attribute.");
	}
	add_img_item(src, align, floating, box);
}

void label_rich::handle_bold_cfg(const config &cfg)
{
	const std::string text = cfg["text"];
	if (text.empty()) {
		throw help::parse_error("Bold markup must have text attribute.");
	}
	add_text_item(text, "", false, -1, true);
}

void label_rich::handle_italic_cfg(const config &cfg)
{
	const std::string text = cfg["text"];
	if (text.empty()) {
		throw help::parse_error("Italic markup must have text attribute.");
	}
	add_text_item(text, "", false, -1, false, true);
}

void label_rich::handle_header_cfg(const config &cfg)
{
	const std::string text = cfg["text"];
	if (text.empty()) {
		throw help::parse_error("Header markup must have text attribute.");
	}
	add_text_item(text, "", false, help::title2_size, true);
}

void label_rich::handle_jump_cfg(const config &cfg)
{
	const std::string amount_str = cfg["amount"];
	const std::string to_str = cfg["to"];
	if (amount_str.empty() && to_str.empty()) {
		throw help::parse_error("Jump markup must have either a to or an amount attribute.");
	}
	unsigned jump_to = curr_loc_.first;
	if (!amount_str.empty()) {
		unsigned amount;
		try {
			amount = lexical_cast<unsigned, std::string>(amount_str);
		}
		catch (bad_lexical_cast&) {
			throw help::parse_error("Invalid amount the amount attribute in jump markup.");
		}
		jump_to += amount;
	}
	if (!to_str.empty()) {
		unsigned to;
		try {
			to = lexical_cast<unsigned, std::string>(to_str);
		}
		catch (bad_lexical_cast&) {
			throw help::parse_error("Invalid amount in the to attribute in jump markup.");
		}
		if (to < jump_to) {
			down_one_line();
		}
		jump_to = to;
	}
	if (jump_to != 0 && static_cast<int>(jump_to) <
			get_max_x(curr_loc_.first, curr_row_height_)) {

		curr_loc_.first = jump_to;
	}
}

void label_rich::handle_format_cfg(const config &cfg)
{
	const std::string text = cfg["text"];
	if (text.empty()) {
		throw help::parse_error("Format markup must have text attribute.");
	}
	bool bold = cfg["bold"].to_bool();
	bool italic = cfg["italic"].to_bool();
	int font_size = cfg["font_size"].to_int(help::normal_font_size);
	color_t color = help::string_to_color(cfg["color"]);
	add_text_item(text, "", false, font_size, bold, italic, color);
}

void label_rich::add_text_item(const std::string& text, const std::string& ref_dst,
								   bool broken_link, int _font_size, bool bold, bool italic,
								   color_t text_color
)
{
	const int font_size = _font_size < 0 ? help::normal_font_size : _font_size;
	// font::line_width(), font::get_rendered_text() are not use scaled font inside
	const int scaled_font_size = preferences::font_scaled(font_size);
	if (text.empty())
		return;
	const int remaining_width = get_remaining_width();
	std::size_t first_word_start = text.find_first_not_of(" ");
	if (first_word_start == std::string::npos) {
		first_word_start = 0;
	}
	if (text[first_word_start] == '\n') {
		down_one_line();
		std::string rest_text = text;
		rest_text.erase(0, first_word_start + 1);
		add_text_item(rest_text, ref_dst, broken_link, _font_size, bold, italic, text_color);
		return;
	}
	const std::string first_word = help::get_first_word(text);
	int state = font::pango_text::STYLE_NORMAL;
	state |= bold ? font::pango_text::STYLE_BOLD : 0;
	state |= italic ? font::pango_text::STYLE_ITALIC : 0;
	if (curr_loc_.first != get_min_x(curr_loc_.second, curr_row_height_)
		&& remaining_width < font::pango_line_width(first_word, scaled_font_size, font::pango_text::FONT_STYLE(state))) {
		// The first word does not fit, and we are not at the start of
		// the line. Move down.
		down_one_line();
		std::string s = help::remove_first_space(text);
		add_text_item(s, ref_dst, broken_link, _font_size, bold, italic, text_color);
	}
	else {
		std::vector<std::string> parts = help::split_in_width(text, font_size, remaining_width);
		std::string first_part = parts.front();
		// Always override the color if we have a cross reference.
		color_t color;
		if(ref_dst.empty())
			color = text_color;
		else if(broken_link)
			color = font::BAD_COLOR;
		else
			color = font::BAD_COLOR;

		// In split_in_width(), no_break_after() and no_break_before() are used(see marked-up_text.cpp).
		// Thus, even if there is enough remaining_width for the next word,
		// sometimes empty string is returned from split_in_width().
		if (first_part.empty()) {
			down_one_line();
		}
		else {
			texture tex(font::pango_render_text(first_part,
				scaled_font_size, color, font::pango_text::FONT_STYLE(state)));
			if (tex) {
				add_item(item(tex, curr_loc_.first, curr_loc_.second,
					first_part, ref_dst));
			}
		}
		if (parts.size() > 1) {

			std::string& s = parts.back();

			const std::string first_word_before = help::get_first_word(s);
			const std::string first_word_after = help::get_first_word(help::remove_first_space(s));
			if (get_remaining_width() >= font::pango_line_width(first_word_after, scaled_font_size, font::pango_text::FONT_STYLE(state))
				&& get_remaining_width()
				< font::pango_line_width(first_word_before, scaled_font_size, font::pango_text::FONT_STYLE(state))) {
				// If the removal of the space made this word fit, we
				// must move down a line, otherwise it will be drawn
				// without a space at the end of the line.
				s = help::remove_first_space(s);
				down_one_line();
			}
			else if (!(font::pango_line_width(first_word_before, scaled_font_size, font::pango_text::FONT_STYLE(state))
					   < get_remaining_width())) {
				s = help::remove_first_space(s);
			}
			add_text_item(s, ref_dst, broken_link, _font_size, bold, italic, text_color);

		}
	}
}

void label_rich::add_img_item(const std::string& path, const std::string& alignment,
								  const bool floating, const bool box)
{
	texture tex(image::get_texture(path));
	if (!tex)
		return;
	ALIGNMENT align = str_to_align(alignment);
	if (align == HERE && floating) {
		DBG_GUI_P << "Floating image with align HERE, aligning left.";
		align = LEFT;
	}
	const int width = tex.w() + (box ? help::box_width * 2 : 0);
	int xpos;
	int ypos = curr_loc_.second;
	int text_width = 0 /*inner_location().w*/;
	switch (align) {
	case HERE:
		xpos = curr_loc_.first;
		break;
	case LEFT:
	default:
		xpos = 0;
		break;
	case MIDDLE:
		xpos = text_width / 2 - width / 2 - (box ? help::box_width : 0);
		break;
	case RIGHT:
		xpos = text_width - width - (box ? help::box_width * 2 : 0);
		break;
	}
	if (curr_loc_.first != get_min_x(curr_loc_.second, curr_row_height_)
		&& (xpos < curr_loc_.first || xpos + width > text_width)) {
		down_one_line();
		add_img_item(path, alignment, floating, box);
	}
	else {
		if (!floating) {
			curr_loc_.first = xpos;
		}
		else {
			ypos = get_y_for_floating_img(width, xpos, ypos);
		}
		add_item(item(tex, xpos, ypos, floating, box, align));
	}
}

int label_rich::get_y_for_floating_img(const int width, const int x, const int desired_y)
{
	int min_y = desired_y;
	for (std::list<item>::const_iterator it = items_.begin(); it != items_.end(); ++it) {
		const item& itm = *it;
		if (itm.floating) {
			if ((itm.rect_.x + itm.rect_.w > x && itm.rect_.x < x + width)
				|| (itm.rect_.x > x && itm.rect_.x < x + width)) {
				min_y = std::max<int>(min_y, itm.rect_.y + itm.rect_.h);
			}
		}
	}
	return min_y;
}

int label_rich::get_min_x(const int y, const int height)
{
	int min_x = 0;
	for (std::list<item>::const_iterator it = items_.begin(); it != items_.end(); ++it) {
		const item& itm = *it;
		if (itm.floating) {
			if (itm.rect_.y < y + height && itm.rect_.y + itm.rect_.h > y && itm.align == LEFT) {
				min_x = std::max<int>(min_x, itm.rect_.w + 5);
			}
		}
	}
	return min_x;
}

int label_rich::get_max_x(const int y, const int height)
{
	int text_width = 0 /*inner_location().w*/;
	int max_x = text_width;
	for (std::list<item>::const_iterator it = items_.begin(); it != items_.end(); ++it) {
		const item& itm = *it;
		if (itm.floating) {
			if (itm.rect_.y < y + height && itm.rect_.y + itm.rect_.h > y) {
				if (itm.align == RIGHT) {
					max_x = std::min<int>(max_x, text_width - itm.rect_.w - 5);
				} else if (itm.align == MIDDLE) {
					max_x = std::min<int>(max_x, text_width / 2 - itm.rect_.w / 2 - 5);
				}
			}
		}
	}
	return max_x;
}

void label_rich::add_item(const item &itm)
{
	items_.push_back(itm);
	if (!itm.floating) {
		curr_loc_.first += itm.rect_.w;
		curr_row_height_ = std::max<int>(itm.rect_.h, curr_row_height_);
		contents_height_ = std::max<int>(contents_height_, curr_loc_.second + curr_row_height_);
		last_row_.push_back(&items_.back());
	}
	else {
		if (itm.align == LEFT) {
			curr_loc_.first = itm.rect_.w + 5;
		}
		contents_height_ = std::max<int>(contents_height_, itm.rect_.y + itm.rect_.h);
	}
}


label_rich::ALIGNMENT label_rich::str_to_align(const std::string &cmp_str)
{
	if (cmp_str == "left") {
		return LEFT;
	} else if (cmp_str == "middle") {
		return MIDDLE;
	} else if (cmp_str == "right") {
		return RIGHT;
	} else if (cmp_str == "here" || cmp_str.empty()) { // Make the empty string be "here" alignment.
		return HERE;
	}
	std::stringstream msg;
	msg << "Invalid alignment string: '" << cmp_str << "'";
	throw help::parse_error(msg.str());
}

void label_rich::down_one_line()
{
	adjust_last_row();
	last_row_.clear();
	curr_loc_.second += curr_row_height_ + (curr_row_height_ == min_row_height_ ? 0 : 2);
	curr_row_height_ = min_row_height_;
	contents_height_ = std::max<int>(curr_loc_.second + curr_row_height_, contents_height_);
	curr_loc_.first = get_min_x(curr_loc_.second, curr_row_height_);
}

void label_rich::adjust_last_row()
{
	for (std::list<item *>::iterator it = last_row_.begin(); it != last_row_.end(); ++it) {
		item &itm = *(*it);
		const int gap = curr_row_height_ - itm.rect_.h;
		itm.rect_.y += gap / 2;
	}
}

int label_rich::get_remaining_width()
{
	const int total_w = get_max_x(curr_loc_.second, curr_row_height_);
	return total_w - curr_loc_.first;
}

void label_rich::draw_contents()
{
	const SDL_Rect& loc = calculate_blitting_rectangle() /*inner_location()*/;
	auto clipper = draw::reduce_clip(loc);
	for(std::list<item>::const_iterator it = items_.begin(), end = items_.end(); it != end; ++it) {
		SDL_Rect dst = it->rect_;
		dst.y -= get_origin().y; //get_position()
		if (dst.y < static_cast<int>(loc.h) && dst.y + it->rect_.h > 0) {
			dst.x += loc.x;
			dst.y += loc.y;
			if (it->box) {
				for (int i = 0; i < help::box_width; ++i) {
					SDL_Rect draw_rect {
						dst.x,
						dst.y,
						it->rect_.w - i * 2,
						it->rect_.h - i * 2
					};
					draw::fill(draw_rect, 0, 0, 0, 0);
					++dst.x;
					++dst.y;
				}
			}
			draw::blit(it->tex, dst);
		}
	}
}

void label_rich::scroll(unsigned int)
{
	// Nothing will be done on the actual scroll event. The scroll
	// position is checked when drawing instead and things drawn
	// accordingly.
	queue_redraw();
}

bool label_rich::item_at::operator()(const item& item) const {
	return item.rect_.contains(x_, y_);
}

std::string label_rich::ref_at(const int x, const int y)
{
	const int local_x = x - 0; //location().x;
	const int local_y = y - 0; //location().y;
	if (local_y < get_size().x && local_y > 0) {
		const int cmp_y = local_y + get_origin().y;
		const std::list<item>::const_iterator it =
			std::find_if(items_.begin(), items_.end(), item_at(local_x, cmp_y));
		if (it != items_.end()) {
			if (!(*it).ref_to.empty()) {
				return ((*it).ref_to);
			}
		}
	}
	return "";
}

// }---------- DEFINITION ---------{

label_rich_definition::label_rich_definition(const config& cfg)
	: styled_widget_definition(cfg)
{
	DBG_GUI_P << "Parsing label_rich " << id;

	load_resolutions<resolution>(cfg);
}

label_rich_definition::resolution::resolution(const config& cfg)
	: resolution_definition(cfg)
	, link_color(cfg["link_color"].empty() ? color_t::from_hex_string("ffff00") : color_t::from_rgba_string(cfg["link_color"].str()))
{
	// Note the order should be the same as the enum state_t is label_rich.hpp.
	state.emplace_back(cfg.child("state_enabled"));
	state.emplace_back(cfg.child("state_disabled"));
}

// }---------- BUILDER -----------{

namespace implementation
{

builder_label_rich::builder_label_rich(const config& cfg)
	: builder_styled_widget(cfg)
	, wrap(cfg["wrap"].to_bool())
	, characters_per_line(cfg["characters_per_line"])
	, text_alignment(decode_text_alignment(cfg["text_alignment"]))
	, can_shrink(cfg["can_shrink"].to_bool(false))
	, link_aware(cfg["link_aware"].to_bool(false))
{
}

std::unique_ptr<widget> builder_label_rich::build() const
{
	auto lbl = std::make_unique<label_rich>(*this);

	const auto conf = lbl->cast_config_to<label_rich_definition>();
	assert(conf);

	lbl->set_text_alignment(text_alignment);
	lbl->set_link_color(conf->link_color);

	DBG_GUI_G << "Window builder: placed label_rich '" << id << "' with definition '"
			  << definition << "'.";

	return lbl;
}

} // namespace implementation

// }------------ END --------------

} // namespace gui2
