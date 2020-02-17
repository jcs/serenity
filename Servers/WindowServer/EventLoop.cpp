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
#include <dev/wscons/wsksymdef.h>

static const struct kbd_trans {
    int wskbd_value;
    int wsusb_value;
    KeyCode keycode;
    u8 character;
    KeyCode shifted_keycode;
    u8 shifted_character;
} kbd_trans_table[] = {
    { KS_Escape,    41,     Key_Escape, '\033', Key_Escape, '\033' },
    { KS_1,		    30,     Key_1,      '1',    Key_ExclamationPoint, '!' },
    { KS_2,		    31,     Key_2,      '2',    Key_AtSign, '@' },
    { KS_3,		    32,     Key_3,      '3',    Key_Hashtag, '#' },
    { KS_4,		    33,     Key_4,      '4',    Key_Dollar, '$' },
    { KS_5,		    34,     Key_5,      '5',    Key_Percent, '%' },
    { KS_6,		    35,     Key_6,      '6',    Key_Circumflex, '^' },
    { KS_7,		    36,     Key_7,      '7',    Key_Ampersand, '&' },
    { KS_8,		    37,     Key_8,      '8',    Key_Asterisk, '*' },
    { KS_9,		    38,     Key_9,      '9',    Key_LeftParen, '(' },
    { KS_0,		    39,     Key_0,      '0',    Key_RightParen, ')' },
    { KS_minus,		45,     Key_Minus,  '-',    Key_Underscore, '_' },
    { KS_equal,		46,     Key_Equal,  '=',    Key_Plus, '+' },
    { KS_Tab,		43,     Key_Tab,    '\t',   Key_Tab, '\t' },
    { KS_Delete,    42,     Key_Backspace, 0x8, Key_Backspace, 0x8 },
    { KS_q,		    20,     Key_Q,      'q',    Key_Q, 'Q' },
    { KS_w,		    26,     Key_W,      'w',    Key_W, 'W' },
    { KS_e,		    8,      Key_E,      'e',    Key_E, 'E' },
    { KS_r,		    21,     Key_R,      'r',    Key_R, 'R' },
    { KS_t,		    23,     Key_T,      't',    Key_T, 'T' },
    { KS_y,		    28,     Key_Y,      'y',    Key_Y, 'Y' },
    { KS_u,		    24,     Key_U,      'u',    Key_U, 'U' },
    { KS_i,		    12,     Key_I,      'i',    Key_I, 'I' },
    { KS_o,		    18,     Key_O,      'o',    Key_O, 'O' },
    { KS_p,		    19,     Key_P,      'p',    Key_P, 'P' },
    { KS_bracketleft, 47,   Key_LeftBracket,  '[', Key_LeftBrace, '{' },
    { KS_bracketright, 48,  Key_RightBracket, ']', Key_RightBrace, '}' },
    { KS_Return,    40,     Key_Return, '\n',   Key_Return, '\n' },
    { KS_Control_L,	224,    Key_Control, 0,     Key_Control, 0 },
    { KS_a,		    4,      Key_A,      'a',    Key_A, 'A' },
    { KS_s,		    22,     Key_S,      's',    Key_S, 'S' },
    { KS_d,		    7,      Key_D,      'd',    Key_D, 'D' },
    { KS_f,		    9,      Key_F,      'f',    Key_F, 'F' },
    { KS_g,		    10,     Key_G,      'g',    Key_G, 'G' },
    { KS_h,		    11,     Key_H,      'h',    Key_H, 'H' },
    { KS_j,		    13,     Key_J,      'j',    Key_J, 'J' },
    { KS_k,		    14,     Key_K,      'k',    Key_K, 'K' },
    { KS_l,		    15,     Key_L,      'l',    Key_L, 'L' },
    { KS_semicolon,	51,     Key_Semicolon, ';', Key_Colon, ':' },
    { KS_apostrophe, 52,    Key_Apostrophe, '\'', Key_DoubleQuote, '"' },
    { KS_grave,		53,     Key_Backtick, '`',  Key_Tilde, '~' },
    { KS_Shift_L,   225,    Key_Shift,  0,      Key_Shift, 0 },
    { KS_backslash, 49,     Key_Backslash, '\\', Key_Pipe, '|' },
    { KS_z,		    29,     Key_Z,      'z',    Key_Z, 'Z' },
    { KS_x,		    27,     Key_X,      'x',    Key_X, 'X' },
    { KS_c,		    6,      Key_C,      'c',    Key_C, 'C' },
    { KS_v,		    25,     Key_V,      'v',    Key_V, 'V' },
    { KS_b,		    5,      Key_B,      'b',    Key_B, 'B' },
    { KS_n,		    17,     Key_N,      'n',    Key_N, 'N' },
    { KS_m,		    16,     Key_M,      'm',    Key_M, 'M' },
    { KS_comma,		54,     Key_Comma,  ',',    Key_LessThan, '<' },
    { KS_period,    55,     Key_Period, '.',    Key_GreaterThan, '>' },
    { KS_slash,		56,     Key_Slash,  '/',    Key_QuestionMark, '?' },
    { KS_Shift_R,   0,      Key_Shift,  0,      Key_Shift, 0 },
    { KS_multiply,  85,     Key_Asterisk, '*', Key_Asterisk, '*' },
    { KS_Alt_L,		226,    Key_Alt,    0,      Key_Alt, 0 },
    { KS_space,		44,     Key_Space,  ' ',    Key_Space, ' ' },
    { KS_Caps_Lock,	57,     Key_CapsLock, 0,    Key_CapsLock, 0 },
    { KS_f1,		58,     Key_F1,     0,      Key_F1, 0 },
    { KS_f2,		59,     Key_F2,     0,      Key_F2, 0 },
    { KS_f3,		60,     Key_F3,     0,      Key_F3, 0 },
    { KS_f4,		61,     Key_F4,     0,      Key_F4, 0 },
    { KS_f5,		62,     Key_F5,     0,      Key_F5, 0 },
    { KS_f6,		63,     Key_F6,     0,      Key_F6, 0 },
    { KS_f7,		64,     Key_F7,     0,      Key_F7, 0 },
    { KS_f8,		65,     Key_F8,     0,      Key_F8, 0 },
    { KS_f9,		66,     Key_F9,     0,      Key_F9, 0 },
    { KS_f10,		67,     Key_F10,    0,      Key_F10, 0 },
    { KS_Num_Lock,  83,     Key_NumLock, 0,     Key_NumLock, 0 },
    { KS_Hold_Screen, 71,   Key_SysRq,  0,      Key_SysRq, 0 },
    { KS_Home,	    74,     Key_Home,   0,      Key_Home, 0 },
    { KS_Up,		82,     Key_Up,     0,      Key_Up, 0 },
    { KS_Prior,	    75,     Key_PageUp, 0,      Key_PageUp, 0 },
    { KS_Alt_R,		230,    Key_Alt,    0,      Key_Alt, 0 },
    { KS_Control_R,	228,    Key_Control, 0,     Key_Control, 0 },
    { KS_KP_Subtract, 86,   Key_Minus, '-',   Key_Minus, '-' },
    { KS_Left,	    80,     Key_Left,   0,      Key_Left, 0 },
    { KS_Right,     79,     Key_Right,  0,      Key_Right, 0 },
    { KS_KP_Add,	87,     Key_Plus,   '+',    Key_Plus, '+' },
    { KS_End,   	77,     Key_End,    0,      Key_End, 0 },
    { KS_Down,  	81,     Key_Down,   0,      Key_Down, 0 },
    { KS_Next,  	78,     Key_PageDown, 0,    Key_PageDown, 0 },
    { KS_Insert,	73,     Key_Insert, 0,      Key_Insert, 0 },
    { KS_Delete,	76,     Key_Delete, 0,      Key_Delete, 0 },
    { KS_Print_Screen, 70,  Key_PrintScreen, 0, Key_PrintScreen, 0 },
    { KS_f11,		68,     Key_F11,    0,      Key_F11, 0 },
    { KS_f12,		69,     Key_F12,    0,      Key_F12, 0 },
    { KS_KP_Divide,	84,     Key_Slash,  '/',    Key_Slash, '/' },
    { KS_Pause,     72,     Key_Invalid, 0,     Key_Invalid, 0 },
    { KS_KP_7,      95,     Key_7,      '7',    Key_7, '7' },
    { KS_KP_8,      96,     Key_8,      '8',    Key_8, '8' },
    { KS_KP_9,      97,     Key_9,      '9',    Key_9, '9' },
    { KS_KP_4,      92,     Key_4,      '4',    Key_4, '4' },
    { KS_KP_5,      93,     Key_5,      '5',    Key_5, '5' },
    { KS_KP_6,      94,     Key_6,      '6',    Key_6, '6' },
    { KS_KP_1,      89,     Key_1,      '1',    Key_1, '1' },
    { KS_KP_2,      90,     Key_2,      '2',    Key_2, '2' },
    { KS_KP_3,      91,     Key_3,      '3',    Key_3, '3' },
    { KS_KP_0,      98,     Key_0,      '0',    Key_0, '0' },
    { KS_KP_Decimal, 99,    Key_Period, '.',    Key_Period, '.' },
    { KS_KP_Enter,  88,     Key_Return, '\n',   Key_Return, '\n' },
    { 219,          101,    Key_Logo,   0,      Key_Logo, 0 },
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
        ssize_t nread = 0;
        if (read(m_keyboard_fd, (u8*)&wsevent, sizeof(wsevent)) < (ssize_t)sizeof(wsevent))
            break;

        memset(&event, 0, sizeof(event));

        for (int i = 0; i < (int)(sizeof(kbd_trans_table) / sizeof(kbd_trans)); i++) {
            // XXX: We are reading events from the wskbd mux which provides no
            // indication of which child device this event came from.  Since
            // each child has its own keyboard type and map, wsevent.value is
            // only useful if you have the keyboard's specific map.
            // I guess the proper way to do this is to open the mux, do
            // WSMUXIO_LIST_DEVICES, then open each device separately and poll
            // on it.  Then we'd know which device each event came from and can
            // look it up properly, but that's a lot of work.  So just
            // hard-code that we're looking at a USB device here...
            if (wsevent.value != kbd_trans_table[i].wsusb_value)
                continue;

            switch (wsevent.type) {
            case WSCONS_EVENT_KEY_UP:
                switch (kbd_trans_table[i].keycode) {
                case Key_Shift:
                    kbd_modifiers &= ~Mod_Shift;
                    break;
                case Key_Control:
                    kbd_modifiers &= ~Mod_Ctrl;
                    break;
                case Key_Alt:
                    kbd_modifiers &= ~Mod_Alt;
                    break;
                default:
                    ;
                }
                break;
            case WSCONS_EVENT_KEY_DOWN:
                event.flags = Is_Press;

                switch (kbd_trans_table[i].keycode) {
                case Key_Shift:
                    kbd_modifiers |= Mod_Shift;
                    break;
                case Key_Control:
                    kbd_modifiers |= Mod_Ctrl;
                    break;
                case Key_Alt:
                    kbd_modifiers |= Mod_Alt;
                    break;
                default:
                    ;
                }
                break;
            }

            if (kbd_modifiers & Mod_Shift) {
                event.key = kbd_trans_table[i].shifted_keycode;
                event.character = kbd_trans_table[i].shifted_character;
            } else {
                event.key = kbd_trans_table[i].keycode;
                event.character = kbd_trans_table[i].character;
            }
            event.flags |= kbd_modifiers;
            nread = sizeof(::KeyEvent);

            //dbg() << "wskbd key " << wsevent.value << " (type " << wsevent.type << ") -> serenity event key " << event.key << " (" << (char)event.character << "), modifiers " << kbd_modifiers;
            break;
        }
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
