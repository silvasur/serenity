/*
 * Copyright (c) 2018-2021, Andreas Kling <kling@serenityos.org>
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

#include <AK/NonnullRefPtr.h>
#include <AK/TypeCasts.h>
#include <AK/Vector.h>
#include <LibGfx/Rect.h>
#include <LibWeb/CSS/StyleProperties.h>
#include <LibWeb/Forward.h>
#include <LibWeb/Layout/BoxModelMetrics.h>
#include <LibWeb/Layout/LayoutPosition.h>
#include <LibWeb/Layout/LayoutStyle.h>
#include <LibWeb/Painting/PaintContext.h>
#include <LibWeb/TreeNode.h>

namespace Web::Layout {

enum class LayoutMode {
    Default,
    AllPossibleLineBreaks,
    OnlyRequiredLineBreaks,
};

enum class PaintPhase {
    Background,
    Border,
    Foreground,
    FocusOutline,
    Overlay,
};

struct HitTestResult {
    RefPtr<Node> layout_node;
    int index_in_node { 0 };

    enum InternalPosition {
        None,
        Before,
        Inside,
        After,
    };
    InternalPosition internal_position { None };
};

enum class HitTestType {
    Exact,      // Exact matches only
    TextCursor, // Clicking past the right/bottom edge of text will still hit the text
};

class Node : public TreeNode<Node> {
public:
    virtual ~Node();

    virtual HitTestResult hit_test(const Gfx::IntPoint&, HitTestType) const;

    bool is_anonymous() const { return !m_dom_node; }
    const DOM::Node* dom_node() const { return m_dom_node; }
    DOM::Node* dom_node() { return m_dom_node; }

    DOM::Document& document() { return m_document; }
    const DOM::Document& document() const { return m_document; }

    const Frame& frame() const;
    Frame& frame();

    const InitialContainingBlockBox& root() const;
    InitialContainingBlockBox& root();

    bool is_root_element() const;

    String class_name() const;

    bool has_style() const { return m_has_style; }

    virtual bool can_have_children() const { return true; }

    bool is_inline() const { return m_inline; }
    void set_inline(bool b) { m_inline = b; }

    bool is_inline_block() const;

    virtual bool wants_mouse_events() const { return false; }

    virtual void handle_mousedown(Badge<EventHandler>, const Gfx::IntPoint&, unsigned button, unsigned modifiers);
    virtual void handle_mouseup(Badge<EventHandler>, const Gfx::IntPoint&, unsigned button, unsigned modifiers);
    virtual void handle_mousemove(Badge<EventHandler>, const Gfx::IntPoint&, unsigned buttons, unsigned modifiers);

    virtual void before_children_paint(PaintContext&, PaintPhase) {};
    virtual void paint(PaintContext&, PaintPhase);
    virtual void paint_fragment(PaintContext&, const LineBoxFragment&, PaintPhase) const { }
    virtual void after_children_paint(PaintContext&, PaintPhase) {};

    bool is_floating() const;
    bool is_positioned() const;
    bool is_absolutely_positioned() const;
    bool is_fixed_position() const;

    const BlockBox* containing_block() const;
    BlockBox* containing_block() { return const_cast<BlockBox*>(const_cast<const Node*>(this)->containing_block()); }

    bool can_contain_boxes_with_position_absolute() const;

    const CSS::StyleProperties& specified_style() const;
    const ImmutableLayoutStyle& style() const;

    NodeWithStyle* parent();
    const NodeWithStyle* parent() const;

    void inserted_into(Node&) { }
    void removed_from(Node&) { }
    void children_changed() { }

    virtual void split_into_lines(InlineFormattingContext&, LayoutMode);

    bool is_visible() const { return m_visible; }
    void set_visible(bool visible) { m_visible = visible; }

    virtual void set_needs_display();

    bool children_are_inline() const { return m_children_are_inline; }
    void set_children_are_inline(bool value) { m_children_are_inline = value; }

    Gfx::FloatPoint box_type_agnostic_position() const;

    float font_size() const;

    enum class SelectionState {
        None,        // No selection
        Start,       // Selection starts in this Node
        End,         // Selection ends in this Node
        StartAndEnd, // Selection starts and ends in this Node
        Full,        // Selection starts before and ends after this Node
    };

    SelectionState selection_state() const { return m_selection_state; }
    void set_selection_state(SelectionState state) { m_selection_state = state; }

    template<typename Callback>
    void for_each_child_in_paint_order(Callback callback) const
    {
        for_each_child([&](auto& child) {
            if (is<Box>(child) && downcast<Box>(child).stacking_context())
                return;
            if (!child.is_positioned())
                callback(child);
        });
        for_each_child([&](auto& child) {
            if (is<Box>(child) && downcast<Box>(child).stacking_context())
                return;
            if (child.is_positioned())
                callback(child);
        });
    }

protected:
    Node(DOM::Document&, DOM::Node*);

private:
    friend class NodeWithStyle;

    NonnullRefPtr<DOM::Document> m_document;
    RefPtr<DOM::Node> m_dom_node;

    bool m_inline { false };
    bool m_has_style { false };
    bool m_visible { true };
    bool m_children_are_inline { false };
    SelectionState m_selection_state { SelectionState::None };
};

class NodeWithStyle : public Node {
public:
    virtual ~NodeWithStyle() override { }

    const CSS::StyleProperties& specified_style() const { return m_specified_style; }
    void set_specified_style(const CSS::StyleProperties& style) { m_specified_style = style; }

    const ImmutableLayoutStyle& style() const { return static_cast<const ImmutableLayoutStyle&>(m_style); }

    void apply_style(const CSS::StyleProperties&);

protected:
    NodeWithStyle(DOM::Document&, DOM::Node*, NonnullRefPtr<CSS::StyleProperties>);

private:
    LayoutStyle m_style;

    NonnullRefPtr<CSS::StyleProperties> m_specified_style;
    CSS::Position m_position;
};

class NodeWithStyleAndBoxModelMetrics : public NodeWithStyle {
public:
    BoxModelMetrics& box_model() { return m_box_model; }
    const BoxModelMetrics& box_model() const { return m_box_model; }

protected:
    NodeWithStyleAndBoxModelMetrics(DOM::Document& document, DOM::Node* node, NonnullRefPtr<CSS::StyleProperties> style)
        : NodeWithStyle(document, node, move(style))
    {
    }

private:
    BoxModelMetrics m_box_model;
};

inline const CSS::StyleProperties& Node::specified_style() const
{
    if (m_has_style)
        return static_cast<const NodeWithStyle*>(this)->specified_style();
    return parent()->specified_style();
}

inline const ImmutableLayoutStyle& Node::style() const
{
    if (m_has_style)
        return static_cast<const NodeWithStyle*>(this)->style();
    return parent()->style();
}

inline const NodeWithStyle* Node::parent() const
{
    return static_cast<const NodeWithStyle*>(TreeNode<Node>::parent());
}

inline NodeWithStyle* Node::parent()
{
    return static_cast<NodeWithStyle*>(TreeNode<Node>::parent());
}

}
