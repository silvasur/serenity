/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
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

#include <AK/QuickSort.h>
#include <AK/StringBuilder.h>
#include <LibWeb/DOM/Node.h>
#include <LibWeb/Layout/Box.h>
#include <LibWeb/Layout/InitialContainingBlockBox.h>
#include <LibWeb/Painting/StackingContext.h>

namespace Web::Layout {

StackingContext::StackingContext(Box& box, StackingContext* parent)
    : m_box(box)
    , m_parent(parent)
{
    ASSERT(m_parent != this);
    if (m_parent) {
        m_parent->m_children.append(this);

        // FIXME: Don't sort on every append..
        quick_sort(m_parent->m_children, [](auto& a, auto& b) {
            return a->m_box.style().z_index().value_or(0) < b->m_box.style().z_index().value_or(0);
        });
    }
}

void StackingContext::paint(PaintContext& context, PaintPhase phase)
{
    if (!is<InitialContainingBlockBox>(m_box)) {
        m_box.paint(context, phase);
    } else {
        // NOTE: InitialContainingBlockBox::paint() merely calls StackingContext::paint()
        //       so we call its base class instead.
        downcast<InitialContainingBlockBox>(m_box).BlockBox::paint(context, phase);
    }
    for (auto* child : m_children) {
        child->paint(context, phase);
    }
}

HitTestResult StackingContext::hit_test(const Gfx::IntPoint& position, HitTestType type) const
{
    HitTestResult result;
    if (!is<InitialContainingBlockBox>(m_box)) {
        result = m_box.hit_test(position, type);
    } else {
        // NOTE: InitialContainingBlockBox::hit_test() merely calls StackingContext::hit_test()
        //       so we call its base class instead.
        result = downcast<InitialContainingBlockBox>(m_box).BlockBox::hit_test(position, type);
    }

    for (auto* child : m_children) {
        auto result_here = child->hit_test(position, type);
        if (result_here.layout_node)
            result = result_here;
    }
    return result;
}

void StackingContext::dump(int indent) const
{
    StringBuilder builder;
    for (int i = 0; i < indent; ++i)
        builder.append(' ');
    builder.appendff("SC for {}({}) {} [children: {}]", m_box.class_name(), m_box.dom_node() ? m_box.dom_node()->node_name().characters() : "(anonymous)", m_box.absolute_rect().to_string().characters(), m_children.size());
    dbgln("{}", builder.string_view());
    for (auto& child : m_children)
        child->dump(indent + 1);
}

}
