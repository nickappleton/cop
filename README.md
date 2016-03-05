# COP

C Compiler, OS and Platform (COP) Abstractions

This is a very small set of headers to give access to a variety of platform/
compiler specific functionality. The APIs should provide a common usage
between OS X, Windows and Linux with minimal fudge-work.

## cop_alloc

A growing allocator which supports custom alignment and pushing and poping of
allocation state.

## cop_attributes

Useful function and variable attributes which may not exist in C90.

## cop_thread

A no-extras two-function threading library with access to a mutex object.
Supports Windows and posix threads.

## cop_vec

A floating point SIMD vector abstraction designed for DSP stuff. Might be
useful for you, may be completely useless.
