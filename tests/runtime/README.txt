P06 runtime behavioral checks

Enable PRISM_BUILD_P06 and build check-p06. Runtime checks cover original error
metadata, DLL normalization/symbol case, duplicate/null rejection, 10,000 handle
reuse cycles, wrong kind/object type/table, stale/null/forged handles, lifetime,
nested scope restoration and concurrent independent thread contexts. Failures
throw explicitly even in Release builds; checks do not depend on assert/NDEBUG.
