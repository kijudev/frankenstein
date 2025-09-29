
#ifdef FRANK_DISABLE_NOEXCEPT
#define FRANK_NOEXCEPT_EMPTY()
#define FRANK_NOEXCEPT_EXPR(Cond)
#define FRANK_NOEXCEPT_IF(Cond)
#define FRANK_NOEXCEPT_DEPENDS_ON(Expr)
#else
// Helper macro for argument counting
#define FRANK_GET_MACRO(_1, _2, NAME, ...) NAME

// Expands to 'noexcept' (no arguments)
#define FRANK_NOEXCEPT_EMPTY()             noexcept

// Expands to 'noexcept(expr)' (one argument)
#define FRANK_NOEXCEPT_EXPR(Expr)          noexcept((Expr))

// Main macro: use as FRANK_NOEXCEPT or FRANK_NOEXCEPT(expr)
#define FRANK_NOEXCEPT(...)                                                    \
    FRANK_GET_MACRO(__VA_ARGS__, FRANK_NOEXCEPT_EXPR, FRANK_NOEXCEPT_EMPTY)(   \
        __VA_ARGS__)

// Additional conditional macros for clarity
#define FRANK_NOEXCEPT_IF(Cond)         noexcept((Cond))
#define FRANK_NOEXCEPT_DEPENDS_ON(Expr) noexcept((Expr))
#endif
