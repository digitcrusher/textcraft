/*
 * world.cpp
 * humrcraft Source Code
 * Available on Github
 *
 * Copyright (c) 2017 Karol "digitcrusher" Łacina
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <iostream>
#include "renderers.hpp"
#include "world.hpp"
#include "shapes.hpp"

Object::Object(Shape* shape) {
    this->id = 0;
    this->family.pushBack("Object");
    this->world = NULL;
    this->shape = shape;
    if(this->shape) {
        this->shape->obj = this;
    }
    this->time = 0;
    this->pos = {0, 0};
    this->ori = {0, 0};
    this->vel = {0, 0};
    this->rot = {0, 0};
    this->stationary = 0;
    this->shared = 0;
}
Object::Object(Shape* shape, bool shared, bool stationary) : Object(shape) {
    this->stationary = stationary;
    this->shared = shared;
}
Object::~Object() {
    if(this->world) {
        this->world->remove(this->id);
    }
    if(this->shape && !this->shape->shared) {
        delete this->shape;
    }
}
void Object::update(double delta) {
    this->time += delta;
    if(this->shape) {
        this->shape->update(delta);
    }
    if(!this->stationary) {
        this->pos.x += cos(this->vel.y)*this->vel.x*delta;
        this->pos.y += sin(this->vel.y)*this->vel.x*delta;
        this->ori.y += rot.y;
    }
//    if(this->world) {
//        this->vspd *= 1-this->world->friction*this->getInvMass()*delta;
//    }
}
void Object::render(Renderer* renderer) {
    if(this->shape) {
        this->shape->render(renderer);
    }
}
void Object::speak(Speaker* speaker) {
    if(this->shape) {
        this->shape->speak(speaker);
    }
}
V2f Object::getPos() {
    return {this->pos.x
           ,this->pos.y};
}
V2f Object::getOri() {
    return this->ori;
}
V2f Object::getVel() {
    return this->vel;
}
V2f Object::getRot() {
    return this->rot;
}
void Object::setAbsPos(V2f pos) {
    this->pos = pos;
}
void Object::setAbsOri(V2f ori) {
    this->ori = ori;
}
void Object::setAbsVel(V2f vel) {
    this->vel = vel;
}
void Object::setAbsRot(V2f rot) {
    this->rot = rot;
}
void Object::applyImpulse(V2f j) {
    if(this->shape && !isinf(j.x)) {
        this->vel = carteToPolar(polarToCarte({j.x*(isinf(this->shape->getInvMass())?1:this->shape->getInvMass()), j.y})+polarToCarte(this->vel));
    }
}
void Object::applyTorque(V2f t) {
    if(this->shape && !isinf(t.x)) {
        this->rot.y = this->rot.y+t.y*(1/t.x*(isinf(this->shape->getInvMass())?1:this->shape->getInvMass()));
    }
}
bool Object::checkFamily(Object* obj, const char* member, unsigned int level) {
    return level < obj->family.size() && !strcmp(obj->family[level], member);
}
void Object::collisionCallback(manifold* manifold) {
    if(this->shape) {
        this->shape->collisionCallback(manifold);
    }
}
Object& Object::operator=(const Object& rvalue) {
    this->time = rvalue.time;
    this->pos = rvalue.pos;
    this->ori = rvalue.ori;
    this->vel = rvalue.vel;
    this->rot = rvalue.rot;
    return *this;
}

World::World() : Object(NULL){
    this->family.pushBack("World");
    this->collisions.pushBack(circleCircle);
}
World::~World() {
}
void World::update(double delta) {
    Object::update(delta);
    for(unsigned int i=0; i<this->objs.size(); i++) {
        if(!this->objs.isFree(i)) {
            this->objs[i]->update(delta);
        }
    }
    for(unsigned int i=0; i<this->objs.size(); i++) {
        if(!this->objs.isFree(i)) {
            Object* a = this->objs[i];
            if(a->shape) {
                for(unsigned int j=i+1; j<this->objs.size(); j++) {
                    if(!this->objs.isFree(j)) {
                        Object* b = this->objs[j];
                        if(b->shape) {
                            manifold manifold;
                            checkCollision(&manifold, a, b, this);
                            resolveCollision(manifold);
                        }
                    }
                }
            }
        }
    }
    this->time += delta;
}
void World::render() {
    Renderer* renderer;
    unsigned int offset = 0;
    while((renderer = (Renderer*)this->getObject("Renderer", 1, offset))) {
        renderer->begin();
        for(unsigned int i=0; i<this->objs.size(); i++) {
            if(!this->objs.isFree(i)) {
                this->objs[i]->render(renderer);
            }
        }
        renderer->end();
        offset = renderer->id+1;
    }
}
void World::speak() {
    Speaker* speaker;
    unsigned int offset = 0;
    while((speaker = (Speaker*)this->getObject("Speaker", 1, offset))) {
        speaker->begin();
        for(unsigned int i=0; i<this->objs.size(); i++) {
            if(!this->objs.isFree(i)) {
                this->objs[i]->speak(speaker);
            }
        }
        speaker->end();
        offset = speaker->id+1;
    }
}
int World::add(Object* obj) {
    obj->world = this;
    return (obj->id = this->objs.add(obj));
}
void World::addCollision(bool (*collision)(manifold* manifold, World* world)) {
    this->collisions.pushBack(collision);
}
bool World::remove(unsigned int id) {
    for(unsigned int i=0; i<this->objs.size(); i++) {
        if(this->objs[i]->id == id && !this->objs.isFree(i)) {
            this->objs.remove(i);
            return 0;
        }
    }
    return 1;
}
bool World::destroy(unsigned int id) {
    for(unsigned int i=0; i<this->objs.size(); i++) {
        if(this->objs[i]->id == id && !this->objs.isFree(i) && !this->objs[i]->shared) {
            delete this->objs[i];
            this->objs.getArray()[i].free = 1;
            return 0;
        }
    }
    return 1;
}
void World::destroyAll() {
    for(unsigned int i=0; i<this->objs.size(); i++) {
        if(!this->objs.isFree(i) && !this->objs[i]->shared) {
            delete this->objs[i];
            this->objs.getArray()[i].free = 1;
        }
    }
}
Object* World::getObject(unsigned int id) {
    for(unsigned int i=0; i<this->objs.size(); i++) {
        if(this->objs[i]->id == id && !this->objs.isFree(i)) {
            return this->objs[i];
        }
    }
    return NULL;
}
Object* World::getObject(const char* member, unsigned int level) {
    for(unsigned int i=0; i<this->objs.size(); i++) {
        if(this->checkFamily(this->objs[i], member, level) && !this->objs.isFree(i)) {
            return this->objs[i];
        }
    }
    return NULL;
}
Object* World::getObject(const char* member, unsigned int level, unsigned int offset) {
    for(unsigned int i=offset; i<this->objs.size(); i++) {
        if(this->checkFamily(this->objs[i], member, level) && !this->objs.isFree(i)) {
            return this->objs[i];
        }
    }
    return NULL;
}
World& World::operator=(const World& rvalue) {
    Object::operator=(rvalue); //Call base class = operator
    this->time = rvalue.time;
    this->objs = rvalue.objs;
    return *this;
}

Shape::Shape() : Object(NULL) {
    this->family.pushBack("Shape");
    this->mat = {0, 0};
    this->r = 255;
    this->g = 255;
    this->b = 255;
    this->a = 255;
}
Shape::Shape(material mat) : Shape() {
    this->mat = mat;
}
Shape::Shape(material mat, int r, int g, int b, int a) : Shape(mat) {
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}
Shape::~Shape() {
}
void Shape::update(double delta) {
    Object::update(delta);
}
void Shape::render(Renderer* renderer) {
    Object::render(renderer);
}
void Shape::speak(Speaker* speaker) {
    Object::speak(speaker);
}
V2f Shape::getPos() {
    return {this->obj->getPos().x+this->pos.x*(float)cos(this->obj->getOri().y)
           ,this->obj->getPos().y+this->pos.y*(float)sin(this->obj->getOri().y)};
}
V2f Shape::getOri() {
    return this->ori+this->obj->getOri();
}
V2f Shape::getVel() {
    return this->vel+this->obj->getVel();
}
V2f Shape::getRot() {
    return this->rot+this->obj->getRot();
}
void Shape::setAbsPos(V2f pos) {
    this->pos = pos-this->obj->getPos();
}
void Shape::setAbsOri(V2f ori) {
    this->ori = ori-this->obj->getOri();
}
void Shape::setAbsVel(V2f vel) {
    this->vel = vel-this->obj->getVel();
}
void Shape::setAbsRot(V2f rot) {
    this->rot = rot-this->obj->getRot();
}
float Shape::getVolume() {
    return 1;
}
float Shape::getInvMass() {
    return 1/(this->getVolume()*this->mat.density);
}
V2f Shape::getRadius(V2f angle) {
    return {0, angle.y};
}
V2f Shape::getNormal(V2f angle) {
    return {angle.y+(float)M_PI, 0};
}
Shape& Shape::operator=(const Shape& rvalue) {
    Object::operator=(rvalue);
    this->mat = mat;
    this->r = rvalue.r;
    this->g = rvalue.g;
    this->b = rvalue.b;
    this->a = rvalue.a;
    return *this;
}

Renderer::Renderer() : Object(NULL) {
    this->family.pushBack("Renderer");
}
Renderer::~Renderer() {
}
void Renderer::begin() {
}
void Renderer::end() {
}
Renderer& Renderer::operator=(const Renderer& rvalue) {
    Object::operator=(rvalue);
    return *this;
}

Speaker::Speaker() : Object(NULL) {
    this->family.pushBack("Speaker");
    SDL_AudioSpec wanted;
    memset(&wanted, 0, sizeof(wanted));
    wanted.freq = 11025;
    wanted.format = AUDIO_U8;
    wanted.channels = 1;
    wanted.samples = 4096;
    wanted.callback = this->audioCallback;
    wanted.userdata = this;
    if(!(this->device = SDL_OpenAudioDevice(NULL, 0, &wanted, &this->spec, SDL_AUDIO_ALLOW_FORMAT_CHANGE))) {
        std::cerr<<"SDL_OpenAudioDevice error: "<<SDL_GetError()<<'\n';
        throw;
    }
    this->pauseAudio(0);
}
Speaker::~Speaker() {
    SDL_CloseAudioDevice(this->device);
}
void Speaker::begin() {
}
void Speaker::end() {
}
void Speaker::playAudio(const char* filename) {
    SDL_LockAudioDevice(this->device);
    uint8_t* buffer;
    uint32_t length;
    SDL_AudioSpec spec=this->spec;
    if(!SDL_LoadWAV(filename, &spec, &buffer, &length)) {
        SDL_UnlockAudioDevice(this->device);
        return;
    }
    SDL_AudioCVT cvt;
    SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq, this->spec.format, this->spec.channels, this->spec.freq);
    cvt.len = length;
    cvt.buf = buffer;
    SDL_ConvertAudio(&cvt);
    uint8_t* newbuffer = (uint8_t*)malloc(sizeof(uint8_t)*cvt.len_cvt);
    memcpy(newbuffer, buffer, sizeof(uint8_t)*cvt.len);
    this->sounds.add({newbuffer, (size_t)cvt.len, 0});
    SDL_FreeWAV(buffer);
    SDL_UnlockAudioDevice(this->device);
}
void Speaker::pauseAudio(bool pause) {
    SDL_PauseAudioDevice(this->device, pause);
}
void Speaker::audioCallback(void* userdata, uint8_t* stream, int size) {
    memset(stream, 0, size);
    if(!userdata) {
        return;
    }
    Speaker* speaker = (Speaker*)userdata;
    for(unsigned int i=0; i<speaker->sounds.size(); i++) {
        if(!speaker->sounds.getArray()[i].free) {
            if(!speaker->sounds[i].buffer || speaker->sounds[i].position>=speaker->sounds[i].length) {
                speaker->sounds.remove(i);
                continue;
            }
            size = (unsigned int)size>speaker->sounds[i].length ? speaker->sounds[i].length : size;
            SDL_MixAudioFormat(stream, speaker->sounds[i].buffer+speaker->sounds[i].position, speaker->spec.format, size, SDL_MIX_MAXVOLUME);
            speaker->sounds.getArray()[i].elem.position += size;
        }
    }
}

bool checkCollision(manifold* manifold, Object* a, Object* b, World* world) {
    if(a && b && a->shape && b->shape) {
        memset(manifold, 0, sizeof(struct manifold));
        V2f angle;
        V2f ra;
        V2f rb;
        float restitution;
        V2f na;
        V2f nb;
        float ma;
        float mb;
        float pa;
        float pb;
        V2f fa;
        V2f fb;
        manifold->a = a;
        manifold->b = b;
        angle = fatp(a->getPos(), b->getPos());
        ra = a->shape->getRadius(angle);
        rb = b->shape->getRadius(angle+(V2f){0, M_PI});
        manifold->ra = ra;
        manifold->rb = rb;
        float penetration=0;
        unsigned int i;
        for(i=0; i<world->collisions.size(); i++) {
            if(!world->collisions[i] || world->collisions[i](manifold, world)) {
                continue;
            }
            break;
        }
        if(i==world->collisions.size()) {
            V2f starta;
            V2f enda;
            V2f tempstartb;
            V2f tempendb;
            V2f startb;
            V2f endb;
            starta.x = 0;
            starta.y = 0;
            enda.x = ra.x;
            enda.y = 0;
            tempstartb.x = cos(-(ra.y))*(b->shape->getPos().x-a->shape->getPos().x)-sin(-(ra.y))*(b->shape->getPos().y-a->shape->getPos().y);
            tempstartb.y = sin(-(ra.y))*(b->shape->getPos().x-a->shape->getPos().x)+cos(-(ra.y))*(b->shape->getPos().y-a->shape->getPos().y);
            tempendb.x = cos(rb.y-ra.y)*rb.x+tempstartb.x;
            tempendb.y = sin(rb.y-ra.y)*rb.x+tempstartb.y;
            startb.x = fmin(tempstartb.x, tempendb.x);
            startb.y = fmin(tempstartb.y, tempendb.y);
            endb.x = fmax(tempstartb.x, tempendb.x);
            endb.y = fmax(tempstartb.y, tempendb.y);
            if(startb.x <= enda.x && endb.x >= starta.x && startb.y <= starta.y && endb.y >= starta.y) {
                penetration = 1;
            }
            manifold->ra = ra;
            manifold->rb = rb;
            manifold->starta = starta;
            manifold->enda = enda;
            manifold->tempstartb = tempstartb;
            manifold->tempendb = tempendb;
            manifold->startb = startb;
            manifold->endb = endb;
        }else {
            penetration = manifold->penetration;
        }
        restitution = fmin(a->shape->mat.restitution, b->shape->mat.restitution);
        na = a->shape->getNormal(ra);
        nb = b->shape->getNormal(rb);
        ma = a->shape->getInvMass();
        mb = b->shape->getInvMass();
        if(isinf(ma)) {
            ma = 1;
        }
        if(isinf(mb)) {
            mb = 1;
        }
        pa = a->getVel().x/ma;
        pb = b->getVel().x/mb;
        fa = {(pa+pb)*(float)cos((a->getVel()-nb).y)*-(1+restitution)/2+penetration, nb.y};
        fb = {(pa+pb)*(float)cos((b->getVel()-na).y)*-(1+restitution)/2+penetration, na.y};
        if(manifold) {
            manifold->angle = angle;
            if(penetration) {
                manifold->penetration = penetration;
                manifold->restitution = restitution;
                manifold->na = na;
                manifold->nb = nb;
                manifold->ma = ma;
                manifold->mb = mb;
                manifold->pa = pa;
                manifold->pb = pb;
                manifold->fa = fa;
                manifold->fb = fb;
            }
        }
        if(penetration > 0) {
            a->collisionCallback(manifold);
            b->collisionCallback(manifold);
            return 1;
        }
    }
    return 0;
}
void resolveCollision(manifold manifold) {
    if(manifold.a && manifold.b && manifold.a->shape && manifold.b->shape) {
        if(manifold.penetration > 0) {
            manifold.a->applyImpulse(manifold.fa);
            manifold.b->applyImpulse(manifold.fb);
        }
    }
}
