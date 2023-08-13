#ifndef _SC_MACROS_H_
#define _SC_MACROS_H_

#define SC_STRINGIFY(x) #x
#define SC_TO_STRING(x) SC_STRINGIFY(x)
#define SC_LOCATION __FILE__ ":" SC_TO_STRING(__LINE__)

#define RET_IF_ERR_MSG(expr, msg)                        \
  {                                                      \
    int err = (expr);                                    \
    if (err) {                                           \
      LOG_ERR("Error %d: " msg " in " SC_LOCATION, err); \
      return err;                                        \
    }                                                    \
  }

#define RET_IF_ERR(expr) RET_IF_ERR_MSG(expr, "")

// Checks that expr evaluates to true, otherwise return 1.
#define RET_CHECK(expr, msg) RET_IF_ERR_MSG(!(expr), msg)

#define UNUSED_OK(expr) (void)expr;

#endif  // _SC_MACROS_H_