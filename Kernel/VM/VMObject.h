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

#include <AK/InlineLinkedList.h>
#include <AK/RefCounted.h>
#include <AK/RefPtr.h>
#include <AK/Vector.h>
#include <AK/Weakable.h>
#include <Kernel/Lock.h>

namespace Kernel {

class Inode;
class PhysicalPage;

class VMObject : public RefCounted<VMObject>
    , public Weakable<VMObject>
    , public InlineLinkedListNode<VMObject> {
    friend class MemoryManager;
    friend class Region;

public:
    virtual ~VMObject();

    virtual RefPtr<VMObject> clone() = 0;

    virtual bool is_anonymous() const { return false; }
    virtual bool is_inode() const { return false; }
    virtual bool is_shared_inode() const { return false; }
    virtual bool is_private_inode() const { return false; }
    virtual bool is_contiguous() const { return false; }

    size_t page_count() const { return m_physical_pages.size(); }
    const Vector<RefPtr<PhysicalPage>>& physical_pages() const { return m_physical_pages; }
    Vector<RefPtr<PhysicalPage>>& physical_pages() { return m_physical_pages; }

    size_t size() const { return m_physical_pages.size() * PAGE_SIZE; }

    virtual const char* class_name() const = 0;

    // For InlineLinkedListNode
    VMObject* m_next { nullptr };
    VMObject* m_prev { nullptr };

    ALWAYS_INLINE void ref_region() { m_regions_count.fetch_add(1, AK::MemoryOrder::memory_order_relaxed); }
    ALWAYS_INLINE void unref_region() { m_regions_count.fetch_sub(1, AK::MemoryOrder::memory_order_relaxed); }
    ALWAYS_INLINE bool is_shared_by_multiple_regions() const { return m_regions_count.load(AK::MemoryOrder::memory_order_relaxed) > 1; }

protected:
    explicit VMObject(size_t);
    explicit VMObject(const VMObject&);

    template<typename Callback>
    void for_each_region(Callback);

    Vector<RefPtr<PhysicalPage>> m_physical_pages;
    Lock m_paging_lock { "VMObject" };

    mutable SpinLock<u8> m_lock;

private:
    VMObject& operator=(const VMObject&) = delete;
    VMObject& operator=(VMObject&&) = delete;
    VMObject(VMObject&&) = delete;

    Atomic<u32> m_regions_count { 0 };
};

}
