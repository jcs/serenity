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

#include <Kernel/CommandLine.h>

namespace Kernel {

static CommandLine* s_the;

const CommandLine& kernel_command_line()
{
    ASSERT(s_the);
    return *s_the;
}

void CommandLine::initialize(const String& string)
{
    s_the = new CommandLine(string);
}

CommandLine::CommandLine(const String& string)
    : m_string(string)
{
    s_the = this;

    for (auto str : m_string.split(' ')) {
        if (str == "") {
            continue;
        }

        auto pair = str.split_limit('=', 2);

        if (pair.size() == 1) {
            m_params.set(pair[0], "");
        } else {
            m_params.set(pair[0], pair[1]);
        }
    }
}

Optional<String> CommandLine::lookup(const String& key) const
{
    return m_params.get(key);
}

String CommandLine::get(const String& key) const
{
    return m_params.get(key).value_or({});
}

bool CommandLine::contains(const String& key) const
{
    return m_params.contains(key);
}

}
