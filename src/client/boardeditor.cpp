/*
   DrawPile - a collaborative drawing program.

   Copyright (C) 2006-2007 Calle Laakkonen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <QColor>

#include "boardeditor.h"
#include "layer.h"
#include "board.h"
#include "user.h"
#include "interfaces.h"
#include "sessionstate.h"
#include "preview.h"

namespace drawingboard {

/**
 * @param board drawing board to edit
 * @param user user to commit the changes as
 * @param brush brush source
 * @param color color source
 */
BoardEditor::BoardEditor(Board *board, User *user,
		interface::BrushSource *brush,
		interface::ColorSource *color)
	: user_(user), board_(board), brush_(brush), color_(color)
{
	Q_ASSERT(board);
	Q_ASSERT(user);
	Q_ASSERT(brush);
	Q_ASSERT(color);
}

/**
 * @return brush generated by the brush source object
 */
Brush BoardEditor::localBrush() const
{
	return brush_->getBrush();
}

/**
 * @param color color to set as foreground
 */
void BoardEditor::setLocalForeground(const QColor& color)
{
	color_->setForeground(color);
}

/**
 * @param color color to set as background
 */
void BoardEditor::setLocalBackground(const QColor& color)
{
	color_->setBackground(color);
}

/**
 * @return color at specified coordinates
 * @retval invalid color if point was outside the board
 */
QColor BoardEditor::colorAt(const QPoint& point)
{
	const QImage& img = board_->image_->image();
	if(img.rect().contains(point,true))
		return img.pixel(point);
	else
		return QColor(-1,-1,-1);
}

/**
 * @param tool tool type to use
 * @param point starting point
 * @param brush brush to preview with
 */
void BoardEditor::startPreview(tools::Type tool, const Point& point, const Brush& brush)
{
	Q_ASSERT(board_->toolpreview_ == 0);
	Q_ASSERT(tool == tools::LINE || tool == tools::RECTANGLE);
	if(tool == tools::LINE)
		board_->toolpreview_ = new StrokePreview(user_->layer(), board_);
	else
		board_->toolpreview_ = new RectanglePreview(user_->layer(), board_);
	board_->toolpreview_->preview(point,point, brush);
}

void BoardEditor::continuePreview(const Point& point)
{
	Q_ASSERT(board_->toolpreview_);
	board_->toolpreview_->moveTo(point);
}

void BoardEditor::endPreview()
{
	Q_ASSERT(board_->toolpreview_);
	delete board_->toolpreview_;
	board_->toolpreview_ = 0;
}

/**
 * @param brush to compare
 * @retval true if brush matches what the user is currently using
 */
bool LocalBoardEditor::isCurrentBrush(const Brush& brush) const
{
	return user_->brush() == brush;
}

/**
 * @param brush brush to use
 */
void LocalBoardEditor::setTool(const Brush& brush)
{
	user_->setBrush(brush);
}

/**
 * @param point point to add
 */
void LocalBoardEditor::addStroke(const Point& point)
{
	user_->addStroke(point);
}

void LocalBoardEditor::endStroke()
{
	user_->endStroke();
}

/**
 * @param board board to user
 * @param user user to draw as
 * @param session network session over which commands are transmitted
 * @param brush brush source
 * @param color color source
 */
RemoteBoardEditor::RemoteBoardEditor(Board *board, User *user,
		network::SessionState *session,
		interface::BrushSource *brush,
		interface::ColorSource *color)
	: BoardEditor(board, user, brush, color), session_(session), lastbrush_(0,0,0)
{
	Q_ASSERT(session);
}

/**
 * Compare with a cached brush to avoid sending extra ToolInfo
 * messages
 * @param brush to compare
 * @retval true if brush matches what the user is currently using
 */
bool RemoteBoardEditor::isCurrentBrush(const Brush& brush) const
{
	return lastbrush_ == brush;
}

/**
 * @param brush brush to set
 */
void RemoteBoardEditor::setTool(const Brush& brush)
{
	lastbrush_ = brush;
	session_->sendToolInfo(brush);
}

/**
 * @param point stroke coordinates
 */
void RemoteBoardEditor::addStroke(const Point& point)
{
	session_->sendStrokeInfo(point);
	board_->addPreview(point);
}

void RemoteBoardEditor::endStroke()
{
	session_->sendStrokeEnd();
	board_->endPreview();
}

}

