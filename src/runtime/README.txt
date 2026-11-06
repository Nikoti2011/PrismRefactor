P06 runtime implementation (contract v1)

Link prism::runtime; include runtime/runtime.hpp from its exported src include
root. Canonical shared types remain in include/prism/. No shared types copied.
Registry implements pe::ImportResolver, normalizes ASCII DLL case only, preserves
symbol case, rejects empty/NUL names, zero addresses and duplicate registrations.
Unknown imports return unresolved_import with original DLL and symbol. Registry
configuration may allocate/throw: configure before entering guest code and catch
failures in the host worker. It does not validate arbitrary adapter addresses;
registrants must supply correctly attributed function addresses with sufficient
lifetime. No dynamic libraries, aliases, ordinal imports or success stubs.

HandleTable owns shared native resource references, keyed by a 32-bit slot and
32-bit process-wide allocation generation. Both tag and concrete object type
must match. Closing invalidates the handle immediately; borrowed shared_ptrs
may intentionally extend native resource lifetime. Table destruction drops its
references. Exhaustion throws instead of wrapping. Tables are worker-thread
confined; only generation allocation is atomic. Do not publish arbitrary native
addresses or reuse a table concurrently. Window/Dc/File tags are canonical.

ContextScope borrows GuestProcessContext and GuestThreadContext, binds them to
host TLS and restores the previous binding even on host exceptions. Each worker
thread owns its own GuestThreadContext. Contexts must outlive the scope; no
cross-thread scope movement. Last-error lives in that context, never errno.
P07 must establish the correct scope during every guest callback as well.

P06 provides isolated native behavior and x64 adapter tests. Production PE
execution is pending P02/P05 integration; this is not a second loader.
