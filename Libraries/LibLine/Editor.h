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

#pragma once

#include <AK/BinarySearch.h>
#include <AK/ByteBuffer.h>
#include <AK/FileSystemPath.h>
#include <AK/Function.h>
#include <AK/HashMap.h>
#include <AK/NonnullOwnPtr.h>
#include <AK/QuickSort.h>
#include <AK/String.h>
#include <AK/Vector.h>
#include <LibCore/DirIterator.h>
#include <Libraries/LibLine/Span.h>
#include <Libraries/LibLine/Style.h>
#include <sys/stat.h>
#include <termios.h>

namespace Line {

class Editor;

struct KeyCallback {
    KeyCallback(Function<bool(Editor&)> cb)
        : callback(move(cb))
    {
    }
    Function<bool(Editor&)> callback;
};

class Editor {
public:
    Editor();
    ~Editor();

    void initialize()
    {
        ASSERT(!m_initialized);
        struct termios termios;
        tcgetattr(0, &termios);
        m_default_termios = termios; // grab a copy to restore
        // Because we use our own line discipline which includes echoing,
        // we disable ICANON and ECHO.
        termios.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(0, TCSANOW, &termios);
        m_termios = termios;
        m_initialized = true;
    }

    String get_line(const String& prompt);

    void add_to_history(const String&);
    const Vector<String>& history() const { return m_history; }

    void register_character_input_callback(char ch, Function<bool(Editor&)> callback);

    Function<Vector<String>(const String&)> on_tab_complete_first_token;
    Function<Vector<String>(const String&)> on_tab_complete_other_token;
    Function<void(Editor&)> on_display_refresh;

    // FIXME: we will have to kindly ask our instantiators to set our signal handlers
    // since we can not do this cleanly ourselves (signal() limitation: cannot give member functions)
    void interrupted() { m_was_interrupted = true; }
    void resized() { m_was_resized = true; }

    size_t cursor() const { return m_cursor; }
    const Vector<char, 1024>& buffer() const { return m_buffer; }
    char buffer_at(size_t pos) const { return m_buffer.at(pos); }

    void clear_line();
    void insert(const String&);
    void insert(const char);
    void cut_mismatching_chars(String& completion, const String& other, size_t start_compare);
    void stylize(const Span&, const Style&);
    void strip_styles()
    {
        m_spans_starting.clear();
        m_spans_ending.clear();
        m_refresh_needed = true;
    }

    const struct termios& termios() const { return m_termios; }
    const struct termios& default_termios() const { return m_default_termios; }

private:
    void vt_save_cursor();
    void vt_restore_cursor();
    void vt_clear_to_end_of_line();
    void vt_clear_lines(size_t count_above, size_t count_below = 0);
    void vt_move_relative(int x, int y);
    void vt_apply_style(const Style&);

    Style find_applicable_style(size_t offset) const;

    void refresh_display();

    // FIXME: These three will report the wrong value because they do not
    //        take the length of the prompt into consideration, and it does not
    //        appear that we can figure that out easily
    size_t num_lines() const
    {
        return (m_buffer.size() + m_num_columns) / m_num_columns;
    }

    size_t cursor_line() const
    {
        return (m_cursor + m_num_columns) / m_num_columns;
    }

    size_t offset_in_line() const
    {
        auto offset = m_cursor % m_num_columns;
        return offset;
    }

    Vector<char, 1024> m_buffer;
    ByteBuffer m_pending_chars;
    size_t m_cursor { 0 };
    size_t m_chars_inserted_in_the_middle { 0 };
    size_t m_times_tab_pressed { 0 };
    size_t m_num_columns { 0 };

    HashMap<char, NonnullOwnPtr<KeyCallback>> m_key_callbacks;

    // TODO: handle signals internally
    struct termios m_termios, m_default_termios;
    bool m_was_interrupted = false;
    bool m_was_resized = false;

    // FIXME: This should be something more take_first()-friendly.
    Vector<String> m_history;
    size_t m_history_cursor { 0 };
    size_t m_history_capacity { 100 };

    enum class InputState {
        Free,
        ExpectBracket,
        ExpectFinal,
        ExpectTerminator,
    };
    InputState m_state { InputState::Free };

    HashMap<u32, HashMap<u32, Style>> m_spans_starting;
    HashMap<u32, HashMap<u32, Style>> m_spans_ending;

    bool m_initialized { false };
    bool m_refresh_needed { false };
};

}
