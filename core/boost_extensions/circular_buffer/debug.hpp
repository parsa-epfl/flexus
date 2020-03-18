//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#if !defined(BOOST_CIRCULAR_BUFFER_DEBUG_HPP)
#define BOOST_CIRCULAR_BUFFER_DEBUG_HPP

#if defined(_MSC_VER) && _MSC_VER >= 1200
#pragma once
#endif

namespace boost {

namespace cb_details {

#if BOOST_CB_ENABLE_DEBUG

class cb_iterator_base;

/*!
    \class cb_iterator_registry
    \brief Registry of valid iterators.

    This class is intended to be a base class of a container.
*/
class cb_iterator_registry {

  //! Pointer to the chain of valid iterators.
  mutable const cb_iterator_base *m_iterators;

public:
  //! Default constructor.
  cb_iterator_registry() : m_iterators(0) {
  }

  //! Register an iterator into the list of valid iterators.
  /*!
      \note The method is const in order to register iterators into const
     containers, too.
  */
  void register_iterator(const cb_iterator_base *it) const; // the implementation is below

  //! Unregister an iterator from the list of valid iterators.
  /*!
      \note The method is const in order to unregister iterators from const
     containers, too.
  */
  void unregister_iterator(const cb_iterator_base *it) const; // the implementation is below

  //! Invalidate all iterators.
  void invalidate_all_iterators(); // the implementation is below

  //! Invalidate every iterator conforming to the condition.
  template <class Condition>
  void invalidate_iterators(const Condition &condition); // twenisch: moved implementation below

private:
  //! Remove the current iterator from the iterator chain.
  void remove(const cb_iterator_base *current,
              const cb_iterator_base *previous) const; // the implementation is below
};

/*!
    \class cb_iterator_base
    \brief Registers/unregisters iterators into the registry of valid iterators.

    This class is intended to be a base class of an iterator.
*/
class cb_iterator_base {

private:
  //! Iterator registry.
  mutable const cb_iterator_registry *m_registry;

  //! Next iterator in the iterator chain.
  mutable const cb_iterator_base *m_next;

public:
  //! Default constructor.
  cb_iterator_base() : m_registry(0), m_next(0) {
  }

  //! Constructor taking the iterator registry as a parameter.
  cb_iterator_base(const cb_iterator_registry *registry) : m_registry(registry), m_next(0) {
    register_self();
  }

  //! Copy constructor.
  cb_iterator_base(const cb_iterator_base &rhs) : m_registry(rhs.m_registry), m_next(0) {
    register_self();
  }

  //! Destructor.
  ~cb_iterator_base() {
    unregister_self();
  }

  //! Assign operator.
  cb_iterator_base &operator=(const cb_iterator_base &rhs) {
    if (m_registry == rhs.m_registry)
      return *this;
    unregister_self();
    m_registry = rhs.m_registry;
    register_self();
    return *this;
  }

  //! Is the iterator valid?
  bool is_valid() const {
    return m_registry != 0;
  }

  //! Invalidate the iterator.
  /*!
      \note The method is const in order to invalidate const iterators, too.
  */
  void invalidate() const {
    m_registry = 0;
  }

  //! Return the next iterator in the iterator chain.
  const cb_iterator_base *next() const {
    return m_next;
  }

  //! Set the next iterator in the iterator chain.
  /*!
      \note The method is const in order to set a next iterator to a const
     iterator, too.
  */
  void set_next(const cb_iterator_base *it) const {
    m_next = it;
  }

private:
  //! Register self as a valid iterator.
  void register_self() {
    if (m_registry != 0)
      m_registry->register_iterator(this);
  }

  //! Unregister self from valid iterators.
  void unregister_self() {
    if (m_registry != 0)
      m_registry->unregister_iterator(this);
  }
};

inline void cb_iterator_registry::register_iterator(const cb_iterator_base *it) const {
  it->set_next(m_iterators);
  m_iterators = it;
}

inline void cb_iterator_registry::unregister_iterator(const cb_iterator_base *it) const {
  const cb_iterator_base *previous = 0;
  for (const cb_iterator_base *p = m_iterators; p != it; previous = p, p = p->next())
    ;
  remove(it, previous);
}

inline void cb_iterator_registry::invalidate_all_iterators() {
  for (const cb_iterator_base *p = m_iterators; p != 0; p = p->next())
    p->invalidate();
  m_iterators = 0;
}

inline void cb_iterator_registry::remove(const cb_iterator_base *current,
                                         const cb_iterator_base *previous) const {
  if (previous == 0)
    m_iterators = m_iterators->next();
  else
    previous->set_next(current->next());
}

template <class Condition>
inline void cb_iterator_registry::invalidate_iterators(const Condition &condition) {
  const cb_iterator_base *previous = 0;
  for (const cb_iterator_base *p = m_iterators; p != 0; p = p->next()) {
    if (condition(p)) {
      p->invalidate();
      remove(p, previous);
      continue;
    }
    previous = p;
  }
}

#else // #if BOOST_CB_ENABLE_DEBUG

class cb_iterator_registry {
#if BOOST_WORKAROUND(__BORLANDC__, < 0x6000)
  char dummy_; // BCB: by default empty structure has 8 bytes
#endif
};

class cb_iterator_base {
#if BOOST_WORKAROUND(__BORLANDC__, < 0x6000)
  char dummy_; // BCB: by default empty structure has 8 bytes
#endif

public:
  cb_iterator_base() {
  }
  cb_iterator_base(const cb_iterator_registry *) {
  }
};

#endif // #if BOOST_CB_ENABLE_DEBUG

} // namespace cb_details

} // namespace boost

#endif // #if !defined(BOOST_CIRCULAR_BUFFER_DEBUG_HPP)
