/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Clipboard.h"
#include <Kernel/KeyCode.h>
#include <Kernel/MousePacket.h>
#include <LibCore/LocalSocket.h>
#include <LibCore/Object.h>
#include <WindowServer/ClientConnection.h>
#include <WindowServer/Cursor.h>
#include <WindowServer/Event.h>
#include <WindowServer/EventLoop.h>
#include <WindowServer/Screen.h>
#include <WindowServer/WindowManager.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifdef __OpenBSD__
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdraw.h>

static struct kbd_trans {
    int wskbd_value;
    KeyCode keycode;
    u8 character;
    KeyCode shifted_keycode;
    u8 shifted_character;
} kbd_trans_table[] = {
    { RAWKEY_Escape,    Key_Escape, '\033', Key_Escape, '\033' },
    { RAWKEY_1,		    Key_1,      '1',    Key_ExclamationPoint, '!' },
    { RAWKEY_2,		    Key_2,      '2',    Key_AtSign, '@' },
    { RAWKEY_3,		    Key_3,      '3',    Key_Hashtag, '#' },
    { RAWKEY_4,		    Key_4,      '4',    Key_Dollar, '$' },
    { RAWKEY_5,		    Key_5,      '5',    Key_Percent, '%' },
    { RAWKEY_6,		    Key_6,      '6',    Key_Circumflex, '^' },
    { RAWKEY_7,		    Key_7,      '7',    Key_Ampersand, '&' },
    { RAWKEY_8,		    Key_8,      '8',    Key_Asterisk, '*' },
    { RAWKEY_9,		    Key_9,      '9',    Key_LeftParen, '(' },
    { RAWKEY_0,		    Key_0,      '0',    Key_RightParen, ')' },
    { RAWKEY_minus,		Key_Minus,  '-',    Key_Underscore, '_' },
    { RAWKEY_equal,		Key_Equal,  '=',    Key_Plus, '+' },
    { RAWKEY_Tab,		Key_Tab,    '\t',   Key_Tab, '\t' },
    { 14,               Key_Backspace, 0x8, Key_Backspace, 0x8 },
    { RAWKEY_q,		    Key_Q,      'q',    Key_Q, 'Q' },
    { RAWKEY_w,		    Key_W,      'w',    Key_W, 'W' },
    { RAWKEY_e,		    Key_E,      'e',    Key_E, 'E' },
    { RAWKEY_r,		    Key_R,      'r',    Key_R, 'R' },
    { RAWKEY_t,		    Key_T,      't',    Key_T, 'T' },
    { RAWKEY_y,		    Key_Y,      'y',    Key_Y, 'Y' },
    { RAWKEY_u,		    Key_U,      'u',    Key_U, 'U' },
    { RAWKEY_i,		    Key_I,      'i',    Key_I, 'I' },
    { RAWKEY_o,		    Key_O,      'o',    Key_O, 'O' },
    { RAWKEY_p,		    Key_P,      'p',    Key_P, 'P' },
    { RAWKEY_bracketleft, Key_LeftBracket,  '[', Key_LeftBrace, '{' },
    { RAWKEY_bracketright, Key_RightBracket, ']', Key_RightBrace, '}' },
    { RAWKEY_Return,    Key_Return, '\n',   Key_Return, '\n' },
    { RAWKEY_Control_L,	Key_Control, 0,     Key_Control, 0 },
    { RAWKEY_a,		    Key_A,      'a',    Key_A, 'A' },
    { RAWKEY_s,		    Key_S,      's',    Key_S, 'S' },
    { RAWKEY_d,		    Key_D,      'd',    Key_D, 'D' },
    { RAWKEY_f,		    Key_F,      'f',    Key_F, 'F' },
    { RAWKEY_g,		    Key_G,      'g',    Key_G, 'G' },
    { RAWKEY_h,		    Key_H,      'h',    Key_H, 'H' },
    { RAWKEY_j,		    Key_J,      'j',    Key_J, 'J' },
    { RAWKEY_k,		    Key_K,      'k',    Key_K, 'K' },
    { RAWKEY_l,		    Key_L,      'l',    Key_L, 'L' },
    { RAWKEY_semicolon,	Key_Semicolon, ';', Key_Colon, ':' },
    { RAWKEY_apostrophe, Key_Apostrophe, '\'', Key_DoubleQuote, '"' },
    { RAWKEY_grave,		Key_Backtick, '`',  Key_Tilde, '~' },
    { RAWKEY_Shift_L,   Key_Shift,  0,      Key_Shift, 0 },
    { RAWKEY_backslash, Key_Backslash, '\\', Key_Pipe, '|' },
    { RAWKEY_z,		    Key_Z,      'z',    Key_Z, 'Z' },
    { RAWKEY_x,		    Key_X,      'x',    Key_X, 'X' },
    { RAWKEY_c,		    Key_C,      'c',    Key_C, 'C' },
    { RAWKEY_v,		    Key_V,      'v',    Key_V, 'V' },
    { RAWKEY_b,		    Key_B,      'b',    Key_B, 'B' },
    { RAWKEY_n,		    Key_N,      'n',    Key_N, 'N' },
    { RAWKEY_m,		    Key_M,      'm',    Key_M, 'M' },
    { RAWKEY_comma,		Key_Comma,  ',',    Key_LessThan, '<' },
    { RAWKEY_period,    Key_Period, '.',    Key_GreaterThan, '>' },
    { RAWKEY_slash,		Key_Slash,  '/',    Key_QuestionMark, '?' },
    { RAWKEY_Shift_R,   Key_Shift,  0,      Key_Shift, 0 },
    { RAWKEY_KP_Multiply, Key_Asterisk, '*', Key_Asterisk, '*' },
    { RAWKEY_Alt_L,		Key_Alt,    0,      Key_Alt, 0 },
    { RAWKEY_space,		Key_Space,  ' ',    Key_Space, ' ' },
    { RAWKEY_Caps_Lock,	Key_CapsLock, 0,    Key_CapsLock, 0 },
    { RAWKEY_f1,		Key_F1,     0,      Key_F1, 0 },
    { RAWKEY_f2,		Key_F2,     0,      Key_F2, 0 },
    { RAWKEY_f3,		Key_F3,     0,      Key_F3, 0 },
    { RAWKEY_f4,		Key_F4,     0,      Key_F4, 0 },
    { RAWKEY_f5,		Key_F5,     0,      Key_F5, 0 },
    { RAWKEY_f6,		Key_F6,     0,      Key_F6, 0 },
    { RAWKEY_f7,		Key_F7,     0,      Key_F7, 0 },
    { RAWKEY_f8,		Key_F8,     0,      Key_F8, 0 },
    { RAWKEY_f9,		Key_F9,     0,      Key_F9, 0 },
    { RAWKEY_f10,		Key_F10,    0,      Key_F10, 0 },
    { RAWKEY_Num_Lock,  Key_NumLock, 0,     Key_NumLock, 0 },
    { RAWKEY_Hold_Screen, Key_SysRq, 0,     Key_SysRq, 0 },
    { RAWKEY_KP_Home,	Key_Home,   0,      Key_Home, 0 },
    { RAWKEY_KP_Up,		Key_Up,     0,      Key_Up, 0 },
    { RAWKEY_KP_Prior,	Key_PageUp, 0,      Key_PageUp, 0 },
    { RAWKEY_KP_Subtract, Key_Minus, '-',   Key_Minus, '-' },
    { RAWKEY_KP_Left,	Key_Left,   0,      Key_Left, 0 },
#if 0
    { RAWKEY_KP_Begin, },
#endif
    { RAWKEY_KP_Right,  Key_Right,  0,      Key_Right, 0 },
    { RAWKEY_KP_Add,	Key_Plus,   '+',    Key_Plus, '+' },
    { RAWKEY_KP_End,	Key_End,    0,      Key_End, 0 },
    { RAWKEY_KP_Down,	Key_Down,   0,      Key_Down, 0 },
    { RAWKEY_KP_Next,	Key_PageDown, 0,    Key_PageDown, 0 },
    { RAWKEY_KP_Insert,	Key_Insert, 0,      Key_Insert, 0 },
    { RAWKEY_KP_Delete,	Key_Delete, 0,      Key_Delete, 0 },
#if 0
    { RAWKEY_less, },
#endif
    { RAWKEY_f11,		Key_F11,    0,      Key_F11, 0 },
    { RAWKEY_f12,		Key_F12,    0,      Key_F12, 0 },
    { RAWKEY_Print_Screen, Key_PrintScreen, 0, Key_PrintScreen, 0 },
#if 0
    { RAWKEY_Pause, },
#endif
    { RAWKEY_Meta_L,	Key_Alt,    0,      Key_Alt, 0 },
    { RAWKEY_Meta_R,	Key_Alt,    0,      Key_Alt, 0 },
    { RAWKEY_KP_Equal,	Key_Equal,  '=',    Key_Equal, '=' },
    { RAWKEY_KP_Enter,	Key_Return, '\n',   Key_Return, '\n' },
    { RAWKEY_Control_R,	Key_Control, 0,     Key_Control, 0 },
    { RAWKEY_KP_Divide,	Key_Slash,  '/',    Key_Slash, '/' },
    { RAWKEY_Alt_R,		Key_Alt,    0,      Key_Alt, 0 },
    { RAWKEY_Home,		Key_Home,   0,      Key_Home, 0 },
    { RAWKEY_Up,		Key_Up,     0,      Key_Up, 0 },
    { RAWKEY_Prior,		Key_PageUp, 0,      Key_PageUp, 0 },
    { RAWKEY_Left,		Key_Left,   0,      Key_Left, 0 },
    { RAWKEY_Right,		Key_Right,  0,      Key_Right, 0 },
    { RAWKEY_End,		Key_End,    0,      Key_End, 0 },
    { RAWKEY_Down,		Key_Down,   0,      Key_Down, 0 },
    { RAWKEY_Next,		Key_PageDown, 0,    Key_PageDown, 0 },
    { RAWKEY_Insert,	Key_Insert, 0,      Key_Insert, 0 },
    { RAWKEY_Delete,    Key_Delete, 0,      Key_Delete, 0 },
    { RAWKEY_Delete,    Key_Delete, 0,      Key_Delete, 0 },
    { 219,              Key_Logo,   0,      Key_Logo, 0 },
};

static int kbd_modifiers = 0;
#endif

//#define WSMESSAGELOOP_DEBUG

namespace WindowServer {

EventLoop::EventLoop()
    : m_server(Core::LocalServer::construct())
{
#ifdef __OpenBSD__
    m_keyboard_fd = open("/dev/wskbd", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    m_mouse_fd = open("/dev/wsmouse", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
#else
    m_keyboard_fd = open("/dev/keyboard", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    m_mouse_fd = open("/dev/mouse", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
#endif

    bool ok = m_server->take_over_from_system_server();
    ASSERT(ok);

    m_server->on_ready_to_accept = [this] {
        auto client_socket = m_server->accept();
        if (!client_socket) {
            dbg() << "WindowServer: accept failed.";
            return;
        }
        static int s_next_client_id = 0;
        int client_id = ++s_next_client_id;
        IPC::new_client_connection<ClientConnection>(*client_socket, client_id);
    };

    ASSERT(m_keyboard_fd >= 0);
    ASSERT(m_mouse_fd >= 0);

    m_keyboard_notifier = Core::Notifier::construct(m_keyboard_fd, Core::Notifier::Read);
    m_keyboard_notifier->on_ready_to_read = [this] { drain_keyboard(); };

    m_mouse_notifier = Core::Notifier::construct(m_mouse_fd, Core::Notifier::Read);
    m_mouse_notifier->on_ready_to_read = [this] { drain_mouse(); };

    Clipboard::the().on_content_change = [&] {
        ClientConnection::for_each_client([&](auto& client) {
            client.notify_about_clipboard_contents_changed();
        });
    };
}

EventLoop::~EventLoop()
{
}

void EventLoop::drain_mouse()
{
    auto& screen = Screen::the();
    MousePacket state;
    state.buttons = screen.mouse_button_state();
    unsigned buttons = state.buttons;
    MousePacket packets[32];

#ifdef __OpenBSD__
    struct wscons_event wsevent;
    ssize_t nread = read(m_mouse_fd, (u8*)&wsevent, sizeof(wsevent));
    if (nread < (ssize_t)sizeof(wsevent))
        return;

    /* fake it */
    size_t npackets = 1;

    memset(&packets[0], 0, sizeof(packets[0]));

    packets[0].buttons = buttons;
    packets[0].is_relative = true;

    switch (wsevent.type) {
    case WSCONS_EVENT_MOUSE_UP:
        packets[0].buttons = 0;
        break;
    case WSCONS_EVENT_MOUSE_DOWN:
        packets[0].buttons = wsevent.value + 1; // button 0 is left-most
        break;
    case WSCONS_EVENT_MOUSE_DELTA_X:
        packets[0].x = wsevent.value;
        break;
    case WSCONS_EVENT_MOUSE_DELTA_Y:
        packets[0].y = wsevent.value;
        break;
    case WSCONS_EVENT_MOUSE_DELTA_Z:
        packets[0].y = wsevent.value;
        break;
    default:
        /* discard */
        npackets = 0;

        switch (wsevent.type) {
        case WSCONS_EVENT_MOUSE_DELTA_W:
            break;
        case WSCONS_EVENT_MOUSE_ABSOLUTE_X:
        case WSCONS_EVENT_MOUSE_ABSOLUTE_Y:
        case WSCONS_EVENT_MOUSE_ABSOLUTE_Z:
        case WSCONS_EVENT_MOUSE_ABSOLUTE_W:
            dbg() << "need absolute coordinate support";
            break;
        case WSCONS_EVENT_HSCROLL:
        case WSCONS_EVENT_VSCROLL:
            /* serenity doesn't support these yet */
            break;
        case WSCONS_EVENT_SYNC:
            break;
        default:
            dbg() << "unknown wscons event of type " << wsevent.type;
        }
    }
    npackets = 1;
#else
    ssize_t nread = read(m_mouse_fd, &packets, sizeof(packets));
    if (nread < 0) {
        perror("EventLoop::drain_mouse read");
        return;
    }
    size_t npackets = nread / sizeof(MousePacket);
#endif
    if (!npackets)
        return;
    for (size_t i = 0; i < npackets; ++i) {
        auto& packet = packets[i];
#ifdef WSMESSAGELOOP_DEBUG
        dbgprintf("EventLoop: Mouse X %d, Y %d, Z %d, relative %d\n", packet.x, packet.y, packet.z, packet.is_relative);
#endif
        buttons = packet.buttons;

        state.is_relative = packet.is_relative;
        if (packet.is_relative) {
            state.x += packet.x;
            state.y -= packet.y;
            state.z += packet.z;
        } else {
            state.x = packet.x;
            state.y = packet.y;
            state.z += packet.z;
        }

        if (buttons != state.buttons) {
            state.buttons = buttons;
#ifdef WSMESSAGELOOP_DEBUG
            dbgprintf("EventLoop: Mouse Button Event\n");
#endif
            screen.on_receive_mouse_data(state);
            if (state.is_relative) {
                state.x = 0;
                state.y = 0;
                state.z = 0;
            }
        }
    }
    if (state.is_relative && (state.x || state.y || state.z))
        screen.on_receive_mouse_data(state);
    if (!state.is_relative)
        screen.on_receive_mouse_data(state);
}

void EventLoop::drain_keyboard()
{
    auto& screen = Screen::the();
    for (;;) {
        ::KeyEvent event;
#ifdef __OpenBSD__
        struct wscons_event wsevent;
        ssize_t nread = read(m_keyboard_fd, (u8*)&wsevent, sizeof(wsevent));
        if (nread < (ssize_t)sizeof(wsevent)) {
            nread = 0;
            break;
        }

        memset(&event, 0, sizeof(event));

        switch (wsevent.type) {
        case WSCONS_EVENT_KEY_UP:
            switch (wsevent.value) {
            case RAWKEY_Shift_L:
            case RAWKEY_Shift_R:
                kbd_modifiers &= ~Mod_Shift;
                break;
            case RAWKEY_Control_L:
            case RAWKEY_Control_R:
                kbd_modifiers &= ~Mod_Ctrl;
                break;
            case RAWKEY_Alt_L:
            case RAWKEY_Alt_R:
            case RAWKEY_Meta_L:
            case RAWKEY_Meta_R:
                kbd_modifiers &= ~Mod_Alt;
                break;
            }
            break;
        case WSCONS_EVENT_KEY_DOWN:
            switch (wsevent.value) {
            case RAWKEY_Shift_L:
            case RAWKEY_Shift_R:
                kbd_modifiers |= Mod_Shift;
                break;
            case RAWKEY_Control_L:
            case RAWKEY_Control_R:
                kbd_modifiers |= Mod_Ctrl;
                break;
            case RAWKEY_Alt_L:
            case RAWKEY_Alt_R:
            case RAWKEY_Meta_L:
            case RAWKEY_Meta_R:
                kbd_modifiers |= Mod_Alt;
                break;
            }
            event.flags = Is_Press;
            break;
        default:
            dbg() << "unknown wscons event of type " << wsevent.type;
        }

        for (long i = 0; i < (int)(sizeof(kbd_trans_table) / sizeof(kbd_trans)); i++) {
            if (wsevent.value == kbd_trans_table[i].wskbd_value) {
                if (kbd_modifiers & Mod_Shift) {
                    event.key = kbd_trans_table[i].shifted_keycode;
                    event.character = kbd_trans_table[i].shifted_character;
                } else {
                    event.key = kbd_trans_table[i].keycode;
                    event.character = kbd_trans_table[i].character;
                }
                break;
            }
        }

        nread = sizeof(::KeyEvent);

        // dbg() << "wskbd key " << wsevent.value << " (type " << wsevent.type << ") -> event key " << event.key << " (" << event.character << "), modifiers " << kbd_modifiers;

        event.flags |= kbd_modifiers;
#else
        ssize_t nread = read(m_keyboard_fd, (u8*)&event, sizeof(::KeyEvent));
#endif
        if (nread == 0)
            break;
        ASSERT(nread == sizeof(::KeyEvent));
        screen.on_receive_keyboard_data(event);
    }
}

}
