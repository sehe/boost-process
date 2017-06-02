// Copyright (c) 2017 Seth Heeren
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_POSIX_FD_RESTRICT_HPP
#define BOOST_PROCESS_DETAIL_POSIX_FD_RESTRICT_HPP

#include <boost/process/detail/posix/handler.hpp>
#include <unistd.h>

namespace boost { namespace process { namespace detail { namespace posix { namespace fd_restrict {
    // customization point for (custom) properties that need to protect fds
    template <typename Property, typename OutputIterator>
        void collect_filedescriptors(Property const& /*property*/, OutputIterator& /*outit*/) {
            // primary template
        }

    // polymorphic function object for ADL dispatch
    template <typename OutputIterator>
    struct collect_fd_f {
        OutputIterator mutable _outit;

        template <typename Property>
            void operator()(Property const& property) const {
                using boost::process::detail::posix::fd_restrict::collect_filedescriptors; // ADL desired
                collect_filedescriptors(property, _outit);
            }
    };

    // launch property
    template <typename=void>
    struct property_ : handler_base_ext
    {
    public:
        property_(size_t capacity) {
            // reserve to avoid allocations between fork/exec which may
            // deadlock with threads
            _protected_fds.reserve(capacity);
        }

        template <class PosixExecutor>
        void on_exec_setup(PosixExecutor& exec) const
        {
            _protected_fds.resize(0);
            auto outit = back_inserter(_protected_fds);
            collect_fd_f<decltype(outit)> visit{outit};

            visit(exec);
            boost::fusion::for_each(exec.seq, visit);

            auto begin = _protected_fds.begin(), end = _protected_fds.end();
            std::sort(begin, end);

            for(int fd=0, maxfd=sysconf(_SC_OPEN_MAX); fd<maxfd; ++fd) {
                if (!std::binary_search(begin, end, fd))
                    ::close(fd);
            }
        }

    private:
        std::vector<int> mutable _protected_fds;
    };

}}}}}

/* 
 * Non-intrusive instrumentation of existing POSIX properties that require filedescriptors
 *
 * All of these could be done with an inline `friend` definition, like above.
 *
 * For now I prefer to keep them separate so that 
 *
 *  - upstream changes merge cleanly
 *  - interface changes in fd_restrict can more easily be applied consistently in 1 file
 *
 * Only bind_fd_ and filedescriptor need friend access, so cannot usefully be kept separate.
 */

#include <boost/process/detail/posix/async_in.hpp>
#include <boost/process/detail/posix/async_out.hpp>
#include <boost/process/detail/posix/null_in.hpp>
#include <boost/process/detail/posix/null_out.hpp>
#include <boost/process/detail/posix/file_in.hpp>
#include <boost/process/detail/posix/file_out.hpp>
#include <boost/process/detail/posix/pipe_in.hpp>
#include <boost/process/detail/posix/pipe_out.hpp>

namespace boost { namespace process { namespace detail { namespace posix {

template<typename... Ts, typename OutputIterator>
void collect_filedescriptors(async_in_buffer<Ts...> const&, OutputIterator& outit) {
    *outit++ = STDIN_FILENO;
}

template<int p1, int p2, typename... Ts, typename OutputIterator>
void collect_filedescriptors(async_out_buffer<p1, p2, Ts...> const&, OutputIterator& outit) {
    if (p1==1||p2==1) *outit++ = STDOUT_FILENO;
    if (p1==2||p2==2) *outit++ = STDERR_FILENO;
}

template<int p1, int p2, typename... Ts, typename OutputIterator>
void collect_filedescriptors(async_out_future<p1, p2, Ts...> const&, OutputIterator& outit) {
    if (p1==1||p2==1) *outit++ = STDOUT_FILENO;
    if (p1==2||p2==2) *outit++ = STDERR_FILENO;
}

template<typename OutputIterator>
void collect_filedescriptors(file_in const&, OutputIterator& outit) {
    *outit++ = STDIN_FILENO;
}

template<int p1, int p2, typename OutputIterator>
void collect_filedescriptors(file_out<p1, p2> const&, OutputIterator& outit) {
    if (p1==1||p2==1) *outit++ = STDOUT_FILENO;
    if (p1==2||p2==2) *outit++ = STDERR_FILENO;
}

template<typename OutputIterator>
void collect_filedescriptors(null_in const&, OutputIterator& outit) {
    *outit++ = STDIN_FILENO;
}

template<int p1, int p2, typename OutputIterator>
void collect_filedescriptors(null_out<p1, p2> const&, OutputIterator& outit) {
    if (p1==1||p2==1) *outit++ = STDOUT_FILENO;
    if (p1==2||p2==2) *outit++ = STDERR_FILENO;
}

template<typename OutputIterator>
void collect_filedescriptors(pipe_in const&, OutputIterator& outit) {
    *outit++ = STDIN_FILENO;
}

template<typename OutputIterator>
void collect_filedescriptors(async_pipe_in const&, OutputIterator& outit) {
    *outit++ = STDIN_FILENO;
}

template<int p1, int p2, typename OutputIterator>
void collect_filedescriptors(pipe_out<p1, p2> const&, OutputIterator& outit) {
    if (p1==1||p2==1) *outit++ = STDOUT_FILENO;
    if (p1==2||p2==2) *outit++ = STDERR_FILENO;
}

template<int p1, int p2, typename OutputIterator>
void collect_filedescriptors(async_pipe_out<p1, p2> const&, OutputIterator& outit) {
    if (p1==1||p2==1) *outit++ = STDOUT_FILENO;
    if (p1==2||p2==2) *outit++ = STDERR_FILENO;
}

}}}}

#endif
