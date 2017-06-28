/*
 * gui.cpp
 * textcraft Source Code
 * Available on Github
 *
 * Copyright (C) 2017 Karol "digitcrusher" Łacina
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "graphics.h"
#include "gui.h"

GUIWidget::GUIWidget() {
    this->family.pushBack("GUIWidget");
    this->time = 0;
    this->eventListener = NULL;
    this->enable = 1;
    this->visible = 1;
    this->state = None;
    this->bgcolor = {0, 0, 0, 0};
    this->fgcolor = {0, 0, 0, 0};
    this->x = 0;
    this->y = 0;
    this->w = 0;
    this->h = 0;
}
GUIWidget::~GUIWidget() {
}
GUIWidget* GUIWidget::setTime(float time) {
    this->time = time;
    return this;
}
GUIWidget* GUIWidget::setEventListener(void (*eventListener)(int state)) {
    this->eventListener = eventListener;
    return this;
}
GUIWidget* GUIWidget::setEnabled(bool enable) {
    this->enable = enable;
    return this;
}
GUIWidget* GUIWidget::setVisibility(bool visible) {
    this->visible = visible;
    return this;
}
GUIWidget* GUIWidget::setState(int state) {
    this->state = state;
    return this;
}
GUIWidget* GUIWidget::setBGColor(SDL_Color bgcolor) {
    this->bgcolor = bgcolor;
    return this;
}
GUIWidget* GUIWidget::setFGColor(SDL_Color fgcolor) {
    this->fgcolor = fgcolor;
    return this;
}
GUIWidget* GUIWidget::setPosition(int x, int y) {
    this->x = x;
    this->y = y;
    return this;
}
GUIWidget* GUIWidget::setSize(int w, int h) {
    this->w = w;
    this->h = h;
    return this;
}
GUIWidget* GUIWidget::setBounds(int x, int y, int w, int h) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
    return this;
}
void GUIWidget::update(float delta) {
    this->time += delta;
}
void GUIWidget::render(SDL_Surface* surface) {
    if(!this->visible) return;
    uint8_t r, g, b, a;
    if(this->enable) {
        r = this->bgcolor.r;
        g = this->bgcolor.g;
        b = this->bgcolor.b;
        a = this->bgcolor.a;
    }else {
        r = 127;
        g = 127;
        b = 127;
        a = this->bgcolor.a;
    }
    SDL_Rect rect = {this->x, this->y, this->w, this->h};
    SDL_FillRect(surface, &rect, SDL_MapRGBA(surface->format, r, g, b, a));
}
void GUIWidget::processEvent(SDL_Event event) {
    int laststate = this->state;
    switch(event.type) {
        case SDL_MOUSEBUTTONDOWN:
            switch(event.button.button) {
                case SDL_BUTTON_LEFT:
                    if(event.button.x > this->x && event.button.x < this->w && event.button.y > this->y && event.button.y < this->h) {
                        this->state = Press;
                    }
                    break;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            switch(event.button.button) {
                case SDL_BUTTON_LEFT:
                    if(event.button.x > this->x && event.button.x < this->w && event.button.y > this->y && event.button.y < this->h) {
                        this->state = Release;
                    }
                    break;
            }
            break;
        case SDL_MOUSEMOTION:
            if(event.motion.x > this->x && event.motion.x < this->w && event.motion.y > this->y && event.motion.y < this->h) {
                this->state = Hover;
            }
            break;
    }
    if(event.button.x > this->x && event.button.x < this->w && event.button.y > this->y && event.button.y < this->h) {
        if(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
            this->state = Press;
        }else if(laststate == Press) {
            this->state = Release;
        }
    }else {
        this->state = None;
    }
    if(laststate != this->state && this->eventListener) {
        this->eventListener(this->state);
    }
}

GUIFrame::GUIFrame() : GUIWidget() {
    this->family.pushBack("GUIFrame");
}
GUIFrame::~GUIFrame() {
}
void GUIFrame::update(float delta) {
    GUIWidget::update(delta);
    for(unsigned int i=0; i<this->widgets.size(); i++) {
        this->widgets[i]->update(delta);
    }
}
void GUIFrame::render(SDL_Surface* surface) {
    GUIWidget::render(surface);
    int rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    SDL_Surface* surface2 = SDL_CreateRGBSurface(0, this->w, this->h, 32, rmask, gmask, bmask, amask);
    SDL_FillRect(surface2, NULL, SDL_MapRGBA(surface2->format, 0, 0, 0, 0));
    for(unsigned int i=0; i<this->widgets.size(); i++) {
        this->widgets[i]->render(surface2);
    }
    drawImage(surface2, this->x, this->y, surface);
    SDL_FreeSurface(surface2);
}
void GUIFrame::add(GUIWidget* widget) {
    this->widgets.pushBack(widget);
}
void GUIFrame::processEvent(SDL_Event event) {
    GUIWidget::processEvent(event);
    for(unsigned int i=0; i<this->widgets.size(); i++) {
        this->widgets[i]->processEvent(event);
    }
}

GUILabel::GUILabel(TTF_Font* font, const char* text) : GUIWidget() {
    this->family.pushBack("GUILabel");
    this->font = font;
    this->text = text;
}
GUILabel::~GUILabel() {
}
GUILabel* GUILabel::setFont(TTF_Font* font) {
    this->font = font;
    return this;
}
GUILabel* GUILabel::setText(const char* text) {
    this->text = text;
    return this;
}
void GUILabel::render(SDL_Surface* surface) {
    uint8_t r, g, b, a;
    if(this->enable) {
        r = this->bgcolor.r;
        g = this->bgcolor.g;
        b = this->bgcolor.b;
        a = this->bgcolor.a;
    }else {
        r = 127;
        g = 127;
        b = 127;
        a = this->bgcolor.a;
    }
    SDL_Rect dstrect = {this->x, this->y, this->w, this->h};
    SDL_FillRect(surface, &dstrect, SDL_MapRGBA(surface->format, r, g, b, a));
    if(this->enable) {
        r = this->fgcolor.r;
        g = this->fgcolor.g;
        b = this->fgcolor.b;
        a = this->fgcolor.a;
    }else {
        r = 127;
        g = 127;
        b = 127;
        a = this->fgcolor.a;
    }
    SDL_Surface* text = TTF_RenderText_Blended_Wrapped(this->font, this->text, {r, g, b, a}, this->w);
    SDL_Rect srcrect = {0, 0, this->w, this->h};
    SDL_BlitSurface(text, &srcrect, surface, &dstrect);
}

GUIButton::GUIButton() : GUIFrame() {
}
GUIButton::~GUIButton() {
}
void GUIButton::render(SDL_Surface* surface) {
    GUIWidget::render(surface);
    uint8_t r, g, b, a;
    if(this->enable) {
        r = this->bgcolor.r-(this->state == Press)*128;
        g = this->bgcolor.g-(this->state == Press)*128;
        b = this->bgcolor.b-(this->state == Press)*128;
        a = this->bgcolor.a;
    }else {
        r = 127;
        g = 127;
        b = 127;
        a = this->bgcolor.a;
    }
    r = r>this->bgcolor.r?0:r;
    g = g>this->bgcolor.g?0:g;
    b = b>this->bgcolor.b?0:b;
    SDL_Rect rect = {this->x, this->y, this->w, this->h};
    SDL_FillRect(surface, &rect, SDL_MapRGBA(surface->format, r, g, b, a));
    uint8_t r1, g1, b1, a1;
    uint8_t r2, g2, b2, a2;
    r1 = r+64;
    g1 = g+64;
    b1 = b+64;
    a1 = a;
    r2 = r-64;
    g2 = g-64;
    b2 = b-64;
    a2 = a;
    r1 = r1<r?r:r1;
    g1 = g1<g?g:g1;
    b1 = b1<b?b:b1;
    r2 = r2>r?0:r2;
    g2 = g2>g?0:g2;
    b2 = b2>b?0:b2;
    if(this->state == Press) {
        drawLine(surface, this->x, this->y, this->x+this->w-1, this->y, SDL_MapRGBA(surface->format, r2, g2, b2, a2));
        drawLine(surface, this->x, this->y, this->x, this->y+this->h-1, SDL_MapRGBA(surface->format, r2, g2, b2, a2));
        drawLine(surface, this->x+this->w-1, this->y, this->x+this->w-1, this->y+this->h-1, SDL_MapRGBA(surface->format, r1, g1, b1, a1));
        drawLine(surface, this->x+this->w-1, this->y+this->h-1, this->x, this->y+this->h-1, SDL_MapRGBA(surface->format, r1, g1, b1, a1));
    }else {
        drawLine(surface, this->x+this->w-1, this->y, this->x+this->w-1, this->y+this->h-1, SDL_MapRGBA(surface->format, r2, g2, b2, a2));
        drawLine(surface, this->x+this->w-1, this->y+this->h-1, this->x, this->y+this->h-1, SDL_MapRGBA(surface->format, r2, g2, b2, a2));
        drawLine(surface, this->x, this->y, this->x+this->w-1, this->y, SDL_MapRGBA(surface->format, r1, g1, b1, a1));
        drawLine(surface, this->x, this->y, this->x, this->y+this->h-1, SDL_MapRGBA(surface->format, r1, g1, b1, a1));
    }
    int border=1;
    int rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    SDL_Surface* surface2 = SDL_CreateRGBSurface(0, this->w-border*2, this->h-border*2, 32, rmask, gmask, bmask, amask);
    SDL_FillRect(surface2, NULL, SDL_MapRGBA(surface2->format, 0, 0, 0, 0));
    for(unsigned int i=0; i<this->widgets.size(); i++) {
        this->widgets[i]->render(surface2);
    }
    drawImage(surface2, this->x+border, this->y+border, surface);
    SDL_FreeSurface(surface2);
}

GUIProgressBar::GUIProgressBar() : GUIWidget() {
    this->progress = 0;
}
GUIProgressBar::~GUIProgressBar() {
}
GUIProgressBar* GUIProgressBar::setProgress(float progress) {
    this->progress = progress;
    return this;
}
void GUIProgressBar::render(SDL_Surface* surface) {
    GUIWidget::render(surface);
    uint8_t r, g, b, a;
    if(this->enable) {
        r = this->fgcolor.r;
        g = this->fgcolor.g;
        b = this->fgcolor.b;
        a = this->fgcolor.a;
    }else {
        r = 127;
        g = 127;
        b = 127;
        a = this->fgcolor.a;
    }
    SDL_Rect rect = {this->x, this->y, (int)fmin(this->w*this->progress, this->w), this->h};
    SDL_FillRect(surface, &rect, SDL_MapRGBA(surface->format, r, g, b, a));
}